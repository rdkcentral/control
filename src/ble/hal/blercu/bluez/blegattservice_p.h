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
//  blegattservice_p.h
//

#ifndef BLUEZ_BLEGATTSERVICE_P_H
#define BLUEZ_BLEGATTSERVICE_P_H

#include "../blegattservice.h"
#include "dbus/dbusvariant.h"
#include <map>


class BleGattCharacteristicBluez;


class BleGattServiceBluez : public BleGattService
{
public:
    BleGattServiceBluez(const GDBusConnection *conn,
                        const std::string &path,
                        const DBusPropertiesMap &properties);
    ~BleGattServiceBluez();

public:
    bool isValid() const override;
    BleUuid uuid() const override;
    int instanceId() const override;
    bool primary() const override;

    std::vector< std::shared_ptr<BleGattCharacteristic> > characteristics() const override;
    std::vector< std::shared_ptr<BleGattCharacteristic> > characteristics(BleUuid charUuid) const override;
    std::shared_ptr<BleGattCharacteristic> characteristic(BleUuid charUuid) const override;

private:
    friend class BleGattProfileBluez;

    void addCharacteristic(const std::shared_ptr<BleGattCharacteristicBluez> &characteristic);

private:
    const std::string m_path;

    bool m_valid;
    bool m_primary;
    BleUuid m_uuid;
    int m_instanceId;

    std::string m_devicePath;

    std::multimap<BleUuid, std::shared_ptr<BleGattCharacteristicBluez>> m_characteristics;
};


#endif // !defined(BLUEZ_BLEGATTSERVICE_P_H)
