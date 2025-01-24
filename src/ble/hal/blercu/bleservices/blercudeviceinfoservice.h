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
//  blercudeviceinfoservice.h
//

#ifndef BLERCUDEVICEINFOSERVICE_H
#define BLERCUDEVICEINFOSERVICE_H

#include "utils/slot.h"

#include <string>


class BleRcuDeviceInfoService
{
protected:
    BleRcuDeviceInfoService() = default;

public:
    ~BleRcuDeviceInfoService() = default;

public:
    enum PnPVendorSource {
        Invalid = 0,
        Bluetooth = 1,
        USB = 2
    };

    virtual std::string manufacturerName() const = 0;
    virtual std::string modelNumber() const = 0;
    virtual std::string serialNumber() const = 0;
    virtual std::string hardwareRevision() const = 0;
    virtual std::string firmwareVersion() const = 0;
    virtual std::string softwareVersion() const = 0;
    virtual uint64_t systemId() const = 0;

    virtual PnPVendorSource pnpVendorIdSource() const = 0;
    virtual uint16_t pnpVendorId() const = 0;
    virtual uint16_t pnpProductId() const = 0;
    virtual uint16_t pnpProductVersion() const = 0;

// signals:
    inline void addManufacturerNameChangedSlot(const Slot<const std::string &> &func)
    {
        m_manufacturerNameChangedSlots.addSlot(func);
    }
    inline void addModelNumberChangedSlot(const Slot<const std::string &> &func)
    {
        m_modelNumberChangedSlots.addSlot(func);
    }
    inline void addSerialNumberChangedSlot(const Slot<const std::string &> &func)
    {
        m_serialNumberChangedSlots.addSlot(func);
    }
    inline void addHardwareRevisionChangedSlot(const Slot<const std::string &> &func)
    {
        m_hardwareRevisionChangedSlots.addSlot(func);
    }
    inline void addFirmwareVersionChangedSlot(const Slot<const std::string &> &func)
    {
        m_firmwareVersionChangedSlots.addSlot(func);
    }
    inline void addSoftwareVersionChangedSlot(const Slot<const std::string &> &func)
    {
        m_softwareVersionChangedSlots.addSlot(func);
    }

protected:
    Slots<const std::string &> m_manufacturerNameChangedSlots;
    Slots<const std::string &> m_modelNumberChangedSlots;
    Slots<const std::string &> m_serialNumberChangedSlots;
    Slots<const std::string &> m_hardwareRevisionChangedSlots;
    Slots<const std::string &> m_firmwareVersionChangedSlots;
    Slots<const std::string &> m_softwareVersionChangedSlots;


};

#endif // !defined(BLERCUDEVICEINFOSERVICE_H)
