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
//  dbusvariant.h
//  BleRcuDaemon
//

#ifndef DBUSVARIANT_H
#define DBUSVARIANT_H

#include <string>
#include <vector>
#include <map>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>


class DBusVariant
{
public:
    DBusVariant();
    DBusVariant(std::string name, GVariant *variant);
    DBusVariant(std::string name, GVariant *variant, GUnixFDList *fdList);
    DBusVariant(const DBusVariant &other);
    DBusVariant(DBusVariant &&other);
    ~DBusVariant();

    DBusVariant &operator=(const DBusVariant &other);
    DBusVariant &operator=(DBusVariant &&other);

    inline void setName(std::string name) { m_name = name; }

    GVariant* getGVariant() const;
    GUnixFDList* getGFdList() const;

    std::string dumpToString() const;

    bool canConvert(const GVariantType *t) const;

    bool toString(std::string &ret) const;
    bool toStringList(std::vector<std::string> &ret) const;
    bool toByteArray(std::vector<uint8_t> &ret) const;
    bool toObjectPath(std::string &ret) const;
    bool toBool(bool &ret) const;
    bool toChar(uint8_t &ret) const;
    bool toInt(int &ret) const;
    bool toInt16(int16_t &ret) const;
    bool toUInt(uint32_t &ret) const;
    bool toUInt16(uint16_t &ret) const;
    bool getFd(int index, int &retFd) const;

private:
    std::string m_name;
    GVariant *m_gVariant;
    GUnixFDList *m_gFdList;
};

typedef std::map<std::string, DBusVariant> DBusPropertiesMap;

#endif // !defined(DBUSVARIANT_H)
