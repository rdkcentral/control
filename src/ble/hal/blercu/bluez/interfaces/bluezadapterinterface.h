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
//  bluezadapterinterface.h
//

#ifndef BLUEZADAPTERINTERFACE_H
#define BLUEZADAPTERINTERFACE_H

#include "dbus/dbusabstractinterface.h"

#include <string>

// -----------------------------------------------------------------------------
/**
 *  @class BluezAdapterInterface
 *  @brief Proxy class for dbus interface org.bluez.Adapter1
 */
class BluezAdapterInterface : public DBusAbstractInterface
{

public:
    static inline const char *staticInterfaceName()
    { return "org.bluez.Adapter1"; }

public:
    BluezAdapterInterface(const std::string &service, const std::string &path, const GDBusConnection *connection)
        : DBusAbstractInterface(service, path, staticInterfaceName(), connection)
    { }

    ~BluezAdapterInterface()
    { }

    inline bool address(std::string &ret) const
    { return getProperty("Address").toString(ret); }

    inline bool alias(std::string &ret) const
    { return getProperty("Alias").toString(ret); }
    inline void setAlias(const std::string &value)
    { setProperty("Alias", g_variant_new("s", value.c_str())); }

    inline bool deviceClass(uint32_t &ret) const
    { return getProperty("Class").toUInt(ret); }

    inline bool discoverable(bool &ret) const
    { return getProperty("Discoverable").toBool(ret); }
    inline void setDiscoverable(bool value)
    { setProperty("Discoverable", g_variant_new("b", value)); }

    inline bool discoverableTimeout(uint32_t &ret) const
    { return getProperty("DiscoverableTimeout").toUInt(ret); }
    inline void setDiscoverableTimeout(uint32_t value)
    { setProperty("DiscoverableTimeout", g_variant_new("u", value)); }

    inline bool discovering(bool &ret) const
    { return getProperty("Discovering").toBool(ret); }

    inline bool modAlias(std::string &ret) const
    { return getProperty("Modalias").toString(ret); }

    inline bool name(std::string &ret) const
    { return getProperty("Name").toString(ret); }

    inline bool pairable(bool &ret) const
    { return getProperty("Pairable").toBool(ret); }
    inline void setPairable(bool value)
    { setProperty("Pairable", g_variant_new("b", value)); }

    inline bool pairableTimeout(uint32_t &ret) const
    { return getProperty("PairableTimeout").toUInt(ret); }
    inline void setPairableTimeout(uint32_t value)
    { setProperty("PairableTimeout", g_variant_new("u", value)); }

    inline bool powered(bool &ret) const
    { return getProperty("Powered").toBool(ret); }
    inline void setPowered(bool value)
    { setProperty("Powered", g_variant_new("b", value)); }

    inline void setPowered(bool value, PendingReply<> &&reply)
    { 
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncSetProperty("Powered", g_variant_new("b", value), variantReply);
    }

    inline bool uuids(std::vector<std::string> &ret) const
    { return getProperty("UUIDs").toStringList(ret); }

    
    inline void RemoveDevice(const std::string &devicePath, PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("RemoveDevice", variantReply, g_variant_new("(o)", devicePath.c_str()));
    }
    inline bool RemoveDeviceSync(const std::string &devicePath, std::string &errorMessage)
    {
        GVariant *reply = syncCall("RemoveDevice", g_variant_new("(o)", devicePath.c_str()), errorMessage);
        bool success = (reply != NULL);
        if (NULL != reply) { g_variant_unref(reply); }
        return success;
    }
    inline bool SetDiscoveryFilterForLESync(std::string &errorMessage)
    {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&builder, "{sv}", "Transport", g_variant_new_string("le"));
        GVariant *dict = g_variant_builder_end(&builder);

        GVariant *reply = syncCall("SetDiscoveryFilter", g_variant_new_tuple(&dict, 1), errorMessage);
        bool success = (reply != NULL);
        if (NULL != reply) { g_variant_unref(reply); }
        return success;
    }
    inline void StartDiscovery(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("StartDiscovery", variantReply);
    }
    inline bool StartDiscoverySync(std::string &errorMessage)
    {
        GVariant *reply = syncCall("StartDiscovery", NULL, errorMessage);
        bool success = (reply != NULL);
        if (NULL != reply) { g_variant_unref(reply); }
        return success;
    }

    inline void StopDiscovery(PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                reply.setError(dbusReply->errorMessage());
                reply.finish();
            };
        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("StopDiscovery", variantReply);
    }
    inline bool StopDiscoverySync(std::string &errorMessage)
    {
        GVariant *reply = syncCall("StopDiscovery", NULL, errorMessage);
        bool success = (reply != NULL);
        if (NULL != reply) { g_variant_unref(reply); }
        return success;
    }
};

#endif // !defined(BLUEZADAPTERINTERFACE_H)
