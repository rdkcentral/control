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
//  bleaddress.cpp
//

#include "bleaddress.h"
#include <cstring>
#include "ctrlm_log_ble.h"

using namespace std;

// -----------------------------------------------------------------------------
/*!
    \class BleAddress
    \brief Implementation modelled on QBluetoothAddress

    \ingroup utils

    Provides an object for parsing and storing bluetooth MAC addresses (BDADDR).
    The object can be used as a key in a QMap or as an object in QSet, in
    addition it provides toString() implementations and debug operators.

    A default constructed BleAddress object will be invalid and return \c true
    if isNull() is called.  If constructed with a QString users should check
    that the string was successifully parsed by running a isNull() check after
    construction.

    \note Our current version of Qt (5.6) doesn't have the bluetooth libraries,
    hence why this is needed.

 */



#define INVALID_ADDRESS  0xffffffffffffffffULL


BleAddress::BleAddress()
    : m_address(INVALID_ADDRESS)
{
}

BleAddress::BleAddress(const uint8_t address[6], AddressOrder order)
    : m_address(INVALID_ADDRESS)
{
    if (order == MSBOrder) {
        m_address = (uint64_t(address[0]) << 40) |
                    (uint64_t(address[1]) << 32) |
                    (uint64_t(address[2]) << 24) |
                    (uint64_t(address[3]) << 16) |
                    (uint64_t(address[4]) << 8) |
                    (uint64_t(address[5]) << 0);
    } else {
        m_address = (uint64_t(address[5]) << 40) |
                    (uint64_t(address[4]) << 32) |
                    (uint64_t(address[3]) << 24) |
                    (uint64_t(address[2]) << 16) |
                    (uint64_t(address[1]) << 8) |
                    (uint64_t(address[0]) << 0);
    }
}

BleAddress::BleAddress(uint64_t address)
    : m_address(address)
{
    if ((m_address >> 48) != 0)
        m_address = INVALID_ADDRESS;
    else if (m_address == 0)
        m_address = INVALID_ADDRESS;
    else if (m_address == 0xffffffffffffULL)
        m_address = INVALID_ADDRESS;
}

BleAddress::BleAddress(const std::string &address)
{
    m_address = _fromString(address.c_str(), address.length());
}

BleAddress::BleAddress(const char *address)
{
    m_address = _fromString(address, strlen(address));
}

// BleAddress::BleAddress(QLatin1String address)
// {
//  m_address = _fromString(address.data(), address.size());
// }

BleAddress::BleAddress(const BleAddress &other)
    : m_address(other.m_address)
{
}

BleAddress::~BleAddress()
{
}

BleAddress & BleAddress::operator=(const BleAddress &rhs)
{
    m_address = rhs.m_address;
    return *this;
}

BleAddress & BleAddress::operator=(BleAddress &&rhs)
{
    m_address = std::move(rhs.m_address);
    return *this;
}

// BleAddress & BleAddress::operator=(const QLatin1String &rhs)
// {
//  m_address = _fromString(rhs.data(), rhs.size());
//  return *this;
// }

BleAddress & BleAddress::operator=(const std::string &rhs)
{
    m_address = _fromString(rhs.c_str(), rhs.length());
    return *this;
}

void BleAddress::clear()
{
    m_address = INVALID_ADDRESS;
}

bool BleAddress::isNull() const
{
    return (m_address == INVALID_ADDRESS);
}

uint64_t BleAddress::_fromString(const char *address, int length) const
{
    // convert to a ASCII string and then sanity check it
    if (length != 17)
        return INVALID_ADDRESS;

    for (int i = 0; i < length; ++i) {

        if (((i + 1) % 3) == 0) {
            // every 3rd char must be ':'
            if (address[i] != ':')
                return INVALID_ADDRESS;
        } else {
            // all other chars must be a hex digit
            if (!isxdigit(address[i]))
                return INVALID_ADDRESS;
        }
    }

    // extract the fields of the string
    uint8_t bytes[6];
    sscanf(address, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]);

    return (uint64_t(bytes[0]) << 40) |
           (uint64_t(bytes[1]) << 32) |
           (uint64_t(bytes[2]) << 24) |
           (uint64_t(bytes[3]) << 16) |
           (uint64_t(bytes[4]) << 8) |
           (uint64_t(bytes[5]) << 0);
}

const char* BleAddress::_toString(char buf[32]) const
{
    if (m_address == INVALID_ADDRESS)
        return nullptr;

    sprintf(buf, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
            uint8_t((m_address >> 40) & 0xff),
            uint8_t((m_address >> 32) & 0xff),
            uint8_t((m_address >> 24) & 0xff),
            uint8_t((m_address >> 16) & 0xff),
            uint8_t((m_address >> 8)  & 0xff),
            uint8_t((m_address >> 0)  & 0xff));
    return buf;
}

// -----------------------------------------------------------------------------
/*!
    Returns the MAC address as a string in the standard format.

    If the address is not valid then an empty string is returned.
 */
std::string BleAddress::toString() const
{
    if (m_address == INVALID_ADDRESS)
        return string();
    else {
        char buf[32];
        return string(_toString(buf));
    }
}

// -----------------------------------------------------------------------------
/*!
    Returns the MAC address in the lower 48-bits of the returned value.

    If the address is not valid then 0 is returned.
 */
uint64_t BleAddress::toUInt64() const
{
    if (m_address == INVALID_ADDRESS)
        return 0;
    else
        return m_address;
}

// -----------------------------------------------------------------------------
/*!
    Returns the MAC address as an array of 6 byte values.  The most significant
    byte will be an index 0 and the least significant at index 5.

    If the address is not valid then an empty array is returned.
 */
std::vector<uint8_t> BleAddress::toArray() const
{
    if (m_address == INVALID_ADDRESS) {
        return vector<uint8_t> (0);
    }

    vector<uint8_t> bdaddr(6);

    bdaddr[0] = uint8_t((m_address >> 40) & 0xff);
    bdaddr[1] = uint8_t((m_address >> 32) & 0xff);
    bdaddr[2] = uint8_t((m_address >> 24) & 0xff);
    bdaddr[3] = uint8_t((m_address >> 16) & 0xff);
    bdaddr[4] = uint8_t((m_address >> 8)  & 0xff);
    bdaddr[5] = uint8_t((m_address >> 0)  & 0xff);

    return bdaddr;
}

// -----------------------------------------------------------------------------
/*!
    Accesses the individual bytes in the address.  Index \c 0 is the most
    significant byte, and \a index 5 is the least significant.

    \a index values less than 0 or greater than 5 are invalid and will always
    return 0.

 */
uint8_t BleAddress::operator[](int index) const
{
    if (index < 0 || index >= 6) {
        XLOGD_ERROR("invalid index <%d>", index);
    } else if (m_address == INVALID_ADDRESS) {
        XLOGD_ERROR("invalid address");
        return 0x00;
    }

    switch (index) {
        case 0:
            return uint8_t((m_address >> 40) & 0xff);
        case 1:
            return uint8_t((m_address >> 32) & 0xff);
        case 2:
            return uint8_t((m_address >> 24) & 0xff);
        case 3:
            return uint8_t((m_address >> 16) & 0xff);
        case 4:
            return uint8_t((m_address >> 8) & 0xff);
        case 5:
            return uint8_t((m_address >> 0) & 0xff);
        default:
            return 0x00;
    }
}

// -----------------------------------------------------------------------------
/*!
    Returns the 24-bit OUI (Organizationally Unique Identifier) part of the
    address.  The OUI is the most significant 3 bytes of the address and is
    equivalent to doing the following:

    \code
        BleRcuAddress bdaddr("11:22:33:44:55:66");
        uint64_t oui = (bdaddr.toUInt64() >> 24) & 0xffffff;
    \endcode

    If the address is not valid then 0 is returned.
 */
uint32_t BleAddress::oui() const
{
    if (m_address == INVALID_ADDRESS) {
        return 0;
    }

    return uint32_t((m_address >> 24) & 0xffffff);
}
