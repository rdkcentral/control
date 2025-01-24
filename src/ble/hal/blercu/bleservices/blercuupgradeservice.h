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
//  blercuupgradeservice.h
//

#ifndef BLERCUUPGRADESERVICE_H
#define BLERCUUPGRADESERVICE_H

#include "utils/pendingreply.h"
#include "utils/slot.h"

#define BLE_RCU_UPGRADE_SERVICE_ERR_TIMEOUT_STR "Timeout was reached"

class BleRcuUpgradeService
{
protected:
    BleRcuUpgradeService() = default;

public:
    virtual ~BleRcuUpgradeService() = default;

public:
    virtual void startUpgrade(const std::string &fwFile, PendingReply<> &&reply) = 0;
    virtual void cancelUpgrade(PendingReply<> &&reply) = 0;

    virtual bool upgrading() const = 0;
    virtual int progress() const = 0;

// signals:
    inline void addUpgradingChangedSlot(const Slot<bool> &func)
    {
        m_upgradingChangedSlots.addSlot(func);
    }
    inline void addProgressChangedSlot(const Slot<int> &func)
    {
        m_progressChangedSlots.addSlot(func);
    }
    inline void addErrorChangedSlot(const Slot<std::string> &func)
    {
        m_errorChangedSlots.addSlot(func);
    }

protected:
    Slots<bool> m_upgradingChangedSlots;
    Slots<int> m_progressChangedSlots;
    Slots<std::string> m_errorChangedSlots;

};


#endif // !defined(BLERCUUPGRADESERVICE_H)

