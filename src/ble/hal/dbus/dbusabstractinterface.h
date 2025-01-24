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
//  dbusabstractinterface.h
//  BleRcuDaemon
//

#ifndef DBUSABSTRACTINTERFACE_H
#define DBUSABSTRACTINTERFACE_H

#include <string>
#include <map>
#include <functional>
#include <mutex>

#include "gdbusabstractinterface.h"
#include "dbusvariant.h"
#include "utils/pendingreply.h"
#include "utils/slot.h"

typedef std::map<std::string, Slots<const DBusVariant&>> PropertyChangedSlotsMap;

class DBusAbstractInterface : public GDBusAbstractInterface
{
public:
    ~DBusAbstractInterface();

protected:
    explicit DBusAbstractInterface(const std::string &service,
                                   const std::string &path,
                                   const std::string &interface,
                                   const GDBusConnection *connection);
public:
    void addPropertyChangedSlot(std::string propertyName, const Slot<const DBusVariant&> &func);
    void addPropertyChangedSlot(std::string propertyName, const Slot<bool> &func);
    void addPropertyChangedSlot(std::string propertyName, const Slot<const std::string&> &func);
    void addPropertyChangedSlot(std::string propertyName, const Slot<const std::vector<uint8_t>&> &func);
    void asyncSetProperty(const std::string method, const GVariant *prop, const PendingReply<DBusVariant> &reply);
    void asyncGetProperty(const std::string method, const PendingReply<DBusVariant> &reply);
    void asyncMethodCall(const std::string method, const PendingReply<DBusVariant> &reply, GVariant *args = NULL);

    DBusPropertiesMap getAllProperties();
    DBusVariant getProperty(std::string name) const;
    bool setProperty(std::string name, GVariant *prop) const;

    //EGTODO: how to deal with removing callbacks when the callback object gets destroyed???
    // maybe its not a big concern because proxies get destroyed along with the callback object,
    // and we already use an isAlive shared_ptr to prevent crashes.
    // could be an issue if the same proxy is used in multiple places, and slots would simply never be
    // removed.  It wouldn't cause crash or anything, but the slots vector could fill up with lots of 
    // invalid callbacks.
    PropertyChangedSlotsMap m_propertyChangedSlots;  
    
    std::mutex m_propertyChangedSlotsLock;
    
private:
    unsigned long m_signalHandlerID;
    unsigned long m_propertiesChangedHandlerID;
};

#endif // DBUSABSTRACTINTERFACE_H
