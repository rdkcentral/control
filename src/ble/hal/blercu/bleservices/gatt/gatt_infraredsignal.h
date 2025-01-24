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
//  gatt_infraredsignal.h
//

#ifndef GATT_INFRAREDSIGNAL_H
#define GATT_INFRAREDSIGNAL_H

#include "../blercuinfraredservice.h"

#include "utils/statemachine.h"

#include <memory>

class BleGattCharacteristic;
class BleGattDescriptor;


class GattInfraredSignal
{
public:
    GattInfraredSignal(const std::shared_ptr<BleGattCharacteristic> &gattCharacteristic,
                       GMainLoop* mainLoop = NULL);
    ~GattInfraredSignal();

public:

    enum Key {
        Key_PowerPrimary = 0,
        Key_PowerSecondary,
        Key_VolUp,
        Key_VolDown,
        Key_Mute,
        Key_Input,
        Key_Select,
        Key_Up,
        Key_Down,
        Key_Left,
        Key_Right,
        Key_INVALID = 0xff
    };


    bool isValid() const;
    bool isReady() const;

    int instanceId() const;

    Key keyCode() const;

public:
    void start();
    void stop();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }

protected:
    Slots<> m_readySlots;


public:
    void program(const std::vector<uint8_t> &data, PendingReply<> &&reply);


private:
    enum State {
        IdleState,
        InitialisingState,
        ReadyState,
        ProgrammingSuperState,
            DisablingState,
            WritingState,
            EnablingState,
    };

    void initStateMachine();

    void onEnteredState(int state);
    void onExitedState(int state);

private:
    void onEnteredInitialisingState();
    void onEnteredDisablingState();
    void onEnteredWritingState();
    void onEnteredEnablingState();

    void onExitedProgrammingState();

private:
    std::shared_ptr<bool> m_isAlive;

    std::shared_ptr<BleGattCharacteristic> m_signalCharacteristic;
    std::shared_ptr<BleGattDescriptor> m_signalReferenceDescriptor;
    std::shared_ptr<BleGattDescriptor> m_signalConfigurationDescriptor;

    Key m_keyCode;

    StateMachine m_stateMachine;

    std::vector<uint8_t> m_infraredData;
    std::shared_ptr< PendingReply<> > m_programmingPromise;

private:
    static const Event::Type StartRequestEvent      = Event::Type(Event::User + 1);
    static const Event::Type StopRequestEvent       = Event::Type(Event::User + 2);
    static const Event::Type ProgramRequestEvent    = Event::Type(Event::User + 3);

    static const Event::Type AckEvent               = Event::Type(Event::User + 4);
    static const Event::Type ErrorEvent             = Event::Type(Event::User + 5);


};

#endif // !defined(GATT_INFRAREDSERVICE_H)

