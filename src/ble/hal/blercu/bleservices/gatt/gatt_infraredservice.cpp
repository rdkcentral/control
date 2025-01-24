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
//  gatt_infraredservice.cpp
//

#include "gatt_infraredservice.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"

#include "ctrlm_log_ble.h"

#include <functional>

using namespace std;

const BleUuid GattInfraredService::m_serviceUuid(BleUuid::RdkInfrared);


// -----------------------------------------------------------------------------
/*!
    Constructs the infrared GATT service.

 */
GattInfraredService::GattInfraredService(const ConfigModelSettings &settings,
                                         const shared_ptr<const GattDeviceInfoService> &deviceInfo,
                                         GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_GMainLoop(mainLoop)
    , m_deviceInfo(deviceInfo)
    , m_irStandbyMode(StandbyModeB)
    , m_codeId(-1)
    , m_irSupport(0)
{

    if (settings.standbyMode() == "C") {
        m_irStandbyMode = StandbyModeC;
    } else {
        m_irStandbyMode = StandbyModeB;
    }

    XLOGD_INFO("will set IR standby mode %c", (m_irStandbyMode == StandbyModeC) ? 'C' : 'B');


    // setup the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

// -----------------------------------------------------------------------------
/*!
    Desctructs the infrared GATT service, stopping it.

 */
GattInfraredService::~GattInfraredService()
{
    *m_isAlive = false;

    // stop the service if it's not already
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.

 */
BleUuid GattInfraredService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattInfraredService::init()
{
    m_stateMachine.setObjectName("GattInfraredService");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(StartingSuperState, "StartingSuperState");
    m_stateMachine.addState(StartingSuperState, SetStandbyModeState, "SetStandbyMode");
    m_stateMachine.addState(StartingSuperState, GetIrSignalsState, "GetIrSignals");
    m_stateMachine.addState(RunningState, "Running");


    // add the transitions:      From State         ->  Event                   ->  To State
    m_stateMachine.addTransition(IdleState,             StartServiceRequestEvent,   SetStandbyModeState);

    m_stateMachine.addTransition(SetStandbyModeState,   SetIrStandbyModeEvent,      GetIrSignalsState);
    m_stateMachine.addTransition(GetIrSignalsState,     IrSignalsReadyEvent,        RunningState);

    m_stateMachine.addTransition(StartingSuperState,    StopServiceRequestEvent,    IdleState);
    m_stateMachine.addTransition(RunningState,          StopServiceRequestEvent,    IdleState);


    // add a slot for state machine notifications
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattInfraredService::onEnteredState, this, std::placeholders::_1)));


    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Loops through all the IrSignal characteristics on this service and creates
    GattInfraredSignal objects to wrap them (if we don't already have a wrapper).

 */
void GattInfraredService::getSignalCharacteristics(const shared_ptr<BleGattService> &gattService)
{
    // get all the characteristics
    const vector< shared_ptr<BleGattCharacteristic> > characteristics =
        gattService->characteristics(BleUuid::InfraredSignal);


    // add a matching IrSignal if we don't already have one
    for (const shared_ptr<BleGattCharacteristic> &characteristic : characteristics) {

        // check if we already have this characteristic wrapped
        bool haveSignal = false;
        for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {

            if (irSignal && irSignal->isValid() &&
                    (irSignal->instanceId() == characteristic->instanceId())) {

                haveSignal = true;
                break;
            }
        }

        if (haveSignal) {
            continue;
        }

        // create a wrapper around the char
        shared_ptr<GattInfraredSignal> irSignal = make_shared<GattInfraredSignal>(characteristic, m_GMainLoop);
        if (irSignal && irSignal->isValid()) {
            m_irSignals.push_back(irSignal);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Starts the service

 */
bool GattInfraredService::start(const shared_ptr<BleGattService> &gattService)
{
    // sanity check the supplied info is valid
    if (!gattService || !gattService->isValid() ||
            (gattService->uuid() != m_serviceUuid)) {

        XLOGD_WARN("invalid infrared gatt service info");
        return false;
    }


    // create the bluez dbus proxy to the standby mode characteristic
    if (!m_standbyModeCharacteristic || !m_standbyModeCharacteristic->isValid()) {

        m_standbyModeCharacteristic = gattService->characteristic(BleUuid::InfraredStandby);
        if (!m_standbyModeCharacteristic || !m_standbyModeCharacteristic->isValid()) {
            XLOGD_WARN("failed to create proxy to the ir standby mode characteristic");
            m_standbyModeCharacteristic.reset();
        }
    }

    // create the bluez dbus proxy to the ir support characteristic
    if (!m_irSupportCharacteristic || !m_irSupportCharacteristic->isValid()) {

        m_irSupportCharacteristic = gattService->characteristic(BleUuid::InfraredSupport);
        if (!m_irSupportCharacteristic || !m_irSupportCharacteristic->isValid()) {
            XLOGD_WARN("failed to create proxy to the ir support characteristic");
            m_irSupportCharacteristic.reset();
        }
    }

    // create the bluez dbus proxy to the ir control characteristic
    if (!m_irControlCharacteristic || !m_irControlCharacteristic->isValid()) {

        m_irControlCharacteristic = gattService->characteristic(BleUuid::InfraredControl);
        if (!m_irControlCharacteristic || !m_irControlCharacteristic->isValid()) {
            XLOGD_WARN("failed to create proxy to the ir control characteristic");
            m_irControlCharacteristic.reset();
        }
    }

    // create the bluez dbus proxy to the code id characteristic
    if (!m_codeIdCharacteristic || !m_codeIdCharacteristic->isValid()) {

        m_codeIdCharacteristic = gattService->characteristic(BleUuid::InfraredCodeId);
        if (!m_codeIdCharacteristic || !m_codeIdCharacteristic->isValid()) {
            XLOGD_WARN("failed to create proxy to the code id characteristic");
            m_codeIdCharacteristic.reset();
        }
    }

    // create the bluez dbus proxy to the emit ir signal characteristic
    if (!m_emitIrCharacteristic || !m_emitIrCharacteristic->isValid()) {

        m_emitIrCharacteristic = gattService->characteristic(BleUuid::EmitInfraredSignal);
        if (!m_emitIrCharacteristic || !m_emitIrCharacteristic->isValid()) {
            XLOGD_WARN("failed to create proxy to the emit ir signal characteristic");
            m_emitIrCharacteristic.reset();
        }
    }

    // creates the GattInfraredSignal objects matched against the characteristics
    getSignalCharacteristics(gattService);

    requestCodeId();
    requestIrSupport();


    // check we're not already started
    if (m_stateMachine.state() != IdleState) {
        XLOGD_WARN("service already started");
        return true;
    }

    m_stateMachine.postEvent(StartServiceRequestEvent);
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattInfraredService::stop()
{
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
bool GattInfraredService::isReady() const
{
    return m_stateMachine.inState(RunningState);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattInfraredService::onEnteredState(int state)
{
    switch (state) {

        case IdleState:
            onEnteredIdleState();
            break;

        case SetStandbyModeState:
            onEnteredSetStandbyModeState();
            break;
        case GetIrSupportState:
            requestIrSupport();
            break;
        case GetCodeIdState:
            requestCodeId();
            break;
        case GetIrSignalsState:
            onEnteredGetIrSignalsState();
            break;

        case RunningState:
            // entered running state so emit the ready signal
            m_readySlots.invoke();
            break;

    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'idle' state, used to stop all the GattInfraredSignal
    objects.

 */
void GattInfraredService::onEnteredIdleState()
{
    // this doesn't do much, just aborts any outstanding operation on the
    // ir signal objects
    for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {
        if (irSignal) {
            irSignal->stop();
        }
    }

}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called at service start time, it sets the standby mode to match the current
    platform.  In theory we only need to do this once after pairing, however
    reading the value then writing is more hassle than just writing the value
    everytime.

 */
void GattInfraredService::onEnteredSetStandbyModeState()
{
    // sanity check we actually have a standby mode characteristic
    if (!m_standbyModeCharacteristic || !m_standbyModeCharacteristic->isValid()) {
        XLOGD_WARN("missing standby mode characteristic");
        m_stateMachine.postEvent(SetIrStandbyModeEvent);
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to write IR standby mode due to <%s>",
                        reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("Successfully set IR standby mode %c",
                        (m_irStandbyMode == StandbyModeC) ? 'C' : 'B');
            }

            // tell the state machine we are now ready, even if we failed
            m_stateMachine.postEvent(SetIrStandbyModeEvent);
        };


    // standby mode C is not supported on LC103s with versions < 5103.2.6
    if (m_deviceInfo->softwareVersion().empty()) {
        XLOGD_WARN("sw version of remote is not available yet");
    } else {
        vector<unsigned long> modeCVersionSupported = {5103, 2, 6};
        vector<unsigned long> versionSplit;

        std::istringstream iss(m_deviceInfo->softwareVersion());
        std::string token;
        unsigned long ul;

        while (std::getline(iss, token, '.')) {
            errno = 0;
            ul = strtoul (token.c_str(), NULL, 0);
            if (errno) {
                versionSplit.push_back(0);
            } else {
                versionSplit.push_back(ul);
            }
        }


        //the first number 5103 is an identifying number for LC103 firmware
        if (modeCVersionSupported[0] == versionSplit[0]) {
            for (unsigned int i = 1; i < versionSplit.size() && i < modeCVersionSupported.size(); i++) {
                if (versionSplit[i] < modeCVersionSupported[i]) {

                    XLOGD_WARN("LC103 firmware version %s does not support IR standby mode C, setting mode B instead",
                            m_deviceInfo->softwareVersion().c_str());

                    m_irStandbyMode = StandbyModeB;
                    break;

                } else if (versionSplit[i] > modeCVersionSupported[i]) {
                    break;
                }
            }
        }
    }
    const uint8_t value = (m_irStandbyMode == StandbyModeC) ? 0x00 : 0x01;

    // send a request to write the standby mode
    m_standbyModeCharacteristic->writeValue(vector<uint8_t>(1, value), PendingReply<>(m_isAlive, replyHandler));
}
// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattInfraredService::requestIrSupport()
{
    // sanity check we actually have a ir support characteristic
    if (!m_irSupportCharacteristic || !m_irSupportCharacteristic->isValid()) {
        XLOGD_WARN("missing ir support characteristic");
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {

                XLOGD_ERROR("failed to get initial ir support code due to <%s>",
                        reply->errorMessage().c_str());

                m_irSupport = 0;

            } else {

                std::vector<uint8_t> value;
                value = reply->result();

                if (value.size() == 1) {
                    uint8_t irSupport_ = value[0];
                    XLOGD_INFO("IR support value = 0x%x", irSupport_);
                    if (irSupport_ != m_irSupport) {
                        m_irSupport = irSupport_;
                        m_irSupportChangedSlots.invoke(m_irSupport);
                    }
                } else {
                    XLOGD_ERROR("IR Support value received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to write the ir support value
    m_irSupportCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a request to get the current tv codes id.

 */
void GattInfraredService::requestCodeId()
{
    // sanity check we actually have a codeID characteristic
    if (!m_codeIdCharacteristic || !m_codeIdCharacteristic->isValid()) {
        XLOGD_WARN("missing code id characteristic");
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {

                XLOGD_ERROR("failed to get initial ir codeId due to <%s>",
                        reply->errorMessage().c_str());

            } else {

                std::vector<uint8_t> value;
                value = reply->result();

                if (value.size() >= 4) {
                    int32_t codeId_ = (int32_t(value[0]) << 0)  |
                                      (int32_t(value[1]) << 8)  |
                                      (int32_t(value[2]) << 16) |
                                      (int32_t(value[3]) << 24);

                    XLOGD_INFO("IR code ID = %d", codeId_);

                    if (codeId_ != m_codeId) {
                        m_codeId = codeId_;
                        m_codeIdChangedSlots.invoke(m_codeId);
                    }

                } else {
                    XLOGD_ERROR("IR codeId value received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to write the standby mode
    m_codeIdCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'getting ir signals' state, which means we have a
    list of Infrared Signal characteristics, but we don't know which keys they
    belong to so we need to read each of their descriptors.


 */
void GattInfraredService::onEnteredGetIrSignalsState()
{
    // iterator through all the signal objects, any that aren't ready need
    // to be poked to start
    for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {

        // irSignal->addReadySlot(Slot<>(m_isAlive, 
        //         std::bind(&GattInfraredService::onIrSignalReady, this)));

        // start the individual IR signal
        irSignal->start();
    }

    // Don't wait for actual ready event from each signal, because there are no
    // retries if it fails to read the descriptor of the signal, which means it will
    // hold up initialization of the remote indefinitely.
    // Further, in the original implementation of this, "ready" didn't mean that it
    // successfully read the descriptor.  It simply meant that the request was sent to
    // read the descriptor.
    // So either way, doing this here is in line with the previous implementation
    m_stateMachine.postEvent(IrSignalsReadyEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when one of the GattInfraredSignal objects has become ready (has
    read it's reference descriptor).  We check again if now all the signals
    are ready, if they are then emit a signal to the state machine.

 */
void GattInfraredService::onIrSignalReady()
{
    // get the count of ready signals
    unsigned int readyCount = 0;
    for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {
        if (irSignal && irSignal->isReady()) {
            readyCount++;
        }
    }

    // check if they're all ready
    if (readyCount == m_irSignals.size()) {
        m_stateMachine.postEvent(IrSignalsReadyEvent);
    }
}


// -----------------------------------------------------------------------------
/*!
    \overload

 */
int32_t GattInfraredService::codeId() const
{
    return m_codeId;
}

uint8_t GattInfraredService::irSupport() const
{
    return m_irSupport;
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to write the \a codeId value into the RCU device.  The returned
    value is a Future that will triggered when the write completes or fails
    with an error.

 */
void GattInfraredService::writeCodeIdValue(int32_t codeId, PendingReply<> &&reply)
{
    // we don't need to check the current state as that should have been done before calling this

    // sanity check we have characteristic to write to though
    if (!m_codeIdCharacteristic || !m_codeIdCharacteristic->isValid()) {
        reply.setError("Missing codeId characteristic");
        reply.finish();
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [codeId, reply](PendingReply<> *dbusReply) mutable
        {
            // check for errors
            if (dbusReply->isError()) {
                XLOGD_WARN("failed to write codeId %d due to <%s>", codeId, 
                        dbusReply->errorMessage().c_str());
                
                reply.setError(dbusReply->errorMessage());

            } else {
                XLOGD_INFO("successfully set code id to %d", codeId);
            }
            reply.finish();
        };


    //TODO: remotes now support both TV and AVR codes, but they are alpha-numeric
    // 5 character codes.  So if we want to use this characeristic on the remote
    // we need to handle the ascii characters.  But this characteristic isn't 
    // currently being used, so do this later when requirements demand it.

    // construct the value to write
    struct {
        int32_t tvCodeId;
        int32_t ampCodeId;
    } __attribute__((packed)) codeIds = { codeId, -1 };

    const char* charArray = reinterpret_cast<const char*>(&codeIds);
    const vector<uint8_t> value(charArray, charArray + sizeof(codeIds));


    // send a request to write the the code ID value
    m_codeIdCharacteristic->writeValue(value, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattInfraredService::eraseIrSignals(PendingReply<> &&reply)
{
    // check the service is ready
    if (m_stateMachine.state() != RunningState) {
        reply.setError("Service not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_outstandingOperation && !m_outstandingOperation->isFinished()) {
        reply.setError("Service is busy");
        reply.finish();
        return;
    }

    m_outstandingOperation.reset();


    // for each signal we just program empty data, this will typically just
    // disable the signal rather than actually programming it
    m_outstandingOperation = make_shared<FutureAggregator>(std::move(reply));

    for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {

        if (!irSignal) {
            continue;
        }
        // request the operation and add the resulting future to the aggregator
        irSignal->program(vector<uint8_t>(), m_outstandingOperation->connectReply());
    }


    // check we've queued at least one option
    if (m_outstandingOperation->isEmpty()) {
        m_outstandingOperation->setError("no requests sent");
        m_outstandingOperation->finish();
        m_outstandingOperation.reset();
    }
}



// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattInfraredService::setIrControl(const uint8_t irControl, PendingReply<> &&reply)
{
    if (!m_irControlCharacteristic || !m_irControlCharacteristic->isValid()) {
        XLOGD_WARN("irControl characteristic is missing/invalid but continuing anyway without an error...");
        reply.finish();
        return;
    }

    XLOGD_INFO("Writing 0x%x to IR control characteristic", irControl);
    const vector<uint8_t> value(1, irControl);
    m_irControlCharacteristic->writeValue(value, std::move(reply));
}


void GattInfraredService::programIrSignalWaveforms(const map<uint32_t, vector<uint8_t>> &irWaveforms, 
                                                   const uint8_t irControl, PendingReply<> &&reply)
{
    // sanity check we've been asked to program at least one key
    if (irWaveforms.empty()) {
        reply.setError("Provided IR waveforms are empty");
        reply.finish();
        return;
    }

    // check the service is ready
    if (m_stateMachine.state() != RunningState) {
        reply.setError("Service not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_outstandingOperation && !m_outstandingOperation->isFinished()) {
        reply.setError("Service is busy");
        reply.finish();
        return;
    }

    m_outstandingOperation.reset();


    // lambda invoked when the request returns
    auto replyHandler = [this, irWaveforms](PendingReply<> *dbusReply) mutable
        {
            if (!m_outstandingOperation) {
                XLOGD_WARN("Cannot continue IR programming due to m_outstandingOperation == NULL"); 
                return;
            }

            // check for errors
            if (dbusReply->isError()) {
                XLOGD_WARN("Cannot continue IR programming due to <%s>", dbusReply->errorMessage().c_str());

                m_outstandingOperation->setError(dbusReply->errorMessage());
                m_outstandingOperation->finish();
                m_outstandingOperation.reset();

            } else {

                for (const shared_ptr<GattInfraredSignal> &irSignal : m_irSignals) {

                    if (!irSignal) {
                        continue;
                    }

                    // check the key is in the dataset to program
                    const uint32_t keyCode = (uint32_t) irSignal->keyCode();
                    if (irWaveforms.find(keyCode) == irWaveforms.end()) {
                        continue;
                    }

                    // request the operation and add the resulting future to the aggregator
                    irSignal->program(irWaveforms.at(keyCode), m_outstandingOperation->connectReply());
                }


                // check we've queued at least one option
                if (m_outstandingOperation->isEmpty()) {
                    m_outstandingOperation->setError("no requests sent");
                    m_outstandingOperation->finish();
                    m_outstandingOperation.reset();
                }
            }
        };


    // create an aggregation of all the pending results
    m_outstandingOperation = make_shared<FutureAggregator>(std::move(reply));


    // Check and write the ir control characteristic
    setIrControl(irControl, PendingReply<>(m_isAlive, replyHandler));
}


// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattInfraredService::emitIrSignal(uint32_t keyCode, PendingReply<> &&reply)
{
    // check the service is ready
    if (!isReady() || !m_emitIrCharacteristic) {
        reply.setError("Service not ready");
        reply.finish();
        return;
    }

    // convert the key code a value specified in the GATT spec (table 2.2)
    const uint8_t gattKeyCode = keyCodeToGattValue((GattInfraredSignal::Key)keyCode);
    if (gattKeyCode == 0xff) {
        reply.setError("Invalid key code");
        reply.finish();
        return;
    }

    // send a request to emit the IR key code
    m_emitIrCharacteristic->writeValue(vector<uint8_t>(1, gattKeyCode), std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Converts a \a keyCode from the enum to the byte value used in the GATT spec.

    Unknown / invalid key codes are converted to \c 0xff.

 */
uint8_t GattInfraredService::keyCodeToGattValue(GattInfraredSignal::Key keyCode) const
{
    switch (keyCode) {
        case GattInfraredSignal::Key_PowerPrimary:      return 0x0C;
        case GattInfraredSignal::Key_PowerSecondary:    return 0x0B;
        case GattInfraredSignal::Key_Input:             return 0x29;
        case GattInfraredSignal::Key_VolUp:             return 0x10;
        case GattInfraredSignal::Key_VolDown:           return 0x11;
        case GattInfraredSignal::Key_Mute:              return 0x0D;
        case GattInfraredSignal::Key_Select:            return 0x5C;
        case GattInfraredSignal::Key_Up:                return 0x58;
        case GattInfraredSignal::Key_Left:              return 0x5A;
        case GattInfraredSignal::Key_Right:             return 0x5B;
        case GattInfraredSignal::Key_Down:              return 0x59;
        default:
            XLOGD_WARN("unknown key code %d", keyCode);
            return 0xff;
    }
}

