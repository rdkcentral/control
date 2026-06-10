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

#include <vector>
#include <map>


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
    }

    struct DetectionData {
        uint16_t start;
        uint16_t end;
        uint16_t confidence;
    };

    struct ModelVersion {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
    };

    void writePrivacy(bool enabled, PendingReply<> &&reply) {}
    void writeModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply) {}

// signals:
    inline void addDetectionTypeChangedSlot(const Slot<int32_t> &func)
    {
        m_detectionTypeChangedSlots.addSlot(func);
    }
    inline void addDetectionDataChangedSlot(const Slot<int32_t> &func)
    {
        m_detectionDataChangedSlots.addSlot(func);
    }
    inline void addPrivacyChangedSlot(const Slot<int32_t> &func)
    {
        m_privacyChangedSlots.addSlot(func);
    }
    inline void addCapabilitiesChangedSlot(const Slot<int32_t> &func)
    {
        m_capabilitiesChangedSlots.addSlot(func);
    }
    inline void addStreamStatsChangedSlot(const Slot<int32_t> &func)
    {
        m_streamStatsChangedSlots.addSlot(func);
    }
    inline void addReadySlot(const Slot<int32_t> &func) 
    {
        m_readySlots.addSlot(func);
    }

protected:
    Slots<int32_t> m_detectionTypeChangedSlots;
    Slots<int32_t> m_detectionDataChangedSlots;
    Slots<int32_t> m_privacyChangedSlots;
    Slots<int32_t> m_capabilitiesChangedSlots;
    Slots<int32_t> m_streamStatsChangedSlots;
    Slots<int32_t> m_readySlots;

};


#endif // BLERCUMFVVOICESERVICE_H
