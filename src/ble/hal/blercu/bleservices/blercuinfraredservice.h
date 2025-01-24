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
//  blercuinfraredservice.h
//

#ifndef BLERCUINFRAREDSERVICE_H
#define BLERCUINFRAREDSERVICE_H

#include "utils/pendingreply.h"
#include "utils/slot.h"

#include <vector>
#include <map>

typedef std::vector<int32_t> IrCodeList;


class BleRcuInfraredService
{

protected:
    BleRcuInfraredService() = default;

public:
    virtual ~BleRcuInfraredService() = default;

public:

    virtual int32_t codeId() const = 0;
    virtual uint8_t irSupport() const = 0;

    virtual void setIrControl(const uint8_t irControl, PendingReply<> &&reply) = 0;
    virtual void eraseIrSignals(PendingReply<> &&reply) = 0;
    virtual void programIrSignalWaveforms(const std::map<uint32_t, std::vector<uint8_t>> &irWaveforms,
                                          const uint8_t irControl,
                                          PendingReply<> &&reply) = 0;

    virtual void emitIrSignal(uint32_t keyCode, PendingReply<> &&reply) = 0;

// signals:
    inline void addCodeIdChangedSlot(const Slot<int32_t> &func)
    {
        m_codeIdChangedSlots.addSlot(func);
    }
    inline void addIrSupportChangedSlot(const Slot<uint8_t> &func)
    {
        m_irSupportChangedSlots.addSlot(func);
    }

protected:
    Slots<int32_t> m_codeIdChangedSlots;
    Slots<uint8_t> m_irSupportChangedSlots;

};


#endif // !defined(BLERCUINFRAREDSERVICE_H)
