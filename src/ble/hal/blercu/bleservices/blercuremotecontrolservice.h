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
//  blercuremotecontrolservice.h
//

#ifndef BLERCUREMOTECONTROLSERVICE_H
#define BLERCUREMOTECONTROLSERVICE_H

#include "utils/slot.h"
#include "utils/pendingreply.h"


class BleRcuRemoteControlService
{
protected:
    BleRcuRemoteControlService() = default;

public:
    virtual ~BleRcuRemoteControlService() = default;

public:
    virtual uint8_t unpairReason() const = 0;
    virtual uint8_t rebootReason() const = 0;
    virtual uint8_t lastKeypress() const = 0;
    virtual uint8_t advConfig() const = 0;
    virtual std::vector<uint8_t> advConfigCustomList() const = 0;
    virtual void sendRcuAction(uint8_t action, PendingReply<> &&reply) = 0;
    virtual void writeAdvertisingConfig(uint8_t config, const std::vector<uint8_t> &customList, PendingReply<> &&reply) = 0;

// signals:
    inline void addUnpairReasonChangedSlot(const Slot<uint8_t> &func)
    {
        m_unpairReasonChangedSlots.addSlot(func);
    }
    inline void addRebootReasonChangedSlot(const Slot<uint8_t, std::string> &func)
    {
        m_rebootReasonChangedSlots.addSlot(func);
    }
    inline void addLastKeypressChangedSlot(const Slot<uint8_t> &func)
    {
        m_lastKeypressChangedSlots.addSlot(func);
    }
    inline void addAdvConfigChangedSlot(const Slot<uint8_t> &func)
    {
        m_advConfigChangedSlots.addSlot(func);
    }
    inline void addAdvConfigCustomListChangedSlot(const Slot<const std::vector<uint8_t> &> &func)
    {
        m_advConfigCustomListChangedSlots.addSlot(func);
    }

protected:
    Slots<uint8_t> m_unpairReasonChangedSlots;
    Slots<uint8_t, std::string> m_rebootReasonChangedSlots;
    Slots<uint8_t> m_lastKeypressChangedSlots;
    Slots<uint8_t> m_advConfigChangedSlots;
    Slots<const std::vector<uint8_t> &> m_advConfigCustomListChangedSlots;
};

#endif // !defined(BLERCUREMOTECONTROLSERVICE_H)
