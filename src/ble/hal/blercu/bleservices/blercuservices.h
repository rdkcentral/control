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
//  blercuservices.h
//

#ifndef BLERCUSERVICES_H
#define BLERCUSERVICES_H

#include <functional>
#include <memory>

#include "utils/slot.h"


class BleRcuAudioService;
class BleRcuBatteryService;
class BleRcuDeviceInfoService;
class BleRcuFindMeService;
class BleRcuInfraredService;
class BleRcuUpgradeService;
class BleRcuRemoteControlService;


class BleRcuServices
{

protected:
    BleRcuServices() = default;

public:
    virtual ~BleRcuServices() = default;

public:

    virtual bool isValid() const = 0;
    virtual bool isReady() const = 0;

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual std::shared_ptr<BleRcuAudioService> audioService() const = 0;
    virtual std::shared_ptr<BleRcuDeviceInfoService> deviceInfoService() const = 0;
    virtual std::shared_ptr<BleRcuBatteryService> batteryService() const = 0;
    virtual std::shared_ptr<BleRcuFindMeService> findMeService() const = 0;
    virtual std::shared_ptr<BleRcuInfraredService> infraredService() const = 0;
    virtual std::shared_ptr<BleRcuUpgradeService> upgradeService() const = 0;
    virtual std::shared_ptr<BleRcuRemoteControlService> remoteControlService() const = 0;


    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }
    
protected:
    Slots<> m_readySlots;
};


#endif // !defined(BLERCUSERVICES_H)
