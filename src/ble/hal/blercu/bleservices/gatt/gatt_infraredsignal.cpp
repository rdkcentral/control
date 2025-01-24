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
//  gatt_infraredsignal.cpp
//

#include "gatt_infraredsignal.h"
#include "gatt_infraredservice.h"

#include "blercu/blegattcharacteristic.h"
#include "blercu/blegattdescriptor.h"

#include "ctrlm_log_ble.h"

using namespace std;


// -----------------------------------------------------------------------------
/*!
    \class GattInfraredSignal
    \brief This runs the state machine for the individual GATT Infrared Signal
    object.

    One of these objects (and hence the underlying GATT characteristic)
    corresponds to a physical button on the RCU that can be programmed with
    different IR signals.

 */



GattInfraredSignal::GattInfraredSignal(const shared_ptr<BleGattCharacteristic> &gattCharacteristic,
                                       GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_signalCharacteristic(gattCharacteristic)
    , m_keyCode(Key_INVALID)
{

    // create a proxy to the dbus gatt interface
    if (!m_signalCharacteristic || !m_signalCharacteristic->isValid()) {
        XLOGD_ERROR("failed to create proxy to infrared signal");
        return;
    }


    // we also want proxies to the characteristic's two descriptors
    m_signalReferenceDescriptor = gattCharacteristic->descriptor(BleUuid::InfraredSignalReference);
    if (!m_signalReferenceDescriptor || !m_signalReferenceDescriptor->isValid()) {
        XLOGD_ERROR("failed to create proxy to infrared signal reference");
        m_signalCharacteristic.reset();
        return;
    }

    m_signalConfigurationDescriptor = gattCharacteristic->descriptor(BleUuid::InfraredSignalConfiguration);
    if (!m_signalConfigurationDescriptor || !m_signalConfigurationDescriptor->isValid()) {
        XLOGD_ERROR("failed to create proxy to infrared signal configuration");
        m_signalReferenceDescriptor.reset();
        m_signalCharacteristic.reset();
        return;
    }


    // finally initialise the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    initStateMachine();
}

GattInfraredSignal::~GattInfraredSignal()
{
    *m_isAlive = false;
    m_signalConfigurationDescriptor.reset();
    m_signalReferenceDescriptor.reset();
    m_signalCharacteristic.reset();
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Sets up the state machine and starts it in the Idle state.

 */
void GattInfraredSignal::initStateMachine()
{
    m_stateMachine.setObjectName("GattInfraredSignal");


    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(InitialisingState, "Initialising");
    m_stateMachine.addState(ReadyState, "Ready");
    m_stateMachine.addState(ProgrammingSuperState, "ProgrammingSuperState");
    m_stateMachine.addState(ProgrammingSuperState, DisablingState, "Disabling");
    m_stateMachine.addState(ProgrammingSuperState, WritingState, "Writing");
    m_stateMachine.addState(ProgrammingSuperState, EnablingState, "Enabling");


    // add the transitions:      From State         ->  Event               ->  To State
    m_stateMachine.addTransition(IdleState,             StartRequestEvent,      InitialisingState);

    m_stateMachine.addTransition(InitialisingState,     AckEvent,               ReadyState);
    m_stateMachine.addTransition(InitialisingState,     ErrorEvent,             IdleState);
    m_stateMachine.addTransition(InitialisingState,     StopRequestEvent,       IdleState);

    m_stateMachine.addTransition(ReadyState,            ProgramRequestEvent,    DisablingState);

    m_stateMachine.addTransition(ProgrammingSuperState, ErrorEvent,             ReadyState);
    m_stateMachine.addTransition(ProgrammingSuperState, StopRequestEvent,       IdleState);

    m_stateMachine.addTransition(DisablingState,        AckEvent,               WritingState);
    m_stateMachine.addTransition(WritingState,          AckEvent,               EnablingState);
    m_stateMachine.addTransition(EnablingState,         AckEvent,               ReadyState);


    // add a slot for state machine notifications
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattInfraredSignal::onEnteredState, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&GattInfraredSignal::onExitedState, this, std::placeholders::_1)));


    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}


// -----------------------------------------------------------------------------
/*!
    Returns \c true if the characteristic is valid.

 */
bool GattInfraredSignal::isValid() const
{
    return (m_signalCharacteristic != nullptr);
}

// -----------------------------------------------------------------------------
/*!
    Returns the instance id of the characteristic that this object is wrapping.

 */
int GattInfraredSignal::instanceId() const
{
    if (!m_signalCharacteristic) {
        return -1;
    }

    return m_signalCharacteristic->instanceId();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the characteristic is valid.

 */
bool GattInfraredSignal::isReady() const
{
    return !m_stateMachine.inState(IdleState);
}

// -----------------------------------------------------------------------------
/*!
    Returns the key code of the physical button that this characteristic
    represents.  This is not initially known, you need to wait till we
    transition to the ready state before we know what the key will be.

    Will return BleRcuInfraredService::InvalidKey if called if the object is not
    valid or is not ready.

 */
GattInfraredSignal::Key GattInfraredSignal::keyCode() const
{
    return m_keyCode;
}

// -----------------------------------------------------------------------------
/*!
    Sends a request to start the service, this will only take affect if the
    the signal is in the Idle state.

 */
void GattInfraredSignal::start()
{
    m_stateMachine.postEvent(StartRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    Stops the signal state machine, this will cancel any outstanding program
    operations and cause their futures to return an error.

 */
void GattInfraredSignal::stop()
{
    m_stateMachine.postEvent(StopRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    Sends a request to program the given data into the signal object.  If the
    object is not valid or ready then an immediately errored future is returned.

    If \a data is empty then the signal data is erased from the RCU.

    If a programming operation is already in progress, the returned future will
    be immediately errored.  This method does NOT queue up writes.

 */
void GattInfraredSignal::program(const vector<uint8_t> &data, PendingReply<> &&reply)
{
    if (m_keyCode == Key_INVALID) {
        // Don't treat this as an error.
        reply.finish();
        return;
    }

    // check if we're ready
    if (!m_stateMachine.inState(ReadyState)) {
        XLOGD_WARN("ir signal (key code = %d) not ready for programming", m_keyCode);
        reply.setError("IR signal is not ready");
        reply.finish();
        return;
    }

    // check we're not already in the middle or programming
    if (m_programmingPromise) {
        XLOGD_WARN("ir signal (key code = %d) is already being programmed", m_keyCode);
        reply.setError("Programming in progress");
        reply.finish();
        return;
    }

    // sanity check the data, it should be less than 256 bytes
    if (data.size() > 256) {
        XLOGD_WARN("ir signal (key code = %d) data is to large, expected to be less than 256 bytes, actual %d bytes", 
                m_keyCode, data.size());
        reply.setError("IR data to large");
        reply.finish();
        return;
    }

    // store the data
    m_infraredData = data;

    // create a new promise to signal the result
    m_programmingPromise = make_shared< PendingReply<> >(std::move(reply));

    // trigger the statemachine and return the future
    m_stateMachine.postEvent(ProgramRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattInfraredSignal::onEnteredState(int state)
{
    switch (State(state)) {
        case InitialisingState:
            onEnteredInitialisingState();
            break;

        case DisablingState:
            onEnteredDisablingState();
            break;
        case WritingState:
            onEnteredWritingState();
            break;
        case EnablingState:
            onEnteredEnablingState();
            break;

        default:
            break;
    }

}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattInfraredSignal::onExitedState(int state)
{
    switch (State(state)) {
        case ProgrammingSuperState:
            onExitedProgrammingState();
            break;

        default:
            break;
    }

}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'read reference descriptor' state, we cache the
    reference descriptor value, so we only need to do work the first time.

    The 'Infrared Signal Reference Descriptor' is used to tell us what physical
    button the parent characteristic corresponds to.  Without this the sate
    machine can't advance and this object never moves into the ready state.

 */
void GattInfraredSignal::onEnteredInitialisingState()
{
    // sanity check
    if (!m_signalReferenceDescriptor) {
        XLOGD_ERROR("missing IR Signal Reference Descriptor proxy");
        return;
    }

    // if we've already read this value, no need to do it again
    if (m_keyCode != Key_INVALID) {
        m_stateMachine.postEvent(AckEvent);
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {

                XLOGD_ERROR("failed to read IR Signal Reference Descriptor due to <%s>", 
                        reply->errorMessage().c_str());

                // TODO: should we retry after a certain amount of time ?
                m_stateMachine.postEvent(ErrorEvent);

            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {

                    switch (value[0]) {
                        case 0x0C:   
                            m_keyCode = Key_PowerPrimary;
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_PowerPrimary)", value[0]);
                            break;
                        case 0x0B:   
                            m_keyCode = Key_PowerSecondary;
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_PowerSecondary)", value[0]);
                            break;
                        case 0x29:   
                            m_keyCode = Key_Input; 
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_Input)", value[0]);
                            break;
                        case 0x10:   
                            m_keyCode = Key_VolUp; 
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_VolUp)", value[0]);
                            break;
                        case 0x11:   
                            m_keyCode = Key_VolDown; 
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_VolDown)", value[0]);
                            break;
                        case 0x0D:   
                            m_keyCode = Key_Mute; 
                            XLOGD_INFO("found characteristic for 0x%02hhx (Key_Mute)", value[0]);
                            break;

                        /* 
                        Below is an excerpt from the IR Service spec regarding these IR slots on the RCU.
                        We aren't currently using them, so set them to invalid for now since programming and
                        erasing them is a waste of time.

                        These signals are expect to only be used within a certain time period after the 
                        Input Selection button is pressed, the intention is for them to be used to navigate 
                        a TV's input selection user interface. There currently isn't any special 
                        handling for these buttons, this may change in future to maintain consistent 
                        behaviour across vendors.
                        */
                        case 0x5C:   m_keyCode = Key_INVALID; break;    // Key_Select; break;
                        case 0x58:   m_keyCode = Key_INVALID; break;    // Key_Up; break;
                        case 0x59:   m_keyCode = Key_INVALID; break;    // Key_Down; break;
                        case 0x5A:   m_keyCode = Key_INVALID; break;    // Key_Left; break;
                        case 0x5B:   m_keyCode = Key_INVALID; break;    // Key_Right; break;

                        default:
                            XLOGD_ERROR("unknown IR Signal Reference Descriptor value - 0x%02hhx", 
                                    value[0]);

                            // TODO: should we retry after a certain amount of time ?
                            m_stateMachine.postEvent(ErrorEvent);
                            return;
                    }

                    m_stateMachine.postEvent(AckEvent);

                    m_readySlots.invoke();

                } else {

                    XLOGD_ERROR("invalid IR Signal Reference Descriptor value, length wrong (%d bytes)", 
                            value.size());

                    // TODO: should we retry after a certain amount of time ?
                    m_stateMachine.postEvent(ErrorEvent);
                }
            }
        };

    // send a request to the RCU to read the value
    m_signalReferenceDescriptor->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on exit from the 'programming' super state, this can happen for
    if an error or cancel occurs, or simply when the programming completes. For
    the later case we don't care, for the error/cancel case we want to complete
    the future with an error.

 */
void GattInfraredSignal::onExitedProgrammingState()
{
    // don't care if the promise has already completed
    if (!m_programmingPromise) {
        return;
    }

    // complete the promise with an error and then clear
    m_programmingPromise->setError("Programming cancelled");
    m_programmingPromise->finish();
    m_programmingPromise.reset();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'disabling' state, simply queues up the write request
    to the configuration descriptor.

 */
void GattInfraredSignal::onEnteredDisablingState()
{
    // sanity check
    if (!m_signalConfigurationDescriptor) {
        XLOGD_ERROR("missing IR Signal Config Descriptor proxy");
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {

                XLOGD_ERROR("failed to write 0x00 to IR Signal Config Descriptor due to <%s>", 
                        reply->errorMessage().c_str());

                m_stateMachine.postEvent(ErrorEvent);

            } else {

                XLOGD_DEBUG("disabled m_keyCode = %d ir signal", m_keyCode);

                m_stateMachine.postEvent(AckEvent);
            }
        };


    // send a request to write the value to disable the IR signal
    const vector<uint8_t> value(1, 0x00);
    m_signalConfigurationDescriptor->writeValue(value, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'writing' state, if the data we've been asked to
    write is empty then we do nothing in this state.

 */
void GattInfraredSignal::onEnteredWritingState()
{
    // sanity check
    if (!m_signalCharacteristic) {
        XLOGD_ERROR("missing ir signal characteristic proxy");
        return;
    }

    // it's not an error if the data to write it empty, it just means that the
    // signal should be disabled, so skip the writing phase
    if (m_infraredData.empty()) {
        m_stateMachine.postEvent(AckEvent);
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {

                XLOGD_ERROR("failed to write ir signal data due to <%s>", 
                        reply->errorMessage().c_str());

                m_stateMachine.postEvent(ErrorEvent);

            } else {

                XLOGD_DEBUG("written m_keyCode = %d ir signal data", m_keyCode);

                m_stateMachine.postEvent(AckEvent);
            }
        };


    // send a request to write the value to disable the IR signal
    m_signalCharacteristic->writeValue(m_infraredData, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'writing' state, if the data we've been asked to
    write is empty then we do nothing in this state.

 */
void GattInfraredSignal::onEnteredEnablingState()
{
    // sanity check
    if (!m_signalConfigurationDescriptor) {
        XLOGD_ERROR("missing IR Signal Config Descriptor proxy");
        return;
    }

    // it's not an error if the data to write it empty, it just means that the
    // signal should be disabled, so skip the enabling phase
    if (m_infraredData.empty()) {

        // complete the future with a positive result
        if (m_programmingPromise) {
            m_programmingPromise->finish();
            m_programmingPromise.reset();
        }

        // send a positive ack to complete the state machine
        m_stateMachine.postEvent(AckEvent);
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {

                XLOGD_ERROR("failed to write 0x01 to IR Signal Config Descriptor due to <%s>", 
                        reply->errorMessage().c_str());

                m_stateMachine.postEvent(ErrorEvent);

            } else {

                XLOGD_DEBUG("enabled m_keyCode = %d ir signal", m_keyCode);

                // complete the future with a positive result
                if (m_programmingPromise) {
                    m_programmingPromise->finish();
                    m_programmingPromise.reset();
                }

                // send a positive ack to complete the state machine
                m_stateMachine.postEvent(AckEvent);
            }
        };


    // send a request to write the value to enable the IR signal
    const vector<uint8_t> value(1, 0x01);
    m_signalConfigurationDescriptor->writeValue(value, PendingReply<>(m_isAlive, replyHandler));
}

