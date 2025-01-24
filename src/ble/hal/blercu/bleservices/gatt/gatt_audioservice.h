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
//  gatt_audioservice.h
//

#ifndef GATT_AUDIOSERVICE_H
#define GATT_AUDIOSERVICE_H

#include "blercu/bleservices/blercuaudioservice.h"
#include "blercu/blercuerror.h"
#include "utils/bleuuid.h"

#include "utils/statemachine.h"
#include <mutex>
#include "ctrlm_hal.h"

class BleGattService;
class BleGattCharacteristic;

class GattAudioPipe;


class GattAudioService : public BleRcuAudioService
{
public:
    GattAudioService(uint8_t frameSize, uint8_t packetsPerFrame, uint32_t audioFrameDuration, GMainLoop* mainLoop = NULL);
    ~GattAudioService();

public:
    bool isReady() const;

public:
    virtual BleUuid uuid() = 0;
    virtual bool start(const std::shared_ptr<const BleGattService> &gattService);
    virtual void stop();
    virtual void onEnteredIdle();
    virtual void onEnteredEnableNotificationsState();
    virtual void onEnteredStartStreamingState();
    virtual void onEnteredStopStreamingState();
    virtual void onAudioDataNotification(const std::vector<uint8_t> &value);

    virtual void validateFrame(const uint8_t *frame, uint32_t frameCount);

    void updateSequenceNumber(uint8_t sequenceNumber, uint32_t frameCount);

    void stateMachinePostEvent(const Event::Type event);
    bool stateMachineIsIdle();

    void setLastError(BleRcuAudioService::StreamingError error);
    std::shared_ptr<bool> getIsAlivePtr();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }
private:
    Slots<> m_readySlots;

public:
    bool isStreaming() const override;
    
    void startStreaming(Encoding encoding, PendingReply<int> &&reply, uint32_t durationMax = 0) override;
    void stopStreaming(uint32_t audioDuration, PendingReply<> &&reply) override;

    void status(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets) override;

    bool getFirstAudioDataTime(ctrlm_timestamp_t &time) override;

private:
    enum State {
        IdleState,
        ReadyState,
        StreamingSuperState,
            EnableNotificationsState,
            StartStreamingState,
            StreamingState,
            StopStreamingState,
            CancelStreamingState
    };

    void init();

    void onEnteredState(int state);
    void onExitedState(int state);

    void onEnteredStreamingState();
    void onExitedStreamingState();

    void onExitedStreamingSuperState();

    void onOutputPipeClosed();

private:
    std::shared_ptr<bool> m_isAlive;

    const uint8_t  m_frameSize;
    const uint8_t  m_packetsPerFrame;
    const uint32_t m_audioFrameDuration;
    
private:
    std::shared_ptr<PendingReply<int>> m_startStreamingPromise;
    // std::shared_ptr<Promise<>> m_startStreamingToPromise;
    std::shared_ptr<PendingReply<>> m_stopStreamingPromise;


private:
    StatusInfo m_lastStats;

    StateMachine m_stateMachine;
    int64_t m_timeoutEventId;

    std::mutex mAudioPipeMutex;
    std::shared_ptr<GattAudioPipe> m_audioPipe;
    bool m_emitOneTimeStreamingSignal;

    uint32_t m_missedSequences;
    uint8_t  m_lastSequenceNumber;

public:
    static const Event::Type StartServiceRequestEvent   = Event::Type(Event::User + 1);
    static const Event::Type StopServiceRequestEvent    = Event::Type(Event::User + 2);

    static const Event::Type StartStreamingRequestEvent = Event::Type(Event::User + 3);
    static const Event::Type StopStreamingRequestEvent  = Event::Type(Event::User + 4);

    static const Event::Type NotificationsEnabledEvent  = Event::Type(Event::User + 5);

    static const Event::Type StreamingStartedEvent      = Event::Type(Event::User + 6);
    static const Event::Type StreamingStoppedEvent      = Event::Type(Event::User + 7);

    static const Event::Type GattErrorEvent             = Event::Type(Event::User + 8);
    static const Event::Type OutputPipeCloseEvent       = Event::Type(Event::User + 9);
};

#endif // !defined(GATT_AUDIOSERVICE_H)
