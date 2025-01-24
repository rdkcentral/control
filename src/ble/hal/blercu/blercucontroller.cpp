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
//  blercucontroller.cpp
//

#include "blercucontroller_p.h"
#include "blercuadapter.h"
#include "blercudevice.h"

#include "configsettings/configsettings.h"

#include "ctrlm_log_ble.h"

#include <glib.h>
#include <list>


using namespace std;


static gboolean onInitializedTimer(gpointer user_data);
static gboolean syncManagedDevicesTimer(gpointer user_data);
static gboolean removeLastConnectedDeviceTimer(gpointer user_data);


BleRcuControllerImpl::BleRcuControllerImpl(const shared_ptr<const ConfigSettings> &config,
                                           const shared_ptr<BleRcuAdapter> &adapter)
    : m_isAlive(make_shared<bool>(true))
    , m_config(config)
    , m_adapter(adapter)
    , m_pairingStateMachine(config, m_adapter)
    , m_scannerStateMachine(config, m_adapter)
    , m_lastError(BleRcuError::NoError)
    , m_maxManagedDevices(1)
    , m_state(Initialising)
    , m_ignoreScannerSignal(false)
{

    // connect to the started signal so we can send pairing state notifications
    m_pairingStateMachine.addStartedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onStartedPairing, this)));
        
    // connect to the finished signal of the pairing statemachine, use to update our list of managed devices
    m_pairingStateMachine.addFinishedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onFinishedPairing, this)));

    // connect to the failed signal so we can send pairing state notifications
    m_pairingStateMachine.addFailedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onFailedPairing, this)));

    // connect to the manager's device pairing change signals
    adapter->addDevicePairingChangedSlot(Slot<const BleAddress&, bool>(m_isAlive,
            std::bind(&BleRcuControllerImpl::onDevicePairingChanged, this, std::placeholders::_1, std::placeholders::_2)));

    // connect to the manager's device ready signals
    adapter->addDeviceReadyChangedSlot(Slot<const BleAddress&, bool>(m_isAlive,
            std::bind(&BleRcuControllerImpl::onDeviceReadyChanged, this, std::placeholders::_1, std::placeholders::_2)));

    // connect to the manager's initialised signal
    adapter->addPoweredInitializedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onInitialized, this)));


    // connect to the scanner signals
    m_scannerStateMachine.addStartedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onStartedScanning, this)));
    m_scannerStateMachine.addFinishedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onFinishedScanning, this)));
    m_scannerStateMachine.addFailedSlot(Slot<>(m_isAlive, std::bind(&BleRcuControllerImpl::onFailedScanning, this)));
    
    // connect to the signal emitted when the scanner found an RCU device in pairing mode
    m_scannerStateMachine.addFoundPairableDeviceSlot(Slot<const BleAddress&, const std::string&>(m_isAlive, 
            std::bind(&BleRcuControllerImpl::onFoundPairableDevice, this, std::placeholders::_1, std::placeholders::_2)));

    // schedule the controller to synchronise the list of managed devices at
    // start-up in the next idle time of the event loop
    g_timeout_add(0, syncManagedDevicesTimer, this);

    // Check if already powered and if so signal the initialised state
    if (m_adapter->isPowered()) {
        g_timeout_add(1000, onInitializedTimer, this);
    }
}

BleRcuControllerImpl::~BleRcuControllerImpl()
{
    *m_isAlive = false;
    XLOGD_INFO("BleRcuController shut down");
}


bool BleRcuControllerImpl::isValid() const
{
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuController::state() const

 */
BleRcuController::State BleRcuControllerImpl::state() const
{
    return m_state;
}


// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuControllerImpl::shutdown() const

    Stop all services.  This is needed during shutdown to fix
    an intermittent bluez crash when bluez attempts to send notifications
    while the system is shutting down.

 */
void BleRcuControllerImpl::shutdown() const
{
    for (const BleAddress &bdaddr : m_managedDevices) {
        XLOGD_INFO("Stopping all services for %s", bdaddr.toString().c_str());
        const shared_ptr<BleRcuDevice> device = m_adapter->getDevice(bdaddr);
        if (!device || !device->isValid()) {
            XLOGD_WARN("Invalid device %s, ignoring...", bdaddr.toString().c_str());
        } else {
            device->shutdown();
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn BleRcuError BleRcuController::lastError() const

    Returns the last error that occured when performing a pairing function.

 */
BleRcuError BleRcuControllerImpl::lastError() const
{
    return m_lastError;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuController::isPairing() const

    Returns \c true if pairing is currently in progress.

    \see startPairing()
 */
bool BleRcuControllerImpl::isPairing() const
{
    return m_pairingStateMachine.isRunning();
}

// -----------------------------------------------------------------------------
/*!
    \fn int BleRcuController::pairingCode()

    Returns the current or last 8-bit pairing code used.

    If startPairing() has never been called \c -1 will be returned. Or if
    pairing was started after a scan then \c -1 will also be returned.

    \see startPairing()
 */
int BleRcuControllerImpl::pairingCode() const
{
    return m_pairingStateMachine.pairingCode();
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuController::startPairing(uint8_t pairingCode)

    Attempts to start the pairing procedure looking for devices that identify
    with the given \a filterByte and \a pairingCode.  Both these byte values
    are sent in the IR pairing signal and are used to help identify the RCU
    model and unique name.

    If the controller is currently in pairing mode this method will fail and
    return \c false.  If the bluetooth adaptor is not available or not powered
    then this function will also fail and return \c false.

    If \c false is returned use BleRcuController::lastError() to get the failure
    reason.

    \note This object doesn't actually run the pairing procedure, instead it
    just starts and stops the \l{BleRcuPairingStateMachine} object.

    \see cancelPairing(), isPairing() & pairingCode()
 */
bool BleRcuControllerImpl::startPairing(uint8_t filterByte, uint8_t pairingCode)
{
    m_ignoreScannerSignal = false;

    // if currently scanning then we have to cancel that first before processing
    // the IR pairing request (nb - pairing request can only come to this
    // function from an IR event)
    if (m_scannerStateMachine.isRunning()) {
        m_scannerStateMachine.stop();

        XLOGD_WARN("received IR pairing request in scanning mode, disabling scanner and when stopped will start IR pairing");
        return false;
    }

    if (m_pairingStateMachine.isRunning()) {
        XLOGD_DEBUG("requested pairing in already pairing state, ignoring request");
        m_lastError = BleRcuError(BleRcuError::Busy, "Already in pairing state");
        return false;
    }

    // check that the manager has powered on the adapter, without this we
    // obviously can't scan / pair / etc. The only time the adaptor should
    // (legitimately) be unavailable is right at start-up
    if (!m_adapter->isAvailable() || !m_adapter->isPowered()) {
        m_lastError = BleRcuError(BleRcuError::General, "Adaptor not available or not powered");
        return false;
    }

    // start the pairing process
    m_pairingStateMachine.start(filterByte, pairingCode);
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuController::startPairingMacHash(uint8_t macHash)

    Attempts to start the pairing procedure looking for a device that has
    a MAC address matching the supplied MAC hash.
    
    The MAC hash is calculated by adding all the bytes of the MAC address
    together, and masking with 0xFF.
    e.g., MAC = AA:BB:CC:DD:EE:FF, hash = (AA+BB+CC+DD+EE+FF) & 0xFF

    If the controller is currently in pairing mode this method will fail and
    return \c false.  If the bluetooth adaptor is not available or not powered
    then this function will also fail and return \c false.

    If \c false is returned use BleRcuController::lastError() to get the failure
    reason.

    \note This object doesn't actually run the pairing procedure, instead it
    just starts and stops the \l{BleRcuPairingStateMachine} object.

    \see cancelPairing(), isPairing() & pairingCode()
 */
bool BleRcuControllerImpl::startPairingMacHash(uint8_t filterByte, uint8_t macHash)
{
    // if currently scanning then we have to cancel that first before processing
    // the IR pairing request (nb - pairing request can only come to this
    // function from an IR event)
    if (m_scannerStateMachine.isRunning()) {
        XLOGD_WARN("received IR pairing request in scanning mode, disabling scanner and when stopped will start IR pairing");
        
        // When the scanner state machine is stopped without any remotes being paired,
        // it surfaces a failed status.  In this case, we simply want to stop the scanning
        // state machine without broadcasting a FAILED status and continue immediately with
        // IR pairing.
        m_ignoreScannerSignal = true;
        m_scannerStateMachine.stop();

        return false;
    }


    if (m_pairingStateMachine.isRunning()) {
        XLOGD_DEBUG("requested pairing in already pairing state, ignoring request");
        m_lastError = BleRcuError(BleRcuError::Busy, "Already in pairing state");
        return false;
    }

    // check that the manager has powered on the adapter, without this we
    // obviously can't scan / pair / etc. The only time the adaptor should
    // (legitimately) be unavailable is right at start-up
    if (!m_adapter->isAvailable() || !m_adapter->isPowered()) {
        m_lastError = BleRcuError(BleRcuError::General, "Adaptor not available or not powered");
        return false;
    }

    // start the pairing process
    m_pairingStateMachine.startMacHash(filterByte, macHash);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuController::cancelPairing()

    Cancels the pairing procedure if running.

    \see startPairing(), isPairing() & pairingCode()
 */
bool BleRcuControllerImpl::cancelPairing()
{
    if (!m_pairingStateMachine.isRunning()) {
        return false;
    }

    m_pairingStateMachine.stop();
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleRcuController::isScanning() const

    Returns \c true if scanning is currently in progress.

    \see startScanning()
 */
bool BleRcuControllerImpl::isScanning() const
{
    return m_scannerStateMachine.isRunning();
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuController::startScanning()

    Starts the scanner looking for RCUs in pairing mode.  The scan will run for
    \a timeoutMs milliseconds, or until cancelled if less than zero.

    The scanner won't start if the pairing state machine is already running.

    \see cancelScanning() & isScanning()
 */
bool BleRcuControllerImpl::startScanning(int timeoutMs)
{
    m_ignoreScannerSignal = false;

    // check we're not currently pairing
    if (m_pairingStateMachine.isRunning()) {
        m_lastError = BleRcuError(BleRcuError::General, "currently performing pairing, cannot start new scan");
        return false;
    }

    // check we're not already scanning
    if (m_scannerStateMachine.isRunning()) {
        m_lastError = BleRcuError(BleRcuError::General, "already scanning, new scan request aborted");
        return false;
    }

    // check that the manager has powered on the adapter, without this we
    // obviously can't scan. The only time the adaptor should (legitimately) be
    // unavailable is right at start-up
    if (!m_adapter->isAvailable() || !m_adapter->isPowered()) {
        m_lastError = BleRcuError(BleRcuError::General, "Adaptor not available or not powered");
        return false;
    }

    // start the scanning process
    if (m_state != Searching) {
        m_state = Searching;
        m_stateChangedSlots.invoke(m_state);
    }

    m_scannerStateMachine.start(timeoutMs);
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleRcuController::cancelScanning()

    Cancels the current scanning process.

    \see startScanning()
 */
bool BleRcuControllerImpl::cancelScanning()
{
    if (!m_scannerStateMachine.isRunning()) {
        return false;
    }

    m_scannerStateMachine.stop();
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn QSet<BleAddress> BleRcuController::managedDevices() const

    Returns a set of all the RCU devices currently been managed.

    \see managedDevice()
 */
set<BleAddress> BleRcuControllerImpl::managedDevices() const
{
    return m_managedDevices;
}

// -----------------------------------------------------------------------------
/*!
    \fn shared_ptr<BleRcuDevice> BleRcuController::managedDevice(const BleAddress &address) const

    Returns a shared pointer to the managed RCU device with the given \a address.
    If there is no managed RCU with the given address an empty shared pointer
    is returned.

    \see managedDevices()
 */
shared_ptr<BleRcuDevice> BleRcuControllerImpl::managedDevice(const BleAddress &address) const
{
    if (m_managedDevices.find(address) == m_managedDevices.end()) {
        return nullptr;
    }

    return m_adapter->getDevice(address);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called from the main event loop to sync managed devices

 */
static gboolean syncManagedDevicesTimer(gpointer user_data)
{
    BleRcuControllerImpl *rcuDevice = (BleRcuControllerImpl*)user_data;
    if (rcuDevice == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
    } else {
        rcuDevice->syncManagedDevices();
    }
    
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called in the following scenarios:
        - At start-up time
        - A device has been added in non-pairing state
        - A device has been removed in non-pairing state
        - On pairing finished (success or failure)

    It gets a list of managed devices from the \l{BleRcuAdapter} object, if
    there are devices in the return list that are paired but not in our local
    managed set then they are added to the set and a signal emitted.  Conversely
    if there is a device in our managed set that doesn't match a paired device
    in the adapter's list then it is removed from the set and 'removed' signal
    is emitted.

 */
void BleRcuControllerImpl::syncManagedDevices()
{
    // get the set of currently paired devices
    const set<BleAddress> paired = m_adapter->pairedDevices();

    // calculate the set of removed devices first (if any)
    set<BleAddress> removed;
    for (const BleAddress &address : m_managedDevices) {

        if (paired.find(address) == paired.end()) {
            XLOGD_DEBUG("removed %s", address.toString().c_str());
            removed.insert(address);
        }
    }

    for (const BleAddress &address : removed) {
        m_managedDevices.erase(address);
        m_managedDeviceRemovedSlots.invoke(address);
    }


    // calculate the set of added devices next (if any)
    set<BleAddress> added;
    for (const BleAddress &address : paired) {

        if (m_managedDevices.find(address) == m_managedDevices.end()) {
            XLOGD_DEBUG("added %s", address.toString().c_str());
            added.insert(address);
        }
    }

    for (const BleAddress &address : added) {
        m_managedDevices.insert(address);
        m_managedDeviceAddedSlots.invoke(address);
    }


    // next check if the number of paired / managed device exceeds the
    // maximum allowed, if so we need to remove the device that was connected the longest ago
    if (m_managedDevices.size() > m_maxManagedDevices) {

        // the following will push the call onto the event queue, didn't
        // want to do this option in the callback slot (although shouldn't be an issue)
        g_timeout_add(0, removeLastConnectedDeviceTimer, this);
    }
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Called from the main event loop to remove last connected device

 */
static gboolean removeLastConnectedDeviceTimer(gpointer user_data)
{
    BleRcuControllerImpl *rcuDevice = (BleRcuControllerImpl*)user_data;
    if (rcuDevice == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
    } else {
        rcuDevice->removeLastConnectedDevice();
    }
    
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued function call that iterates through the set of currently managed
    devices and removes the one(s) that were the last to go to the 'ready' state.

    This function should only be called after we detect that more than the
    maximum number of devices have entered the ready state, i.e. we have more
    than the maximum actively connected.

    \sa removeDevice(), onDeviceReadyChanged()
 */
void BleRcuControllerImpl::removeLastConnectedDevice()
{
    // because this is a queued callback we need to check once again the we
    // have exceeded the maximum number of devices, here we create an ordered
    // list of all the 'paired' devices, the oldest ones to enter the ready
    // state are at the front

    list<shared_ptr<const BleRcuDevice>> pairedDevices;
    for (const BleAddress &bdaddr : m_managedDevices) {

        shared_ptr<BleRcuDevice> device = m_adapter->getDevice(bdaddr);
        if (device && device->isValid() && device->isPaired()) {

            // find the spot in the list to insert the item, devices with the
            // oldest ready transition at the front
            list<shared_ptr<const BleRcuDevice>>::iterator it = pairedDevices.begin();

            XLOGD_DEBUG("device %s msecsSinceReady = %lf", 
                    device->address().toString().c_str(), device->msecsSinceReady());

            for (; it != pairedDevices.end(); ++it) {
                if ((*it)->msecsSinceReady() < device->msecsSinceReady()) {
                    break;
                }
            }
            pairedDevices.insert(it, device);
        }
    }

    // remove the first n number of paired devices
    while (pairedDevices.size() > m_maxManagedDevices) {

        // take the first device from the queue
        const shared_ptr<const BleRcuDevice> device = pairedDevices.front();
        pairedDevices.pop_front();

        XLOGD_INFO("unpairing %s because exceeded maximum number of managed devices", 
                device->address().toString().c_str());

        // ask bluez to remove it, this will disconnect and unpair the device
        m_adapter->removeDevice(device->address());
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued function call to unpair a device

    \sa removeDevice()
 */
bool BleRcuControllerImpl::unpairDevice(const BleAddress &address) const
{
    XLOGD_DEBUG("unpairing device on external request %s", address.toString().c_str());
    // ask bluez to remove it, this will disconnect and unpair the device
    return m_adapter->removeDevice(address);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the pairing state machine has started.
 */
void BleRcuControllerImpl::onStartedPairing()
{
    // a queued event so check the state
    const bool pairing = m_pairingStateMachine.isRunning();

    // tell clients that the pairing state has changed
    m_pairingStateChangedSlots.invoke(pairing);

    m_state = Pairing;
    m_stateChangedSlots.invoke(Pairing);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the pairing state machine has finished.  This
    doesn't necessarily mean it succeeded, this is called on failure as well.

 */
void BleRcuControllerImpl::onFinishedPairing()
{
    // a queued event so check the state
    const bool pairing = m_pairingStateMachine.isRunning();

    // (re)sync our list of managed devices now pairing has finished
    if (!pairing) {
        syncManagedDevices();
    }

    // tell clients that the pairing state is changed
    m_pairingStateChangedSlots.invoke(pairing);

    m_state = Complete;
    m_stateChangedSlots.invoke(Complete);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the pairing state machine has failed. This doesn't
    necessarily mean it succeeded, this is called on failure as well.

 */
void BleRcuControllerImpl::onFailedPairing()
{
    // a queued event so check the state
    const bool pairing = m_pairingStateMachine.isRunning();

    // (re)sync our list of managed devices now pairing has finished
    if (!pairing) {
        syncManagedDevices();
    }

    // tell clients that the pairing state is changed
    m_pairingStateChangedSlots.invoke(pairing);

    m_state = Failed;
    m_stateChangedSlots.invoke(Failed);
}



// -----------------------------------------------------------------------------
/*!
    \internal

    Called from the main event loop after we've been initialised

 */
static gboolean onInitializedTimer(gpointer user_data)
{
    BleRcuControllerImpl *rcuDevice = (BleRcuControllerImpl*)user_data;
    if (rcuDevice == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
    } else {
        rcuDevice->onInitialized();
    }
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the adaptor is powered on 
 */
void BleRcuControllerImpl::onInitialized()
{
    if (m_state == Initialising) {
        m_state = Idle;
        m_stateChangedSlots.invoke(Idle);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the \l{BleRcuManager} has added a new device.
 */
void BleRcuControllerImpl::onDevicePairingChanged(const BleAddress &address,
                                                  bool paired)
{
    if (!paired) {
        // if the removed device is in our managed set then we remove it
        // immediately even if the pairing state machine is running.
        // Previously we didn't do this while in pairing mode, that meant if you
        // repaired the same device the client never got the removed / added
        // notifications.  So although this is not technically wrong, some
        // clients were expecting an added notification to indicate pairing
        // succeeded, when in fact all they got was an unchanged list of devices.
        if (m_managedDevices.find(address) != m_managedDevices.end()) {
            m_managedDevices.erase(address);
            m_managedDeviceRemovedSlots.invoke(address);
        }
    }

    // a device has bonded / unbonded, so if the state-machine is not running
    // then re-sync the list of devices we are managing
    if (!m_pairingStateMachine.isRunning()) {
        syncManagedDevices();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Event signaled by a \l{BleRcuDevice} object when it's ready state has
    changed. The 'ready' state implies that the device is paired, connected, and
    has gone through the initial setup such that it is now ready.

    This is also the point were we check the number of devices we have in the
    'paired' state, if it exceeds the maximum allowed then we remove the last
    device to enter the 'ready' state.

 */
void BleRcuControllerImpl::onDeviceReadyChanged(const BleAddress &address,
                                                bool ready)
{
    if (ready && !m_pairingStateMachine.isRunning()) {
        syncManagedDevices();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the scanner state machine indicates it has started.
 */
void BleRcuControllerImpl::onStartedScanning()
{
    m_scanningStateChangedSlots.invoke(true);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the scanner state machine indicates it has stopped.
    This may be because it was cancelled, found a target device or timed out.
 */
void BleRcuControllerImpl::onFinishedScanning()
{
    m_scanningStateChangedSlots.invoke(false);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the scanner state machine indicates it has failed to
    find a device from scanning. This may be because it was cancelled, found a
    target device or timed out.
 */
void
BleRcuControllerImpl::onFailedScanning()
{
    if (m_ignoreScannerSignal) {
        m_ignoreScannerSignal = false;
    } else {
        m_state = Failed;
        m_stateChangedSlots.invoke(Failed);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Queued slot called when the scanner state machine found an RCU that was in
    'pairing' mode.

    This triggers us to start the pairing state machine targeting the device
    with the given address.

 */
void BleRcuControllerImpl::onFoundPairableDevice(const BleAddress &address,
                                                 const string &name)
{
    XLOGD_INFO("found %s RCU device in pairing mode, kicking off the pairing state machine", 
            address.toString().c_str());

    // sanity check (needed?)
    if (m_pairingStateMachine.isRunning()) {
        XLOGD_WARN("found target device in scan but pairing state machine already running?");
        return;
    }

    // start pairing the device
    m_pairingStateMachine.start(address, name);
}
