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
//  blercumfvvoiceservice.h
//

#ifndef BLERCUMFVVOICESERVICE_H
#define BLERCUMFVVOICESERVICE_H

#include "utils/pendingreply.h"
#include "utils/slot.h"

#include <cstdint>
#include <vector>


class BleRcuMfvVoiceService
{

protected:
    BleRcuMfvVoiceService() = default;

public:
    virtual ~BleRcuMfvVoiceService() = default;

public:
    enum DetectionType {
        FullPower=0x01,
        Aad=0x02,
        BelowThreshold=0x03,
    };

    struct DetectionData {
        uint16_t start;
        uint16_t end;
        uint16_t confidence_x10;
    };

    struct ModelVersion {
        uint8_t major;
        uint8_t minor;
    };

    struct StreamStatsRaw {
        std::vector<uint8_t> bytes;
    };

    enum Capabilities {
        MidfieldVoiceCapable = (1 << 0),
        SoftwarePrivacyControl = (1 << 1),
        SowwEowwTimingAvailable = (1 << 2),
        AadSensitivityControlAvailable = (1 << 3)
    };

public:
    virtual DetectionType detectionType() const = 0;
    virtual DetectionData detectionData() const = 0;
    virtual ModelVersion wakeWordModelVersion() const = 0;
    virtual bool privacyEnabled() const = 0;
    virtual std::vector<uint8_t> modelConfiguration() const = 0;
    virtual uint8_t capabilities() const = 0;
    virtual StreamStatsRaw streamStats() const = 0;

    virtual void writePrivacy(bool enabled, PendingReply<> &&reply) = 0;
    virtual void writeModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply) = 0;

// signals:
    inline void addDetectionTypeChangedSlot(const Slot<DetectionType> &func)
    {
        m_detectionTypeChangedSlots.addSlot(func);
    }
    inline void addDetectionDataChangedSlot(const Slot<const DetectionData &> &func)
    {
        m_detectionDataChangedSlots.addSlot(func);
    }
    inline void addPrivacyChangedSlot(const Slot<bool> &func)
    {
        m_privacyChangedSlots.addSlot(func);
    }
    inline void addCapabilitiesChangedSlot(const Slot<uint8_t> &func)
    {
        m_capabilitiesChangedSlots.addSlot(func);
    }
    inline void addStreamStatsChangedSlot(const Slot<const StreamStatsRaw &> &func)
    {
        m_streamStatsChangedSlots.addSlot(func);
    }
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }

protected:
    Slots<DetectionType> m_detectionTypeChangedSlots;
    Slots<const DetectionData &> m_detectionDataChangedSlots;
    Slots<bool> m_privacyChangedSlots;
    Slots<uint8_t> m_capabilitiesChangedSlots;
    Slots<const StreamStatsRaw &> m_streamStatsChangedSlots;
    Slots<> m_readySlots;

};


#endif // BLERCUMFVVOICESERVICE_H
