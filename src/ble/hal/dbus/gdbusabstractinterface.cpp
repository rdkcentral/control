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
//  GDBusAbstractInterface.cpp
//  BleRcuDaemon
//

#include "gdbusabstractinterface.h"
#include "ctrlm_log_ble.h"

using namespace std;

const string GDBusAbstractInterface::m_dbusPropertiesInterface("org.freedesktop.DBus.Properties");

GDBusAbstractInterface::GDBusAbstractInterface(const std::string &service, 
                                               const std::string &path, 
                                               const std::string &interface,
                                               const GDBusConnection *connection)
    : m_service(service)
    , m_path(path)
    , m_interface(interface)
    , m_timeout(-1)
    , m_connection(connection)
{
    // Should these be async operations?
    // During a dirty shutdown I've seen it can get stuck here for 30 seconds,
    // but that may have been because I had BleRcuDaemon running at the same time as this library... 
    // monitor this...
    
    GError *error = NULL;
    m_proxy = g_dbus_proxy_new_sync(const_cast<GDBusConnection*>(m_connection), 
                                    G_DBUS_PROXY_FLAGS_NONE, 
                                    NULL, 
                                    m_service.c_str(), 
                                    m_path.c_str(), 
                                    m_interface.c_str(), 
                                    NULL, 
                                    &error);
    if (NULL == m_proxy) {
        XLOGD_ERROR("Failed to acquire gdbus_proxy for interface <%s> at path <%s>, error = <%s>", 
                    m_interface.c_str(), m_path.c_str(), (error == NULL) ? "" : error->message);
        g_clear_error (&error);
    }

    m_propertiesProxy = g_dbus_proxy_new_sync(const_cast<GDBusConnection*>(m_connection), 
                                    G_DBUS_PROXY_FLAGS_NONE, 
                                    NULL, 
                                    m_service.c_str(), 
                                    m_path.c_str(), 
                                    m_dbusPropertiesInterface.c_str(), 
                                    NULL, 
                                    &error);
    if (NULL == m_propertiesProxy) {
        XLOGD_ERROR("Failed to acquire gdbus_proxy for interface <%s> at path <%s>, error = <%s>", 
                    m_dbusPropertiesInterface.c_str(), m_path.c_str(), (error == NULL) ? "" : error->message);
        g_clear_error (&error);
    }                     
}
GDBusAbstractInterface::~GDBusAbstractInterface()
{
    if (m_proxy != NULL) { g_object_unref(m_proxy); }
    if (m_propertiesProxy != NULL) { g_object_unref(m_propertiesProxy); }
}

bool GDBusAbstractInterface::isValid() const
{
    return (m_connection != NULL) &&
           (m_proxy != NULL) &&
           (m_propertiesProxy != NULL);
}

std::string GDBusAbstractInterface::service() const
{
    return m_service;
}
std::string GDBusAbstractInterface::path() const
{
    return m_path;
}
std::string GDBusAbstractInterface::interface() const
{
    return m_interface;
}
int GDBusAbstractInterface::timeout() const
{
    return m_timeout;
}

void GDBusAbstractInterface::setTimeout(int timeout)
{
    m_timeout = timeout;
}

GVariant* GDBusAbstractInterface::syncCall(const std::string &method,
                                           GVariant *args,
                                           std::string &errorMessage) const
{
    GVariant *reply = NULL;
    if (m_proxy == NULL) {

        XLOGD_ERROR("proxy for interface <%s> at path <%s> is NULL", 
                    m_interface.c_str(), m_path.c_str());
        errorMessage = "dbus proxy for interface is NULL";
        // g_dbus_proxy_call consumes the floating GVariant reference, but if that's not
        // getting called then we need to manually unref here.
        g_variant_unref(args);

    } else {
        GError *error = NULL;
        reply = g_dbus_proxy_call_with_unix_fd_list_sync (m_proxy, 
                                                           method.c_str(), 
                                                           args, 
                                                           G_DBUS_CALL_FLAGS_NONE, 
                                                           m_timeout, 
                                                           NULL, 
                                                           NULL, 
                                                           NULL, 
                                                           &error);
        if (NULL == reply) {
            // g_dbus_proxy_call_sync will return NULL if there's an error reported
            XLOGD_DEBUG("DBus call <%s> on interface <%s> at path <%s> failed!! error = <%s>", 
                        method.c_str(), g_dbus_proxy_get_interface_name(m_proxy), m_path.c_str(), 
                        (error == NULL) ? "" : error->message);
            errorMessage = error->message;
            g_clear_error (&error);
        } else {
            XLOGD_DEBUG("DBus call <%s> on interface <%s> at path <%s> succeeded.", 
                    method.c_str(), g_dbus_proxy_get_interface_name(m_proxy), m_path.c_str());
        }
    }
    return reply;
}

GVariant* GDBusAbstractInterface::syncPropertiesCall(const std::string &method,
                                                    GVariant *args,
                                                    std::string &errorMessage) const
{
    GVariant *reply = NULL;

    if (m_propertiesProxy == NULL) {

        XLOGD_ERROR("proxy for interface <%s> at path <%s> is NULL", 
                    m_dbusPropertiesInterface.c_str(), m_path.c_str());

        // g_dbus_proxy_call consumes the floating GVariant reference, but if that's not
        // getting called then we need to manually unref here.
        g_variant_unref(args);

    } else {
        GError *error = NULL;

        reply = g_dbus_proxy_call_sync (m_propertiesProxy,
                                        method.c_str(),
                                        args,
                                        G_DBUS_CALL_FLAGS_NONE, 
                                        m_timeout,
                                        NULL, 
                                        &error);
        if (NULL == reply) {
            // g_dbus_proxy_call_sync will return NULL if there's an error reported
            // XLOGD_ERROR("DBus call <%s> on interface <%s> at path <%s> failed!! error = <%s>", 
            //             method.c_str(), g_dbus_proxy_get_interface_name(m_propertiesProxy), m_path.c_str(), 
            //             (error == NULL) ? "" : error->message);
            errorMessage = error->message;
            g_clear_error (&error);
        } else {
            // XLOGD_DEBUG("DBus call <%s> on interface <%s> at path <%s> succeeded.", 
            //         method.c_str(), g_dbus_proxy_get_interface_name(m_propertiesProxy), m_path.c_str());
        }
    }
    return reply;
}

bool GDBusAbstractInterface::asyncCall(const std::string &method,
                                       GVariant *args,
                                       GAsyncReadyCallback callback,
                                       gpointer user_data) const
{
    if (m_proxy == NULL) {
        
        XLOGD_ERROR("proxy for interface <%s> at path <%s> is NULL", 
                    m_interface.c_str(), m_path.c_str());
        // g_dbus_proxy_call consumes the floating GVariant reference, but if that's not
        // getting called then we need to manually unref here.
        g_variant_unref(args);
        return false;

    } else {
        g_dbus_proxy_call_with_unix_fd_list (m_proxy,
                                             method.c_str(), 
                                             args, 
                                             G_DBUS_CALL_FLAGS_NONE, 
                                             -1, 
                                             NULL, 
                                             NULL, 
                                             callback,
                                             user_data);
    }
    return true;
}

bool GDBusAbstractInterface::asyncPropertiesCall(const std::string &method,
                                                GVariant *args,
                                                GAsyncReadyCallback callback,
                                                gpointer user_data) const
{
    if (m_propertiesProxy == NULL) {

        XLOGD_ERROR("proxy for interface <%s> at path <%s> is NULL", 
                    m_dbusPropertiesInterface.c_str(), m_path.c_str());

        // g_dbus_proxy_call consumes the floating GVariant reference, but if that's not
        // getting called then we need to manually unref here.
        g_variant_unref(args);

        return false;

    } else {
        g_dbus_proxy_call (m_propertiesProxy,
                            method.c_str(),
                            args,
                            G_DBUS_CALL_FLAGS_NONE, 
                            -1,
                            NULL, 
                            callback,
                            user_data);
    }
    return true;
}

gulong GDBusAbstractInterface::connectSignal(const std::string &signal,
                                            GCallback callback, 
                                            gpointer user_data) const
{
    gulong id = 0;
    if (m_proxy) {
        id = g_signal_connect(m_proxy, signal.c_str(), G_CALLBACK(callback), user_data);
        if (id == 0) {
            XLOGD_ERROR("Failed to connect to signal <%s> on interface <%s> at path <%s>", 
                        "g-signal", m_interface.c_str(), m_path.c_str());
        }
    }
    return id;
}

void GDBusAbstractInterface::disconnectSignal(gulong &handlerID) const
{
    if (m_proxy) {
        g_clear_signal_handler (&handlerID, m_proxy);
    }
}
