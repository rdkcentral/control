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
//  blercudevice.cpp
//

#include "blercudevice_p.h"
#include "blegattprofile_p.h"
#include "blegattservice_p.h"
#include "blegattcharacteristic_p.h"
#include "blegattdescriptor_p.h"

#include "blercu/bleservices/blercuservices.h"
#include "blercu/bleservices/blercuservicesfactory.h"

#include "blercu/bleservices/blercuaudioservice.h"
#include "blercu/bleservices/blercubatteryservice.h"
#include "blercu/bleservices/blercudeviceinfoservice.h"
#include "blercu/bleservices/blercuinfraredservice.h"
#include "blercu/bleservices/blercuupgradeservice.h"
#include "blercu/bleservices/blercufindmeservice.h"
#include "blercu/bleservices/blercuremotecontrolservice.h"

#include "interfaces/bluezdeviceinterface.h"

#include "ctrlm_log_ble.h"


#include <functional>

using namespace std;

class BleRcuDeviceBluez_userdata {
public:
    BleRcuDeviceBluez_userdata(shared_ptr<bool> isAlive_, BleRcuDeviceBluez* rcuDevice_)
        : isAlive(std::move(isAlive_))
        , rcuDevice(rcuDevice_)
    { }

    shared_ptr<bool> isAlive;
    BleRcuDeviceBluez* rcuDevice;
};

static gboolean getInitialDeviceProperties(gpointer user_data);



BleRcuDeviceBluez::BleRcuDeviceBluez()
    : m_isAlive(make_shared<bool>(true))
    , m_bluezObjectPath(string())
    , m_address(BleAddress())
    , m_name("")
    , m_lastConnectedState(false)
    , m_lastPairedState(false)
    , m_lastServicesResolvedState(false)
    , m_isPairing(false)
    , m_timeSinceReady(0)
    , m_recoveryAttempts(0)
    , m_maxRecoveryAttempts(100)
{
}

BleRcuDeviceBluez::BleRcuDeviceBluez(const BleAddress &bdaddr,
                                     const std::string &name,
                                     const GDBusConnection *bluezDBusConn,
                                     const std::string &bluezDBusPath,
                                     const std::shared_ptr<BleRcuServicesFactory> &servicesFactory,
                                     GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_bluezObjectPath(bluezDBusPath)
    , m_address(bdaddr)
    , m_name(name)
    , m_lastConnectedState(false)
    , m_lastPairedState(false)
    , m_lastServicesResolvedState(false)
    , m_isPairing(false)
    , m_timeSinceReady(0)
    , m_recoveryAttempts(0)
    , m_maxRecoveryAttempts(100)
{

    // initialise and start the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    setupStateMachine();

    // initialise the dbus interface to bluez
    if (!init(bluezDBusConn, bluezDBusPath)) {
        return;
    }

    // create an empty GATT profile, this will be populated when the services
    // are resolved
    m_gattProfile = std::make_shared<BleGattProfileBluez>(bluezDBusConn, bluezDBusPath);

    // create the services object for the device (this may fail if there is
    // no daemon on the box to support the given device)
    m_services = servicesFactory->createServices(m_address, m_gattProfile, m_name, mainLoop);
    if (!m_services) {
        XLOG_WARN("failed to create services for %s, name %s", bdaddr.toString().c_str(), m_name.c_str());
        return;
    }
}

BleRcuDeviceBluez::~BleRcuDeviceBluez()
{
    *m_isAlive = false;

    // we don't have to do this here, the tear down will automatically free
    // the services, however may help with debugging
    if (m_services) {
        m_services->stop();
        m_services.reset();
    }

    if (m_timeSinceReady) {
        g_timer_destroy(m_timeSinceReady);
    }
    m_deviceProxy.reset();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Initialise the proxy to the bluez device over dbus.


 */
bool BleRcuDeviceBluez::init(const GDBusConnection *bluezDBusConn,
                             const std::string &bluezDBusPath)
{
    // create a proxy to the 'org.bluez.Device1' interface on the device object
    m_deviceProxy = std::make_shared<BluezDeviceInterface>("org.bluez", bluezDBusPath, bluezDBusConn);
    if (!m_deviceProxy) {
        XLOGD_ERROR("failed to create device proxy");
        return false;
    } else if (!m_deviceProxy->isValid()) {
        XLOGD_ERROR("failed to create device proxy, proxy invalid");
        m_deviceProxy.reset();
        return false;
    }


    // connect to the property change notifications from the daemon
    m_deviceProxy->addPropertyChangedSlot("Connected", 
            Slot<bool>(m_isAlive, std::bind(&BleRcuDeviceBluez::onDeviceConnectedChanged, this, std::placeholders::_1)));
    m_deviceProxy->addPropertyChangedSlot("Paired", 
            Slot<bool>(m_isAlive, std::bind(&BleRcuDeviceBluez::onDevicePairedChanged, this, std::placeholders::_1)));
    m_deviceProxy->addPropertyChangedSlot("ServicesResolved",
            Slot<bool>(m_isAlive, std::bind(&BleRcuDeviceBluez::onDeviceServicesResolvedChanged, this, std::placeholders::_1)));
    m_deviceProxy->addPropertyChangedSlot("Name", 
            Slot<const std::string&>(m_isAlive, std::bind(&BleRcuDeviceBluez::onDeviceNameChanged, this, std::placeholders::_1)));


    // schedule an event next time through the event loop to go and fetch the
    // initial state of the paired / connected and serviceResolved properties
    BleRcuDeviceBluez_userdata *userData = new BleRcuDeviceBluez_userdata (m_isAlive, this);
    g_timeout_add(0, getInitialDeviceProperties, userData);

    return true;
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Called from the main event loop after we've been initialised, used to get
     the initial states for the state machine.

 */
static gboolean getInitialDeviceProperties(gpointer user_data)
{
    BleRcuDeviceBluez_userdata *userData = (BleRcuDeviceBluez_userdata*)user_data;
    if (userData == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
        return false;
    } else if (*userData->isAlive == false) {
        XLOGD_ERROR("BleRcuDeviceBluez object is not valid!!!!");
        delete userData;
        return false;
    }

    // get the initial connected and paired status and the name
    bool connected = false;
    bool paired = false;
    bool servicesResolved = false;
    
    userData->rcuDevice->m_deviceProxy->connected(connected);
    userData->rcuDevice->m_deviceProxy->paired(paired);
    userData->rcuDevice->m_deviceProxy->servicesResolved(servicesResolved);


    // simulate the parameters change as if they were notifications, this will
    // move the state machine into the correct initial place
    // (nb: the order of the following calls is important)
    userData->rcuDevice->onDeviceConnectedChanged(connected);
    userData->rcuDevice->onDevicePairedChanged(paired);
    userData->rcuDevice->onDeviceServicesResolvedChanged(servicesResolved);
    
    delete userData;
    
    return false;
}

// -----------------------------------------------------------------------------
/*!
    Sends a pairing request to the bluez daemon.  Note this doesn't directly
    affect the state machine, this can be called from any state.  The state
    machine responses to events for bluez.

    The \a timeout value is the number of milliseconds to wait before issuing
    a 'cancel pairing' request to the bluez daemon.  The timer is automatically
    cancelled when we receive an event that shifts unpaired to paired.

    Lastly this is an asynchronous call, if an error happens with the pair
    request then the \a pairingError signal will be emitted.

 */
void BleRcuDeviceBluez::pair(int timeout)
{
    m_deviceProxy->Pair(
            PendingReply<>(m_isAlive, std::bind(&BleRcuDeviceBluez::onPairRequestReply, this, std::placeholders::_1)));

    // set the flag, it may be cleared if the call fails
    m_isPairing = true;
}

// -----------------------------------------------------------------------------
/*!
    Called when we get a reply to the dbus message to pair this device. We
    use this to detect any errors that we just log and emit a signal to
    anyone who's interested.

 */
void BleRcuDeviceBluez::onPairRequestReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        m_isPairing = false;

        // an error occurred so log it
        XLOGD_ERROR("%s pairing request failed with error: <%s>", 
                m_address.toString().c_str(), reply->errorMessage().c_str());

        // emit pairingError(m_address, error.message());
    } else {
        XLOGD_DEBUG("%s pairing request successful", m_address.toString().c_str());
        // TODO: start the timer to cancel the pairing after a certain amount of time
    }
}

// -----------------------------------------------------------------------------
/*!
    Sends a request to the bluez daemon to cancel paring.

 */
void BleRcuDeviceBluez::cancelPairing()
{
    XLOGD_INFO("canceling pairing for device %s", m_address.toString().c_str());

    m_deviceProxy->CancelPairing(
            PendingReply<>(m_isAlive, std::bind(&BleRcuDeviceBluez::onCancelPairingRequestReply, this, std::placeholders::_1)));

    // regardless of whether the cancel succeeds we clear the isPairing flag
    m_isPairing = false;
}

// -----------------------------------------------------------------------------
/*!
    Called when we get a reply to the dbus message to cancel the pairing of this
    device. We use this to detect any errors that we just log and emit a signal
    to anyone who's interested.

 */
void BleRcuDeviceBluez::onCancelPairingRequestReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        // an error occurred so log it
        XLOGD_ERROR("%s cancel pairing request failed with error: <%s>", 
                m_address.toString().c_str(), reply->errorMessage().c_str());
        // emit pairingError(m_address, error.message());
    } else {
        XLOGD_DEBUG("%s cancel pairing request successful", m_address.toString().c_str());
        // TODO: start the timer to cancel the pairing after a certain amount of time
    }
}

// -----------------------------------------------------------------------------
/*!
    Sends a request to the bluez daemon to connect to the device.

 */
void BleRcuDeviceBluez::connect()
{
    XLOGD_INFO("connecting to device %s", m_address.toString().c_str());
    m_deviceProxy->Connect(
            PendingReply<>(m_isAlive, std::bind(&BleRcuDeviceBluez::onConnectRequestReply, this, std::placeholders::_1)));
}
// -----------------------------------------------------------------------------
/*!
    Called when we get a reply to the dbus message to connect to this
    device. We use this to detect any errors that we just log and emit a signal
    to anyone who's interested.

 */
void BleRcuDeviceBluez::onConnectRequestReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        // an error occurred so log it and emit an error signal
        XLOGD_ERROR("%s connect request failed with error: <%s>",
                m_address.toString().c_str(), reply->errorMessage().c_str());
    } else {
        XLOGD_INFO("%s connect request successful", m_address.toString().c_str());
    }
}
// -----------------------------------------------------------------------------
/*!



 */
void BleRcuDeviceBluez::setupStateMachine()
{
    // set the name of the state machine for logging
    m_stateMachine.setObjectName("DeviceStateMachine");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(PairedState, "Paired");
    m_stateMachine.addState(ConnectedState, "Connected");
    m_stateMachine.addState(ResolvingServicesState, "ResolvingServices");

    m_stateMachine.addState(RecoverySuperState, "RecoverySuperState");
    m_stateMachine.addState(RecoverySuperState, RecoveryDisconnectingState, "RecoveryDisconnecting");
    m_stateMachine.addState(RecoverySuperState, RecoveryReconnectingState, "RecoveryReconnecting");

    m_stateMachine.addState(SetupSuperState, "SetupSuperState");
    m_stateMachine.addState(SetupSuperState, StartingServicesState, "StartingServices");
    m_stateMachine.addState(SetupSuperState, ReadyState, "ReadyState");


    // set the initial state of the state machine
    m_stateMachine.setInitialState(IdleState);

    // add the transitions:      From State            ->   Event                    ->  To State
    m_stateMachine.addTransition(IdleState,                 DevicePairedEvent,           PairedState);
    m_stateMachine.addTransition(IdleState,                 DeviceConnectedEvent,        ConnectedState);

    m_stateMachine.addTransition(PairedState,               DeviceUnpairedEvent,         IdleState);
    m_stateMachine.addTransition(PairedState,               DeviceConnectedEvent,        ResolvingServicesState);

    m_stateMachine.addTransition(ConnectedState,            DeviceDisconnectedEvent,     IdleState);
    m_stateMachine.addTransition(ConnectedState,            DevicePairedEvent,           ResolvingServicesState);

    m_stateMachine.addTransition(ResolvingServicesState,    DeviceDisconnectedEvent,     PairedState);
    m_stateMachine.addTransition(ResolvingServicesState,    DeviceUnpairedEvent,         ConnectedState);
    m_stateMachine.addTransition(ResolvingServicesState,    ServicesResolvedEvent,       StartingServicesState);
    m_stateMachine.addTransition(ResolvingServicesState,    ServicesResolveTimeoutEvent, RecoveryDisconnectingState);

    m_stateMachine.addTransition(RecoverySuperState,        DeviceUnpairedEvent,         ConnectedState);
    m_stateMachine.addTransition(RecoverySuperState,        DeviceConnectedEvent,        ResolvingServicesState);
    m_stateMachine.addTransition(RecoverySuperState,        ServicesResolvedEvent,       StartingServicesState);
    m_stateMachine.addTransition(RecoveryDisconnectingState,DeviceDisconnectedEvent,     RecoveryReconnectingState);

    m_stateMachine.addTransition(SetupSuperState,           ServicesNotResolvedEvent,    ResolvingServicesState);
    m_stateMachine.addTransition(SetupSuperState,           DeviceDisconnectedEvent,     PairedState);
    m_stateMachine.addTransition(SetupSuperState,           DeviceUnpairedEvent,         ConnectedState);

    m_stateMachine.addTransition(StartingServicesState,     ServicesStartedEvent,        ReadyState);


    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&BleRcuDeviceBluez::onEnteredState, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&BleRcuDeviceBluez::onExitedState, this, std::placeholders::_1)));


    // start the state machine
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    Called when bluetoothd notifies us of a change in the device name.

    We simply pass this to the upper layers as a signal.

 */
void BleRcuDeviceBluez::onDeviceNameChanged(const std::string &name)
{
    XLOGD_INFO("%s device name changed from %s to %s", m_address.toString().c_str(), m_name.c_str(), name.c_str());
    if (m_name != name) {
        m_name = name;
        m_nameChangedSlots.invoke(m_name);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the connection status of the device changes.

    This is trigger for the state machine and therefore obvious the device may
    move into a new state as a result of this signal.
 */
void BleRcuDeviceBluez::onDeviceConnectedChanged(bool connected)
{
    // log a milestone on any change
    if (connected != m_lastConnectedState) {
        XLOGD_INFO("%s %s", m_address.toString().c_str(), (connected ? "connected" : "disconnected"));

        m_connectedChangedSlots.invoke(connected);

        m_lastConnectedState = connected;
    }

    // post an event to update the state machine
    if (connected) {
        m_stateMachine.postEvent(DeviceConnectedEvent);
    } else {
        m_stateMachine.postEvent(DeviceDisconnectedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the paired status of the device changes.

    This is trigger for the state machine and therefore obvious the device may
    move into a new state as a result of this signal.
 */
void BleRcuDeviceBluez::onDevicePairedChanged(bool paired)
{
    // either way the pairing procedure has finished
    m_isPairing = false;

    // post an event to update the state machine
    // This needs to be done before calling the pairedChangedSlots so that
    // this state machine is in the proper state prior to any callbacks
    if (paired) {
        m_stateMachine.postEvent(DevicePairedEvent);
    } else {
        m_stateMachine.postEvent(DeviceUnpairedEvent);
    }

    // log a milestone on any change and trigger callbacks
    if (paired != m_lastPairedState) {
        XLOGD_INFO("%s %s", m_address.toString().c_str(), (paired ? "paired" : "unpaired"));
        
        m_pairedChangedSlots.invoke(paired);

        m_lastPairedState = paired;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the 'ServicesResolved' status of the device changes.

    This is trigger for the state machine and therefore obvious the device may
    move into a new state as a result of this signal.
 */
void BleRcuDeviceBluez::onDeviceServicesResolvedChanged(bool resolved)
{
    // log a milestone on any change
    if (resolved != m_lastServicesResolvedState) {
        XLOGD_INFO("%s services %s", m_address.toString().c_str(), (resolved ? "resolved" : "unresolved"));
        m_lastServicesResolvedState = resolved;
    }

    // post an event to update the state machine
    if (resolved) {
        m_stateMachine.postEvent(ServicesResolvedEvent);
    } else {
        m_stateMachine.postEvent(ServicesNotResolvedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called from the state machine object when we've entered a new state.
    We hook this point to farm out the event to specific event handlers.

 */
void BleRcuDeviceBluez::onEnteredState(int state)
{
    switch (state) {
        case IdleState:
            break;

        case ResolvingServicesState:
            onEnteredResolvingServicesState();
            break;
        case StartingServicesState:
            onEnteredStartingServicesState();
            break;

        case RecoveryDisconnectingState:
            onEnteredRecoveryDisconnectingState();
            break;
        case RecoveryReconnectingState:
            onEnteredRecoveryReconnectingState();
            break;

        case ReadyState:
            onEnteredReadyState();
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called from the state machine object when we've exited a state.
    We hook this point to farm out the event to specific event handlers.

 */
void BleRcuDeviceBluez::onExitedState(int state)
{
    switch (State(state)) {
        case SetupSuperState:
            onExitedSetupSuperState();
            break;
        case ReadyState:
            onExitedReadyState();
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called on entry to the ready state.

 */
void BleRcuDeviceBluez::onEnteredReadyState()
{
    m_timeSinceReady = g_timer_new();

    // notify everyone that we are now ready
    m_readyChangedSlots.invoke(true);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called on exit from the ready state.

 */
void BleRcuDeviceBluez::onExitedReadyState()
{
    // notify everyone that we're no longer ready
    m_readyChangedSlots.invoke(false);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called on exit from the setup super state, basically when either we've
    disconnected, unpaired or the services have become unresolved.

 */
void BleRcuDeviceBluez::onExitedSetupSuperState()
{
    // stop the services as we've either not connected or lost pairing, either
    // way we won't have much luck talking to the RCU
    if (m_services) {
        m_services->stop();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'resolving services' state, we check the services
    are not already resolved, if they are we just post the notification message.

 */
void BleRcuDeviceBluez::onEnteredResolvingServicesState()
{
    XLOGD_INFO("%s entered RESOLVING_SERVICES state", m_address.toString().c_str());

    // cancel any delayed events that may have previously been posted
    m_stateMachine.cancelDelayedEvents(ServicesResolveTimeoutEvent);

    // check if the services are already resolved
    if (m_lastServicesResolvedState == true) {

        // emit a fake signal to move the state machine on
        m_stateMachine.postEvent(ServicesResolvedEvent);

    } else {
        // services haven't been resolved so start a timer to check that the
        // services are resolved within 10 seconds, if they aren't then
        // something has gone wrong and we should try a manual re-connect
        if (m_recoveryAttempts < m_maxRecoveryAttempts) {
            m_stateMachine.postDelayedEvent(ServicesResolveTimeoutEvent, 10000);
        }
    }
}

void BleRcuDeviceBluez::shutdown()
{
    if (m_services) {
        m_services->stop();
        m_services.reset();
    }
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'recovery' state, here we try and disconnect from
    device if connected.  Once disconnected we move on to the 'recovery
    reconnect' state.

 */
void BleRcuDeviceBluez::onEnteredRecoveryDisconnectingState()
{
    // increment the number of recovery attempts
    m_recoveryAttempts++;

    // log the attempt
    XLOGD_ERROR("entered recovery state after device %s failed to resolve services (attempt #%d)", 
            m_address.toString().c_str(), m_recoveryAttempts);


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                // an error occurred so log it and emit an error signal
                XLOGD_ERROR("%s disconnect request failed with error: <%s>", 
                        m_address.toString().c_str(), reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("%s disconnect request successful", m_address.toString().c_str());
            }

            // if the device is now disconnected then update the state machine
            if (!m_lastConnectedState) {
                m_stateMachine.postEvent(DeviceDisconnectedEvent);
            }
        };

    // always send a disconnect request, even if bluez is telling us we're disconnected (sometimes it lies)
    m_deviceProxy->Disconnect(PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'recovery re-connect' state, here we ask bluez to
    try and (re)connect to the device.

 */
void BleRcuDeviceBluez::onEnteredRecoveryReconnectingState()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                // an error occurred so log it and emit an error signal
                XLOGD_ERROR("%s connect request failed with error: <%s>", 
                        m_address.toString().c_str(), reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("%s connect request successful", m_address.toString().c_str());
            }

            // if the device is now connected then update the state machine
            if (m_lastConnectedState) {
                m_stateMachine.postEvent(DeviceConnectedEvent);
            }
        };

    // always send a connect request, even if bluez is telling us we're
    // connected (sometimes it lies)
    m_deviceProxy->Connect(PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'start services' state...

 */
void BleRcuDeviceBluez::onEnteredStartingServicesState()
{
    XLOGD_INFO("%s entered STARTING_SERVICES state", m_address.toString().c_str());

    // sanity check
    if (!m_services) {
        XLOGD_ERROR("no services available for device");
        return;
    }

    // start the services
    m_services->start();

    // check if the services are already ready
    if (m_services->isReady()) {
        onServicesReady();
    } else {
        // chain the ready signal from the service to a local callback
        m_services->addReadySlot(Slot<>(m_isAlive, std::bind(&BleRcuDeviceBluez::onServicesReady, this)));
    }
}

// -----------------------------------------------------------------------------
/*!
    Called upon entry to the 'setup device info service' state, here we create
    the \a DeviceInfoService object attached to the message tunnel created
    in the previous state.
 */
void BleRcuDeviceBluez::onServicesReady()
{
    // post the event to the state machine saying the services are ready
    m_stateMachine.postEvent(ServicesStartedEvent);
}

std::shared_ptr<BleRcuBatteryService> BleRcuDeviceBluez::batteryService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->batteryService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuDeviceInfoService> BleRcuDeviceBluez::deviceInfoService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->deviceInfoService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuFindMeService> BleRcuDeviceBluez::findMeService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->findMeService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuAudioService> BleRcuDeviceBluez::audioService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->audioService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuInfraredService> BleRcuDeviceBluez::infraredService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->infraredService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuUpgradeService> BleRcuDeviceBluez::upgradeService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->upgradeService();
    } else {
        return nullptr;
    }
}

std::shared_ptr<BleRcuRemoteControlService> BleRcuDeviceBluez::remoteControlService() const
{
    if (m_services && m_services->isValid()) {
        return m_services->remoteControlService();
    } else {
        return nullptr;
    }
}
// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuDevice::isValid() const

    Returns \c true if the device was successifully created and has all the
    services added.

 */
bool BleRcuDeviceBluez::isValid() const
{
    return (m_deviceProxy && m_deviceProxy->isValid() &&
            m_services && m_services->isValid());
}

// -----------------------------------------------------------------------------
/*!
    \fn BleAddress BleRcuDevice::address() const

    Returns the BDADDR/MAC address of the device.

 */
BleAddress BleRcuDeviceBluez::address() const
{
    return m_address;
}

// -----------------------------------------------------------------------------
/*!
    \fn std::string BleRcuDevice::name() const

    Returns the current name of the device, this may be an empty string if the
    device doesn't have a name.

 */
std::string BleRcuDeviceBluez::name() const
{
    return m_name;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the device is currently connected.

    More specifically it returns true if in any one of the following states;
    ConnectedState, ResolvingServicesState or SetupSuperState.

 */
bool BleRcuDeviceBluez::isConnected() const
{
    return m_stateMachine.inState({ ConnectedState, ResolvingServicesState,
                                    SetupSuperState });
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the device is currently paired.

    More specifically it returns true if in any one of the following states;
    PairedState, ResolvingServicesState, RecoverySuperState or SetupSuperState.

 */
bool BleRcuDeviceBluez::isPaired() const
{
    return m_stateMachine.inState({ PairedState, ResolvingServicesState,
                                    RecoverySuperState, SetupSuperState });
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the device is currently in the processing of pairing.


 */
bool BleRcuDeviceBluez::isPairing() const
{
    return m_isPairing;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the device is connected, paired and all the services have
    been initialised.

    More specifically it returns \c true in in the ReadyState.

 */
bool BleRcuDeviceBluez::isReady() const
{
    return m_stateMachine.inState(ReadyState);
}

// -----------------------------------------------------------------------------
/*!
    Returns the number of milliseconds since the device last transitioned into
    the ready state.

    If the device has never transitioned to the ready state then \c G_MAXDOUBLE
    is returned.

    \sa isReady(), readyChanged()
 */
double BleRcuDeviceBluez::msecsSinceReady() const
{
    if (!m_timeSinceReady) {
        return G_MAXDOUBLE;
    }

    gulong microseconds;
    return g_timer_elapsed(m_timeSinceReady, &microseconds);
}

// -----------------------------------------------------------------------------
/*!
    Returns the bluez dbus object path of the 'org.bluez.Device1' interface we
    are using to control this device.

    This value is constant and set at the construction time of this object.

 */
std::string BleRcuDeviceBluez::bluezObjectPath() const
{
    return m_bluezObjectPath;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device Info Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string BleRcuDeviceBluez::firmwareRevision() const
{
    return deviceInfoService()->firmwareVersion();
}
std::string BleRcuDeviceBluez::hardwareRevision() const
{
    return deviceInfoService()->hardwareRevision();
}
std::string BleRcuDeviceBluez::softwareRevision() const
{
    return deviceInfoService()->softwareVersion();
}
std::string BleRcuDeviceBluez::manufacturer() const
{
    return deviceInfoService()->manufacturerName();
}
std::string BleRcuDeviceBluez::model() const
{
    return deviceInfoService()->modelNumber();
}
std::string BleRcuDeviceBluez::serialNumber() const
{
    return deviceInfoService()->serialNumber();
}
void BleRcuDeviceBluez::addManufacturerNameChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addManufacturerNameChangedSlot(func);
}
void BleRcuDeviceBluez::addModelNumberChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addModelNumberChangedSlot(func);
}
void BleRcuDeviceBluez::addSerialNumberChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addSerialNumberChangedSlot(func);
}
void BleRcuDeviceBluez::addHardwareRevisionChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addHardwareRevisionChangedSlot(func);
}
void BleRcuDeviceBluez::addFirmwareVersionChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addFirmwareVersionChangedSlot(func);
}
void BleRcuDeviceBluez::addSoftwareVersionChangedSlot(const Slot<const std::string &> &func)
{
    deviceInfoService()->addSoftwareVersionChangedSlot(func);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////
void BleRcuDeviceBluez::addBatteryLevelChangedSlot(const Slot<int> &func)
{
    batteryService()->addLevelChangedSlot(func);
}

uint8_t BleRcuDeviceBluez::batteryLevel() const
{
    const int level = batteryService()->level();

    if (level < 0) {
        return (uint8_t) 255;
    } else {
        return (uint8_t)std::min(level, 100);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// FindMe Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////
void BleRcuDeviceBluez::findMe(uint8_t level, PendingReply<> &&reply) const
{
    const auto service = findMeService();

    if (level == 0)
        service->stopBeeping(std::move(reply));
    else if (level == 1)
        service->startBeeping(BleRcuFindMeService::Mid, std::move(reply));
    else if (level == 2)
        service->startBeeping(BleRcuFindMeService::High, std::move(reply));
    else {
        reply.setError("Invalid argument");
        reply.finish();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remote Control Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BleRcuDeviceBluez::unpairReason() const
{
    return remoteControlService()->unpairReason();

}
uint8_t BleRcuDeviceBluez::rebootReason() const
{
    return remoteControlService()->rebootReason();
}
uint8_t BleRcuDeviceBluez::lastKeypress() const
{
    return remoteControlService()->lastKeypress();
}
uint8_t BleRcuDeviceBluez::advConfig() const
{
    return remoteControlService()->advConfig();
}
std::vector<uint8_t> BleRcuDeviceBluez::advConfigCustomList() const
{
    return remoteControlService()->advConfigCustomList();
}
void BleRcuDeviceBluez::sendRcuAction(uint8_t action, PendingReply<> &&reply)
{
    remoteControlService()->sendRcuAction(action, std::move(reply));
}
void BleRcuDeviceBluez::writeAdvertisingConfig(uint8_t config, const std::vector<uint8_t> &customList, PendingReply<> &&reply)
{
    remoteControlService()->writeAdvertisingConfig(config, customList, std::move(reply));
}
void BleRcuDeviceBluez::addUnpairReasonChangedSlot(const Slot<uint8_t> &func)
{
    remoteControlService()->addUnpairReasonChangedSlot(func);
}
void BleRcuDeviceBluez::addRebootReasonChangedSlot(const Slot<uint8_t, std::string> &func)
{
    remoteControlService()->addRebootReasonChangedSlot(func);
}
void BleRcuDeviceBluez::addLastKeypressChangedSlot(const Slot<uint8_t> &func)
{
    remoteControlService()->addLastKeypressChangedSlot(func);
}
void BleRcuDeviceBluez::addAdvConfigChangedSlot(const Slot<uint8_t> &func)
{
    remoteControlService()->addAdvConfigChangedSlot(func);
}
void BleRcuDeviceBluez::addAdvConfigCustomListChangedSlot(const Slot<const std::vector<uint8_t> &> &func)
{
    remoteControlService()->addAdvConfigCustomListChangedSlot(func);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// Voice Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t BleRcuDeviceBluez::audioGainLevel() const
{
    return audioService()->gainLevel();
}
void BleRcuDeviceBluez::setAudioGainLevel(uint8_t value)
{
    audioService()->setGainLevel(value);
}
uint32_t BleRcuDeviceBluez::audioCodecs() const
{
    return audioService()->audioCodecs();
}
bool BleRcuDeviceBluez::audioStreaming() const
{
    return audioService()->isStreaming();
}
bool BleRcuDeviceBluez::getAudioFormat(Encoding encoding, AudioFormat &format) const
{
    if(encoding == BleRcuDevice::Encoding::ADPCM_FRAME) {
        return(audioService()->audioFormat(BleRcuAudioService::Encoding::ADPCM_FRAME, format));
    } else if(encoding == BleRcuDevice::Encoding::PCM16) {
        return(audioService()->audioFormat(BleRcuAudioService::Encoding::PCM16, format));
    }
    return(false);
}
void BleRcuDeviceBluez::startAudioStreaming(uint32_t encoding, PendingReply<int> &&reply, uint32_t durationMax)
{
    audioService()->startStreaming(BleRcuAudioService::Encoding(encoding), std::move(reply), durationMax);
}
void BleRcuDeviceBluez::stopAudioStreaming(uint32_t audioDuration, PendingReply<> &&reply)
{
    audioService()->stopStreaming(audioDuration, std::move(reply));
}
void BleRcuDeviceBluez::getAudioStatus(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets)
{
    audioService()->status(lastError, expectedPackets, actualPackets);
}
bool BleRcuDeviceBluez::getFirstAudioDataTime(ctrlm_timestamp_t &time)
{
    return(audioService()->getFirstAudioDataTime(time));
}
void BleRcuDeviceBluez::addStreamingChangedSlot(const Slot<bool> &func)
{
    audioService()->addStreamingChangedSlot(func);
}
void BleRcuDeviceBluez::addGainLevelChangedSlot(const Slot<uint8_t> &func)
{
    audioService()->addGainLevelChangedSlot(func);
}
void BleRcuDeviceBluez::addAudioCodecsChangedSlot(const Slot<uint32_t> &func)
{
    audioService()->addAudioCodecsChangedSlot(func);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// IR Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t BleRcuDeviceBluez::irCode() const
{
    return infraredService()->codeId();
}
uint8_t BleRcuDeviceBluez::irSupport() const
{
    return infraredService()->irSupport();
}
void BleRcuDeviceBluez::setIrControl(const uint8_t irControl, PendingReply<> &&reply)
{
    infraredService()->setIrControl(irControl, std::move(reply));
}
void BleRcuDeviceBluez::programIrSignalWaveforms(const map<uint32_t, vector<uint8_t>> &irWaveforms,
                                const uint8_t irControl, PendingReply<> &&reply)
{
    infraredService()->programIrSignalWaveforms(irWaveforms, irControl, std::move(reply));
}
void BleRcuDeviceBluez::eraseIrSignals(PendingReply<> &&reply)
{
    infraredService()->eraseIrSignals(std::move(reply));
}
void BleRcuDeviceBluez::emitIrSignal(uint32_t keyCode, PendingReply<> &&reply)
{
    infraredService()->emitIrSignal(keyCode, std::move(reply));
}
void BleRcuDeviceBluez::addCodeIdChangedSlot(const Slot<int32_t> &func)
{
    infraredService()->addCodeIdChangedSlot(func);
}
void BleRcuDeviceBluez::addIrSupportChangedSlot(const Slot<uint8_t> &func)
{
    infraredService()->addIrSupportChangedSlot(func);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Upgrade Service 
////////////////////////////////////////////////////////////////////////////////////////////////////////

void BleRcuDeviceBluez::startUpgrade(const std::string &fwFile, PendingReply<> &&reply)
{
    upgradeService()->startUpgrade(fwFile, std::move(reply));
}
void BleRcuDeviceBluez::cancelUpgrade(PendingReply<> &&reply)
{
    upgradeService()->cancelUpgrade(std::move(reply));
}
void BleRcuDeviceBluez::addUpgradingChangedSlot(const Slot<bool> &func)
{
    upgradeService()->addUpgradingChangedSlot(func);
}
void BleRcuDeviceBluez::addUpgradeProgressChangedSlot(const Slot<int> &func)
{
    upgradeService()->addProgressChangedSlot(func);
}
void BleRcuDeviceBluez::addUpgradeErrorSlot(const Slot<std::string> &func)
{
    upgradeService()->addErrorChangedSlot(func);
}
