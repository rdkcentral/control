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
//  blercupairingstatemachine.cpp
//

#include "blercupairingstatemachine.h"
#include "blercuadapter.h"

#include "configsettings/configsettings.h"

#include "ctrlm_log_ble.h"

using namespace std;


BleRcuPairingStateMachine::BleRcuPairingStateMachine(const shared_ptr<const ConfigSettings> &config,
                                                     const shared_ptr<BleRcuAdapter> &adapter)
    : m_isAlive(make_shared<bool>(true))
    , m_adapter(adapter)
    , m_isAutoPairing(false)
    , m_pairingCode(-1)
    , m_pairingMacHash(-1)
    , m_discoveryTimeout(config->discoveryTimeout())
    , m_discoveryTimeoutDefault(config->discoveryTimeout())
    , m_pairingTimeout(config->pairingTimeout())
    , m_setupTimeout(config->setupTimeout())
    , m_unpairingTimeout(config->upairingTimeout())
    , m_pairingAttempts(0)
    , m_pairingSuccesses(0)
    , m_pairingSucceeded(false)
{

    // constructs a list of name printf style formats for searching for device names that match
    for (const ConfigModelSettings &model : config->modelSettings()) {
        if (!model.disabled()) {
            if (!model.pairingNameFormat().empty()) {
                m_pairingPrefixFormats.push_back(model.pairingNameFormat());
            }
            m_supportedPairingNames.push_back(model.scanNameMatcher());
        }
    }

    // setup (but don't start) the state machine
    setupStateMachine();

    // connect up the events from the manager
    m_adapter->addDiscoveryChangedSlot(Slot<bool>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDiscoveryChanged, this, std::placeholders::_1)));
    m_adapter->addPairableChangedSlot(Slot<bool>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onPairableChanged, this, std::placeholders::_1)));
    m_adapter->addDeviceFoundSlot(Slot<const BleAddress&, const std::string&>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDeviceFound, this, std::placeholders::_1, std::placeholders::_2)));
    m_adapter->addDeviceRemovedSlot(Slot<const BleAddress&>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDeviceRemoved, this, std::placeholders::_1)));
    m_adapter->addDeviceNameChangedSlot(Slot<const BleAddress&, const std::string&>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDeviceNameChanged, this, std::placeholders::_1, std::placeholders::_2)));
    m_adapter->addDeviceReadyChangedSlot(Slot<const BleAddress&, bool>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDeviceReadyChanged, this, std::placeholders::_1, std::placeholders::_2)));
    m_adapter->addDevicePairingChangedSlot(Slot<const BleAddress&, bool>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDevicePairingChanged, this, std::placeholders::_1, std::placeholders::_2)));
    m_adapter->addPoweredChangedSlot(Slot<bool>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onAdapterPoweredChanged, this, std::placeholders::_1)));
    m_adapter->addDevicePairingErrorSlot(Slot<const BleAddress&, const std::string&>(m_isAlive,
            std::bind(&BleRcuPairingStateMachine::onDevicePairingError, this, std::placeholders::_1, std::placeholders::_2)));
}

BleRcuPairingStateMachine::~BleRcuPairingStateMachine()
{
    *m_isAlive = false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures the internal state machine object.

 */
void BleRcuPairingStateMachine::setupStateMachine()
{
    // set the name of the statemachine for logging
    m_stateMachine.setObjectName("PairingStateMachine");
    m_stateMachine.setGMainLoop(m_adapter->getGMainLoop());
    
    // add all the states
    m_stateMachine.addState(RunningSuperState, "RunningSuperState");

    m_stateMachine.addState(RunningSuperState, DiscoverySuperState, "DiscoverySuperState");
    m_stateMachine.addState(DiscoverySuperState, StartingDiscoveryState, "StartingDiscoveryState");
    m_stateMachine.addState(DiscoverySuperState, DiscoveringState, "DiscoveringState");

    m_stateMachine.addState(RunningSuperState, StoppingDiscoveryState, "StoppingDiscoveryState");
    m_stateMachine.addState(RunningSuperState, PairingSuperState, "PairingSuperState");
    m_stateMachine.addState(PairingSuperState, EnablePairableState, "EnablePairableState");
    m_stateMachine.addState(PairingSuperState, PairingState, "PairingState");
    m_stateMachine.addState(PairingSuperState, SetupState, "SetupState");

    m_stateMachine.addState(RunningSuperState, UnpairingState, "UnpairingState");
    m_stateMachine.addState(FinishedState, "FinishedState");

    m_stateMachine.addState(RunningSuperState, StoppingDiscoveryStartedExternally, "StoppingDiscoveryStartedExternally");

    // add the transitions       From State           ->                Event                  ->   To State
    m_stateMachine.addTransition(RunningSuperState,                     AdapterPoweredOffEvent,     FinishedState);
    
    m_stateMachine.addTransition(StartingDiscoveryState,                DiscoveryStartedEvent,      DiscoveringState);
    m_stateMachine.addTransition(DiscoverySuperState,                   DeviceFoundEvent,           StoppingDiscoveryState);
    m_stateMachine.addTransition(DiscoverySuperState,                   DiscoveryStartTimeoutEvent, FinishedState);
    m_stateMachine.addTransition(DiscoverySuperState,                   DiscoveryStoppedEvent,      FinishedState);
    m_stateMachine.addTransition(DiscoverySuperState,                   CancelRequestEvent,         FinishedState);

    m_stateMachine.addTransition(StoppingDiscoveryState,                DiscoveryStoppedEvent,      EnablePairableState);
    m_stateMachine.addTransition(StoppingDiscoveryState,                PairingTimeoutEvent,        FinishedState);
    m_stateMachine.addTransition(StoppingDiscoveryState,                PairingErrorEvent,          FinishedState);
    m_stateMachine.addTransition(StoppingDiscoveryState,                DiscoveryStopTimeoutEvent,  FinishedState);

    m_stateMachine.addTransition(EnablePairableState,                   PairableEnabledEvent,       PairingState);
    m_stateMachine.addTransition(PairingState,                          PairableDisabledEvent,      UnpairingState);
    m_stateMachine.addTransition(PairingState,                          DevicePairedEvent,          SetupState);
    m_stateMachine.addTransition(PairingSuperState,                     DeviceReadyEvent,           FinishedState);
    m_stateMachine.addTransition(PairingSuperState,                     DeviceUnpairedEvent,        FinishedState);
    m_stateMachine.addTransition(PairingSuperState,                     DeviceRemovedEvent,         FinishedState);
    m_stateMachine.addTransition(EnablePairableState,                   PairingTimeoutEvent,        UnpairingState);
    m_stateMachine.addTransition(EnablePairableState,                   PairingErrorEvent,          FinishedState);
    m_stateMachine.addTransition(PairingState,                          PairingTimeoutEvent,        UnpairingState);
    m_stateMachine.addTransition(PairingState,                          PairingErrorEvent,          FinishedState);
    m_stateMachine.addTransition(SetupState,                            SetupTimeoutEvent,          UnpairingState);

    m_stateMachine.addTransition(UnpairingState,                        DeviceUnpairedEvent,        FinishedState);
    m_stateMachine.addTransition(UnpairingState,                        DeviceRemovedEvent,         FinishedState);
    m_stateMachine.addTransition(UnpairingState,                        UnpairingTimeoutEvent,      FinishedState);

    m_stateMachine.addTransition(StoppingDiscoveryStartedExternally,    DiscoveryStoppedEvent,      StartingDiscoveryState);

    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&BleRcuPairingStateMachine::onStateEntry, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&BleRcuPairingStateMachine::onStateExit, this, std::placeholders::_1)));
    m_stateMachine.addTransitionHandler(Slot<int, int>(m_isAlive, (std::bind(&BleRcuPairingStateMachine::onStateTransition, this, 
            std::placeholders::_1, std::placeholders::_2))));


    // set the initial state
    m_stateMachine.setInitialState(StartingDiscoveryState);
    m_stateMachine.setFinalState(FinishedState);
}

// -----------------------------------------------------------------------------
/*!
    Returns the current or last pairing code used by this state machine.

    If the state machine hasn't been run 0 is returned.

 */
int BleRcuPairingStateMachine::pairingCode() const
{
    return m_pairingCode;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the state machine is currently running.

 */
bool BleRcuPairingStateMachine::isRunning() const
{
    return m_stateMachine.isRunning();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the state machine is currently running the auto pairing
    operation.

    This special state is needed because auto pairing consists of running a scan for
    an undeterminate amount of time until one of the supported devices listed in the 
    config file is found.  We want to be able to cancel this operation if another 
    pair request comes in that targets a specific device (like pairWithCode or
    pairWithMacHash)

 */
bool BleRcuPairingStateMachine::isAutoPairing() const
{
    return isRunning() && m_isAutoPairing;
}


// -----------------------------------------------------------------------------
/*!
    Starts the state machine using the supplied \a pairingCode and
    \a namePrefixes.

 */
void BleRcuPairingStateMachine::startAutoWithTimeout(int timeoutMs)
{
    // sanity check the statemachine is not already running
    if (m_stateMachine.isRunning()) {
        XLOG_WARN("state machine already running");
        return;
    }

    m_discoveryTimeout = timeoutMs;
    m_isAutoPairing = true;

    // clear the target device
    m_targetAddress.clear();

    // clear the pairing code and mac hash
    m_pairingCode = -1;
    m_pairingMacHash = -1;

    // create list of supported remotes regex to match to the name of the device
    m_targetedPairingNames.clear();

    for (const auto &name : m_supportedPairingNames) {
        // add to the list to use for compare when a device is found
        m_targetedPairingNames.push_back(name);
    }

    // start the state machine
    m_stateMachine.start();

    m_pairingAttempts++;
    m_pairingSucceeded = false;
    XLOGD_INFO("Started auto pairing procedure");
}


// -----------------------------------------------------------------------------
/*!
    Starts the state machine using the supplied \a pairingCode and
    \a namePrefixes.

 */
void BleRcuPairingStateMachine::startWithCode(uint8_t pairingCode)
{
    // sanity check the statemachine is not already running
    if (m_stateMachine.isRunning()) {
        XLOG_WARN("state machine already running");
        return;
    }

    m_discoveryTimeout = m_discoveryTimeoutDefault;
    m_isAutoPairing = false;

    // clear the target device
    m_targetAddress.clear();

    // clear the list of addresses to filter for
    m_pairingMacList.clear();

    // store the pairing code
    m_pairingCode = pairingCode;
    m_pairingMacHash = -1;

    // create list of supported remotes regex to match to the name of the device
    m_targetedPairingNames.clear();

    char nameWithCode[100];

    for (const auto &pairingFormat : m_pairingPrefixFormats) {
        // construct the wildcard match
        snprintf(nameWithCode, sizeof(nameWithCode), pairingFormat.c_str(), pairingCode);

        XLOGD_INFO("added pairing regex for supported remote '%s'", nameWithCode);

        // add to the list to use for compare when a device is found
        m_targetedPairingNames.push_back(std::regex(nameWithCode, std::regex_constants::ECMAScript));
    }

    // start the state machine
    m_stateMachine.start();

    m_pairingAttempts++;
    m_pairingSucceeded = false;
    XLOGD_INFO("started pairing using name prefix code %03d", m_pairingCode);
}

// -----------------------------------------------------------------------------
/*!
    Starts the state machine using the supplied \a pairingCode and
    \a namePrefixes.

 */
void BleRcuPairingStateMachine::startWithMacHash(uint8_t macHash)
{
    // sanity check the statemachine is not already running
    if (m_stateMachine.isRunning()) {
        XLOGD_WARN("state machine already running");
        return;
    }

    m_discoveryTimeout = m_discoveryTimeoutDefault;
    m_isAutoPairing = false;

    // clear the target device
    m_targetAddress.clear();

    // clear the pairing code
    m_pairingCode = -1;

    // clear the list of addresses to filter for
    m_pairingMacList.clear();

    // store the MAC hash
    m_pairingMacHash = macHash;

    // clear the maps, we are trying to pair to a specific device using a hash of the MAC address
    m_targetedPairingNames.clear();

    // start the state machine
    m_stateMachine.start();

    m_pairingAttempts++;
    m_pairingSucceeded = false;
    XLOGD_INFO("started pairing, searching for device with MAC hash 0x%02X", m_pairingMacHash);
}

// -----------------------------------------------------------------------------
/*!
    Starts the pairing state machine, but skips the discovery phase as we
    already have a \a target device.

 */
void BleRcuPairingStateMachine::start(const BleAddress &target, const string &name)
{
    // sanity check the state machine is not already running
    if (m_stateMachine.isRunning()) {
        XLOGD_WARN("state machine already running");
        return;
    }

    m_discoveryTimeout = m_discoveryTimeoutDefault;
    m_isAutoPairing = false;

    // set the target device
    m_targetAddress = target;

    // clear the pairing code
    m_pairingCode = -1;
    m_pairingMacHash = -1;

    // clear the list of addresses to filter for
    m_pairingMacList.clear();

    // set the pairing prefix map to contain just the one name match
    m_targetedPairingNames.clear();
    m_targetedPairingNames.push_back(std::regex(name, std::regex_constants::ECMAScript));

    // start the state machine
    m_stateMachine.start();

    m_pairingAttempts++;
    m_pairingSucceeded = false;
    XLOGD_INFO("started pairing targeting %s", target.toString().c_str());
}

// -----------------------------------------------------------------------------
/*!
    Starts the state machine for pairing using the supplied \a list of addresses

 */
void BleRcuPairingStateMachine::startWithMacList(const std::vector<BleAddress> &macList)
{
    // sanity check
    if (m_stateMachine.isRunning()) {
        XLOGD_ERROR("scanner already running");
        return;
    }

    // clear the target device
    m_targetAddress.clear();

    // store the pairing code
    m_pairingCode = -1;
    m_pairingMacHash = -1;

    // create list of supported remotes regex to match to the name of the device
    m_supportedPairingNames.clear();

    // set the list of addresses to filter for
    m_pairingMacList = macList;

    // start the state machine
    m_stateMachine.start();

    m_pairingAttempts++;
    m_pairingSucceeded = false;

    XLOGD_INFO("starting pairing with a list of target addresses:");
    for (const auto &address : macList) {
        XLOGD_INFO("<%s>", address.toString().c_str());
    }

}


// -----------------------------------------------------------------------------
/*!
    Stops the state machine by posting a cancel message to it.

    The stop may be asynchronous, you can either poll on the isRunning() call or
    wait for either the failed() or succeeded() signals.

 */
void BleRcuPairingStateMachine::stop()
{
    // sanity check
    if (!m_stateMachine.isRunning()) {
        XLOGD_INFO("pairing state machine not running");
        return;
    }

    XLOGD_INFO("cancelling pairing");

    // post a cancel event and let the state-machine clean up
    m_stateMachine.postEvent(CancelRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when entering a new state.

 */
void BleRcuPairingStateMachine::onStateEntry(int state)
{
    switch (state) {
        case StartingDiscoveryState:
            onEnteredStartDiscoveryState();
            break;
        case DiscoveringState:
            onEnteredDiscoveringState();
            break;
        case StoppingDiscoveryState:
            onEnteredStoppingDiscoveryState();
            break;
        case EnablePairableState:
            onEnteredEnablePairableState();
            break;
        case PairingState:
            onEnteredPairingState();
            break;
        case SetupState:
            onEnteredSetupState();
            break;
        case UnpairingState:
            onEnteredUnpairingState();
            break;
        case FinishedState:
            onEnteredFinishedState();
            break;
        case StoppingDiscoveryStartedExternally:
            onEnteredStoppingDiscoveryStartedExternally();
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when existing a state.

 */
void BleRcuPairingStateMachine::onStateExit(int state)
{
    switch (state) {
        case DiscoverySuperState:
            onExitedDiscoverySuperState();
            break;
        case PairingSuperState:
            onExitedPairingSuperState();
            break;
        case UnpairingState:
            onExitedUnpairingState();
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when transitioning states.  This is only used for logging

 */
void BleRcuPairingStateMachine::onStateTransition(int oldState, int newState)
{
    if (newState == FinishedState) {
        if (oldState == UnpairingState) {
            XLOGD_WARN("timed-out in un-pairing phase (failed rcu may be left paired)");
        } else if (oldState == StartingDiscoveryState) {
            XLOGD_ERROR("timed-out waiting for discovery started signal");
        } else if (oldState == DiscoveringState) {
            XLOGD_ERROR("timed-out in discovery phase (didn't find target rcu device to pair to)");
        } else if (oldState == StoppingDiscoveryState) {
            XLOGD_ERROR("timed-out waiting for discovery to stop (suggesting something has gone wrong inside bluez)");
        }
    } else if (newState == UnpairingState) {
        if (oldState == EnablePairableState || oldState == PairingState) {
            XLOGD_WARN("timed-out in pairing phase (rcu device didn't pair within %dms)", m_pairingTimeout);
        } else if (oldState == SetupState) {
            XLOGD_WARN("timed-out in setup phase (rcu didn't response to all requests within %dms)", m_setupTimeout);
        }
    }
}

void BleRcuPairingStateMachine::onEnteredStoppingDiscoveryStartedExternally()
{
    XLOGD_WARN("there is already a discovery started externally, requesting that btrMgr stop its discovery");
    lastOperationType = m_btrMgrAdapter.stopDiscovery();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'starting discovery' state.  We hook this point
    to post a delayed message to the state machine to handle the discovery
    timeout.

    This method also emits the started() signal.

 */
void BleRcuPairingStateMachine::onEnteredStartDiscoveryState()
{
    // start a timer for timing out the discovery, if -1 it should run forever so no timeout
    if (m_discoveryTimeout > 0) {
        m_stateMachine.postDelayedEvent(DiscoveryStartTimeoutEvent, m_discoveryTimeout);
    }

    // tell anyone who cares that pairing has started
    m_startedSlots.invoke();

    // check if we're already in discovery mode (we shouldn't be) and if so then
    // post a message to move off the initial state
    if (m_adapter->isDiscovering()) {
        XLOGD_WARN("adapter was already in discovery mode, this is unusual but shouldn't be a problem");

        // even though the adapter is telling us we're in discovery mode, sometimes
        // it lies, so issue another start request anyway, it doesn't hurt if it arrives twice
        m_adapter->startDiscovery();

        // trigger a move to the discovering state
        m_stateMachine.postEvent(DiscoveryStartedEvent);

    } else {
        // otherwise ask the manager (to ask bluez) to start the scan
        m_adapter->startDiscovery();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the discovery status of the bluetooth adapter changes.

 */
void BleRcuPairingStateMachine::onDiscoveryChanged(bool discovering)
{
    if (!m_stateMachine.isRunning()) {
        XLOGD_DEBUG("running onDiscoveryChanged when state machine is not running, let's store current discovery status = %s", 
                discovering ? "true" : "false");

        discoveryStartedExternally = discovering;
        if (discoveryStartedExternally) {
            m_stateMachine.setInitialState(StoppingDiscoveryStartedExternally);
        } else {
            m_stateMachine.setInitialState(StartingDiscoveryState);
        }
        return;
    }

    if (discovering) {
        m_stateMachine.postEvent(DiscoveryStartedEvent);
    } else {
        m_stateMachine.postEvent(DiscoveryStoppedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'discovering' state.  At this point we query the
    manager for the current list of devices and their names.  We use this to
    determine if any existing devices match the pairing prefix.

 */
void BleRcuPairingStateMachine::onEnteredDiscoveringState()
{
    // get the current list of devices
    const map<BleAddress, string> deviceNames = m_adapter->deviceNames();

    // process each existing device
    map<BleAddress, string>::const_iterator it = deviceNames.begin();
    for (; it != deviceNames.end(); ++it) {
        processDevice(it->first, it->second);
    }
}

void BleRcuPairingStateMachine::onExitedDiscoverySuperState()
{
    // stop the discovery timeout timer
    m_stateMachine.cancelDelayedEvents(DiscoveryStartTimeoutEvent);

    // and stop the actually discovery
    m_adapter->stopDiscovery();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'Stopping Discovery' state, the request to stop has
    already been sent (on the exit of the 'Discovering' super state), so we
    just need to check that discovery is not already stopped.

 */
void BleRcuPairingStateMachine::onEnteredStoppingDiscoveryState()
{
    // start the pairing timeout timer
    m_stateMachine.postDelayedEvent(PairingTimeoutEvent, m_pairingTimeout);

    // if we've got to this state it means we have a target device
    if (m_targetAddress.isNull()) {
        XLOGD_ERROR("Entered stopping discovery state, so a target device is expected but its null");
    }

    // if entered this state and discovery is not running then post a discovery
    // stopped to get out of this event
    if (!m_adapter->isDiscovering()) {
        m_stateMachine.postEvent(DiscoveryStoppedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the 'pairable' state of the adaptor changes, the pairable state
    is important because if you pair to an RCU when not in the pairable state
    then bluez doesn't set the correct secure pairing flags.

    \note Pairability only applies to the bluez backend, on android the system
    is always pairable when the bonding request is sent.

 */
void BleRcuPairingStateMachine::onPairableChanged(bool pairable)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    // if pairable is disabled while in the pairing state it causes the state
    // machine to (correctly) abort the process, however the following is
    // here to log it for debugging
    if (m_stateMachine.inState(PairingSuperState) && (pairable == false)) {
        XLOGD_WARN("adaptor 'pairable' disabled before target device became ready");
    }

    if (pairable) {
        m_stateMachine.postEvent(PairableEnabledEvent);
    } else {
        m_stateMachine.postEvent(PairableDisabledEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'enable pairable' state.  We hook this point to
    cancel the discovery timeout event, start a new timer for the pairing timeout.

 */
void BleRcuPairingStateMachine::onEnteredEnablePairableState()
{
    // if we've got to this state it means we have a target device
    if (m_targetAddress.isNull()) {
        XLOGD_ERROR("Entered enable pairable state, so a target device is expected but its null");
    }

    // Set pairable state to true with a timeout.  We want to do this even if its already
    // in pairable mode because we don't know if the previous timeout is long enough for our purposes.
    // The timeout is set to 5 seconds past the overall time we've given the state machine
    // to pair with the rcu.
    m_adapter->enablePairable(m_pairingTimeout + 5000);

    if (m_adapter->isPairable()) {
        // is already pairable so just post the 'enabled' event
        m_stateMachine.postEvent(PairableEnabledEvent);
    }

    // if pairable mode isn't enabled yet, then wait for the notification from bluez.
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'pairing' state.  We hook this point to request
    the adapter to add (pair/bond to) the target device.

 */
void BleRcuPairingStateMachine::onEnteredPairingState()
{
    // if we've got to this state it means we have a target device
    if (m_targetAddress.isNull()) {
        XLOGD_ERROR("Entered pairing state, so a target device is expected but its null");
    }

    m_pairingSlots.invoke();

    // request the manager to pair with the device
    m_adapter->addDevice(m_targetAddress);
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void BleRcuPairingStateMachine::onEnteredSetupState()
{
    // start the setup timeout timer
    m_stateMachine.postDelayedEvent(SetupTimeoutEvent, m_setupTimeout);
    XLOGD_DEBUG("starting setup timeout timer for %dms", m_setupTimeout);
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void BleRcuPairingStateMachine::onExitedPairingSuperState()
{
    // stop the pairing and setup timeout timers
    m_stateMachine.cancelDelayedEvents(PairingTimeoutEvent);
    m_stateMachine.cancelDelayedEvents(SetupTimeoutEvent);

    // clear the pairable state of the adaptor
    m_adapter->disablePairable();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'pairing' state.  We hook this point to cancel the
    discovery timeout event, start a new timer for the pairing timeout and then
    emit the enteredPairingState() signal with the BDADDR of the device we're
    trying to pair to.

 */
void BleRcuPairingStateMachine::onEnteredUnpairingState()
{
    // start the unpairing timeout timer
    m_stateMachine.postDelayedEvent(UnpairingTimeoutEvent, m_unpairingTimeout);

    // if we've got to this state it means we have a target device
    if (m_targetAddress.isNull()) {
        XLOGD_ERROR("Entered unpairing state, so a target device is expected but its null");
    }

    // remove (unpair) the target device because we've failed :-(
    if (m_adapter->removeDevice(m_targetAddress) == false) {
        m_stateMachine.postEvent(DeviceUnpairedEvent);
    }
}

void BleRcuPairingStateMachine::onExitedUnpairingState()
{
    // stop the unpairing timeout timer
    m_stateMachine.cancelDelayedEvents(UnpairingTimeoutEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'finished' state.  We hook this point to cancel to
    clean up any posted delayed events and then emit a finished() signal.

 */
void BleRcuPairingStateMachine::onEnteredFinishedState()
{
    if (m_stateMachine.cancelDelayedEvents(DiscoveryStartTimeoutEvent)) {
        XLOGD_ERROR("Entered finished state with DISCOVERY timer still active");
    }
    if (m_stateMachine.cancelDelayedEvents(PairingTimeoutEvent)) {
        XLOGD_ERROR("Entered finished state with PAIRING timer still active");
    }
    if (m_stateMachine.cancelDelayedEvents(SetupTimeoutEvent)) {
        XLOGD_ERROR("Entered finished state with SETUP timer still active");
    }
    if (m_stateMachine.cancelDelayedEvents(UnpairingTimeoutEvent)) {
        XLOGD_ERROR("Entered finished state with UNPAIRING timer still active");
    }

    if (discoveryStartedExternally) {
        XLOGD_INFO("discovery has been started externally and then stopped, so let's resume it");
        m_btrMgrAdapter.startDiscovery(lastOperationType);
        discoveryStartedExternally = false;
        lastOperationType = BtrMgrAdapter::unknownOperation;
    }

    // finally just emit a finished signal to the BleRcuManagerImpl object
    if (m_pairingSucceeded) {
        m_finishedSlots.invoke();
    } else {
        m_failedSlots.invoke();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the an outside object has called either deviceAdded() or
    deviceNameChanged(), it then checks if the \a name matches our expected
    pairing prefix.  If it does then we check if we already have a pairing
    target, if not we use this new device.

 */
void BleRcuPairingStateMachine::processDevice(const BleAddress &address,
                                              const string &name)
{
    // Iterate through list of supported remotes and compare names
    vector<regex>::const_iterator it_name = m_targetedPairingNames.begin();
    for (; it_name != m_targetedPairingNames.end(); ++it_name) {
        if (std::regex_match(name.c_str(), *it_name)) {
            XLOGD_INFO("Device (%s, %s) has a name targeted for pairing!", 
                    name.c_str(), address.toString().c_str());
            break;
        }
    }

    if (it_name == m_targetedPairingNames.end()) {
        // Device not found through conventional means, see if we are pairing based on MAC hash
        // Because if we are pairing based on MAC hash, m_targetedPairingNames is first cleared
        if (m_pairingMacHash != -1) {
            // Check if MAC hash matches
            int macHash = 0;
            for (int i = 0; i < 6; ++i) {
                macHash += (int)address[i];
            }
            macHash &= 0xFF;
            XLOGD_INFO("Pairing based on MAC hash, requested MAC hash = 0x%02X, this device = 0x%02X (%s, %s)", 
                    m_pairingMacHash, macHash, name.c_str(), address.toString().c_str());
            if (m_pairingMacHash != macHash) {
                return;
            }
        // Device not found through conventional means or MAC hash so let's check a mac address list
        // Pairing via a mac address list clears supported names and the pairing mac hash
        } else if (m_pairingMacList.size() != 0) {
            if (m_pairingMacList.size() != 0) {
                // check if the mac address matches any of the ones in the filter list (if it exists)
                bool found = false;
                for (const auto &filterAddress : m_pairingMacList) {
                    if (address == filterAddress) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    XLOGD_DEBUG("device with address %s is not in the mac address filter list - ignoring", address.toString().c_str());
                    return;
                }
            }
        } else {
            // log an error if we don't already have a target device
            if (m_targetAddress.isNull()) {
                XLOGD_INFO("Device (%s, %s) not targeted for pairing, ignoring...", 
                        name.c_str(), address.toString().c_str());
            }
            return;
        }
    }

    // if we don't already have a target address then store this now
    if (m_targetAddress.isNull()) {

        // is the device currently paired? if so we have to remove (unpair)
        // it and then remain in the current state
        if (m_adapter->isDevicePaired(address)) {
            if (m_adapter->isDeviceConnected(address)) {
                XLOGD_INFO("Ignoring device (%s, %s)... it is currently paired and connected, no need to re-pair.", 
                        name.c_str(), address.toString().c_str());
                return;
            }

            XLOGD_INFO("Found target device (%s, %s) but it's currently paired.  Will unpair and wait till it shows up in a scan again.", 
                    name.c_str(), address.toString().c_str());

            m_adapter->removeDevice(address);
            return;
        }

        XLOGD_INFO("Found target device (%s, %s)", name.c_str(), address.toString().c_str());

        // store the target address
        m_targetAddress = address;

    } else if (m_targetAddress != address) {

        // this may happen if two remotes have the same pairing prefix,
        // in such situations we stick with the first one we found, there
        // is no way to know which is the correct device to pair to
        XLOGD_WARN("Device (%s, %s) is targeted for pairing, but we already have a target (%s).  Keeping previous target.",
                name.c_str(), address.toString().c_str(), m_targetAddress.toString().c_str());
        return;
    }

    // post the event to move the state machine
    m_stateMachine.postEvent(DeviceFoundEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot expected to be called from outside this object to indicate that
    a new device has been detected by the bluetooth adapter.

    The \a address and \a name of the added device should be supplied, these
    are checked to see if they match the pairing target.

 */
void BleRcuPairingStateMachine::onDeviceFound(const BleAddress &address,
                                              const string &name)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    XLOGD_DEBUG("device added %s %s (target %s)", 
            address.toString().c_str(), name.c_str(), m_targetAddress.toString().c_str());

    processDevice(address, name);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called by the bluez code when a remote device has been removed.  This means
    the device has been unpaired and has disconnected.

 */
void BleRcuPairingStateMachine::onDeviceRemoved(const BleAddress &address)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    XLOGD_DEBUG("device removed %s (target %s)", address.toString().c_str(), m_targetAddress.toString().c_str());

    // check if the device removed is the one we're targeting
    if (!m_targetAddress.isNull() && (m_targetAddress == address)) {
        m_stateMachine.postEvent(DeviceRemovedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    Slot expected to be called from outside this object to indicate that
    a new device has changed it's name.

    The \a address and new \a name of the device should be supplied, these
    are checked to see if they match the pairing target.

 */
void BleRcuPairingStateMachine::onDeviceNameChanged(const BleAddress &address,
                                                    const string &name)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    XLOGD_DEBUG("device name changed %s %s (target %s)", 
            address.toString().c_str(), name.c_str(), m_targetAddress.toString().c_str());

    processDevice(address, name);
}

// -----------------------------------------------------------------------------
/*!
    Slot expected to be called from outside this object to indicate that
    a new device has changed it's name.

    The \a address and new \a name of the device should be supplied, these
    are checked to see if they match the pairing target.

 */
void BleRcuPairingStateMachine::onDevicePairingError(const BleAddress &address,
                                                    const string &error)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    XLOGD_ERROR("Device (%s) pairing failed, shutting down state machine...", address.toString().c_str());

    // stop the pairing and setup timeout timers
    m_stateMachine.cancelDelayedEvents(PairingTimeoutEvent);
    m_stateMachine.postEvent(PairingErrorEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when a device's \a paired state changes.

    We check if the device who's paired state has changed is our target device
    and if it is now paired then we send a \l{DevicePairedEvent} event to the
    state machine.

    \note There is deliberately no event emitted for an unpaired event, we leave
    it to the timeouts to handle that case.
 */
void BleRcuPairingStateMachine::onDevicePairingChanged(const BleAddress &address,
                                                       bool paired)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    // check if the device whos pairing has changed is the one we're trying to pair to
    if (!m_targetAddress.isNull() && (m_targetAddress == address)) {
        if (paired) {
            m_stateMachine.postEvent(DevicePairedEvent);
        } else {
            m_stateMachine.postEvent(DeviceUnpairedEvent);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the given device has become 'ready'.  Ready is the state
    where the device is bonded and connected and all the GATT services have been
    resolved and lastly our GATT service objects have run through their
    initialisation code (successfully).

 */
void BleRcuPairingStateMachine::onDeviceReadyChanged(const BleAddress &address,
                                                     bool ready)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    // check if the device now ready is the one we're trying to pair to
    if (!m_targetAddress.isNull() && (m_targetAddress == address)) {
        if (ready) {
            m_pairingSuccesses++;
            m_pairingSucceeded = true;
            m_stateMachine.postEvent(DeviceReadyEvent);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when lost connection to the BLE adaptor, this should never
    really happen and means we've lost connection to the bluetoothd daemon. The
    only sensible thing we can do is abort that pairing.

 */
void BleRcuPairingStateMachine::onAdapterPoweredChanged(bool powered)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }

    if (!powered) {
        m_stateMachine.postEvent(AdapterPoweredOffEvent);
    }
}
