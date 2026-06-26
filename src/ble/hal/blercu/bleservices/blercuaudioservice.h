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

#include <cstdint>
#include <vector>

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
        int32_t  voiceKeyHeldMs;
    };

    virtual void status(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets, int32_t &voiceKeyHeldMs) = 0;

// MFV types
public:
    enum DetectionType {
        FullPower = 0x01,
        Aad = 0x02,
        BelowThreshold = 0x03,
    };

    struct DetectionData {
        uint16_t start = 0;
        uint16_t end = 0;
        uint16_t confidence = 0; // encoded as percentage * 10 (e.g. 953 = 95.3%)
    };

    struct ModelVersion {
        uint8_t major = 0;
        uint8_t minor = 0;
    };

    struct StreamStatsRaw {
        std::vector<uint8_t> bytes;
    };

    enum Capabilities {
        MidfieldVoiceCapable = (1 << 0),
        SoftwarePrivacyControl = (1 << 1),
        SowwEowwTimingAvailable = (1 << 2),
        AadSensitivityControlAvailable = (1 << 3),
    };

// MFV accessors (default no-ops for audio services without MFV support)
public:
    virtual DetectionType mfvDetectionType() const { return FullPower; }
    virtual DetectionData mfvDetectionData() const { return {}; }
    virtual ModelVersion mfvModelVersion() const { return {}; }
    virtual bool mfvPrivacyEnabled() const { return false; }
    virtual std::vector<uint8_t> mfvModelConfiguration() const { return {}; }
    virtual uint8_t mfvCapabilities() const { return 0; }
    virtual StreamStatsRaw mfvStreamStats() const { return {}; }

    virtual void writeMfvPrivacy(bool enabled, PendingReply<> &&reply)
    {
        reply.setError("MFV not supported");
        reply.finish();
    }
    virtual void writeMfvModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply)
    {
        reply.setError("MFV not supported");
        reply.finish();
    }

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

// MFV signals:
    inline void addMfvDetectionTypeChangedSlot(const Slot<DetectionType> &func)
    {
        m_mfvDetectionTypeChangedSlots.addSlot(func);
    }
    inline void addMfvDetectionDataChangedSlot(const Slot<const DetectionData &> &func)
    {
        m_mfvDetectionDataChangedSlots.addSlot(func);
    }
    inline void addMfvPrivacyChangedSlot(const Slot<bool> &func)
    {
        m_mfvPrivacyChangedSlots.addSlot(func);
    }
    inline void addMfvCapabilitiesChangedSlot(const Slot<uint8_t> &func)
    {
        m_mfvCapabilitiesChangedSlots.addSlot(func);
    }
    inline void addMfvStreamStatsChangedSlot(const Slot<const StreamStatsRaw &> &func)
    {
        m_mfvStreamStatsChangedSlots.addSlot(func);
    }

protected:
    Slots<bool> m_streamingChangedSlots;
    Slots<uint8_t> m_gainLevelChangedSlots;
    Slots<uint32_t> m_audioCodecsChangedSlots;

    Slots<DetectionType> m_mfvDetectionTypeChangedSlots;
    Slots<const DetectionData &> m_mfvDetectionDataChangedSlots;
    Slots<bool> m_mfvPrivacyChangedSlots;
    Slots<uint8_t> m_mfvCapabilitiesChangedSlots;
    Slots<const StreamStatsRaw &> m_mfvStreamStatsChangedSlots;
};


#endif // !defined(BLERCUAUDIOSERVICE_H)
