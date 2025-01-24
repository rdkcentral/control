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
//  blercucscannerstatemachine.cpp
//

#include "blercuscannerstatemachine.h"
#include "blercuadapter.h"

#include "configsettings/configsettings.h"

#include "ctrlm_log_ble.h"

#include <cinttypes>


using namespace std;

BleRcuScannerStateMachine::BleRcuScannerStateMachine(const shared_ptr<const ConfigSettings> &config,
                                                     const shared_ptr<BleRcuAdapter> &adapter)
    : m_isAlive(make_shared<bool>(true))
    , m_adapter(adapter)
    , m_scanTimeoutMs(-1)
{
    // constructs a map of name printf style formats for searching for device
    // names that match
    for (const ConfigModelSettings &model : config->modelSettings()) {
        if (!model.disabled()) {
            m_supportedPairingNames.push_back(model.scanNameMatcher());
        }
    }

    // setup (but don't start) the state machine
    setupStateMachine();

    // connect up the events from the manager
    m_adapter->addDiscoveryChangedSlot(Slot<bool>(m_isAlive,
            std::bind(&BleRcuScannerStateMachine::onDiscoveryChanged, this, std::placeholders::_1)));

    m_adapter->addDeviceFoundSlot(Slot<const BleAddress&, const std::string&>(m_isAlive,
            std::bind(&BleRcuScannerStateMachine::onDeviceFound, this, std::placeholders::_1, std::placeholders::_2)));

    m_adapter->addDeviceNameChangedSlot(Slot<const BleAddress&, const std::string&>(m_isAlive,
            std::bind(&BleRcuScannerStateMachine::onDeviceNameChanged, this, std::placeholders::_1, std::placeholders::_2)));

    m_adapter->addPoweredChangedSlot(Slot<bool>(m_isAlive,
            std::bind(&BleRcuScannerStateMachine::onAdapterPoweredChanged, this, std::placeholders::_1)));
}

BleRcuScannerStateMachine::~BleRcuScannerStateMachine()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures the internal state machine object.

 */
void BleRcuScannerStateMachine::setupStateMachine()
{
    // set the name of the state machine for logging
    m_stateMachine.setObjectName("ScannerStateMachine");
    m_stateMachine.setGMainLoop(m_adapter->getGMainLoop());

    // add all the states
    m_stateMachine.addState(RunningSuperState, "RunningSuperState");
    m_stateMachine.addState(RunningSuperState, StartingDiscoveryState, "StartingDiscoveryState");
    m_stateMachine.addState(RunningSuperState, DiscoveringState, "DiscoveringState");
    m_stateMachine.addState(RunningSuperState, StoppingDiscoveryState, "StoppingDiscoveryState");
    m_stateMachine.addState(FinishedState, "FinishedState");


    // add the transitions       From State           ->    Event                  ->   To State
    m_stateMachine.addTransition(RunningSuperState,         AdapterPoweredOffEvent,     FinishedState);

    m_stateMachine.addTransition(StartingDiscoveryState,    DiscoveryStartedEvent,      DiscoveringState);
    m_stateMachine.addTransition(StartingDiscoveryState,    CancelRequestEvent,         StoppingDiscoveryState);
    m_stateMachine.addTransition(StartingDiscoveryState,    DiscoveryStartTimeoutEvent, FinishedState);

    m_stateMachine.addTransition(DiscoveringState,          DeviceFoundEvent,           StoppingDiscoveryState);
    m_stateMachine.addTransition(DiscoveringState,          CancelRequestEvent,         StoppingDiscoveryState);
    m_stateMachine.addTransition(DiscoveringState,          DiscoveryTimeoutEvent,      StoppingDiscoveryState);
    m_stateMachine.addTransition(DiscoveringState,          DiscoveryStoppedEvent,      FinishedState);

    m_stateMachine.addTransition(StoppingDiscoveryState,    DiscoveryStoppedEvent,      FinishedState);
    m_stateMachine.addTransition(StoppingDiscoveryState,    DiscoveryStopTimeoutEvent,  FinishedState);


    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&BleRcuScannerStateMachine::onStateEntry, this, std::placeholders::_1)));

    // set the initial state
    m_stateMachine.setInitialState(StartingDiscoveryState);
    m_stateMachine.setFinalState(FinishedState);
}


// -----------------------------------------------------------------------------
/*!
    Returns \c true if the state machine is currently running.

 */
bool BleRcuScannerStateMachine::isRunning() const
{
    return m_stateMachine.isRunning();
}

// -----------------------------------------------------------------------------
/*!
    Starts the state machine for the scanner.  The scan will run for the given
    \c timeoutMs or until an RCU is found in pairing mode or cancelled.

 */
void BleRcuScannerStateMachine::start(int timeoutMs)
{
    // sanity check
    if (m_stateMachine.isRunning()) {
        XLOGD_ERROR("scanner already running");
        return;
    }

    XLOGD_INFO("starting scanner with timeout %dms", timeoutMs);

    // ensure the found device is cleared
    m_foundDevice.clear();

    // set the discovery time-out
    m_scanTimeoutMs = timeoutMs;

    // start the state machine
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    Cancels the scanning by injecting a cancel event into the state machine
    should clean up the discovery.

    Note this is an async call, you should listen for the finished signal which
    indicates the scanning is finished.

 */
void BleRcuScannerStateMachine::stop()
{
    // sanity check
    if (!m_stateMachine.isRunning()) {
        XLOGD_INFO("scanner not running");
        return;
    }

    XLOGD_INFO("cancelling scanner");

    // post a cancel event and let the state-machine clean up
    m_stateMachine.postEvent(CancelRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when lost connection to the BLE adaptor, this should never
    really happen and means we've lost connection to the bluetoothd daemon. The
    only sensible thing we can do is abort the scanning.

 */
void BleRcuScannerStateMachine::onAdapterPoweredChanged(bool powered)
{
    if (!m_stateMachine.isRunning()) {
        return;
    }
    if (!powered) {
        m_stateMachine.postEvent(AdapterPoweredOffEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the discovery state of the adapter changed.

 */
void BleRcuScannerStateMachine::onDiscoveryChanged(bool discovering)
{
    // ignore if not running
    if (!m_stateMachine.isRunning()) {
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

    Called when a new device is found by the bluetooth adapter.

 */
void BleRcuScannerStateMachine::onDeviceFound(const BleAddress &address,
                                              const string &name)
{
    // ignore if not running or not in the discovery phase
    if (!m_stateMachine.isRunning() || !m_stateMachine.inState(DiscoveringState)) {
        return;
    }

    // process the new device
    processDevice(address, name);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a device's name has changed.

 */
void BleRcuScannerStateMachine::onDeviceNameChanged(const BleAddress &address,
                                                    const string &name)
{
    // ignore if not running or not in the discovery phase
    if (!m_stateMachine.isRunning() || !m_stateMachine.inState(DiscoveringState)) {
        return;
    }

    // process the new device name
    processDevice(address, name);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a device name has changed or a new device is found.  Here we
    check if the device name indicates it's an RCU in pairing mode.

 */
void BleRcuScannerStateMachine::processDevice(const BleAddress &address,
                                              const string &name)
{
    // if we've already found a target then skip out early
    if (!m_foundDevice.isNull()) {
        return;
    }

    // check if the name is a match for one of our RCU types
    vector<regex>::const_iterator it_name = m_supportedPairingNames.begin();
    for (; it_name != m_supportedPairingNames.end(); ++it_name) {
        if (std::regex_match(name.c_str(), *it_name)) {
            XLOGD_INFO("Scanner state machine matched remote name successfully, name: %s, address: %s", 
                    name.c_str(), address.toString().c_str());
            break;
        }
    }

    if (it_name == m_supportedPairingNames.end()) {
        return;
    }

    if (m_adapter->isDevicePaired(address) && m_adapter->isDeviceConnected(address)) {
        XLOGD_INFO("Ignoring device... it is currently paired and connected - address: %s, name: %s", 
                address.toString().c_str(), name.c_str());
        return;
    }

    XLOGD_INFO("found pairable device %s with name %s", address.toString().c_str(), name.c_str());

    // store the address
    m_foundDevice.address = address;
    m_foundDevice.name = name;

    // update the state machine
    m_stateMachine.postEvent(DeviceFoundEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when entering a new state.

 */
void BleRcuScannerStateMachine::onStateEntry(int state)
{
    switch (state) {
        case StartingDiscoveryState:
            onEnteredStartDiscoveryState();
            break;
        case DiscoveringState:
            onEnteredDiscoveringState();
            break;
        case StoppingDiscoveryState:
            onEnteredStopDiscoveryState();
            break;
        case FinishedState:
            onEnteredFinishedState();
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'start discovering' state.  This is where we
    request the adapter to start discovery.

    This method also emits the started() signal.

 */
void BleRcuScannerStateMachine::onEnteredStartDiscoveryState()
{
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

        // post a timed-out delay message, if we're still in the 'start
        // discovery' phase when the timeout event arrives we cancel it
        m_stateMachine.postDelayedEvent(DiscoveryStartTimeoutEvent, 5000);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'discovering' state.  At this point we query the
    manager for the current list of devices and their names.  We use this to
    determine if any existing devices match the name of a pairable RCU.

 */
void BleRcuScannerStateMachine::onEnteredDiscoveringState()
{
    // if the scanner was started with a timeout then add a delayed event
    // to the state machine that'll stop the scanner after x number of milliseconds
    if (m_scanTimeoutMs >= 0) {
        m_stateMachine.postDelayedEvent(DiscoveryTimeoutEvent, m_scanTimeoutMs);
    }

    // get the current list of devices
    const map<BleAddress, string> deviceNames = m_adapter->deviceNames();

    // process each existing device
    map<BleAddress, string>::const_iterator it = deviceNames.begin();
    for (; it != deviceNames.end(); ++it) {
        processDevice(it->first, it->second);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called upon entry to the 'Stopping Discovery' state, here we send the request
    to stop the discovery.

 */
void BleRcuScannerStateMachine::onEnteredStopDiscoveryState()
{
    // send the request to stop discovery
    m_adapter->stopDiscovery();

    // check if already stopped
    if (!m_adapter->isDiscovering()) {
        m_stateMachine.postEvent(DiscoveryStoppedEvent);

    } else {
        // post a timed-out delay message, if we're still in the 'stop
        // discovery' phase when the event arrives we assuem something
        // has gone wrong and give up
        m_stateMachine.postDelayedEvent(DiscoveryStopTimeoutEvent, 3000);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when entering the 'finished' state.  We can enter this state if
    we've failed, been cancelled or on success.  Regardless we emit the finished
    signal.

    If we did manage to find a target device then we also emit the
    foundPairableDevice() signal.

 */
void BleRcuScannerStateMachine::onEnteredFinishedState()
{
    // if we found a device then tell any clients
    if (!m_foundDevice.address.isNull()) {
        m_foundPairableDeviceSlots.invoke(m_foundDevice.address, m_foundDevice.name);
        m_foundDevice.clear();
    } else {
        m_failedSlots.invoke();
    }

    // and we're done
    m_finishedSlots.invoke();
}
