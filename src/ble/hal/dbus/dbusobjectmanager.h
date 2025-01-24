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
//  dbusobjectmanager.h
//  BleRcuDaemon
//

#ifndef DBUSOBJECTMANAGER_H
#define DBUSOBJECTMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "gdbusabstractinterface.h"
#include "dbusvariant.h"
#include "utils/pendingreply.h"
#include "utils/slot.h"

typedef std::map<std::string, DBusPropertiesMap> DBusInterfaceList;
typedef std::map<std::string, DBusInterfaceList> DBusManagedObjectList;


class DBusObjectManagerInterface: public GDBusAbstractInterface
{

public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.DBus.ObjectManager"; }

public:
    DBusObjectManagerInterface(const std::string &service, const std::string &path,
                               const GDBusConnection *connection);
    ~DBusObjectManagerInterface();

    bool isValid() const;

    bool GetManagedObjects(DBusManagedObjectList &managedObjects);              //sync
    void GetManagedObjects(const PendingReply<DBusManagedObjectList> &reply);   //async

    inline void addInterfacesAddedSlot(const Slot<const std::string&, const DBusInterfaceList&> &func)
    {
        m_interfacesAddedSlots.addSlot(func);
    }
    void addInterfacesRemovedSlot(const Slot<const std::string&, const std::vector<std::string>&> &func)
    {
        m_interfacesRemovedSlots.addSlot(func);
    }


    Slots<const std::string&, const DBusInterfaceList&> m_interfacesAddedSlots;
    Slots<const std::string&, const std::vector<std::string>&> m_interfacesRemovedSlots;

private:
    unsigned long m_signalHandlerID;
};


#endif // !defined(DBUSOBJECTMANAGER_H)
