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
//  blegattdescriptor.cpp
//

#include "blegattdescriptor_p.h"
// #include "blegatthelpers.h"

#include "ctrlm_log_ble.h"

#include <algorithm>

using namespace std;

// -----------------------------------------------------------------------------
/*!
    \internal

    Constructs a new object parsing the details of the GATT descriptor from
    the dictionary received from bluez over dbus.

    The following is an example of the \a properties we get over dbus when a
    service is found

    \startcode

        dict entry(
            string "org.bluez.GattDescriptor1"
            array [
                dict entry(
                    string "UUID"
                    variant                   string "00030003-bdf0-407c-aaff-d09967f31acd"
                )
                dict entry(
                    string "Characteristic"
                    variant                   object path "/org/bluez/hci0/dev_1C_A2_B1_BE_EF_02/service003d/char0040"
                )
                dict entry(
                    string "Value"
                    variant                   array [
                    ]
                )
            ]
        )

    \endcode
 */
BleGattDescriptorBluez::BleGattDescriptorBluez(const GDBusConnection *conn,
                                                const std::string &path,
                                                const DBusPropertiesMap &properties)
    : BleGattDescriptorBluezProxy(conn, path)
    , m_isAlive(make_shared<bool>(true))
    , m_valid(false)
    , m_flags(0)
    , m_cacheable(false)
{
    // get the uuid of the descriptor
    string uuid_str;
    DBusPropertiesMap::const_iterator property = properties.find("UUID");
    if ((property == properties.end()) || !property->second.toString(uuid_str)) {
        XLOGD_WARN("invalid uuid property of gatt descriptor <%s>", m_path);
        return;
    }
    m_uuid = BleUuid(uuid_str);

    // get the parent characteristic object path (used for constructing the tree)
    property = properties.find("Characteristic");
    if ((property == properties.end()) || !property->second.toObjectPath(m_characteristicPath)) {
        XLOGD_WARN("failed to get the characteristic path of the descriptor with uuid: %s", m_uuid.toString().c_str());
    }

    // the flags are a string array
    std::vector<std::string> flagsList;
    property = properties.find("Flags");
    if ((property == properties.end()) || !property->second.toStringList(flagsList)) {
        // its expected that some descriptors don't have flags, like 2902 (CCCD), so silence this log
        // XLOGD_DEBUG("invalid flags of gatt descriptor with uuid: %s", m_uuid.toString().c_str());
    } else {

        // possible flags
        static const map<string, uint16_t> flagsMap = {
            { string("read"),                        Read },
            { string("write"),                       Write },
            { string("encrypt-read"),                EncryptRead },
            { string("encrypt-write"),               EncryptWrite },
            { string("encrypt-authenticated-read"),  EncryptAuthenticatedRead },
            { string("encrypt-authenticated-write"), EncryptAuthenticatedWrite }
        };

        // parse all the flags
        for (string &flag : flagsList) {
              std::transform(flag.begin(), flag.end(), flag.begin(), ::tolower);
            map<string, uint16_t>::const_iterator it = flagsMap.find(flag);
            if (it == flagsMap.end()) {
                XLOGD_WARN("unknown flag for gatt descriptor 0x%X", flag);
            } else {
                m_flags |= it->second;
            }
        }
    }

    m_valid = true;
}

// -----------------------------------------------------------------------------
/*!

 */
BleGattDescriptorBluez::~BleGattDescriptorBluez()
{
    *m_isAlive = false;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattDescriptor::timeout()

    Returns the current value of the timeout in milliseconds. \c -1 means the
    default timeout (usually 25 seconds).

 */
int BleGattDescriptorBluez::timeout()
{
    auto proxy = getProxy();
    
    if (proxy && proxy->isValid()) {
        return proxy->timeout();
    } else {
        return -1;
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattDescriptor::setTimeout(int timeout) const

    Sets the timeout in milliseconds for all future DBus calls to timeout. \c -1
    means the default timeout (usually 25 seconds).

 */
void BleGattDescriptorBluez::setTimeout(int timeout)
{
    auto proxy = getProxy();
    
    if (proxy && proxy->isValid()) {
        if (timeout < 0) {
            proxy->setTimeout(-1);
        } else {
            proxy->setTimeout(std::max(1000, std::min(timeout, 60000)));
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattDescriptor::isValid() const

    Returns \c true if the object was successifully created from the java / JNI
    class object.

 */
bool BleGattDescriptorBluez::isValid() const
{
    return m_valid;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleUuid BleGattDescriptor::uuid() const

    Returns the UUID of the descriptor.  If the descriptor is not valid the
    returned value is undefined.

 */
BleUuid BleGattDescriptorBluez::uuid() const
{
    return m_uuid;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleGattDescriptor::Flags BleGattDescriptor::flags() const

    Returns the properties of the descriptor. If the descriptor is not valid the
    returned value is undefined.

 */
uint16_t BleGattDescriptorBluez::flags() const
{
    return m_flags;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleGattDescriptor::setCacheable(bool cacheable)

    Sets whether this descriptor is \a cacheable; cacheable descriptors store the
    last read / written value and on a new read won't actually send a request to
    the remote device, instead will return the last read / written value.

    By default the cacheable property is \c false.

 */
void BleGattDescriptorBluez::setCacheable(bool cacheable)
{
    if (m_cacheable != cacheable) {
        m_lastValue.clear();
    }

    m_cacheable = cacheable;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattDescriptor::cacheable() const

    Returns the current cacheable property value.

    \see BleGattDescriptor::setCacheable()
 */
bool BleGattDescriptorBluez::cacheable() const
{
    return m_cacheable;
}

std::vector<uint8_t> BleGattDescriptorBluez::readValueSync(std::string &errorMessage) 
{
    std::vector<uint8_t> ret;
    auto proxy = getProxy();

    if (!proxy || !proxy->isValid()) {
        errorMessage = "no proxy connection";
        XLOGD_ERROR("failed: <%s>", errorMessage.c_str());
        return ret;
    }
    return proxy->ReadValueSync();
}

// -----------------------------------------------------------------------------
/*!
    \fn BleGattDescriptor::readValue()

    Requests a read of the value stored in the descriptor.  The request is async
    with the result being returned in the PendingReply callback.

 */
void BleGattDescriptorBluez::readValue(PendingReply<std::vector<uint8_t>> &&reply)
{
    std::vector<uint8_t> ret;
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }
    proxy->ReadValue(std::move(reply));
}



// -----------------------------------------------------------------------------
/*!
    \fn Future<void> BleGattDescriptor::writeValue()

    Requests a write of the value stored in the descriptor.  The request is async
    with the result being returned in the future object.

 */
void BleGattDescriptorBluez::writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply)
{
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }
    proxy->WriteValue(value, std::move(reply));
}

