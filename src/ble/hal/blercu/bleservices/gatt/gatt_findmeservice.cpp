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
//  gatt_findmeservice.cpp
//

#include "gatt_findmeservice.h"
#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"
#include "blercu/blercuerror.h"

#include "ctrlm_log_ble.h"

using namespace std;


const BleUuid GattFindMeService::m_serviceUuid(BleUuid::ImmediateAlert);


GattFindMeService::GattFindMeService(GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_level(0)
{
    // initialise the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattFindMeService::~GattFindMeService()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.

 */
BleUuid GattFindMeService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Initialises and starts the state machine.  The state machine always starts
    in the idle state.

 */
void GattFindMeService::init()
{
    m_stateMachine.setObjectName("GattFindMeService");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(StartingState, "Starting");
    m_stateMachine.addState(RunningState, "Running");


    // add the transitions:      From State  -> Event                   ->  To State
    m_stateMachine.addTransition(IdleState,     StartServiceRequestEvent,   StartingState);
    m_stateMachine.addTransition(StartingState, ServiceReadyEvent,          RunningState);
    m_stateMachine.addTransition(StartingState, StopServiceRequestEvent,    IdleState);
    m_stateMachine.addTransition(RunningState,  StopServiceRequestEvent,    IdleState);


    // connect to the state entry signal
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattFindMeService::onEnteredState, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&GattFindMeService::onExitedState, this, std::placeholders::_1)));


    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if not if the service is ready
 */
bool GattFindMeService::isReady() const
{
    return m_stateMachine.inState(RunningState);
}

// -----------------------------------------------------------------------------
/*!
    Returns the current beeping state of the find me service
 */
BleRcuFindMeService::State GattFindMeService::state() const
{
    switch (m_level) {
        case 0:
            return BleRcuFindMeService::State::BeepingOff;
        case 1:
            return BleRcuFindMeService::State::BeepingMid;
        case 2:
            return BleRcuFindMeService::State::BeepingHigh;
        default:
            XLOGD_WARN("unknown find me level");
            return BleRcuFindMeService::State::BeepingOff;
    }
}

// -----------------------------------------------------------------------------
/*!
    Starts the service, this should move the state machine on from the idle
    state to the ready state.

 */
bool GattFindMeService::start(const shared_ptr<const BleGattService> &gattService)
{
    // create the proxy to the characteristic
    if (!m_alertLevelCharacteristic || !m_alertLevelCharacteristic->isValid()) {

        // sanity check the supplied info is valid
        if (!gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
            XLOGD_WARN("invalid alert level gatt service info");
            return false;
        }

        // get the chararacteristic for the alert level
        m_alertLevelCharacteristic = gattService->characteristic(BleUuid::AlertLevel);
        if (!m_alertLevelCharacteristic || !m_alertLevelCharacteristic->isValid()) {
            XLOGD_WARN("failed to get alert level characteristic");
            return false;
        }
    }

    // check we're not already started
    if (m_stateMachine.state() != IdleState) {
        XLOGD_WARN("trying to start an already running findme service");
        return false;
    }

    // send the event to the state machine
    m_stateMachine.postEvent(StartServiceRequestEvent);
    return true;
}

// -----------------------------------------------------------------------------
/*!

 */
void GattFindMeService::stop()
{
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattFindMeService::onEnteredState(int state)
{
    if (state == IdleState) {

        // clear the characteristic, will be re-created on servce start
        m_alertLevelCharacteristic.reset();

    } else if (state == StartingState) {

        // will attempt to disable find me buzzer
        onEnteredStartingState();

    } else if (state == RunningState) {

        // entered running state so emit the ready signal
        m_readySlots.invoke();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'StartingState', here we just send a request to the
    rcu to stop findme, this is just to pipe clean the interface ... on the reply
    we signal that we are ready.

 */
void GattFindMeService::onEnteredStartingState()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("failed to disable findme due to <%s>", reply->errorMessage().c_str());

                // signal we're ready, even though we failed
                m_stateMachine.postEvent(ServiceReadyEvent);
            } else {
                XLOGD_DEBUG("disabled buzzer during start-up");

                // signal we're ready
                m_stateMachine.postEvent(ServiceReadyEvent);
            }
        };

    // set the alert level to 0
    m_level = 0;

    // write the beeper level
    vector<uint8_t> value(1, m_level); 
    
    m_alertLevelCharacteristic->writeValueWithoutResponse(value, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattFindMeService::onExitedState(int state)
{
    if (state == RunningState) {
        // we're leaving the running state so cancel any pending operations with an error
        if (m_promiseResults) {
            m_promiseResults->setError("Service stopped");
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    }
    
}

// -----------------------------------------------------------------------------
/*!
    Starts sending the 'find me' signal, or more precisely adds an event to the
    state machine to trigger it to move into the 'start signalling state', which
    will send the request to the RCU.

    This method will fail and return \c false if the service is not started or
    it is already signalling, i.e. should only be called it \a state() returns
    \a FindMeService::Ready.

 */
void GattFindMeService::setFindMeLevel(uint8_t level, PendingReply<> &&reply)
{
    // check the service is ready
    if (!isReady()) {
        reply.setError("Service is not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_promiseResults) {
        reply.setError("Request already in progress");
        reply.finish();
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply_)
        {
            // check for errors
            if (reply_->isError()) {
                XLOGD_ERROR("failed to signal findme due to <%s>", reply_->errorMessage().c_str());

                // signal the client that we failed
                if (m_promiseResults) {
                    m_promiseResults->setError(reply_->errorMessage());
                    m_promiseResults->finish();
                    m_promiseResults.reset();
                }
            } else {
                // there should be a valid pending results object, if not something has gone wrong
                if (!m_promiseResults) {
                    XLOGD_ERROR("received a dbus reply message with no matching pending operation");
                    return;
                }

                XLOGD_DEBUG("findme signal written successfully");

                // non-error so complete the pending operation and post a message to the caller
                m_promiseResults->finish();
                m_promiseResults.reset();

            }
        };

    // set the new level
    m_level = level;
    vector<uint8_t> value(1, m_level);

    // create a new pending result object to notify the caller when the request is complete
    m_promiseResults = make_shared<PendingReply<>>(std::move(reply));

    // send a request to the vendor daemon start the find me beep, and connect the reply to a listener
    m_alertLevelCharacteristic->writeValueWithoutResponse(value, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    Starts sending the 'find me' signal, or more precisely adds an event to the
    state machine to trigger it to move into the 'start signalling state', which
    will send the request to the RCU.

    This method will fail and return \c false if the service is not started or
    it is already signalling, i.e. should only be called it \a state() returns
    \a GattFindMeService::Ready.

 */
void GattFindMeService::startBeeping(Level level, PendingReply<> &&reply)
{
    // convert the level to a value for the vendor daemon
    uint8_t level_ = 0;
    switch (level) {
        case Level::High:   level_ = 2;   break;
        case Level::Mid:    level_ = 1;   break;
    }

    setFindMeLevel(level_, std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    Stops sending the 'find me' signal.

    This simply post the message to the state machine, if it's in one of the
    beeping states then this will move it to the stopping / stopped state.
    If not in a beeping state then this function doesn't nothing.

 */
void GattFindMeService::stopBeeping(PendingReply<> &&reply)
{
    setFindMeLevel(0, std::move(reply));
}
