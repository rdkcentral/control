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
//  bluezgattdescriptorinterface.h
//

#ifndef BLUEZGATTDESCRIPTORINTERFACE_H
#define BLUEZGATTDESCRIPTORINTERFACE_H

#include "dbus/dbusabstractinterface.h"

#include "ctrlm_log_ble.h"


class BluezGattDescriptorInterface : public DBusAbstractInterface
{
public:
    static inline const char *staticInterfaceName()
    { return "org.bluez.GattDescriptor1"; }

public:
    BluezGattDescriptorInterface(const std::string &service, const std::string &path,
                                const GDBusConnection *connection)
        : DBusAbstractInterface(service, path, staticInterfaceName(), connection)
    { }

    ~BluezGattDescriptorInterface() = default;

    inline bool characteristic(std::string &ret) const
    { return getProperty("Characteristic").toString(ret); }


    inline bool uuid(std::string &ret) const
    { return getProperty("UUID").toString(ret); }

    inline bool value(std::vector<uint8_t> &ret) const
    { 
        gchar *result_str = g_variant_print(getProperty("Value").getGVariant(), false);
        XLOGD_INFO("path <%s>, Value is type <%s> = <%s>\n", path(), g_variant_get_type_string(getProperty("Value").getGVariant()), result_str);
        g_free(result_str);
        //EGTODO: parse the variant into the byte array (ret).  This function isn't currently used,
        // so leaving it unimplemented for now.
        return false; 
    }

    inline bool flags(std::vector<std::string> &ret) const
    { return getProperty("Flags").toStringList(ret); }


    inline std::vector<uint8_t> ReadValueSync()
    {
        std::string errorMessage;
        GVariant *reply = syncCall("ReadValue", NULL, errorMessage);

        std::vector<uint8_t> ret;
        if (reply != NULL) {
            // TODO: parse the reply into a vector<uint8_t>

            gchar *result_str = g_variant_print(reply, false);
            XLOGD_INFO("path <%s>, reply is type <%s> = <%s>\n", path().c_str(), g_variant_get_type_string(reply), result_str);
            g_free(result_str);

            g_variant_unref(reply);
        } else {
            XLOGD_INFO("failed... path <%s>, error = <%s>\n", path().c_str(), errorMessage.c_str());
        }

        return ret;
    }


    inline void ReadValue(PendingReply<std::vector<uint8_t>> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                std::vector<uint8_t> ret;
                reply.setName(dbusReply->getName());
                if (dbusReply->isError()) {
                    reply.setError(dbusReply->errorMessage());
                } else {
                    dbusReply->result().toByteArray(ret);
                    reply.setResult(ret);
                }
                reply.finish();
            };

        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);


        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
        GVariant *dict = g_variant_builder_end(&builder);

        asyncMethodCall("ReadValue", variantReply, g_variant_new_tuple(&dict, 1));
    }

    inline void WriteValue(const std::vector<uint8_t> &value, PendingReply<> &&reply)
    {
        auto convertVariant = [reply](PendingReply<DBusVariant> *dbusReply) mutable
            {
                reply.setName(dbusReply->getName());
                if (dbusReply->isError()) {
                    reply.setError(dbusReply->errorMessage());
                }
                reply.finish();
            };


        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("(aya{sv})"));
        g_variant_builder_open(&builder, G_VARIANT_TYPE("ay"));
        for (const auto v : value) {
            g_variant_builder_add(&builder, "y", v);
        }
        g_variant_builder_close(&builder);
        g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&builder, "{sv}", "type", g_variant_new_string("request"));
        g_variant_builder_close(&builder);


        std::shared_ptr<bool> valid = std::make_shared<bool>(true);
        PendingReply<DBusVariant> variantReply(valid, convertVariant);

        asyncMethodCall("WriteValue", variantReply, g_variant_builder_end(&builder));
    }
};

#endif // !defined(BLUEZGATTDESCRIPTORINTERFACE_H)
