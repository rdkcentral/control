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
//  blercumanager.h
//

#ifndef BLERCUADAPTER_H
#define BLERCUADAPTER_H

#include "utils/bleaddress.h"
#include "utils/slot.h"

#include <set>
#include <memory>
#include <map>
#include <glib.h>

class BleRcuDevice;


class BleRcuAdapter
{
protected:
    BleRcuAdapter() = default;

public:
    virtual ~BleRcuAdapter() = default;

public:
    virtual bool isValid() const = 0;
    virtual bool isAvailable() const = 0;
    virtual bool isPowered() const = 0;

    virtual bool isDiscovering() const = 0;
    virtual bool startDiscovery() = 0;
    virtual bool stopDiscovery() = 0;

    virtual bool isPairable() const = 0;
    virtual bool enablePairable(unsigned int timeout) = 0;
    virtual bool disablePairable() = 0;

    virtual void reconnectAllDevices() = 0;

    virtual std::set<BleAddress> pairedDevices() const = 0;
    virtual std::map<BleAddress, std::string> deviceNames() const = 0;

    virtual std::shared_ptr<BleRcuDevice> getDevice(const BleAddress &address) const = 0;

    virtual bool isDevicePaired(const BleAddress &address) const = 0;
    virtual bool isDeviceConnected(const BleAddress &address) const = 0;

    virtual bool addDevice(const BleAddress &address) = 0;
    virtual bool removeDevice(const BleAddress &address) = 0;

    virtual bool setConnectionParams(BleAddress address, double minInterval, double maxInterval,
                                     int32_t latency, int32_t supervisionTimeout) = 0;


    inline void addPoweredChangedSlot(const Slot<bool> &func)
    {
        m_poweredChangedSlots.addSlot(func);
    }
    inline void addPoweredInitializedSlot(const Slot<> &func)
    {
        m_poweredInitializedSlots.addSlot(func);
    }
    inline void addDiscoveryChangedSlot(const Slot<bool> &func)
    {
        m_discoveryChangedSlots.addSlot(func);
    }
    inline void addPairableChangedSlot(const Slot<bool> &func)
    {
        m_pairableChangedSlots.addSlot(func);
    }
    inline void addDeviceFoundSlot(const Slot<const BleAddress&, const std::string&> &func)
    {
        m_deviceFoundSlots.addSlot(func);
    }
    inline void addDeviceRemovedSlot(const Slot<const BleAddress&> &func)
    {
        m_deviceRemovedSlots.addSlot(func);
    }
    inline void addDeviceNameChangedSlot(const Slot<const BleAddress&, const std::string&> &func)
    {
        m_deviceNameChangedSlots.addSlot(func);
    }
    inline void addDevicePairingErrorSlot(const Slot<const BleAddress&, const std::string&> &func)
    {
        m_devicePairingErrorSlots.addSlot(func);
    }
    inline void addDevicePairingChangedSlot(const Slot<const BleAddress&, bool> &func)
    {
        m_devicePairingChangedSlots.addSlot(func);
    }
    inline void addDeviceReadyChangedSlot(const Slot<const BleAddress&, bool> &func)
    {
        m_deviceReadyChangedSlots.addSlot(func);
    }

    inline GMainLoop* getGMainLoop()
    {
        return m_GMainLoop;
    }

protected:
    GMainLoop *m_GMainLoop;

    Slots<bool>                                     m_poweredChangedSlots;
    Slots<>                                         m_poweredInitializedSlots;
    Slots<bool>                                     m_discoveryChangedSlots;
    Slots<bool>                                     m_pairableChangedSlots;
    Slots<const BleAddress&, const std::string&>    m_deviceFoundSlots;
    Slots<const BleAddress&>                        m_deviceRemovedSlots;
    Slots<const BleAddress&, const std::string&>    m_deviceNameChangedSlots;
    Slots<const BleAddress&, const std::string&>    m_devicePairingErrorSlots;
    Slots<const BleAddress&, bool>                  m_devicePairingChangedSlots;
    Slots<const BleAddress&, bool>                  m_deviceReadyChangedSlots;

};

#endif // !defined(BLERCUADAPTER_H)
