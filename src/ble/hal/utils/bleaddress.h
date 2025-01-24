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
//  bleaddress.h
//

#ifndef BLEADDRESS_H
#define BLEADDRESS_H

#include <string>
#include <vector>


class BleAddress
{
public:
    enum AddressOrder {
        LSBOrder,
        MSBOrder,
    };

public:
    BleAddress();
    BleAddress(uint64_t address);
    explicit BleAddress(const char *address);
    explicit BleAddress(const uint8_t address[6], AddressOrder order = MSBOrder);
    explicit BleAddress(const std::string &address);
    // explicit BleAddress(QLatin1String address);
    BleAddress(const BleAddress &other);
    ~BleAddress();


public:
    BleAddress &operator=(const BleAddress &rhs);
    BleAddress &operator=(BleAddress &&rhs);
    BleAddress &operator=(const std::string &rhs);

    uint8_t operator[](int index) const;


public:
    void clear();
    bool isNull() const;

    uint32_t oui() const;

    std::string toString() const;
    uint64_t toUInt64() const;
    std::vector<uint8_t> toArray() const;

private:
    // friend QDebug operator<<(QDebug dbg, const BleAddress &address);

    friend bool operator<(const BleAddress &bdaddr1, const BleAddress &bdaddr2);
    friend bool operator==(const BleAddress &bdaddr1, const BleAddress &bdaddr2);
    friend bool operator!=(const BleAddress &bdaddr1, const BleAddress &bdaddr2);
    // friend uint qHash(const BleAddress &key, uint seed);

    const char* _toString(char buf[32]) const;
    uint64_t _fromString(const char *address, int length) const;

private:
    uint64_t m_address;
};


inline bool operator<(const BleAddress &bdaddr1, const BleAddress &bdaddr2)
{
    return bdaddr1.m_address < bdaddr2.m_address;
}

inline bool operator==(const BleAddress &bdaddr1, const BleAddress &bdaddr2)
{
    return bdaddr1.m_address == bdaddr2.m_address;
}

inline bool operator!=(const BleAddress &bdaddr1, const BleAddress &bdaddr2)
{
    return bdaddr1.m_address != bdaddr2.m_address;
}

// inline uint qHash(const BleAddress &key, uint seed)
// {
//  return qHash(key.m_address, seed);
// }

#endif // !defined(BLEADDRESS_H)
