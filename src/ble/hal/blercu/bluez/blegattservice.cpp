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
//  blergattservice.cpp
//

#include "blegattservice_p.h"
#include "blegattcharacteristic_p.h"
#include "ctrlm_log_ble.h"

#include <vector>
#include <memory>

using namespace std;

// -----------------------------------------------------------------------------
/*!
    Constructs a new object parsing the details of the GATT service from the
    dictionary received from bluez.

    The following is an example of the \a properties we get over dbus when a
    service is found

    \startcode

        dict entry(
            string "org.bluez.GattService1"
            array [
                dict entry(
                    string "UUID"
                    variant                   string "00010001-bdf0-407c-aaff-d09967f31acd"
                )
                dict entry(
                    string "Device"
                    variant                   object path "/org/bluez/hci0/dev_1C_A2_B1_BE_EF_02"
                )
                dict entry(
                    string "Primary"
                    variant                   boolean true
                )
                dict entry(
                    string "Includes"
                    variant                   array [
                    ]
                )
            ]
        )

    \endcode

 */
BleGattServiceBluez::BleGattServiceBluez(const GDBusConnection *conn,
                                         const std::string &path,
                                         const DBusPropertiesMap &properties)
    : m_path(path)
    , m_valid(false)
    , m_primary(false)
    , m_instanceId(0)
{

    // get the uuid of the service
    string uuid_str;
    DBusPropertiesMap::const_iterator property = properties.find("UUID");
    if ((property == properties.end()) || !property->second.toString(uuid_str)) {
        XLOGD_WARN("invalid uuid property of gatt service <%s>", m_path);
        return;
    }
    m_uuid = BleUuid(uuid_str);

    // get the parent device object path (only used for sanity checking)
    property = properties.find("Device");
    if ((property == properties.end()) || !property->second.toObjectPath(m_devicePath)) {
        XLOGD_WARN("failed to get the device path of the service with uuid: %s", m_uuid.toString().c_str());
    }

    // check if this is a primary service
    property = properties.find("Primary");
    if ((property == properties.end()) || !property->second.toBool(m_primary)) {
        XLOGD_WARN("failed to get primary service flag of the service with uuid: %s", m_uuid.toString().c_str());
    }

    // the instance id is used to distinguish between different instances of the
    // same service, for bluez we use the dbus path id, ie. a typical dbus
    // path would be '/org/bluez/hci0/dev_D4_B8_FF_11_76_EE/service0043' we
    // trim off the last bit and parse the 0043 as the instance id
    size_t found = path.find_last_of("/");
    string serviceId = path.substr(found+1);
    if (sscanf(serviceId.c_str(), "service%04x", &m_instanceId) != 1) {
        XLOGD_WARN("failed to parse service '%s' to get the instance id", serviceId.c_str());
        m_instanceId = -1;
    }

    m_valid = true;
}

BleGattServiceBluez::~BleGattServiceBluez()
{

}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattService::isValid() const

    Returns \c true if the object was successifully created from the java / JNI
    class object.

 */
bool BleGattServiceBluez::isValid() const
{
    return m_valid;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleUuid BleGattService::uuid() const

    Returns the UUID of the service.  If the service is not valid the returned
    value is undefined.

 */
BleUuid BleGattServiceBluez::uuid() const
{
    return m_uuid;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleUuid BleGattService::instanceId() const

    Returns the instance ID for this service. If a remote device offers multiple
    service with the same UUID, the instance ID is used to distuinguish between
    services.

 */
int BleGattServiceBluez::instanceId() const
{
    return m_instanceId;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattService::primary() const

    Returns \c true if this service is the primary service.

 */
bool BleGattServiceBluez::primary() const
{
    return m_primary;
}

// -----------------------------------------------------------------------------
/*!

 */
vector< shared_ptr<BleGattCharacteristic> > BleGattServiceBluez::characteristics() const
{
    vector< shared_ptr<BleGattCharacteristic> > characteristics;
    characteristics.reserve(m_characteristics.size());

    for (auto const &characteristic : m_characteristics) {
        characteristics.push_back(characteristic.second);
    }

    return characteristics;
}

// -----------------------------------------------------------------------------
/*!

 */
vector< shared_ptr<BleGattCharacteristic> > BleGattServiceBluez::characteristics(BleUuid charUuid) const
{
    vector< shared_ptr<BleGattCharacteristic> > characteristics;
    characteristics.reserve(m_characteristics.size());

    for (auto const &characteristic : m_characteristics) {
        if (characteristic.first == charUuid)
            characteristics.push_back(characteristic.second);
    }

    return characteristics;
}

// -----------------------------------------------------------------------------
/*!

 */
shared_ptr<BleGattCharacteristic> BleGattServiceBluez::characteristic(BleUuid charUuid) const
{
    auto it = m_characteristics.find(charUuid);
    if (it != m_characteristics.end()) {
        return static_pointer_cast<BleGattCharacteristic>(it->second);
    } else {
        return nullptr;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Adds the characteristic to the service, basically just inserts it into the
    internal multimap.

 */
void BleGattServiceBluez::addCharacteristic(const shared_ptr<BleGattCharacteristicBluez> &characteristic)
{
    m_characteristics.insert(std::pair<BleUuid,shared_ptr<BleGattCharacteristicBluez>>(characteristic->uuid(), characteristic));
}

