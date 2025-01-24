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
//  DBusAbstractInterface.cpp
//  BleRcuDaemon
//

#include "dbusabstractinterface.h"
#include "dbusutils.h"

#include "ctrlm_log_ble.h"

using namespace std;



static void onPropertiesChanged (GDBusProxy *proxy,
                                GVariant   *changed_properties,
                                GStrv       invalidated_properties,
                                gpointer    user_data);

static void signalHandler (GDBusProxy *proxy,
                             gchar      *sender_name,
                             gchar      *signal_name,
                             GVariant   *parameters,
                             gpointer    user_data);

// -----------------------------------------------------------------------------
/*!
    \class DBusAbstractInterface
    \brief Wrapper around the QDBusAbstractInterface class to provide notify
    signals for property changes.

    The dbus specification defines the org.freedesktop.DBus.Properties interface
    for getting / setting properties, Qt already implements this, however it
    doesn't implement a handler for the org.freedesktop.DBus.Properties.PropertiesChanged
    signal.

    This is a problem for us as bluetoothd uses this to notify us of all sorts
    of things; i.e. scan on/off, powered on/off, etc

    \see https://randomguy3.wordpress.com/2010/09/07/the-magic-of-qtdbus-and-the-propertychanged-signal/
    \see https://github.com/nemomobile/qtdbusextended


    So how do you use this class ?
       1. Generate an interface class using the Qt qdbusxml2cpp tool
       2. Change the generated class so that it inherits from the
          \a DBusAbstractInterface class rather than \a QDBusAbstractInterface
       3. Add the \c NOTIFY option to the properties of the generated class and
          matching signals (just like you would for an ordinary notify property)

 */

DBusAbstractInterface::DBusAbstractInterface(const string &service,
                                             const string &path,
                                             const string &interface,
                                             const GDBusConnection *connection)
    : GDBusAbstractInterface(service, path, interface, connection)
{
    m_signalHandlerID = connectSignal("g-signal", G_CALLBACK(signalHandler), this);
    m_propertiesChangedHandlerID = connectSignal("g-properties-changed", G_CALLBACK(onPropertiesChanged), this);
}

DBusAbstractInterface::~DBusAbstractInterface()
{
    if (m_signalHandlerID > 0) { disconnectSignal(m_signalHandlerID); }
    if (m_propertiesChangedHandlerID > 0) { disconnectSignal(m_propertiesChangedHandlerID); }
}

DBusPropertiesMap DBusAbstractInterface::getAllProperties()
{
    DBusPropertiesMap propertiesList;

    string error;

    XLOGD_DEBUG("calling org.freedesktop.DBus.Properties.GetAll on interface <%s>", this->interface().c_str());
    GVariant *reply = syncPropertiesCall("GetAll", g_variant_new("(s)", this->interface().c_str()), error);
    
    
    if (NULL != reply) {

        gchar *result_str;
        result_str = g_variant_print(reply, false);
        XLOGD_DEBUG("reply =  <%s>", result_str);
        g_free(result_str);


        GVariant *dict;
        dict = g_variant_get_child_value(reply, 0);
        parsePropertiesList(dict, propertiesList);
        g_variant_unref(reply);
    } else {
        XLOGD_ERROR("Failed to get all properties at path <%s>, error = <%s>", path().c_str(), error.c_str());
    }

    return propertiesList;
}

DBusVariant DBusAbstractInterface::getProperty(std::string name) const
{
    string error;

    GVariant *reply = syncPropertiesCall("Get", g_variant_new("(ss)", interface().c_str(), name.c_str()), error);
    
    // gchar *result_str;
    // result_str = g_variant_print(reply, false);
    // XLOGD_DEBUG("reply =  <%s>", result_str);
    // g_free(result_str);
    
    if (NULL != reply) {
        GVariant  *v = NULL;
        g_variant_get (reply, "(v)", &v);
        g_variant_unref(reply);
        return DBusVariant(std::move(name), v);
    } else {
        XLOGD_ERROR("Failed to get property (%s) at path <%s>, error = <%s>", name.c_str(), path().c_str(), error.c_str());
    }

    return DBusVariant();
}

/**
 * this function will handle freeing GVariant *prop
*/
bool DBusAbstractInterface::setProperty(std::string name, GVariant *prop) const
{
    string error;
    GVariant *reply = syncPropertiesCall("Set", g_variant_new("(ssv)", interface().c_str(), name.c_str(), prop), error);
    
    if (NULL == reply) {
        XLOGD_ERROR("Failed to set property (%s) at path <%s>, error = <%s>", name.c_str(), path().c_str(), error.c_str());
        return false;
    }

    return true;
}


static void signalHandler (GDBusProxy *proxy,
                             gchar      *sender_name,
                             gchar      *signal_name,
                             GVariant   *parameters,
                             gpointer    user_data)
{
    XLOGD_DEBUG ("Enter, sender_name = %s, signal: %s", sender_name, signal_name);


    gchar *result_str;
    result_str = g_variant_print(parameters, false);
    XLOGD_INFO("parameters =  <%s>", result_str);
    g_free(result_str);

    // if (0 == g_strcmp0(signal_name, "InterfacesAdded")) {
    //     DBusInterfaceList interfaceList;
    //     GVariant *dict;
    //     gchar *objPath;
    //     g_variant_get(parameters, "(o@a{sa{sv}})", &objPath, &dict);
    //     XLOGD_DEBUG("objPath = <%s>", objPath);

    //     parseInterfaceList(dict, interfaceList);

    //     DBusObjectManagerInterface *ifce = (DBusObjectManagerInterface*)user_data;
    //     if (ifce) {
    //         if (ifce->m_interfacesAddedSlots) {
    //             ifce->m_interfacesAddedSlots(objPath, interfaceList);
    //         }
    //     }
    //     g_variant_unref(dict);
    //     g_free(objPath);

    // } else {
    //     XLOGD_DEBUG("Unhandled signal received <%s> from interface <%s>", 
    //             signal_name, g_dbus_proxy_get_interface_name(proxy));
    // }
}

// -----------------------------------------------------------------------------
/*!
    dbus callback called when PropertiesChanged signal is received

    Handles the \c org.freedesktop.DBus.Properties.PropertiesChanged signal.

    This will go through the list of properties that have changed and searches
    to see if this object has a \c Q_PROPERTY that matches, if it does and that
    property has a \c NOTIFY signal we call the signal.

    The notify signal may either have no args or a single arg that matches
    the property type.  This code will handle both types.

    Note more than one property change could be signaled by this method

 */

static void onPropertiesChanged (GDBusProxy *proxy,
                                GVariant   *changed_properties,
                                GStrv       invalidated_properties,
                                gpointer    user_data)
{
    DBusAbstractInterface *ifce = (DBusAbstractInterface*)user_data;
    if (!ifce) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }


    gchar *result_str;
    result_str = g_variant_print(changed_properties, false);
    XLOGD_DEBUG("changed_properties =  <%s> (%s)", result_str, ifce->path().c_str());
    g_free(result_str);
    
    // sanity check the interface is correct
    if (string(g_dbus_proxy_get_interface_name(proxy)) != ifce->interface()) {
        XLOGD_WARN("odd, received PropertiesChanged signal from wrong interface");
    } else {
        // XLOGD_ERROR("its type <%s>", g_variant_get_type_string(changed_properties));

        DBusPropertiesMap changedProperties;
        parsePropertiesList(changed_properties, changedProperties);

        // iterate through the changed properties
        DBusPropertiesMap::const_iterator it = changedProperties.begin();
        for (; it != changedProperties.end(); ++it) {

            const string &propName = it->first;
            const DBusVariant &propValue = it->second;

            ifce->m_propertyChangedSlotsLock.lock();

            PropertyChangedSlotsMap::const_iterator callback = ifce->m_propertyChangedSlots.find(propName);
            if ((callback == ifce->m_propertyChangedSlots.end())) {
                XLOGD_DEBUG("No property changed callback for %s.%s", ifce->interface().c_str(), propName.c_str());
                ifce->m_propertyChangedSlotsLock.unlock();
                continue;
            }
            auto slots = callback->second;
            ifce->m_propertyChangedSlotsLock.unlock();

            // invoke the callbacks
            slots.invoke(propValue);
        }

        // TODO: handle invalidatedProperties as well??
        if (g_strv_length(invalidated_properties) > 0) {
            XLOGD_WARN("Properties Invalidated:");
            for (guint n = 0; invalidated_properties[n] != NULL; n++) {
                XLOGD_WARN("      <%s>", invalidated_properties[n]);
            }
        }
    }
}


static void asyncMethodCB (GDBusProxy *proxy,
                           GAsyncResult *res,
                           gpointer user_data)
{
    GError *error = NULL;
    GVariant *reply;

    PendingReply<DBusVariant> *callData = (PendingReply<DBusVariant> *)user_data;
    if (!callData) {
        XLOGD_ERROR("user_data is NULL");
        return;
    }

    GUnixFDList *out_fd_list = NULL;

    reply = g_dbus_proxy_call_with_unix_fd_list_finish(proxy, &out_fd_list, res, &error);

    if (NULL == reply) {
        // will return NULL if there's an error reported
        callData->setError(error->message);
        g_clear_error(&error);
    } else {
        // no need to unref reply here, DBusVariant will handle that
        DBusVariant variant(callData->getName(), reply, out_fd_list);
        callData->setResult(std::move(variant));
    }

    callData->finish();
    delete callData;
}

// -----------------------------------------------------------------------------
/*!
    Performs a org.freedesktop.DBus.Properties.Set method call requesting
    a set on the given property, however it returns the pending reply rather
    than blocking.

 */
void DBusAbstractInterface::asyncSetProperty(const std::string method, const GVariant *prop, const PendingReply<DBusVariant> &reply)
{
    string name = path() + " (SET " + method + ")";
    XLOGD_DEBUG("calling async method <%s>", name.c_str());

    PendingReply<DBusVariant> *userData = new PendingReply<DBusVariant>(reply);
    userData->setName(name);
    if (false == asyncPropertiesCall("Set", 
                                    g_variant_new("(ssv)", interface().c_str(), method.c_str(), prop),
                                    (GAsyncReadyCallback)asyncMethodCB, 
                                    userData))
    {
        userData->setError("failed to send async call");
        userData->finish();
        delete userData;
    }
}

// -----------------------------------------------------------------------------
/*!
    Performs a org.freedesktop.DBus.Properties.Set method call requesting
    a get on the given property, however it returns the pending reply rather
    than blocking.

 */
void DBusAbstractInterface::asyncGetProperty(const std::string method, const PendingReply<DBusVariant> &reply)
{
    string name = path() + " (GET " + method + ")";
    XLOGD_DEBUG("calling async method <%s>", name.c_str());

    PendingReply<DBusVariant> *userData = new PendingReply<DBusVariant>(reply);
    userData->setName(name);
    if (false == asyncPropertiesCall("Get", 
                                    g_variant_new("(ss)", interface().c_str(), method.c_str()),
                                    (GAsyncReadyCallback)asyncMethodCB, 
                                    userData))
    {
        userData->setError("failed to send async call");
        userData->finish();
        delete userData;
    }
}

// -----------------------------------------------------------------------------
/*!
    Performs an asynchronous method call on the interface of this object

 */
void DBusAbstractInterface::asyncMethodCall(const std::string method, const PendingReply<DBusVariant> &reply, GVariant *args)
{
    string name = path() + " (" + method + ")";

    PendingReply<DBusVariant> *userData = new PendingReply<DBusVariant>(reply);
    userData->setName(name);
    if (false == asyncCall(method, args, (GAsyncReadyCallback)asyncMethodCB, userData)) {
        userData->setError("failed to send async call");
        userData->finish();
        delete userData;
    }
}

void DBusAbstractInterface::addPropertyChangedSlot(std::string propertyName, const Slot<const DBusVariant&> &func)
{
    // take the lock before accessing the callbacks map
    std::lock_guard<std::mutex> lock(m_propertyChangedSlotsLock);
    m_propertyChangedSlots[propertyName].addSlot(func);
}

void DBusAbstractInterface::addPropertyChangedSlot(std::string propertyName, const Slot<bool> &func)
{
    // lamda called to convert the dbus variant to the requested type
    auto convertVariant = [func](const DBusVariant &variant)
        {
            bool value;
            if (!variant.toBool(value)) {
                XLOGD_WARN("property variant <%s> cannot convert to boolean", variant.dumpToString().c_str());
            } else {
                func.invokeCallback(value);
            }
        };

    std::shared_ptr<bool> valid = std::make_shared<bool>(true);
    addPropertyChangedSlot(std::move(propertyName), Slot<const DBusVariant&>(valid, convertVariant));
}

void DBusAbstractInterface::addPropertyChangedSlot(std::string propertyName, const Slot<const std::string&> &func)
{
    // lamda called to convert the dbus variant to the requested type
    auto convertVariant = [func](const DBusVariant &variant)
        {
            string value;
            if (!variant.toString(value)) {
                XLOGD_WARN("property variant <%s> cannot convert to string", variant.dumpToString().c_str());
            } else {
                func.invokeCallback(value);
            }
        };

    std::shared_ptr<bool> valid = std::make_shared<bool>(true);
    addPropertyChangedSlot(std::move(propertyName), Slot<const DBusVariant&>(valid, convertVariant));
}

void DBusAbstractInterface::addPropertyChangedSlot(std::string propertyName, const Slot<const std::vector<uint8_t>&> &func)
{
    // lamda called to convert the dbus variant to the requested type
    auto convertVariant = [func](const DBusVariant &variant)
        {
            std::vector<uint8_t> value;
            if (!variant.toByteArray(value)) {
                XLOGD_WARN("property variant <%s> cannot convert to byte array", variant.dumpToString().c_str());
            } else {
                func.invokeCallback(value);
            }
        };

    std::shared_ptr<bool> valid = std::make_shared<bool>(true);
    addPropertyChangedSlot(std::move(propertyName), Slot<const DBusVariant&>(valid, convertVariant));
}
