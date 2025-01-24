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
//  gatt_audioservice.cpp
//

#include "gatt_audioservice.h"
#include "gatt_audiopipe.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"

#include "ctrlm_log_ble.h"

#include <unistd.h>
#include <errno.h>

using namespace std;


GattAudioService::GattAudioService(uint8_t frameSize, uint8_t packetsPerFrame, uint32_t audioFrameDuration, GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_frameSize(frameSize)
    , m_packetsPerFrame(packetsPerFrame)
    , m_audioFrameDuration(audioFrameDuration)
    , m_timeoutEventId(-1)
    , m_emitOneTimeStreamingSignal(true)
{
    // clear the last stats
    m_lastStats.lastError = NoError;
    m_lastStats.actualPackets = 0;
    m_lastStats.expectedPackets = 0;

    // setup the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattAudioService::~GattAudioService()
{
    *m_isAlive = false;
    // ensure the service is stopped
    stop();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattAudioService::init()
{
    m_stateMachine.setObjectName("GattAudioService");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(ReadyState, "Ready");

    m_stateMachine.addState(StreamingSuperState, "StreamingSuperState");
    m_stateMachine.addState(StreamingSuperState, EnableNotificationsState, "EnableNotifications");
    m_stateMachine.addState(StreamingSuperState, StartStreamingState, "StartStreaming");
    m_stateMachine.addState(StreamingSuperState, StreamingState, "Streaming");
    m_stateMachine.addState(StreamingSuperState, StopStreamingState, "StopStreaming");


    // add the transitions:      From State         ->      Event                   ->  To State
    m_stateMachine.addTransition(IdleState,                 StartServiceRequestEvent,   ReadyState);
    m_stateMachine.addTransition(ReadyState,                StopServiceRequestEvent,    IdleState);
    m_stateMachine.addTransition(ReadyState,                StartStreamingRequestEvent, EnableNotificationsState);

    m_stateMachine.addTransition(EnableNotificationsState,  NotificationsEnabledEvent,  StartStreamingState);

    m_stateMachine.addTransition(StartStreamingState,       StreamingStartedEvent,      StreamingState);

    m_stateMachine.addTransition(StreamingState,            StopStreamingRequestEvent,  StopStreamingState);
    m_stateMachine.addTransition(StreamingState,            OutputPipeCloseEvent,       StopStreamingState);

    m_stateMachine.addTransition(StopStreamingState,        StreamingStoppedEvent,      ReadyState);

    m_stateMachine.addTransition(StreamingSuperState,       GattErrorEvent,             ReadyState);

    m_stateMachine.addTransition(StreamingSuperState,       StopServiceRequestEvent,    IdleState);


    // connect to the state entry / exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattAudioService::onEnteredState, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&GattAudioService::onExitedState, this, std::placeholders::_1)));


    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    Starts the service by sending a initial request for the audio to stop
    streaming.  When a reply is received we signal that this service is now
    ready.
 */
bool GattAudioService::start(const shared_ptr<const BleGattService> &gattService)
{
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
    Stops the service

 */
void GattAudioService::stop()
{
    // if currently streaming then set the last error as 'disconnected'
    if (m_stateMachine.inState(StreamingSuperState)) {
        m_lastStats.lastError = DeviceDisconnectedError;
    }

    // post the event to move the state machine
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \reimp

 */
bool GattAudioService::isReady() const
{
    return m_stateMachine.inState( { ReadyState, StreamingSuperState } );
}

// -----------------------------------------------------------------------------
/*!
    \reimp

 */
bool GattAudioService::isStreaming() const
{
    return m_stateMachine.inState(StreamingState);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattAudioService::onEnteredState(int state)
{
    switch (state) {
        case IdleState:
            onEnteredIdle();
            break;

        case ReadyState:
            m_readySlots.invoke();
            break;

        case EnableNotificationsState:
            onEnteredEnableNotificationsState();
            break;

        case StartStreamingState:
            onEnteredStartStreamingState();
            break;
        case StreamingState:
            onEnteredStreamingState();
            break;
        case StopStreamingState:
            onEnteredStopStreamingState();
            break;

        default:
            // don't care about other states
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattAudioService::onExitedState(int state)
{
    switch (state) {
        case StreamingState:
            onExitedStreamingState();
            break;
            
        case StreamingSuperState:
            onExitedStreamingSuperState();
            break;

        default:
            // don't care about other states
            break;
    }
}

void GattAudioService::onEnteredIdle()
{
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called upon entry to the 'enable notifications' state, this is
    where we ask the system to set the CCCD and start funneling notifications to
    the app.

 */
void GattAudioService::onEnteredEnableNotificationsState()
{
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the start streaming state, here we need to perform the
    GATT write to enable the audio streaming on the RCU.

 */
void GattAudioService::onEnteredStartStreamingState()
{
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the streaming state, here we just tell the audio pipe to
    start streaming and complete the promise.

 */
void GattAudioService::onEnteredStreamingState()
{
    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    // sanity check we have an audio output pipe
    if (!m_audioPipe) {
        XLOGD_ERROR("odd, no audio pipe already created");
        m_lastStats.lastError = StreamingError::InternalError;
        m_stateMachine.postEvent(GattErrorEvent);
        return;
    }

    if (!m_audioPipe->isOutputOpen()) {
        XLOGD_ERROR("output pipe closed before streaming started");
    }


    // connect to the closed signal from the client audio pipe
    m_audioPipe->addOutputPipeClosedSlot(Slot<>(m_isAlive, 
            std::bind(&GattAudioService::onOutputPipeClosed, this)));


    // and finally start the audio pipe
    m_audioPipe->start();



    // complete the pending operation with a positive result
    if (m_startStreamingPromise) {
        XLOGD_DEBUG("about to call m_audioPipe->takeOutputReadFd");

        int fd = m_audioPipe->takeOutputReadFd();
        XLOGD_DEBUG("returned from m_audioPipe->takeOutputReadFd, fd = %d", fd);

        m_startStreamingPromise->setResult(fd);
        m_startStreamingPromise->finish();
        m_startStreamingPromise.reset();
    } else {
        XLOGD_ERROR("odd, missing promise to send the reply to");
    }

    // schedule a timeout event for automatically cancelling the voice search after 30 seconds
    m_timeoutEventId = m_stateMachine.postDelayedEvent(StopStreamingRequestEvent, 30000);

    // Once streaming data is actually received, emit the streamingChanged signal a single time
    m_emitOneTimeStreamingSignal = true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on exit from the streaming state, here we just tell the audio pipe to
    stop streaming.

 */
void GattAudioService::onExitedStreamingState()
{
    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    // destroy the audio pipe
    if (!m_audioPipe) {
        XLOGD_ERROR("odd, audio pipe not created ?");
    } else {

        // before destruction get the frame stats
        m_audioPipe->stop();
        m_lastStats.actualPackets = m_audioPipe->framesReceived() * m_packetsPerFrame;
        m_lastStats.expectedPackets = m_audioPipe->framesExpected(m_missedSequences, m_audioFrameDuration) * m_packetsPerFrame;
        m_lastStats.expectedPackets = std::max(m_lastStats.expectedPackets, m_lastStats.actualPackets);

        XLOGD_INFO("audio frame stats: actual=%u, expected=%u",
              m_lastStats.actualPackets, m_lastStats.expectedPackets);

        // destroy the audio pipe (closes all file handles)
        m_audioPipe.reset();
    }
    guard.unlock();

    // cancel the timeout event
    if (m_timeoutEventId >= 0) {
        m_stateMachine.cancelDelayedEvent(m_timeoutEventId);
        m_timeoutEventId = -1;
    }

    // tell anyone who cares that streaming has stopped, but only if we've received actual
    // audio data and streamingChanged(true) was previously signaled
    if (!m_emitOneTimeStreamingSignal) {
        m_streamingChangedSlots.invoke(false);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called when the user requests to stop streaming or the output
    pipe has been closed and therefore we've entered the 'stop streaming' state.
    In this state we just send a request to the RCU to stop streaming.

 */
void GattAudioService::onEnteredStopStreamingState()
{
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a notification is received from the Audio Data characteristic.

 */
void GattAudioService::onAudioDataNotification(const vector<uint8_t> &value)
{
    // This way the streamingChanged signal is emitted only when we actually receive
    // audio data.  But this should only be emitted for the first notification.
    if (m_emitOneTimeStreamingSignal) {
        m_streamingChangedSlots.invoke(true);
        m_emitOneTimeStreamingSignal = false;
    }

    std::unique_lock<std::mutex> guard(mAudioPipeMutex);
    // add the notification to the audio pipe
    if (m_audioPipe) {
        bool endOfStream = m_audioPipe->addNotification(reinterpret_cast<const uint8_t*>(value.data()), value.size());
        guard.unlock();
        if(endOfStream) {
            m_stateMachine.postEvent(StopStreamingRequestEvent);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called when the state machine leaves the 'streaming super'
    state. In this state we destroy the output fifo as the streaming has stopped.
 */
void GattAudioService::onExitedStreamingSuperState()
{
    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    // close the streaming pipe (if we haven't already)
    if (m_audioPipe) {
        m_lastStats.actualPackets = m_audioPipe->framesReceived() * m_packetsPerFrame;
        m_lastStats.expectedPackets = m_audioPipe->framesExpected(m_missedSequences, m_audioFrameDuration) * m_packetsPerFrame;
        m_lastStats.expectedPackets = std::max(m_lastStats.expectedPackets, m_lastStats.actualPackets);

        m_audioPipe.reset();

        XLOGD_INFO("audio frame stats: actual=%u, expected=%u",
              m_lastStats.actualPackets, m_lastStats.expectedPackets);
    }
    guard.unlock();

    // complete any promises that may still be outstanding
    if (m_stopStreamingPromise) {
        m_stopStreamingPromise->finish();
        m_stopStreamingPromise.reset();
    }

    if (m_startStreamingPromise) {
        m_startStreamingPromise->setError("Streaming stopped");
        m_startStreamingPromise->finish();
        m_startStreamingPromise.reset();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when AS has closed the voice audio pipe, it does this when it wants
    to stop the audio streaming.

 */
void GattAudioService::onOutputPipeClosed()
{
    XLOGD_INFO("audio output pipe closed");

    m_stateMachine.postEvent(OutputPipeCloseEvent);
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattAudioService::startStreaming(Encoding encoding, PendingReply<int> &&reply, uint32_t durationMax)
{
    // check the current state
    if (m_stateMachine.state() != ReadyState) {
        reply.setError("Service is not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_startStreamingPromise || m_stopStreamingPromise) {
        reply.setError("Service is busy");
        reply.finish();
        return;
    }

    // clear the last stats
    m_lastStats.lastError = NoError;
    m_lastStats.actualPackets = 0;
    m_lastStats.expectedPackets = 0;
    m_missedSequences = 0;
    m_lastSequenceNumber = 0;

    GattAudioPipe::cbFrameValidator frameValidator = std::bind(&GattAudioService::validateFrame, this, std::placeholders::_1, std::placeholders::_2);

    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    uint32_t frameCountMax = 0;
    if(durationMax) { // Convert durationMax from milliseconds to audio frames
        frameCountMax = ((durationMax * 1000) + m_audioFrameDuration - 1) / m_audioFrameDuration;
    }

    // create a new audio pipe for the client
    m_audioPipe = make_shared<GattAudioPipe>(m_frameSize, frameCountMax, frameValidator);
    if (!m_audioPipe || !m_audioPipe->isValid()) {
        m_audioPipe.reset();
        guard.unlock();
        m_lastStats.lastError = StreamingError::InternalError;
        reply.setError("Failed to create streaming pipe");
        reply.finish();
        return;
    }


    // create a promise to store the result of the streaming
    m_startStreamingPromise = make_shared<PendingReply<int>>(std::move(reply));


    // post a message to the state machine to start moving into the streaming state
    m_stateMachine.postEvent(StartStreamingRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
void GattAudioService::stopStreaming(uint32_t audioDuration, PendingReply<> &&reply)
{
    // check the service is ready
    if (m_stateMachine.state() != StreamingState) {
        reply.setError("Service not currently streaming");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_startStreamingPromise || m_stopStreamingPromise) {
        reply.setError("Service is busy");
        reply.finish();
        return;
    }

    // create a promise to store the result of the streaming
    m_stopStreamingPromise = make_shared<PendingReply<>>(std::move(reply));

    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    if(m_audioPipe && audioDuration > 0) { // if the audio pipe is still alive then wait for the rest of the audio to stream
        uint32_t frameCountMax = ((audioDuration * 1000) + m_audioFrameDuration - 1) / m_audioFrameDuration;

        if(m_missedSequences >= frameCountMax) {
            XLOGD_ERROR("missed frames greater than frame count max");
        } else {
            frameCountMax -= m_missedSequences; // compensate for missed frames

            if(m_audioPipe->setFrameCountMax(frameCountMax)) {
                #if 0
                // if the controller supports it, we can wait for the remaining frames to arrive and the stream will end when the frame count is reached
                XLOGD_INFO("wait for remaining frames to arrive");
                return;
                #endif
            }
        }
    }
    
    guard.unlock();

    // post a message to the state machine
    m_stateMachine.postEvent(StopStreamingRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \overload
 
    Examines the header of each frame to check for errors.

 */
void GattAudioService::validateFrame(const uint8_t *frame, uint32_t frameCount)
{

}

void GattAudioService::updateSequenceNumber(uint8_t sequenceNumber, uint32_t frameCount)
{
    if (frameCount != 0) {
        const uint8_t expectedSeqNumber = m_lastSequenceNumber + 1;
        if (expectedSeqNumber != sequenceNumber) {
            uint8_t missed = sequenceNumber - expectedSeqNumber;
            m_missedSequences += missed;
        }
    }

    // store the last sequence number
    m_lastSequenceNumber = sequenceNumber;
}

// -----------------------------------------------------------------------------
/*!
    \overload
 
    Returns the status of the current or previous audio recording.  Because this
    API can return to previous state, this object must store the last error
    state even across the service starting / stopping.

 */
void GattAudioService::status(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets)
{
    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    // if the audio pipe is still alive then use the latest stats from that
    if (m_audioPipe) {
        lastError = NoError;
        actualPackets = m_audioPipe->framesReceived() * m_packetsPerFrame;
        expectedPackets = m_audioPipe->framesExpected(m_missedSequences, m_audioFrameDuration) * m_packetsPerFrame;
        expectedPackets = std::max(expectedPackets, actualPackets);

    } else {
        // otherwise use the stored stats
        lastError = m_lastStats.lastError;
        actualPackets = m_lastStats.actualPackets;
        expectedPackets = m_lastStats.expectedPackets;
    }
}

bool GattAudioService::getFirstAudioDataTime(ctrlm_timestamp_t &time)
{
    std::unique_lock<std::mutex> guard(mAudioPipeMutex);

    if (m_audioPipe) {
        return m_audioPipe->getFirstAudioDataTime(time);
    }

    return false;
}

void GattAudioService::stateMachinePostEvent(const Event::Type event)
{
    m_stateMachine.postEvent(event);
}

bool GattAudioService::stateMachineIsIdle()
{
    return m_stateMachine.inState(IdleState);
}

void GattAudioService::setLastError(BleRcuAudioService::StreamingError error)
{
    m_lastStats.lastError = error;
}

std::shared_ptr<bool> GattAudioService::getIsAlivePtr()
{
    return m_isAlive;
}
