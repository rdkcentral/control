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
//  gdbusabstractinterface.h
//  BleRcuDaemon
//

#ifndef GDBUSABSTRACTINTERFACE_H
#define GDBUSABSTRACTINTERFACE_H

#include <gio/gio.h>
#include <string>


class GDBusAbstractInterface
{
public:
    GDBusAbstractInterface(const std::string &service, 
                           const std::string &path, 
                           const std::string &interface,
                           const GDBusConnection *connection);
    ~GDBusAbstractInterface();

    static const std::string m_dbusPropertiesInterface;

    bool isValid() const;

    std::string service() const;
    std::string path() const;
    std::string interface() const;
    int timeout() const;

    void setTimeout(int timeout);

    GVariant* syncCall(const std::string &method,
                        GVariant *args,
                        std::string &errorMessage) const;

    GVariant* syncPropertiesCall(const std::string &method,
                                 GVariant *args,
                                 std::string &errorMessage) const;

    bool asyncCall(const std::string &method,
                    GVariant *args,
                    GAsyncReadyCallback callback,
                    gpointer user_data) const;

    bool asyncPropertiesCall(const std::string &method,
                            GVariant *args,
                            GAsyncReadyCallback callback,
                            gpointer user_data) const;

protected:
    gulong connectSignal(const std::string &signal, GCallback callback, gpointer user_data) const;
    void disconnectSignal(gulong &handlerID) const;

private:
    std::string m_service;
    std::string m_path;
    std::string m_interface;
    int m_timeout;
    const GDBusConnection *m_connection;
    GDBusProxy *m_proxy;
    GDBusProxy *m_propertiesProxy;
};
#endif // GDBUSABSTRACTINTERFACE_H
