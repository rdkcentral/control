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
//  blercucscannerstatemachine.h
//

#ifndef BLERCUSCANNERSTATEMACHINE_H
#define BLERCUSCANNERSTATEMACHINE_H

#include "utils/bleaddress.h"
#include "utils/statemachine.h"
#include "utils/slot.h"

#include <memory>
#include <regex>

class BleRcuAdapter;
class ConfigSettings;


class BleRcuScannerStateMachine
{
public:
    enum State {
        RunningSuperState,
            StartingDiscoveryState,
            DiscoveringState,
            StoppingDiscoveryState,
        FinishedState
    };

public:
    BleRcuScannerStateMachine(const std::shared_ptr<const ConfigSettings> &config,
                              const std::shared_ptr<BleRcuAdapter> &adapter);
    ~BleRcuScannerStateMachine();

public:
    bool isRunning() const;

// public slots:
    void start(int timeoutMs);
    void stop();


// signals:

    inline void addStartedSlot(const Slot<> &func)
    {
        m_startedSlots.addSlot(func);
    }
    inline void addFinishedSlot(const Slot<> &func)
    {
        m_finishedSlots.addSlot(func);
    }
    inline void addFailedSlot(const Slot<> &func)
    {
        m_failedSlots.addSlot(func);
    }
    inline void addFoundPairableDeviceSlot(const Slot<const BleAddress &, const std::string &> &func)
    {
        m_foundPairableDeviceSlots.addSlot(func);
    }


// private slots:
    void onDiscoveryChanged(bool discovering);

    void onDeviceFound(const BleAddress &address, const std::string &name);
    void onDeviceNameChanged(const BleAddress &address, const std::string &name);

    void onAdapterPoweredChanged(bool powered);

    void onStateEntry(int state);

    void onEnteredStartDiscoveryState();
    void onEnteredDiscoveringState();
    void onEnteredStopDiscoveryState();
    void onEnteredFinishedState();


private:
    void setupStateMachine();
    void processDevice(const BleAddress &address, const std::string &name);

private:
    std::shared_ptr<bool> m_isAlive;

    Slots<> m_startedSlots;
    Slots<> m_finishedSlots;
    Slots<> m_failedSlots;
    Slots<const BleAddress&, const std::string&> m_foundPairableDeviceSlots;


    const std::shared_ptr<BleRcuAdapter> m_adapter;

    std::vector<std::regex> m_supportedPairingNames;

    StateMachine m_stateMachine;

    int m_scanTimeoutMs;

    struct {
        BleAddress address;
        std::string name;

        bool isNull() const {
            return address.isNull();
        }

        void clear() {
            address.clear();
            name.clear();
        }

    } m_foundDevice;

private:
    static const Event::Type DiscoveryStartedEvent      = Event::Type(Event::User + 1);
    static const Event::Type DiscoveryStoppedEvent      = Event::Type(Event::User + 2);
    static const Event::Type DiscoveryTimeoutEvent      = Event::Type(Event::User + 3);

    static const Event::Type DeviceFoundEvent           = Event::Type(Event::User + 4);

    static const Event::Type DiscoveryStartTimeoutEvent = Event::Type(Event::User + 5);
    static const Event::Type DiscoveryStopTimeoutEvent  = Event::Type(Event::User + 6);

    static const Event::Type CancelRequestEvent         = Event::Type(Event::User + 7);

    static const Event::Type AdapterPoweredOffEvent     = Event::Type(Event::User + 8);
};



#endif // !defined(BLERCUSCANNERSTATEMACHINE_H)
