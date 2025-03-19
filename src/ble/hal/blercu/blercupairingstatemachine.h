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
//  blercucpairingstatemachine.h
//

#ifndef BLERCUPAIRINGSTATEMACHINE_H
#define BLERCUPAIRINGSTATEMACHINE_H

#include "utils/bleaddress.h"
#include "utils/statemachine.h"
#include "utils/slot.h"

#include "btrmgradapter.h"

#include <memory>
#include <regex>
#include <vector>


class BleRcuAdapter;
class ConfigSettings;


class BleRcuPairingStateMachine
{
public:
    enum State {
        RunningSuperState,
            DiscoverySuperState,
                StartingDiscoveryState,
                DiscoveringState,
            PairingSuperState,
                StoppingDiscoveryState,
                EnablePairableState,
                PairingState,
                SetupState,
            UnpairingState,
            StoppingDiscoveryStartedExternally,
        FinishedState
    };

public:
    BleRcuPairingStateMachine(const std::shared_ptr<const ConfigSettings> &config,
                              const std::shared_ptr<BleRcuAdapter> &adapter);
    ~BleRcuPairingStateMachine();

public:
    bool isRunning() const;
    bool isAutoPairing() const;
    int pairingCode() const;

// public slots:
    void start(const BleAddress &target, const std::string &name);
    void startAutoWithTimeout(int timeoutMs);
    void startWithCode(uint8_t pairingCode);
    void startWithMacHash(uint8_t macHash);
    void startWithMacList(const std::vector<BleAddress> &macList);
    void stop();


// signals:
    inline void addStartedSlot(const Slot<> &func)
    {
        m_startedSlots.addSlot(func);
    }
    inline void addPairingSlot(const Slot<> &func)
    {
        m_pairingSlots.addSlot(func);
    }
    inline void addFinishedSlot(const Slot<> &func)
    {
        m_finishedSlots.addSlot(func);
    }
    inline void addFailedSlot(const Slot<> &func)
    {
        m_failedSlots.addSlot(func);
    }

private:
    Slots<> m_startedSlots;
    Slots<> m_pairingSlots;
    Slots<> m_finishedSlots;
    Slots<> m_failedSlots;


// private slots:
    void onDiscoveryChanged(bool discovering);
    void onPairableChanged(bool pairable);

    void onDeviceFound(const BleAddress &address, const std::string &name);
    void onDeviceRemoved(const BleAddress &address);
    void onDeviceNameChanged(const BleAddress &address, const std::string &name);
    void onDevicePairingChanged(const BleAddress &address, bool paired);
    void onDeviceReadyChanged(const BleAddress &address, bool ready);
    void onDevicePairingError(const BleAddress &address, const std::string &error);

    void onAdapterPoweredChanged(bool powered);

    void onStateEntry(int state);
    void onStateExit(int state);
    void onStateTransition(int oldState, int newState);

    void onEnteredStartDiscoveryState();
    void onEnteredDiscoveringState();
    void onExitedDiscoverySuperState();

    void onEnteredStoppingDiscoveryState();
    void onEnteredEnablePairableState();
    void onEnteredPairingState();
    void onEnteredSetupState();
    void onExitedPairingSuperState();

    void onEnteredUnpairingState();
    void onExitedUnpairingState();

    void onEnteredFinishedState();

    void onEnteredStoppingDiscoveryStartedExternally();


private:
    void setupStateMachine();
    void processDevice(const BleAddress &address, const std::string &name);

private:
    std::shared_ptr<bool> m_isAlive;

    const std::shared_ptr<BleRcuAdapter> m_adapter;

    std::vector<std::string> m_pairingPrefixFormats;
    std::vector<std::regex> m_supportedPairingNames;

    bool m_isAutoPairing;
    int m_pairingCode;
    int m_pairingMacHash;
    std::vector<BleAddress> m_pairingMacList;
    std::vector<std::regex> m_targetedPairingNames;

    BleAddress m_targetAddress;

    StateMachine m_stateMachine;

    int m_discoveryTimeout;
    int m_discoveryTimeoutDefault;
    int m_pairingTimeout;
    int m_setupTimeout;
    int m_unpairingTimeout;

    int m_pairingAttempts;
    int m_pairingSuccesses;
    bool m_pairingSucceeded;

    BtrMgrAdapter m_btrMgrAdapter;
    bool discoveryStartedExternally = false;
    BtrMgrAdapter::OperationType lastOperationType = BtrMgrAdapter::unknownOperation;

private:
    static const Event::Type DiscoveryStartedEvent      = Event::Type(Event::User + 1);
    static const Event::Type DiscoveryStoppedEvent      = Event::Type(Event::User + 2);
    static const Event::Type DiscoveryStartTimeoutEvent = Event::Type(Event::User + 3);
    static const Event::Type DiscoveryStopTimeoutEvent  = Event::Type(Event::User + 4);

    static const Event::Type PairableEnabledEvent       = Event::Type(Event::User + 5);
    static const Event::Type PairableDisabledEvent      = Event::Type(Event::User + 6);

    static const Event::Type PairingTimeoutEvent        = Event::Type(Event::User + 7);
    static const Event::Type SetupTimeoutEvent          = Event::Type(Event::User + 8);
    static const Event::Type UnpairingTimeoutEvent      = Event::Type(Event::User + 9);

    static const Event::Type DeviceFoundEvent           = Event::Type(Event::User + 10);
    static const Event::Type DeviceUnpairedEvent        = Event::Type(Event::User + 11);
    static const Event::Type DeviceRemovedEvent         = Event::Type(Event::User + 12);
    static const Event::Type DevicePairedEvent          = Event::Type(Event::User + 13);
    static const Event::Type DeviceReadyEvent           = Event::Type(Event::User + 14);

    static const Event::Type CancelRequestEvent         = Event::Type(Event::User + 15);
    
    static const Event::Type PairingErrorEvent          = Event::Type(Event::User + 16);

    static const Event::Type AdapterPoweredOffEvent     = Event::Type(Event::User + 17);

};


#endif // !defined(BLERCUPAIRINGSTATEMACHINE_H)
