/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  blercuadapter.cpp
//

#include "blercuadapter_p.h"
#include "blercudevice_p.h"
// #include "blercurecovery.h"
#include "blercu/bleservices/blercuservicesfactory.h"

#include "dbus/dbusobjectmanager.h"
#include "interfaces/bluezadapterinterface.h"
#include "interfaces/bluezdeviceinterface.h"

#include "configsettings/configsettings.h"

#include "ctrlm_log_ble.h"

#include <algorithm>

using namespace std;


static gboolean onDiscoveryWatchdog(gpointer user_data);

// -----------------------------------------------------------------------------
/*!
    \class BleRcuAdapterBluez
    \brief The BleRcuAdapterBluez is essentially a wrapper around the bluez
    adapter interface, however it runs it's own state machine and also stores
    the BleRcuDeviceBluez objects for any known device.


    State Machine
    This class implements the following state machine, it tries at all times
    to ensure that the adapter is available and powered.

    \image BleRcuManager.svg

    The service registered / unregistered refer to the dbus service that the
    bluetooth daemon exposes.  It should always be registered, however if
    for whatever reason the daemon crashes and is restarted, then the service
    will be unregistered and then re-registered, this object attempts to handle
    those cases gracefully.

    The adapter availability events should also also never really happen on a
    stable running system, an adapter refers to the bluetooth device on the
    host.  On current STB's this is always present, there is no hotplugging of
    bluetooth adapters with the possible exception of brief periods when waking
    from deepsleep.  However the state changes are handled in this object
    in case the adapter had to be reset for any reason.
 */



// -----------------------------------------------------------------------------
/*!
    \internal

    Static function used at construction time to create a set of supported
    device names from the \l{ConfigSettings} vendor details list.
        std::regex name_regex(supportedName.c_str(), std::regex_constants::ECMAScript);
 */
std::vector<std::regex> BleRcuAdapterBluez::getSupportedPairingNames(const std::vector<ConfigModelSettings> &modelDetails)
{
    vector<regex> names;

    for (const ConfigModelSettings &model : modelDetails) {
        if (!model.disabled()) {
            names.push_back(model.connectNameMatcher());
        }
    }
    return names;
}


BleRcuAdapterBluez::BleRcuAdapterBluez(const std::shared_ptr<const ConfigSettings> &config,
                                       const std::shared_ptr<BleRcuServicesFactory> &servicesFactory,
                                       const GDBusConnection *bluezBusConn,
                                       GMainLoop *mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_servicesFactory(servicesFactory)
    , m_bluezDBusConn(bluezBusConn)
    , m_bluezService("org.bluez")
    , m_discovering(false)
    , m_pairable(false)
    , m_discoveryRequests(0)
    , m_discoveryRequested(StopDiscovery)
    , m_discoveryWatchdogID(0)
    , m_discoveryWatchdogTimeout(5000)
    , m_supportedPairingNames(getSupportedPairingNames(config->modelSettings()))
    , m_retryEventId(-1)
{
    m_GMainLoop = mainLoop;

    XLOGD_INFO("Create HciSocket");
    m_hciSocket = HciSocket::create(0, -1);
    if (!m_hciSocket || !m_hciSocket->isValid()) {
        m_hciSocket.reset();
        XLOGD_ERROR("failed to setup hci socket to hci%u", 0);
    }

    // create a dbus service watcher object so we can detect if the bluez daemon
    // (bluetoothd) falls off the bus or arrives back on the bus
    m_bluezServiceWatcher = std::make_shared<DBusServiceWatcher> (m_bluezService, m_bluezDBusConn);
    m_bluezServiceWatcher->addServiceRegisteredHandler(std::bind(&BleRcuAdapterBluez::onBluezServiceRegistered, this, std::placeholders::_1));
    m_bluezServiceWatcher->addServiceUnRegisteredHandler(std::bind(&BleRcuAdapterBluez::onBluezServiceUnregistered, this, std::placeholders::_1));

    // initialise and start the state machine
    initStateMachine();

#if 0
    // also listen for any recovery events, these are just requests that any of
    // the code can trigger and should be last resort events when we think
    // that something is broken and needs resetting.
    QObject::connect(bleRcuRecovery, &BleRcuRecovery::powerCycleAdapter,
                     this, &BleRcuAdapterBluez::onPowerCycleAdapter,
                     Qt::QueuedConnection);
    QObject::connect(bleRcuRecovery, &BleRcuRecovery::reconnectDevice,
                     this, &BleRcuAdapterBluez::onDisconnectReconnectDevice,
                     Qt::QueuedConnection);


#endif
}

BleRcuAdapterBluez::~BleRcuAdapterBluez()
{
    *m_isAlive = false;

    if (m_stateMachine.isRunning()) {
        m_stateMachine.postEvent(ShutdownEvent);
        m_stateMachine.stop();
    }

    XLOGD_INFO("BleRcuAdapterBluez shut down");
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Intialises and starts the state machine for managing the bluez service and
    adapter proxy.

 */
void BleRcuAdapterBluez::initStateMachine()
{
    // set the name of the statemachine for logging
    m_stateMachine.setObjectName("BleRcuAdapterBluez");
    m_stateMachine.setGMainLoop(m_GMainLoop);

    // add all the states
    m_stateMachine.addState(ServiceUnavailableState, "ServiceUnavailableState");
    m_stateMachine.addState(ServiceAvailableSuperState, "ServiceAvailableSuperState");

    m_stateMachine.addState(ServiceAvailableSuperState, AdapterUnavailableState, "AdapterUnavailableState");
    m_stateMachine.addState(ServiceAvailableSuperState, AdapterAvailableSuperState, "AdapterAvailableSuperState");

    m_stateMachine.addState(AdapterAvailableSuperState, AdapterPoweredOffState, "AdapterPoweredOffState");
    m_stateMachine.addState(AdapterAvailableSuperState, AdapterPoweredOnState, "AdapterPoweredOnState");

    m_stateMachine.addState(ShutdownState, "ShutdownState");


    // add the transitions       From State                ->   Event                  ->   To State
    m_stateMachine.addTransition(ServiceUnavailableState,       ServiceAvailableEvent,      AdapterUnavailableState);
    m_stateMachine.addTransition(ServiceUnavailableState,       ServiceRetryEvent,          ServiceUnavailableState);
    m_stateMachine.addTransition(ServiceAvailableSuperState,    ServiceUnavailableEvent,    ServiceUnavailableState);
    m_stateMachine.addTransition(ServiceAvailableSuperState,    ShutdownEvent,              ShutdownState);

    m_stateMachine.addTransition(AdapterUnavailableState,       AdapterAvailableEvent,      AdapterPoweredOffState);
    m_stateMachine.addTransition(AdapterUnavailableState,       AdapterRetryAttachEvent,    AdapterUnavailableState);
    m_stateMachine.addTransition(AdapterAvailableSuperState,    AdapterUnavailableEvent,    AdapterUnavailableState);

    m_stateMachine.addTransition(AdapterPoweredOffState,        AdapterPoweredOnEvent,      AdapterPoweredOnState);
    m_stateMachine.addTransition(AdapterPoweredOffState,        AdapterRetryPowerOnEvent,   AdapterPoweredOffState);
    m_stateMachine.addTransition(AdapterPoweredOnState,         AdapterPoweredOffEvent,     AdapterPoweredOffState);


    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&BleRcuAdapterBluez::onStateEntry, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&BleRcuAdapterBluez::onStateExit, this, std::placeholders::_1)));


    // set the initial state
    m_stateMachine.setInitialState(ServiceUnavailableState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void BleRcuAdapterBluez::onStateEntry(int state)
{
    switch (State(state)) {
        case ServiceUnavailableState:
            onEnteredServiceUnavailableState();
            break;
        case AdapterUnavailableState:
            onEnteredAdapterUnavailableState();
            break;
        case AdapterPoweredOffState:
            onEnteredAdapterPoweredOffState();
            break;
        case AdapterPoweredOnState:
            onEnteredAdapterPoweredOnState();
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void BleRcuAdapterBluez::onStateExit(int state)
{
    switch (State(state)) {
        case ServiceAvailableSuperState:
            onExitedServiceAvailableSuperState();
            break;
        case AdapterAvailableSuperState:
            onExitedAdapterAvailableSuperState();
            break;
        case AdapterPoweredOnState:
            onExitedAdapterPoweredOnState();
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry (or re-entry) to the 'Service Unavailable State', we just
    check if the service is still unavailable.

 */
void BleRcuAdapterBluez::onEnteredServiceUnavailableState()
{
    // cancel any pending delayed retry event
    if (m_retryEventId >= 0) {
        m_stateMachine.cancelDelayedEvent(m_retryEventId);
        m_retryEventId = -1;
    }

    // check if the service proxy object is valid
    if (m_bluezObjectMgr == nullptr || !m_bluezObjectMgr->isValid()) {

        m_bluezObjectMgr = std::make_shared<DBusObjectManagerInterface> (m_bluezService, "/", m_bluezDBusConn);

        if (m_bluezObjectMgr == nullptr || !m_bluezObjectMgr->isValid()) {
            XLOGD_ERROR("failed to create adapter object manager proxy");
            m_bluezObjectMgr.reset();
        } else {
            XLOGD_INFO("Successfully created DBusObjectManagerInterface");
            // install handlers for interfaces added / removed notifications
            m_bluezObjectMgr->addInterfacesAddedSlot(Slot<const std::string&, const DBusInterfaceList&>(m_isAlive,
                    std::bind(&BleRcuAdapterBluez::onBluezInterfacesAdded, this, std::placeholders::_1, std::placeholders::_2)));
            m_bluezObjectMgr->addInterfacesRemovedSlot(Slot<const std::string&, const std::vector<std::string>&>(m_isAlive,
                    std::bind(&BleRcuAdapterBluez::onBluezInterfacesRemoved, this, std::placeholders::_1, std::placeholders::_2)));
        }
    }


    // check once again if the proxy is valid
    if (m_bluezObjectMgr != nullptr && m_bluezObjectMgr->isValid()) {
        m_stateMachine.postEvent(ServiceAvailableEvent);
    } else {
        m_retryEventId = m_stateMachine.postDelayedEvent(ServiceRetryEvent, 1000);
    }
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Called on exit from the 'Service Available Super State', this means that the
    bluez daemon has fallen off the bus and therefore we need to clear the
    dbus proxy object we are using to talk to the daemon as it's now defunct.

 */
void BleRcuAdapterBluez::onExitedServiceAvailableSuperState()
{
    m_bluezObjectMgr.reset();
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry (or re-entry) to the 'Adapter Unavailable State', we just
    check if the adapter is still unavailable.

    We may re-enter this state when the retry timer expires.

 */
void BleRcuAdapterBluez::onEnteredAdapterUnavailableState()
{
    // cancel any pending delayed retry event
    if (m_retryEventId >= 0) {
        m_stateMachine.cancelDelayedEvent(m_retryEventId);
        m_retryEventId = -1;
    }


    // try and find the first adapter (there should only be one)
    if (m_adapterObjectPath.empty()) {

        m_adapterObjectPath = findAdapter();
        if (m_adapterObjectPath.empty()) {
            XLOGD_ERROR("failed to find the bluez adapter object, is the bluetoothd daemon running?");

            m_retryEventId = m_stateMachine.postDelayedEvent(AdapterRetryAttachEvent, 1000);
            return;
        }
    }

    // we need to attach two dbus proxy interfaces to the adapter object;
    //   org.bluez.Adapter1                  - for the bluez events
    //   org.freedesktop.DBus.ObjectManager  - for notification on device add / remove
    
    if (!m_adapterProxy || !m_adapterProxy->isValid()) {

        if (!attachAdapter(m_adapterObjectPath)) {
            XLOGD_ERROR("failed to create proxies to the bluez adapter object");

            m_retryEventId = m_stateMachine.postDelayedEvent(AdapterRetryAttachEvent, 1000);
            return;
        }
    }

    m_stateMachine.postEvent(AdapterAvailableEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on exit from the 'Adapter Available Super State', this should only
    happen at shutdown or if the daemon falls of the bus.  In theory it could
    happen if the bluetooth adapter attached to the STB disappears, but since
    it is fixed to the board this shouldn't ever happen.

    Anyway when exiting this state for whatever reason we clear the list of
    ble device objects where holding and reset our dbus proxy interface to the
    adapter as it's now defunct.

 */
void BleRcuAdapterBluez::onExitedAdapterAvailableSuperState()
{
    XLOGD_INFO("Bluetooth adapter not available, keeping ble device objects but resetting proxy to adapter...");

    m_adapterObjectPath = "";
    m_adapterProxy.reset();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry (or re-entry) to the 'Powered Off State', we just check if
    the adapter is still un-powered and if so send a request to the daemon to
    power it on.

    The power on request is asynchronous; we wait for the power property change
    notification to get out of this state.

    We may re-enter this state when the retry timer expires.

 */
void BleRcuAdapterBluez::onEnteredAdapterPoweredOffState()
{
    if (!m_adapterProxy || !m_adapterProxy->isValid()) {
        XLOGD_WARN("no valid proxy to adapter");
        return;
    }

    // cancel any pending delayed retry event
    if (m_retryEventId >= 0) {
        m_stateMachine.cancelDelayedEvent(m_retryEventId);
        m_retryEventId = -1;
    }

    // skip out early if already powered
    bool powered = false;
    if (m_adapterProxy->powered(powered) && powered) {
        m_stateMachine.postEvent(AdapterPoweredOnEvent);
        return;
    }

    XLOGD_WARN("adapter is not powered, attempting to power on now");

    // we don't call setPowered() directly as that can block for up to 10 seconds, 
    // it is one of the few properties that do - normally it's only methods that block
    m_adapterProxy->setPowered(true, 
            PendingReply<>(m_isAlive, std::bind(&BleRcuAdapterBluez::onPowerOnReply, this, std::placeholders::_1)));

    // post an event so we retry power on again in 10 seconds time if we don't
    // get an acknowledgement
    m_retryEventId = m_stateMachine.postDelayedEvent(AdapterRetryPowerOnEvent, 10000);

}
// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called with the reply to the request to change the 'Powered' property
    to true. This slot is installed just for logging errors it doesn't affect
    the state machine.

 */
void BleRcuAdapterBluez::onPowerOnReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        XLOGD_ERROR("power on request failed with error: <%s>", reply->errorMessage().c_str());
    } else {
        XLOGD_INFO("power on request successful");
    }
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'Powered On State', this is the final setup state and
    it's expected to be the steady state.

    When entering this state we reset the scan filter for just BLE devices,
    then disable pairable mode (this may affect BT audio device pairing if
    active, but very slight corner case), before finally get a list of devices
    that are currently attached to the adapter.

 */
void BleRcuAdapterBluez::onEnteredAdapterPoweredOnState()
{
    // cancel any pending delayed retry event
    if (m_retryEventId >= 0) {
        m_stateMachine.cancelDelayedEvent(m_retryEventId);
        m_retryEventId = -1;
    }

    // check if the adapter is already in discovery mode (really shouldn't be)
    // and stop it if so and then sets the discovery filter for BT LE
    if (!setAdapterDiscoveryFilter()) {
        XLOGD_ERROR("failed to configure discovery filter");
        // not fatal
    }

    // disable the pairable flag on the adapter
    disablePairable();

    // signal the power state change before iterating and adding any devices
    m_poweredChangedSlots.invoke(true);

    // finally get a list of currently registered devices (RCUs)
    getRegisteredDevices();

    // signal that the adapter is powered and we got the list of paired devices
    m_poweredInitializedSlots.invoke();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when leaving the 'Powered On State', implemented to simply emit a
    signal on the power change.

 */
void BleRcuAdapterBluez::onExitedAdapterPoweredOnState()
{
    m_poweredChangedSlots.invoke(false);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when dbus tells us that the bluez service (org.bluez) has
    been registered once again on the bus.  We don't immediately try and
    connect as typically you have a race where the service is registered
    first followed by the objects and interfaces.  So instead we trigger the
    \l{onBluezServiceRetryTimer} to check again in one second time.

 */
void BleRcuAdapterBluez::onBluezServiceRegistered(const std::string &serviceName)
{
    if (serviceName != m_bluezService) {
        return;
    }

    XLOGD_INFO("detected bluez service registration, will retry connecting in 1s");

    m_stateMachine.postDelayedEvent(ServiceRetryEvent, 1000);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when dbus tells us that the bluez service (org.bluez) has
    been un-registered from the bus.  This triggers us to clear the availablity
    flag and remove all devices.

 */
void BleRcuAdapterBluez::onBluezServiceUnregistered(const std::string &serviceName)
{
    if (serviceName != m_bluezService) {
        return;
    }

    XLOGD_ERROR("detected bluez service has dropped off the dbus, has it crashed?");

    m_stateMachine.postEvent(ServiceUnavailableEvent);
}
// -----------------------------------------------------------------------------
/*!
    Returns \c true if the manager has been correctly constructed and managed
    to connect to the blue interface.

 */
bool BleRcuAdapterBluez::isValid() const
{
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the manager has been correctly constructed and managed
    to connect to the blue interface.

 */
bool BleRcuAdapterBluez::isAvailable() const
{
    return m_stateMachine.inState(AdapterAvailableSuperState);
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the manager has been correctly constructed and managed
    to power it on.

 */
bool BleRcuAdapterBluez::isPowered() const
{
    return m_stateMachine.inState(AdapterPoweredOnState);
}
#if 0
// -----------------------------------------------------------------------------
/*!
    Called by the system when someone invokes dumpsys on this service.

 */
void BleRcuAdapterBluez::dump(Dumper out) const
{
    out.printLine("stack:     bluez");
    out.printLine("address:   %s", qPrintable(m_address.toString()));
    out.printBoolean("available: ", isAvailable());
    out.printBoolean("powered:   ", isPowered());
    out.printBoolean("scanning:  ", m_discovering);
    out.printBoolean("pairable:  ", m_pairable);
}
#endif

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to find the first bluetooth (HCI) adapter

    This first gets a list of all the managed objects on the 'org.bluez'
    service. Then iterates through the objects trying to find an object that
    has the 'org.bluez.Adapter1' service.  We assume there is only one bluetooth
    adapter so we use the first one found.

    Nb: I found it helps to run dbus-monitor while running the bluez example
    python scripts to figure out what to expect

 */
std::string BleRcuAdapterBluez::findAdapter(int timeout)
{
    // sanity check the dbus connection
    if (m_bluezObjectMgr == nullptr) {
        XLOGD_ERROR("dbus connection not valid");
        return string();
    }

    // temporarily set the specified timeout
    m_bluezObjectMgr->setTimeout(timeout);

    DBusManagedObjectList objects;
    bool success = m_bluezObjectMgr->GetManagedObjects(objects);
    
    // restore the default timeout
    m_bluezObjectMgr->setTimeout(-1);
    
    if (success) {
        DBusManagedObjectList::iterator object = objects.begin();
        for (; object != objects.end(); ++object) {

            // get the object path and interfaces
            const string &path = object->first;
            DBusInterfaceList &interfaces = object->second;

            DBusInterfaceList::iterator interface = interfaces.begin();
            for (; interface != interfaces.end(); ++interface) {

                // get the interface name and properties
                const string &name = interface->first;
                DBusPropertiesMap &properties = interface->second;

                // if this object has an adapter interface then it's one for us
                if (name == BluezAdapterInterface::staticInterfaceName()) {
                    string addrStr;
                    if (properties.end() != properties.find("Address") && properties["Address"].toString(addrStr)) {
                        m_address = BleAddress(addrStr);
                    }
                    // use the supplied properties to set the initial discovery and pairable states
                    bool discovering;
                    if (properties.end() != properties.find("Discovering") && properties["Discovering"].toBool(discovering)) {
                        m_discovering = discovering;
                    }
                    bool pairable;
                    if (properties.end() != properties.find("Pairable") && properties["Pairable"].toBool(pairable)) {
                        m_pairable = pairable;
                    }
                    XLOGD_INFO("found bluez adapter at path <%s> with address <%s>", path.c_str(), m_address.toString().c_str());
                    return path;
                }
            }
        }
    }
    return string();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    This creates dbus proxy interface objects to communicate with the
    adapter interface on the bluez daemon.

    We attach to two interfaces on the daemon, the first is 'org.bluez.Adapter1'
    which is the one used to control / monitor the adapter (i.e. things like
    dicovery start/stop, power on/off, etc).

    The second interface is 'org.freedesktop.DBus.ObjectManager' this is used
    to notify us when devices are added and removed from the adapter.

    Once the proxy objects are created, then we attach all the signals to our
    internal slots.

 */
bool BleRcuAdapterBluez::attachAdapter(const std::string &adapterPath)
{
    // create a proxy to the 'org.bluez.Adapter1' interface on the adapter object
    m_adapterProxy = std::make_shared<BluezAdapterInterface>(m_bluezService, adapterPath, m_bluezDBusConn);
    if (!m_adapterProxy) {
        XLOGD_ERROR("failed to create adapter proxy");
        return false;
    } else if (!m_adapterProxy->isValid()) {
        XLOGD_ERROR("failed to create adapter proxy, proxy invalid");
        m_adapterProxy.reset();
        return false;
    }

    // install handlers for the interesting adapter notifications
    m_adapterProxy->addPropertyChangedSlot("Discovering", 
            Slot<bool>(m_isAlive, 
                std::bind(&BleRcuAdapterBluez::onAdapterDiscoveringChanged, this, std::placeholders::_1)));
    m_adapterProxy->addPropertyChangedSlot("Pairable", 
            Slot<bool>(m_isAlive, 
                std::bind(&BleRcuAdapterBluez::onAdapterPairableChanged, this, std::placeholders::_1)));
    m_adapterProxy->addPropertyChangedSlot("Powered", 
            Slot<bool>(m_isAlive,
                std::bind(&BleRcuAdapterBluez::onAdapterPowerChanged, this, std::placeholders::_1)));

    return true;
}

// -----------------------------------------------------------------------------
/*!
    First checks if the adapter is already in discovery mode, if so it is
    cancelled, then it attempts to set the discovery filter so we only get
    bluetooth LE devices in the scan results.

 */
bool BleRcuAdapterBluez::setAdapterDiscoveryFilter()
{
    // check if the adapter is currently in discovery mode, stop it if so
    if (m_adapterProxy->discovering(m_discovering) && m_discovering) {

        XLOGD_WARN("bt adapter is unexpectedly already in discovery mode on start-up");

        string error;
        if (!m_adapterProxy->StopDiscoverySync(error)) {
            XLOGD_ERROR("failed to stop discovery due to <%s>",error.c_str());
            // not fatal, fall through
        }

        // get the new state, it should be false but just in case
        m_adapterProxy->discovering(m_discovering);

        // and also just in case, start the watchdog to ensure discovery has
        // stopped later
        m_discoveryRequested = StopDiscovery;
        if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
        m_discoveryWatchdogID = g_timeout_add(m_discoveryWatchdogTimeout, onDiscoveryWatchdog, this);
    }

    string error;
    XLOGD_DEBUG("setting discovery filter to only get bluetooth LE devices");
    if (!m_adapterProxy->SetDiscoveryFilterForLESync(error)) {
        XLOGD_ERROR("failed to set discovery filter due to <%s>", error.c_str());
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called at start-up to get the list of devices already available in the
    bluez daemon.

    For each device found we call addDevice().

 */
void BleRcuAdapterBluez::getRegisteredDevices()
{
    DBusManagedObjectList objects;
    if (false == m_bluezObjectMgr->GetManagedObjects(objects)) {
        XLOGD_WARN("failed to get managed objects");
        return;
    }

    DBusManagedObjectList::iterator object = objects.begin();
    for (; object != objects.end(); ++object) {

        // get the object path and interfaces
        const string &path = object->first;
        DBusInterfaceList &interfaces = object->second;

        DBusInterfaceList::iterator interface = interfaces.begin();
        for (; interface != interfaces.end(); ++interface) {

            // get the interface name and properties
            const string &name = interface->first;
            DBusPropertiesMap &properties = interface->second;

            // if this object has an 'org.bluez.Device1' interface then attempt to add the device
            if (name == BluezDeviceInterface::staticInterfaceName()) {
                XLOGD_INFO("device at path <%s> has interface %s, add it to our internal map", path.c_str(), BluezDeviceInterface::staticInterfaceName());

                // add the device to our internal map
                onDeviceAdded(path, properties);
            }
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the power changed property on the adapter is signaled. We
    hook this notification to detect if anyone has powered down the adapter,
    which would be fatal for the RCU.

 */
void BleRcuAdapterBluez::onAdapterPowerChanged(bool powered)
{
    if (powered) {
        XLOGD_INFO("BT adapter powered on");
        m_stateMachine.postDelayedEvent(AdapterPoweredOnEvent, 10);

    } else {
        XLOGD_INFO("odd, someone has powered down the BT adapter unexpectedly");
        m_stateMachine.postDelayedEvent(AdapterPoweredOffEvent, 100);
    }
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::isDiscovering()

    Returns \c true if the bluetooth adapter currently has discovery (scanning)
    enabled.

    \sa startDiscovery(), stopDiscovery(), discoveryChanged()
 */
bool BleRcuAdapterBluez::isDiscovering() const
{
    return m_discovering;
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::startDiscovery()

    Sends a dbus request to the bluez daemon to start discovery (aka scanning).
    The request is sent regardless of internal cached state, therefore an
    'operation already progress' error may be logged if trying to start discovery
    whilst already running.

    This request is asynchronous, to check it succeeded you should monitor the
    discoveryChanged() signal.

    \sa isDiscovering(), stopDiscovery(), discoveryChanged()
 */
bool BleRcuAdapterBluez::startDiscovery()
{
    if (!m_stateMachine.inState(AdapterPoweredOnState)) {
        XLOGD_ERROR("adapter not powered, can't start discovery");
        return false;
    }

    // set the expected discovery state for the watchdog
    m_discoveryRequested = StartDiscovery;

    // if already discovering don't send a request
    if (m_discovering) {
        return true;
    }

    // reset the discovery watchdog and increment the discovery pending count
    m_discoveryRequests++;

    if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
    m_discoveryWatchdogID = g_timeout_add(m_discoveryWatchdogTimeout, onDiscoveryWatchdog, this);

    XLOGD_DEBUG("starting discoveryWatchdog, m_discoveryRequests = %d, m_discoveryWatchdogID = %u", 
            m_discoveryRequests, m_discoveryWatchdogID);

    // otherwise send the request to start discovery
    m_adapterProxy->StartDiscovery(
            PendingReply<>(m_isAlive, std::bind(&BleRcuAdapterBluez::onStartDiscoveryReply, this, std::placeholders::_1)));

    return true;
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called with the reply to the request to start discovery. This slot is
    installed just for logging errors.

    \sa startDiscovery()
 */
void BleRcuAdapterBluez::onStartDiscoveryReply(PendingReply<> *reply)
{
    // reset the discovery watchdog and decrement the discovery pending count
    if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
    m_discoveryWatchdogID = g_timeout_add(m_discoveryWatchdogTimeout, onDiscoveryWatchdog, this);

    m_discoveryRequests--;
    XLOGD_DEBUG("starting discoveryWatchdog, m_discoveryRequests = %d", m_discoveryRequests);
    if(m_discoveryRequests == 0) {
        XLOGD_DEBUG("there is no outstanding discovery requests, let's stop discoveryWatchdog");
        if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
        m_discoveryWatchdogID = 0;
    }

    if (reply->isError()) {
        XLOGD_ERROR("discovery start request failed with error <%s>", reply->errorMessage().c_str());
    } else {
        XLOGD_DEBUG("discovery start request successful");
    }
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuAdapter::stopDiscovery()

    Sends a dbus request to the bluez daemon to stop discovery (aka scanning).
    The request is sent regardless of internal cached state, therefore an
    'operation not running' error may be logged if trying to stop discovery
    when it's not already running.

    This request is asynchronous, to check it succeeded you should monitor the
    discoveryChanged() signal.

    \sa startDiscovery(), isDiscovering(), discoveryChanged()
 */
bool BleRcuAdapterBluez::stopDiscovery()
{
    if (!m_stateMachine.inState(AdapterPoweredOnState)) {
        return false;
    }

    // set the expected discovery state for the watchdog
    m_discoveryRequested = StopDiscovery;

    // regardless of whether we think we are in the discovery mode or not
    // send the request to stop, this is a workaround for a bluetoothd issue
    // where it gets stuck in the 'starting' phase

    // reset the discovery watchdog and increment the discovery pending count
    m_discoveryRequests++;
    XLOGD_DEBUG("starting discoveryWatchdog, m_discoveryRequests = %d", m_discoveryRequests);
    if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
    m_discoveryWatchdogID = g_timeout_add(m_discoveryWatchdogTimeout, onDiscoveryWatchdog, this);

    // send the request to stop discovery
    m_adapterProxy->StopDiscovery(
            PendingReply<>(m_isAlive, std::bind(&BleRcuAdapterBluez::onStopDiscoveryReply, this, std::placeholders::_1)));

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called with the reply to the request to stop discovery. This slot is
    installed just for logging errors.

    \sa stopDiscovery()
 */
void BleRcuAdapterBluez::onStopDiscoveryReply(PendingReply<> *reply)
{
    // reset the discovery watchdog and decrement the discovery pending count
    if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
    m_discoveryWatchdogID = g_timeout_add(m_discoveryWatchdogTimeout, onDiscoveryWatchdog, this);

    m_discoveryRequests--;
    XLOGD_DEBUG("starting discoveryWatchdog, m_discoveryRequests = %d", m_discoveryRequests);
    if(m_discoveryRequests == 0) {
        XLOGD_DEBUG("there is no outstanding discovery requests, let's stop discoveryWatchdog");
        if (m_discoveryWatchdogID > 0) { g_source_remove(m_discoveryWatchdogID); }
        m_discoveryWatchdogID = 0;
    }

    if (reply->isError()) {
        XLOGD_ERROR("discovery stop request failed with error <%s>", reply->errorMessage().c_str());
    } else {
        XLOGD_DEBUG("discovery stop request successful");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called periodically to check that the current discovery / scan state
    matches what has been requested.  This is to work around issues in the
    bluetoothd daemon that causes Discovery Start / Stop requests to be delayed
    for a long time.

    The bluetoothd daemon can get commands backed up during pairing which
    means that Discovery Start / Stop are not acted on for up to 30 seconds
    which causes things like scans to start well after the pairing process
    has finished.  This time is used to cancel discovery that may have
    been left running

 */
static gboolean onDiscoveryWatchdog(gpointer user_data)
{
    BleRcuAdapterBluez *rcuAdapter = (BleRcuAdapterBluez*)user_data;
    if (rcuAdapter == nullptr) {
        return false;
    }
    XLOGD_DEBUG("Enter...");
    rcuAdapter->m_discoveryWatchdogID = 0;

    // wait for any outstanding requests to finish
    if (rcuAdapter->m_discoveryRequests > 0) {
        XLOGD_WARN("wait for any outstanding requests to finish");
        return false;
    }

    // check if the current discovery mode is in the correct state
    const bool requestedMode = (rcuAdapter->m_discoveryRequested == BleRcuAdapterBluez::StartDiscovery);
    if (rcuAdapter->m_discovering != requestedMode) {

        XLOGD_ERROR("detected discovery in the wrong state (expected:%s actual:%s)",
               (rcuAdapter->m_discoveryRequested == BleRcuAdapterBluez::StartDiscovery) ? "on" : "off",
               rcuAdapter->m_discovering ? "on" : "off");

        // in the wrong state so Start / Stop discovery
        if (rcuAdapter->m_discoveryRequested == BleRcuAdapterBluez::StartDiscovery) {
            rcuAdapter->startDiscovery();
        } else {
            rcuAdapter->stopDiscovery();
        }
    }
    return false;
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::isPairable()

    Returns \c true if the bluetooth adapter currently has pairable enabled.

    \sa disablePairable(), enablePairable(), pairableChanged()
 */
bool BleRcuAdapterBluez::isPairable() const
{
    return m_pairable;
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::enablePairable()

    Enables the pairable flag on the adapter with the given \a timeout in
    milliseconds.

    This method sends two dbus requests to the bluez daemon to first set the
    pairable timeout and then to set the pairable property to \c true.

    \sa disablePairable(), isPairable(), pairableChanged()
 */
bool BleRcuAdapterBluez::enablePairable(unsigned int timeout)
{
    if (!m_stateMachine.inState(AdapterPoweredOnState)) {
        return false;
    }

    XLOGD_INFO("enabling pairable mode for %d seconds", timeout / 1000);

    // TODO: should we switch to non-blocking propery write ?
    m_adapterProxy->setPairableTimeout(timeout / 1000);
    m_adapterProxy->setPairable(true);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::disablePairable()

    Disables the pairable flag on the adapter.

    This method sends a dbus request to the bluez daemon to set the pairable
    property to \c false.

    \sa enablePairable(), isPairable(), pairableChanged()
 */
bool BleRcuAdapterBluez::disablePairable()
{
    if (!m_stateMachine.inState(AdapterPoweredOnState)) {
        return false;
    }

    // if any of our devices are in the pairing state then cancel it
    for (auto const &device : m_devices) {
        if (device.second->isPairing()) {
            device.second->cancelPairing();
        }
    }

    XLOGD_INFO("disabling pairable mode");

    // TODO: should we switch to non-blocking property write ?
    m_adapterProxy->setPairable(false);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::getDevice()

    Returns a shared pointer to the device with the given \a address. If the
    device is unknown then an empty / invalid shared pointer is returned.

    \sa deviceNames()
 */
std::shared_ptr<BleRcuDevice> BleRcuAdapterBluez::getDevice(const BleAddress &address) const
{

    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.find(address);
    const shared_ptr<BleRcuDeviceBluez> device = (it == m_devices.end()) ? nullptr : it->second;
    if (!device || !device->isValid()) {
        XLOGD_INFO("failed to find device with address %s", address.toString().c_str());
        return make_shared<BleRcuDeviceBluez>();
    }

    return device;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::pairedDevices()

    Returns the set of all currently paired devices.

    \sa deviceNames(), getDevice()
 */
std::set<BleAddress> BleRcuAdapterBluez::pairedDevices() const
{
    set<BleAddress> paired;

    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.begin();
    for (; it != m_devices.end(); ++it) {

        const shared_ptr<BleRcuDeviceBluez> &device = it->second;
        if (device && device->isValid() && device->isPaired()) {
            paired.insert(it->first);
        }
    }
    return paired;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuManager::deviceNames()

    Returns map of device names stored against their address.

    \sa pairedDevices(), getDevice()
 */
std::map<BleAddress, std::string> BleRcuAdapterBluez::deviceNames() const
{
    map<BleAddress, string> namesMap;

    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.begin();
    for (; it != m_devices.end(); ++it) {

        const shared_ptr<BleRcuDeviceBluez> &device = it->second;
        if (device && device->isValid()) {
            namesMap[it->first] = device->name();
        }
    }
    return namesMap;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuManager::isDevicePaired(const BleAddress &address)

    Returns \c true if the device with the given \a address is paired.  This
    request works on cached values, it's possible it may not be accurate if
    an unpair notification is yet to arrive over dbus.

    \sa pairDevice(), removeDevice()
 */
bool BleRcuAdapterBluez::isDevicePaired(const BleAddress &address) const
{
    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.find(address);
    const shared_ptr<BleRcuDeviceBluez> device = (it == m_devices.end()) ? nullptr : it->second;
    if (!device || !device->isValid()) {

        XLOGD_INFO("failed to find device with address %s to query paired status", address.toString().c_str());
        return false;
    }

    return device->isPaired();
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuManager::isDeviceConnected(const BleAddress &address)

    Returns \c true if the device with the given \a address is connected.  This
    request works on cached values, it's possible it may not be accurate if
    an unpair notification is yet to arrive over dbus.

 */
bool BleRcuAdapterBluez::isDeviceConnected(const BleAddress &address) const
{
    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.find(address);
    const shared_ptr<BleRcuDeviceBluez> device = (it == m_devices.end()) ? nullptr : it->second;
    if (!device || !device->isValid()) {

        XLOGD_INFO("failed to find device with address %s to query connected status", address.toString().c_str());
        return false;
    }

    return device->isConnected();
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuManager::addDevice(const BleAddress &address)

    Sends a request to the bluez daemon to pair the device with the given
    \a address.  The request is sent even if the device is already paired,
    this is to handle the case where a pending unpair notification is sitting
    in the dbus queue but not yet processed.

    This request is asynchronous.

    \sa isDevicePaired(), removeDevice()
 */
bool BleRcuAdapterBluez::addDevice(const BleAddress &address)
{
    if (!m_stateMachine.inState(AdapterPoweredOnState)) {
        return false;
    }

    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.find(address);

    const shared_ptr<BleRcuDeviceBluez> device = (it == m_devices.end()) ? nullptr : it->second;

    if (!device || !device->isValid()) {
        XLOGD_INFO("failed to find device with address %s to pair", address.toString().c_str());
        return false;
    }

    XLOGD_INFO("requesting bluez pair %s", device->address().toString().c_str());


    device->addPairingErrorSlot(Slot<const std::string&>(m_isAlive,
            std::bind(&BleRcuAdapterBluez::onDevicePairingError, this, address, std::placeholders::_1)));

    device->pair(0);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to bluez to reconnect all devices stored in our internal map

 */
void BleRcuAdapterBluez::reconnectAllDevices()
{
    for (auto const &device : m_devices) {
        XLOGD_INFO("reconnecting to %s", device.first.toString().c_str());
        device.second->connect();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called whenever the 'discovering' state changes on the bluetooth adapter.
    This is used to move the state machine on, or report an error if this
    occurs in the wrong state.

    This is a callback from the dbus notification of the discovering change.

 */
void BleRcuAdapterBluez::onAdapterDiscoveringChanged(bool discovering)
{
    XLOGD_INFO("adapter %s discovering", discovering ? "started" : "stopped");

    // skip out early if nothings actually changed
    if (m_discovering == discovering) {
        return;
    }

    // set the new state then emit a signal
    m_discovering = discovering;
    m_discoveryChangedSlots.invoke(m_discovering);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called whenever the 'pairable' state changes on the bluetooth adapter.

    This is a callback from the dbus notification of the pairable property
    change.

 */
void BleRcuAdapterBluez::onAdapterPairableChanged(bool pairable)
{
    XLOGD_INFO("adapter pairable state changed to %s", pairable ? "TRUE" : "FALSE");

    // skip out early if nothings actually changed
    if (m_pairable == pairable) {
        return;
    }

    // set the new state then emit a signal
    m_pairable = pairable;
    m_pairableChangedSlots.invoke(m_pairable);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a device is added to the bluez adapter interface.

    A device is typically added once discovery is started, when a device is
    added it is typically not paired or connected.

    The properties supplied should match all the values that can be queried /
    modified on the 'org.bluez.Device1' interface. For example the following
    is the properties dump taken from a dbus-monitor session.

    \code
        array [ dict entry( string "Address"
                            variant string "60:03:08:CE:49:F3")
                dict entry( string "Alias"
                            variant string "60-03-08-CE-49-F3" )
                dict entry( string "Paired"
                            variant boolean false )
                dict entry( string "Trusted"
                            variant boolean false )
                dict entry( string "Blocked"
                            variant boolean false )
                dict entry( string "LegacyPairing"
                            variant boolean false )
                dict entry( string "RSSI"
                            variant int16 -77 )
                dict entry( string "Connected"
                            variant boolean false )
                dict entry( string "UUIDs"
                            variant array [ ] )
                dict entry( string "Adapter"
                            variant object path "/org/bluez/hci0" )
         ]
    \endcode

    This function is called for all manor of devices, so to filter out only
    RCUs we use the BDADDR to match only ruwido remotes.

 */
void BleRcuAdapterBluez::onDeviceAdded(const std::string &path,
                                       const DBusPropertiesMap &properties)
{
    // it's unlikely but possible that we already have this device stored, as
    // this function may be called at start-up when we've queried the bluez
    // daemon but the signal handlers are also installed.  Anyway it just means
    // we should ignore this call, it's not an error
    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.begin();
    for (; it != m_devices.end(); ++it) {
        if (it->second->bluezObjectPath() == path) {
            return;
        }
    }


    // we only require the "Adapter" and "Address" properties from the
    // notification, the rest of the details (like whether it's paired /
    // connected) are read by the newly created BleRcuDevice.


    // get the adapter path and verify the device is attached to our adapter
    // (in reality there should only be one adapter, but doesn't hurt to check)
    string adapterPath;
    DBusPropertiesMap::const_iterator property = properties.find("Adapter");
    if ((property == properties.end()) || !property->second.toObjectPath(adapterPath)) {
        XLOGD_WARN("device 'Adapter' property is missing or invalid");
        return;
    }
    if (adapterPath != m_adapterObjectPath) {
        XLOGD_WARN("odd, the device added doesn't belong to our adapter");
        return;
    }

    // get the device address
    string bdaddrStr;
    property = properties.find("Address");
    if ((property == properties.end()) || !property->second.toString(bdaddrStr)) {
        XLOGD_WARN("device 'Address' property is missing or invalid");
        return;
    }
    // convert the address to a BDADDR object
    const BleAddress bdaddr(bdaddrStr);
    if (bdaddr.isNull()) {
        XLOGD_WARN("failed to parse the device address (%s)", property->second.dumpToString().c_str());
        return;
    }

    // get the device name
    string name;
    property = properties.find("Name");
    if ((property == properties.end()) || !property->second.toString(name)) {
        XLOGD_DEBUG("device 'Name' property is missing or invalid");
    }

    bool match = false;
    for (const auto &supportedName : m_supportedPairingNames) {
        if (std::regex_match(name.c_str(), supportedName)) {
            XLOGD_INFO("found pairable device %s with name %s", bdaddr.toString().c_str(), name.c_str());
            match = true;
            break;
        }
    }
    if (!match) {
        XLOGD_DEBUG("device with address %s, and name: %s is not an RCU, so ignoring.",
                bdaddr.toString().c_str(), name.c_str());
        return;
    }

    // get the connected and paired properties
    bool connected = false;
    property = properties.find("Connected");
    if ((property == properties.end()) || !property->second.toBool(connected)) {
        XLOGD_WARN("device 'Connected' property is missing or invalid");
    }

    bool paired = false;
    property = properties.find("Paired");
    if ((property == properties.end()) || !property->second.toBool(paired)) {
        XLOGD_WARN("device 'Paired' property is missing or invalid");
    }


    // so we now have all the params we need, create a device object to manage
    // the (RCU) device
    std::shared_ptr<BleRcuDeviceBluez> device =
            std::make_shared<BleRcuDeviceBluez> (bdaddr, name,
                                                 m_bluezDBusConn, path,
                                                 m_servicesFactory,
                                                 m_GMainLoop);
    if (!device || !device->isValid()) {
        XLOGD_WARN("failed to create device with bdaddr %s", bdaddr.toString().c_str());
        return;
    }

    // connect up the signals from the device, we use functors to bind the
    // device bdaddr in with the slot callback
    device->addNameChangedSlot(Slot<const std::string&>(m_isAlive,
            std::bind(&BleRcuAdapterBluez::onDeviceNameChanged, this, bdaddr, std::placeholders::_1)));
    device->addPairedChangedSlot(Slot<bool>(m_isAlive, 
            std::bind(&BleRcuAdapterBluez::onDevicePairedChanged, this, bdaddr, std::placeholders::_1)));
    device->addReadyChangedSlot(Slot<bool>(m_isAlive, 
            std::bind(&BleRcuAdapterBluez::onDeviceReadyChanged, this, bdaddr, std::placeholders::_1)));

    // add the device to the list
    m_devices[bdaddr] = device;

    XLOGD_INFO("added device %s named %s (connected: %s, paired: %s)", 
            bdaddr.toString().c_str(), name.c_str(), connected ? "TRUE" : "FALSE", paired ? "TRUE" : "FALSE");

    if (paired && !connected) {
        XLOGD_INFO("Device paired but not connected, sending connection request to bluez...");
        device->connect();
    }

    m_deviceFoundSlots.invoke(device->address(), device->name());
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an object managed by bluez was added.

    The function checks that one of the interfaces added for the object is
    'org.bluez.Device1' and if so then it calls onDevicedAdded() to add the
    device to our internal map.

 */
void BleRcuAdapterBluez::onBluezInterfacesAdded(const std::string &objectPath,
                                                const DBusInterfaceList &interfacesAndProperties)
{
    // loop through the interfaces added
    DBusInterfaceList::const_iterator it = interfacesAndProperties.begin();
    for (; it != interfacesAndProperties.end(); ++it) {

        const string &interface = it->first;
        const DBusPropertiesMap &properties = it->second;

        // XLOGD_DEBUG("received interface %s added event", interface.c_str());

        if (interface == BluezDeviceInterface::staticInterfaceName()) {
            // if the interface is 'org.bluez.Device1' add the device
            onDeviceAdded(objectPath, properties);
        } else if (interface == BluezAdapterInterface::staticInterfaceName()) {
            // if the interface is 'org.bluez.Adapter1' trigger a retry
            XLOGD_DEBUG("trigger a retry because Adapter1 interface added");
            m_stateMachine.postDelayedEvent(AdapterRetryAttachEvent, 10);
        }
    }
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an object with interface org.bluez.Device1 was removed.

    We use the object path to find the device to remove, it's not an error if
    we don't have a device stored for the given path as we only store devices
    that have a BDADDR that matches an RCU.

 */
void BleRcuAdapterBluez::onDeviceRemoved(const std::string &objectPath)
{
    // check if we have an RCU device at the given dbus path
    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::iterator it = m_devices.begin();
    for (; it != m_devices.end(); ++it) {
        if (it->second->bluezObjectPath() == objectPath) {
            break;
        }
    }

    // its not an error if the removed device is not in our map
    if (it == m_devices.end()) {
        XLOGD_DEBUG("Device removed at path <%s> not managed by us, doing nothing...", objectPath.c_str());
        return;
    }


    // get the BDADDR of the device we're removing
    const BleAddress bdaddr = it->first;

    // check if it was paired
    const bool wasPaired = it->second->isPaired();

    XLOGD_INFO("removed device %s", bdaddr.toString().c_str());

    // remove the device from the map and send a signal saying the device
    // has disappeared
    m_devices.erase(it);

    // if was paired then we clearly no longer are so emit a signal
    if (wasPaired) {
        m_devicePairingChangedSlots.invoke(bdaddr, false);
    }

    m_deviceRemovedSlots.invoke(bdaddr);
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an object managed by bluez is removed.

    The function checks that one of the interfaces for the removed object is
    'org.bluez.Device1' and if so then it calls onDeviceRemoved() to remove the
    device from our internal map.
 */
void BleRcuAdapterBluez::onBluezInterfacesRemoved(const std::string &objectPath,
                                                  const std::vector<std::string> &interfaces)
{
    // XLOGD_DEBUG("received interface(s) removed event from <%s>", objectPath.c_str());

    // check if it's the adapter that was removed (this should never really happen) 
    // otherwise check if one of the interfaces being removed is 'org.bluez.Device1'
    if (objectPath == m_adapterObjectPath) {
        XLOGD_WARN("adapter interface <%s> removed!!!!", m_adapterObjectPath.c_str());

        // Adapter powered off event can come before this, so make sure those delayed events are cancelled.
        // It will be powered on later through the AdapterUnavailableEvent path
        m_stateMachine.cancelDelayedEvents(AdapterPoweredOffEvent);

        // Clear the adapter path now so that we know to ignore other interface removed notifications later.
        // The adapter proxy will be cleared during next state transition.
        m_adapterObjectPath = "";

        m_stateMachine.postDelayedEvent(AdapterUnavailableEvent, 10);

    } else if (std::find(interfaces.begin(), interfaces.end(), BluezDeviceInterface::staticInterfaceName()) != interfaces.end()) {
        if (m_adapterObjectPath.empty()) {
            XLOGD_WARN("Don't have a valid adapter, so ignoring <%s> interface removed notification...", objectPath.c_str());
        } else {
            onDeviceRemoved(objectPath);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuManager::removeDevice(const BleAddress &address)

    Sends a request over dbus to the bluez daemon to remove the device, this
    has the effect of disconnecting and unpairing any device (if it was
    connected and paired).

    This request is asynchronous.

    \sa pairDevice(), isDevicePaired()
 */
bool BleRcuAdapterBluez::removeDevice(const BleAddress &address)
{
    if (!m_stateMachine.inState(AdapterAvailableSuperState)) {
        return false;
    }

    // find the device with the address in our map, this is so we can cancel
    // pairing if currently in the pairing procedure
    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::iterator it = m_devices.find(address);

    const shared_ptr<BleRcuDeviceBluez> device = (it == m_devices.end()) ? nullptr : it->second;
    if (!device || !device->isValid()) {
        XLOGD_INFO("failed to find device with address %s to remove", address.toString().c_str());
        return false;
    }

    XLOGD_INFO("requesting bluez remove %s", device->address().toString().c_str());

    // if currently pairing then cancel it before removing the device
    if (device->isPairing()) {
        device->cancelPairing();
    }

    // Ask the adapter to remove this device since if failed pairing.
    // We add a listener on the result just so we can log any errors.
    m_adapterProxy->RemoveDevice(device->bluezObjectPath(), 
            PendingReply<>(m_isAlive, std::bind(&BleRcuAdapterBluez::onRemoveDeviceReply, this, std::placeholders::_1)));

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we get a response to our asynchronous request to remove a device
    from the adapter.  We only hook this point so we can log any errors.

    \see removeDevice()
 */
void BleRcuAdapterBluez::onRemoveDeviceReply(PendingReply<> *reply)
{
    // check for error
    if (reply->isError()) {
        XLOGD_ERROR("remove device request failed with error <%s>", reply->errorMessage().c_str());
    } else {
        XLOGD_DEBUG("remove device request successful");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Event signaled by an \l{BleRcuDevice} when it detects it's name has changed.

    We need this info for pairing as an already known device may change it's
    name when it enters pairing mode.
 */
void BleRcuAdapterBluez::onDeviceNameChanged(const BleAddress &address,
                                             const std::string &name)
{
    XLOGD_INFO("renamed device %s to %s", address.toString().c_str(), name.c_str());

    m_deviceNameChangedSlots.invoke(address, name);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Event signaled by an \l{BleRcuDevice} when a pairing request returns an error.

 */
void BleRcuAdapterBluez::onDevicePairingError(const BleAddress &address,
                                             const std::string &error)
{
    m_devicePairingErrorSlots.invoke(address, error);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Event signaled by an \l{BleRcuDevice} when it detects it's 'paired' state
    has changed.

    Although this is not a necessity but helpful for the pairing state machine.
 */
void BleRcuAdapterBluez::onDevicePairedChanged(const BleAddress &address,
                                               bool paired)
{
    // nb: already logged as milestone in BleRcuDeviceImpl, don't log again

    m_devicePairingChangedSlots.invoke(address, paired);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Event signaled by a \t{BleRcuDevice} object when it's ready state has
    changed. The 'ready' state implies that the device is paired, connected, and
    has gone through the initial setup such that it is now ready.

    This notification is passed on to the coupling state machine if it's running
    and the device is now 'ready'.

    This is also the point were we check the number of devices we have in the
    'paired' state, if it exceeds the maximum allowed then we remove the last
    device to enter the 'ready' state.

 */
void BleRcuAdapterBluez::onDeviceReadyChanged(const BleAddress &address,
                                              bool ready)
{
    XLOGD_INFO("device with address %s is %sREADY", address.toString().c_str(), ready ? "" : "NOT ");

    map<BleAddress, shared_ptr<BleRcuDeviceBluez>>::const_iterator it = m_devices.find(address);

    if (ready && it != m_devices.end()) {
        if (!m_hciSocket || !m_hciSocket->isValid()) {
            m_hciSocket.reset();
            m_hciSocket = HciSocket::create(0, -1);
        }

        if (m_hciSocket && m_hciSocket->isValid()) {
            // Find the HCI handle and send VSC to BT/Wifi chip
            const auto deviceInfos = m_hciSocket->getConnectedDevices();
            for (const auto &deviceInfo : deviceInfos) {
                XLOGD_INFO("found connected device %s", deviceInfo.address.toString().c_str());

                if (address == deviceInfo.address) {
                    XLOGD_INFO("HCI connection handle: %u, device: %s, sending " 
                        "VSC to increase BT data capability in the chip BT/WIFI coexistence engine. ",
                        deviceInfo.handle, deviceInfo.address.toString().c_str());

                    m_hciSocket->sendIncreaseDataCapability(deviceInfo.handle);
                }
            }
        }
    }
    m_deviceReadyChangedSlots.invoke(address, ready);
}


bool BleRcuAdapterBluez::setConnectionParams(BleAddress address, double minInterval, double maxInterval,
                                             int32_t latency, int32_t supervisionTimeout)
{
    if (!m_hciSocket || !m_hciSocket->isValid()) {
        m_hciSocket.reset();
        m_hciSocket = HciSocket::create(0, -1);
    }

    if (m_hciSocket && m_hciSocket->isValid()) {
        BleConnectionParameters desiredParams(minInterval, maxInterval, latency, supervisionTimeout);
        const auto deviceInfos = m_hciSocket->getConnectedDevices();

        for (const auto &deviceInfo : deviceInfos) {
            XLOGD_INFO("found connected device %s", deviceInfo.address.toString().c_str());

            if (address == deviceInfo.address) {

                XLOGD_INFO("HCI connection handle: %u, device: %s requesting an update of connection parameters to " 
                        "minInterval=%f, maxInterval=%f, latency=%d, supervisionTimeout=%d",
                        deviceInfo.handle, deviceInfo.address.toString().c_str(),
                        minInterval, maxInterval, latency, supervisionTimeout);

                if (m_hciSocket->requestConnectionUpdate(deviceInfo.handle, desiredParams)) {
                    return true;
                } else {
                    XLOGD_ERROR("failed to update connection parameters");
                    return false;
                }
            }
        }
    } else {
        XLOGD_ERROR("HCI socket is NULL");
        return false;
    }

    XLOGD_ERROR("failed to find HCI connection handle for %s", address.toString().c_str());
    return false;
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Recovery method, not expected to be called unless something has gone
    seriously wrong.

    This is called whenever someone calls the following
    \code
        emit bleRcuRecovery->powerCycleAdapter();
    \endcode

 */
void BleRcuAdapterBluez::onPowerCycleAdapter()
{
    XLOGD_WARN("deliberately power cycling the adapter to try and recover from error state");

    if (!m_adapterProxy) {
        XLOGD_ERROR("bluez not available so can't power cycle the adapter");
        return;
    }

    m_adapterProxy->setPowered(false);

    // if everything is not completely hosed, this will trigger a power event
    // from bluetoothd daemon that will detected by onAdapterPowerChanged()
    // which will in turn schedule a re-power event
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Recovery method, not expected to be called unless something has gone
    seriously wrong.

    This is called whenever someone calls the following
    \code
        emit bleRcuRecovery->reconnectDevice(m_address);
    \endcode

 */
void BleRcuAdapterBluez::onDisconnectReconnectDevice(const BleAddress &device)
{
    // TODO: implement if needed, it is tricky and power cycling is generally
    // the better option
#if 1

    XLOGD_ERROR("recovery method not implemented, use power cycle instead");

#else
    qMilestone() << "deliberately disconnecting / reconnecting to device"
                 << device << "to try and recover from error state";

    if (!m_devices.contains(device)) {
        qError() << "failed to find device" << device << "to try and recover";
        return;
    }

    const QSharedPointer<BleRcuDeviceBluez> device_ = m_devices[device];
    if (!device_ || !device_->isValid()) {
        qError() << "failed to find device" << device << "to try and recover";
        return;
    }

    // TODO

#endif
}

