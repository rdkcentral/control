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
//  dbusobjectmanager.cpp
//  BleRcuDaemon
//

#include "dbusobjectmanager.h"
#include "dbusutils.h"

#include "ctrlm_log_ble.h"

using namespace std;

static void signalHandler (GDBusProxy *proxy,
                             gchar      *sender_name,
                             gchar      *signal_name,
                             GVariant   *parameters,
                             gpointer    user_data);

static void parseInterfaceList(GVariant *variant, DBusInterfaceList &interfaceList);
static void parseManagedObjects(GVariant *variant, DBusManagedObjectList &managedObjects);

// -----------------------------------------------------------------------------
/**
 *  @brief
 *
 *
 *
 */
DBusObjectManagerInterface::DBusObjectManagerInterface(const string &service,
                                                       const string &path,
                                                       const GDBusConnection *connection)
    : GDBusAbstractInterface(service, path, staticInterfaceName(), connection)
{
    m_signalHandlerID = connectSignal("g-signal", G_CALLBACK(signalHandler), this);
}

DBusObjectManagerInterface::~DBusObjectManagerInterface()
{
    disconnectSignal(m_signalHandlerID);
}

bool DBusObjectManagerInterface::isValid() const
{
    return GDBusAbstractInterface::isValid();
}



static void signalHandler (GDBusProxy *proxy,
                             gchar      *sender_name,
                             gchar      *signal_name,
                             GVariant   *parameters,
                             gpointer    user_data)
{
    // XLOGD_DEBUG ("Enter, signal: %s", signal_name);
    DBusObjectManagerInterface *ifce = (DBusObjectManagerInterface*)user_data;
    if (!ifce) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }

    gchar *objPath;

    if (0 == g_strcmp0(signal_name, "InterfacesAdded")) {
        DBusInterfaceList interfaceList;
        GVariant *dict;
        g_variant_get(parameters, "(o@a{sa{sv}})", &objPath, &dict);
        // XLOGD_DEBUG("InterfacesAdded objPath = <%s>", objPath);

        parseInterfaceList(dict, interfaceList);

        ifce->m_interfacesAddedSlots.invoke(objPath, interfaceList);

        g_variant_unref(dict);
        g_free(objPath);

    } else if (0 == g_strcmp0(signal_name, "InterfacesRemoved")) {

        vector<string> interfaceList;
        GVariant *ifce_array;
        GVariantIter ifce_iter;
        gchar *interfaceName;

        g_variant_get(parameters, "(o@as)", &objPath, &ifce_array);
        // XLOGD_DEBUG("InterfacesRemoved objPath = <%s>", objPath);

        g_variant_iter_init (&ifce_iter, ifce_array);
        while (g_variant_iter_next (&ifce_iter, "s", &interfaceName)) {
            interfaceList.push_back(string(interfaceName));
            g_free(interfaceName);
        }

        ifce->m_interfacesRemovedSlots.invoke(objPath, interfaceList);

        g_variant_unref(ifce_array);
        g_free(objPath);
    } else {
        XLOGD_DEBUG("Unhandled signal received <%s> from interface <%s>", 
                signal_name, g_dbus_proxy_get_interface_name(proxy));
    }
}

static void asyncMethodCB (GDBusProxy *proxy,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    GVariant *reply;

    PendingReply<DBusManagedObjectList> *callData = (PendingReply<DBusManagedObjectList> *)user_data;
    if (!callData) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }

    reply = g_dbus_proxy_call_finish(proxy, res, &error);
    if (NULL == reply) {
        // will return NULL if there's an error reported
        XLOGD_ERROR("method <%s> failed with error = <%s>", callData->getName().c_str(), (error == NULL) ? "" : error->message);
        if (error) {
            callData->setError(error->message);
            g_clear_error(&error);
        }
    } else {
        DBusManagedObjectList managedObjects;
        parseManagedObjects(reply, managedObjects);
        g_variant_unref(reply);

        callData->setResult(std::move(managedObjects));
    }

    callData->finish();
    delete callData;
}


void DBusObjectManagerInterface::GetManagedObjects(const PendingReply<DBusManagedObjectList> &reply)    //async
{
    string method = "GetManagedObjects";
    PendingReply<DBusManagedObjectList> *userData = new PendingReply<DBusManagedObjectList>(reply);
    userData->setName(method);
    if (false == asyncCall(method, NULL, (GAsyncReadyCallback)asyncMethodCB, userData)) {
        userData->setError("failed to send async call");
        userData->finish();
        delete userData;
    }
}

bool DBusObjectManagerInterface::GetManagedObjects(DBusManagedObjectList &managedObjects)   //sync
{
    string error;
    
    GVariant *reply = syncCall("GetManagedObjects", g_variant_new("()"), error);
    if (NULL != reply) {
        parseManagedObjects(reply, managedObjects);
        g_variant_unref(reply);
        return true;
    } else {
        XLOGD_ERROR("Failed to get managed objects!! error = <%s>", error.c_str());
        return false;
    }
}

static void parseInterfaceList(GVariant *variant, DBusInterfaceList &interfaceList)
{
    GVariantIter obj_dict_entry_iter;
    GVariant *interface_dict_entry;
    gchar *interface_name;

    g_variant_iter_init (&obj_dict_entry_iter, variant);
    while (g_variant_iter_next (&obj_dict_entry_iter, "{s@a{sv}}", &interface_name, &interface_dict_entry)) {
        // XLOGD_DEBUG("-----------------------------------------------------------------------------------------");
        // XLOGD_DEBUG("interface_name = <%s>", interface_name);

        // gchar *result_str = g_variant_print(interface_dict_entry, false);
        // XLOGD_INFO("interface_dict_entry =  <%s>\n", result_str);
        // g_free(result_str);

        DBusPropertiesMap propertiesList;
        parsePropertiesList(interface_dict_entry, propertiesList);
        
        interfaceList[string(interface_name)] = std::move(propertiesList);
        g_free(interface_name);
        g_variant_unref(interface_dict_entry);
    }
}

static void parseManagedObjects(GVariant *variant, DBusManagedObjectList &managedObjects)
{
    GVariantIter obj_iter;
    GVariant *root, *obj_dict_entry;
    gchar *objPath;

    root = g_variant_get_child_value(variant, 0);
    g_variant_iter_init(&obj_iter, root);
    while (g_variant_iter_next (&obj_iter, "{o@a{sa{sv}}}", &objPath, &obj_dict_entry)) {
        // XLOGD_DEBUG("=================================================================================================================================");
        // XLOGD_DEBUG("objPath = <%s>", objPath);

        // gchar *result_str = g_variant_print(obj_dict_entry, false);
        // XLOGD_INFO("obj_dict_entry =  <%s>\n", result_str);
        // g_free(result_str);

        DBusInterfaceList interfaceList;
        parseInterfaceList(obj_dict_entry, interfaceList);

        managedObjects[string(objPath)] = std::move(interfaceList);
        g_free(objPath);
        g_variant_unref(obj_dict_entry);
    }
}
