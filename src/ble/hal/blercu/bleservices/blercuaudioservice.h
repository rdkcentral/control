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
//  blercuaudioservice.h
//

#ifndef BLERCUAUDIOSERVICE_H
#define BLERCUAUDIOSERVICE_H

#include "utils/pendingreply.h"
#include "utils/slot.h"
#include "utils/filedescriptor.h"
#include "utils/audioformat.h"
#include "ctrlm_hal.h"

class BleRcuAudioService
{
protected:
    BleRcuAudioService() = default;
    
public:
    virtual ~BleRcuAudioService() = default;

public:
    enum Encoding {
        PCM16,
        ADPCM_FRAME,
        InvalidEncoding
    };

public:
    virtual bool isStreaming() const = 0;

    virtual uint8_t gainLevel() const = 0;
    virtual void setGainLevel(uint8_t level) = 0;

    virtual uint32_t audioCodecs() const = 0;

    virtual bool audioFormat(Encoding encoding, AudioFormat &format) const = 0;
    virtual void startStreaming(Encoding encoding, PendingReply<int> &&reply, uint32_t durationMax = 0) = 0;
    virtual void stopStreaming(uint32_t audioDuration, PendingReply<> &&reply) = 0;
    
    virtual bool getFirstAudioDataTime(ctrlm_timestamp_t &time) = 0;

public:
    enum StreamingError {
        NoError = 0,
        DeviceDisconnectedError = 1,
        InternalError = 2,
    };

    struct StatusInfo {
        uint32_t lastError;
        uint32_t expectedPackets;
        uint32_t actualPackets;
    };

    virtual void status(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets) = 0;

// signals:
    inline void addStreamingChangedSlot(const Slot<bool> &func)
    {
        m_streamingChangedSlots.addSlot(func);
    }
    inline void addGainLevelChangedSlot(const Slot<uint8_t> &func)
    {
        m_gainLevelChangedSlots.addSlot(func);
    }
    inline void addAudioCodecsChangedSlot(const Slot<uint32_t> &func)
    {
        m_audioCodecsChangedSlots.addSlot(func);
    }

protected:
    Slots<bool> m_streamingChangedSlots;
    Slots<uint8_t> m_gainLevelChangedSlots;
    Slots<uint32_t> m_audioCodecsChangedSlots;
};


#endif // !defined(BLERCUAUDIOSERVICE_H)
