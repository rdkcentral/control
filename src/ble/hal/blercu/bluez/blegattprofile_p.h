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
//  blegattprofile_p.h
//

#ifndef BLUEZ_BLEGATTPROFILE_P_H
#define BLUEZ_BLEGATTPROFILE_P_H

#include "../blegattprofile.h"
#include "dbus/dbusvariant.h"
#include "dbus/dbusobjectmanager.h"
#include "utils/pendingreply.h"


class BleGattServiceBluez;


class BleGattProfileBluez : public BleGattProfile
{

public:
    BleGattProfileBluez(const GDBusConnection *bluezDBusConn,
                        const std::string &bluezDBusPath);
    ~BleGattProfileBluez() final;

public:
    bool isValid() const override;
    bool isEmpty() const override;

    void updateProfile() override;

    std::vector< std::shared_ptr<BleGattService> > services() const override;
    std::vector< std::shared_ptr<BleGattService> > services(const BleUuid &serviceUuid) const override;
    std::shared_ptr<BleGattService> service(const BleUuid &serviceUuid) const override;

private:
    void onGetObjectsReply(PendingReply<DBusManagedObjectList> *reply);
    void updateBluezVersion(const DBusPropertiesMap &properties);
    void dumpGattTree();

private:
    std::shared_ptr<bool> m_isAlive;
    const GDBusConnection *m_dbusConn;
    const std::string m_dbusPath;

    // QVersionNumber m_bluezVersion;

    bool m_valid;
    std::multimap<BleUuid, std::shared_ptr<BleGattServiceBluez>> m_services;
};



#endif // !defined(BLUEZ_BLEGATTPROFILE_P_H)
