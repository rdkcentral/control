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
//  gatt_audiopipe.h
//

#ifndef GATT_AUDIOPIPE_H
#define GATT_AUDIOPIPE_H

#include "utils/filedescriptor.h"
#include "utils/slot.h"
#include "ctrlm_hal.h"

#include <memory>
#include <gio/gio.h>

// class VoiceCodec;
// class UnixPipeNotifier;

class GattAudioPipe
{
public:
    using cbFrameValidator = std::function<void(const uint8_t *frame, uint32_t frameCount)>;

    explicit GattAudioPipe(uint8_t frameSize, uint32_t frameCountMax, cbFrameValidator frameValidator = NULL, int outputPipeFd = -1);
    ~GattAudioPipe();

public:
    bool isValid() const;

    bool isOutputOpen() const;

    bool start();
    void stop();

    uint32_t framesReceived() const;
    uint32_t framesExpected(uint32_t lostFrameCount, uint32_t usecPerFrame) const;

    int takeOutputReadFd();

    bool addNotification(const uint8_t value[20], const uint8_t length);

    bool setFrameCountMax(uint32_t frameCountMax);
    bool getFirstAudioDataTime(struct timespec &time);

// signals:
    inline void addOutputPipeClosedSlot(const Slot<> &func)
    {
        m_outputPipeClosedSlots.addSlot(func);
    }
private:
    Slots<> m_outputPipeClosedSlots;

private:
    void onOutputPipeException(int pipeFd);

    bool processAudioFrame();

private:
    std::shared_ptr<bool> m_isAlive;
    
    int m_outputPipeRdFd;
    int m_outputPipeWrFd;

    uint8_t m_frameSize;
    uint8_t m_frameBuffer[140];
    size_t m_frameBufferOffset;
    cbFrameValidator m_frameValidator;

    bool m_running;

    uint32_t m_frameCount;
    uint32_t m_frameCountMax;
    GTimer* m_recordingTimer;
    double m_recordingDuration;
    ctrlm_timestamp_t m_firstAudioDataTime;
};


#endif // !defined(GATT_AUDIOPIPE_H)

