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
//  bluezdeviceinterface.h
//

#ifndef BLUEZDEVICEINTERFACE_H
#define BLUEZDEVICEINTERFACE_H

#include "dbus/dbusabstractinterface.h"
#include "ctrlm_log_ble.h"


typedef std::map<uint16_t, DBusVariant> ManufacturerDataMap;


// -----------------------------------------------------------------------------
/**
 *  @class BluezDeviceInterface
 *  @brief Proxy class for dbus interface org.bluez.Device1
 */
class BluezDeviceInterface : public DBusAbstractInterface
{
public:
    static inline const char *staticInterfaceName()
    { return "org.bluez.Device1"; }

public:
    BluezDeviceInterface(const std::string &service, const std::string &path, const GDBusConnection *connection)
        : DBusAbstractInterface(service, path, staticInterfaceName(), connection)
    { }

    ~BluezDeviceInterface()
    { }

    inline bool adapter(std::string &ret) const
    { return getProperty("Adapter").toObjectPath(ret); }

    inline bool address(std::string &ret) const
    { return getProperty("Address").toString(ret); }

    inline bool alias(std::string &ret) const
    { return getProperty("Alias").toString(ret); }
    inline void setAlias(const std::string &value)
    { setProperty("Alias", g_variant_new("s", value.c_str())); }

    inline bool appearance(uint16_t &ret) const
    { return getProperty("Appearance").toUInt16(ret); }

    inline bool blocked(bool &ret) const
    { return getProperty("Blocked").toBool(ret); }
    inline void setBlocked(bool value)
    { setProperty("Blocked", g_variant_new("b", value)); }

    inline bool connected(bool &ret) const
    { return getProperty("Connected").toBool(ret); }

    inline bool icon(std::string &ret) const
    { return getProperty("Icon").toString(ret); }

    inline bool legacyPairing(bool &ret) const
    { return getProperty("LegacyPairing").toBool(ret); }

    inline bool modAlias(std::string &ret) const
    { return getProperty("Modalias").toString(ret); }

    inline bool name(std::string &ret) const
    { return getProperty("Name").toString(ret); }

    inline bool paired(bool &ret) const
    { return getProperty("Paired").toBool(ret); }

    inline bool rssi(int16_t &ret) const
    { return getProperty("RSSI").toInt16(ret); }

    inline bool trusted(bool &ret) const
    { return getProperty("Trusted").toBool(ret); }
    inline void setTrusted(bool value)
    { setProperty("Trusted", g_variant_new("b", value)); }

    inline bool uuids(std::vector<std::string> &ret) const
    { return getProperty("UUIDs").toStringList(ret); }

    inline bool deviceClass(uint32_t &ret) const
    { return getProperty("Class").toUInt(ret); }

    inline bool servicesResolved(bool &ret) const
    { return getProperty("ServicesResolved").toBool(ret); }

    inline bool txPower(int16_t &ret) const
    { return getProperty("TxPower").toInt16(ret); }

    // Q_PROPERTY(ManufacturerDataMap ManufacturerData READ manufacturerData NOTIFY manufacturerDataChanged)
    // inline ManufacturerDataMap manufacturerData() const
    // { return qvariant_cast< ManufacturerDataMap >(property("ManufacturerData")); }

    // Q_PROPERTY(QByteArray AdvertisingFlags READ advertisingFlags NOTIFY advertisingFlagsChanged)
    // inline QByteArray advertisingFlags() const
    // { return qvariant_cast< QByteArray >(property("AdvertisingFlags")); }

    inline void CancelPairing(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("CancelPairing", variantReply);
    }

    inline void Connect(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("Connect", variantReply);
    }

    inline void ConnectProfile(const std::string &UUID, PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("ConnectProfile", variantReply, g_variant_new("(s)", UUID.c_str()));
    }

    inline void Disconnect(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("Disconnect", variantReply);
    }

    inline bool DisconnectSync(std::string &errorMessage)
    {
        GVariant *reply = syncCall("Disconnect", NULL, errorMessage);
        bool success = (reply != NULL);
        if (NULL != reply) { g_variant_unref(reply); }
        return success;
    }

    inline void DisconnectProfile(const std::string &UUID, PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("DisconnectProfile", variantReply, g_variant_new("(s)", UUID.c_str()));
    }

    inline void Pair(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("Pair", variantReply);
    }


};

#endif // !defined(BLUEZDEVICEINTERFACE_H)
