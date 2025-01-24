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
//  blercucontroller.h
//

#ifndef BLERCUCONTROLLER_H
#define BLERCUCONTROLLER_H

#include "utils/bleaddress.h"
#include "utils/slot.h"
#include "blercuerror.h"

#include <set>
#include <memory>


class BleRcuDevice;


class BleRcuController
{
public:
    virtual ~BleRcuController() = default;

protected:
    BleRcuController() = default;

public:
    enum State
    {
        Initialising,
        Idle,
        Searching,
        Pairing,
        Complete,
        Failed
    };

public:
    // virtual void dump(Dumper out) const = 0;

    virtual bool isValid() const = 0;
    virtual State state() const = 0;

    virtual BleRcuError lastError() const = 0;

    virtual bool isPairing() const = 0;
    virtual int pairingCode() const = 0;

    virtual bool startPairing(uint8_t filterByte, uint8_t pairingCode) = 0;
    virtual bool startPairingMacHash(uint8_t filterByte, uint8_t macHash) = 0;
    virtual bool cancelPairing() = 0;

    virtual bool isScanning() const = 0;
    virtual bool startScanning(int timeoutMs) = 0;
    virtual bool cancelScanning() = 0;

    virtual std::set<BleAddress> managedDevices() const = 0;
    virtual std::shared_ptr<BleRcuDevice> managedDevice(const BleAddress &address) const = 0;

    virtual bool unpairDevice(const BleAddress &address) const = 0;
    virtual void shutdown() const = 0;


// signals:

    inline void addManagedDeviceAddedSlot(const Slot<const BleAddress&> &func)
    {
        m_managedDeviceAddedSlots.addSlot(func);
    }
    inline void addManagedDeviceRemovedSlot(const Slot<const BleAddress&> &func)
    {
        m_managedDeviceRemovedSlots.addSlot(func);
    }
    inline void addScanningStateChangedSlot(const Slot<bool> &func)
    {
        m_scanningStateChangedSlots.addSlot(func);
    }
    inline void addPairingStateChangedSlot(const Slot<bool> &func)
    {
        m_pairingStateChangedSlots.addSlot(func);
    }
    inline void addStateChangedSlot(const Slot<State> &func)
    {
        m_stateChangedSlots.addSlot(func);
    }

protected:
    Slots<const BleAddress&> m_managedDeviceAddedSlots;
    Slots<const BleAddress&> m_managedDeviceRemovedSlots;
    Slots<bool> m_scanningStateChangedSlots;
    Slots<bool> m_pairingStateChangedSlots;
    Slots<State> m_stateChangedSlots;
};

#endif // !defined(BLERCUCONTROLLER_H)
