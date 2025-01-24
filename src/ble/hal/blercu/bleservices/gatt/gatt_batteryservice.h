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
//  gatt_batteryservice.h
//

#ifndef GATT_BATTERYSERVICE_H
#define GATT_BATTERYSERVICE_H

#include "blercu/bleservices/blercubatteryservice.h"
#include "utils/bleuuid.h"
#include "utils/statemachine.h"

#include <memory>

class BleGattService;
class BleGattCharacteristic;

class GattBatteryService : public BleRcuBatteryService
{
public:
    GattBatteryService(GMainLoop* mainLoop = NULL);
    ~GattBatteryService() final;

public:
    static BleUuid uuid();

public:
    bool isReady() const;

    int level() const override;

public:
    bool start(const std::shared_ptr<const BleGattService> &gattService);
    void stop();

    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }

private:
    Slots<> m_readySlots;

private:
    enum State {
        IdleState,
        StartNotifyState,
        RunningSuperState,
            RunningState,
            RereadBatteryLevelState
    };

    void init();

    unsigned int sanitiseBatteryLevel(uint8_t level) const;
    
    void onEnteredState(int state);
    void onBatteryLevelChanged(const std::vector<uint8_t> &newValue);

private:
    void requestStartNotify();
    void requestBatteryLevel();

private:
    std::shared_ptr<bool> m_isAlive;
    std::shared_ptr<BleGattCharacteristic> m_battLevelCharacteristic;

    StateMachine m_stateMachine;

    unsigned int m_batteryLevel;

private:
    static const BleUuid m_serviceUuid;
    static const BleUuid m_batteryLevelCharUuid;

private:
    static const Event::Type StartServiceRequestEvent = Event::Type(Event::User + 1);
    static const Event::Type StopServiceRequestEvent  = Event::Type(Event::User + 2);
    static const Event::Type StartedNotifingEvent     = Event::Type(Event::User + 3);
    static const Event::Type RetryStartNotifyEvent    = Event::Type(Event::User + 4);
    static const Event::Type RereadBatteryLevelEvent  = Event::Type(Event::User + 5);
};

#endif // !defined(GATT_BATTERYSERVICE_H)
