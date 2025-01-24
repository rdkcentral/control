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
//  DBusServiceWatcher.cpp
//

#include "dbusservicewatcher.h"
#include "ctrlm_log_ble.h"

using namespace std;


static void nameAppearedCallback (GDBusConnection *connection,
                             const gchar *name,
                             const gchar *name_owner,
                             gpointer user_data);

static void nameVanishedCallback (GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data);

// -----------------------------------------------------------------------------
/*!
    \class DBusServiceWatcher
    \brief 

 */

DBusServiceWatcher::DBusServiceWatcher(const std::string &service,
                                        const GDBusConnection *connection)
{
    m_serviceWatcherHandlerID = g_bus_watch_name_on_connection (const_cast<GDBusConnection*>(connection),
                                                                service.c_str(),
                                                                G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                                (GBusNameAppearedCallback) nameAppearedCallback,
                                                                (GBusNameVanishedCallback) nameVanishedCallback,
                                                                this,
                                                                NULL);
}
DBusServiceWatcher::~DBusServiceWatcher()
{
    g_bus_unwatch_name(m_serviceWatcherHandlerID);
}


static void nameAppearedCallback (GDBusConnection *connection,
                                const gchar *name,
                                const gchar *name_owner,
                                gpointer user_data)
{
    DBusServiceWatcher *ifce = (DBusServiceWatcher*)user_data;
    if (!ifce) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }

    if (ifce->m_serviceRegisteredCB) {
        ifce->m_serviceRegisteredCB(name);
    }
}

static void nameVanishedCallback (GDBusConnection *connection,
                                const gchar *name,
                                gpointer user_data)
{
    DBusServiceWatcher *ifce = (DBusServiceWatcher*)user_data;
    if (!ifce) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }

    if (ifce->m_serviceUnRegisteredCB) {
        ifce->m_serviceUnRegisteredCB(name);
    }
}



void DBusServiceWatcher::addServiceRegisteredHandler(std::function<void(const std::string&)> func)
{
    m_serviceRegisteredCB = std::move(func);
}
void DBusServiceWatcher::addServiceUnRegisteredHandler(std::function<void(const std::string&)> func)
{
    m_serviceUnRegisteredCB = std::move(func);
}
