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
//  dbusvariant.cpp
//  BleRcuDaemon
//


#include "dbusvariant.h"
#include "ctrlm_log_ble.h"

using namespace std;

DBusVariant::DBusVariant()
    : m_name("")
    , m_gVariant(NULL)
    , m_gFdList(NULL)
{
}
DBusVariant::DBusVariant(std::string name, GVariant *variant) 
    : m_name(std::move(name))
    , m_gVariant(variant)
    , m_gFdList(NULL)
{
}
DBusVariant::DBusVariant(std::string name, GVariant *variant, GUnixFDList *fdList) 
    : m_name(std::move(name))
    , m_gVariant(variant)
    , m_gFdList(fdList)
{
}
DBusVariant::~DBusVariant()
{
    if (m_gVariant) { g_variant_unref(m_gVariant); }
    if (m_gFdList) { g_object_unref(m_gFdList); }
}


DBusVariant::DBusVariant(const DBusVariant &other)
{
    m_name = other.m_name;
    m_gVariant = other.getGVariant() ? g_variant_ref(other.getGVariant()) : NULL;
    m_gFdList = other.getGFdList() ? (GUnixFDList*)g_object_ref(other.getGFdList()) : NULL;
    // XLOGD_DEBUG("DBusVariant(const DBusVariant &other) - g_variant_ref <%s>", dumpToString().c_str());
}

DBusVariant::DBusVariant(DBusVariant &&other)
{
    m_name = other.m_name;
    m_gVariant = other.getGVariant() ? g_variant_ref(other.getGVariant()) : NULL;
    m_gFdList = other.getGFdList() ? (GUnixFDList*)g_object_ref(other.getGFdList()) : NULL;
    // XLOGD_DEBUG("DBusVariant(const DBusVariant &&other) - g_variant_ref <%s>", dumpToString().c_str());
}

DBusVariant &DBusVariant::operator=(const DBusVariant &other)
{
    m_name = other.m_name;
    m_gVariant = other.getGVariant() ? g_variant_ref(other.getGVariant()) : NULL;
    m_gFdList = other.getGFdList() ? (GUnixFDList*)g_object_ref(other.getGFdList()) : NULL;
    // XLOGD_DEBUG("DBusVariant &operator=(const DBusVariant &other) - g_variant_ref <%s>", dumpToString().c_str());
    return *this;
}

DBusVariant &DBusVariant::operator=(DBusVariant &&other)
{
    m_name = other.m_name;
    m_gVariant = other.getGVariant() ? g_variant_ref(other.getGVariant()) : NULL;
    m_gFdList = other.getGFdList() ? (GUnixFDList*)g_object_ref(other.getGFdList()) : NULL;
    // XLOGD_DEBUG("DBusVariant &operator=(DBusVariant &&other) - g_variant_ref <%s>", dumpToString().c_str());
    return *this;
}


GVariant* DBusVariant::getGVariant() const 
{
    return m_gVariant;
}

GUnixFDList* DBusVariant::getGFdList() const 
{
    return m_gFdList;
}

std::string DBusVariant::dumpToString() const 
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return string();
    }
    char buf[100];
    gchar *value = g_variant_print(m_gVariant, false);
    snprintf(buf, sizeof(buf), "name <%s>, value <%s>", m_name.c_str(), value);
    g_free(value);

    return string(buf);
}

bool DBusVariant::canConvert(const GVariantType *t) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    // XLOGD_DEBUG("Variant (%s) is of type <%s>", 
            // dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
    return g_variant_is_of_type(m_gVariant, t);
}

bool DBusVariant::toString(std::string &ret) const 
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_STRING)) {
        gchar *str_variant = NULL;
        g_variant_get (m_gVariant, "s", &str_variant);
        ret = str_variant;
        g_free(str_variant);

    } else {
        XLOGD_ERROR("Variant <%s> is not of type STRING, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}
bool DBusVariant::toStringList(std::vector<std::string> &ret) const 
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_STRING_ARRAY)) {
        gchar *str_variant = NULL;
        GVariantIter obj_iter;
        g_variant_iter_init(&obj_iter, m_gVariant);
        while (g_variant_iter_next (&obj_iter, "s", &str_variant)) {
            ret.push_back(str_variant);
            g_free(str_variant);
        }
    } else {
        XLOGD_ERROR("Variant <%s> is not of type STRING ARRAY, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toByteArray(std::vector<uint8_t> &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    GVariantIter *iter = NULL;
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_TUPLE)) {
        g_variant_get (m_gVariant, "(ay)", &iter);
    } else if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_BYTESTRING)) {
        g_variant_get (m_gVariant, "ay", &iter);
    }
    
    if (iter != NULL) {
        guchar char_variant;
        while (g_variant_iter_loop (iter, "y", &char_variant)) {
            ret.push_back(char_variant);
        }
        g_variant_iter_free (iter);
    } else {
        XLOGD_ERROR("Variant <%s> is not of type BYTESTRING, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toObjectPath(std::string &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_OBJECT_PATH)) {
        gchar *str_variant = NULL;
        g_variant_get (m_gVariant, "o", &str_variant);
        ret = str_variant;
        g_free(str_variant);
    } else {
        XLOGD_ERROR("Variant <%s> is not of type OBJECT PATH, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toBool(bool &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_BOOLEAN)) {
        gboolean bool_variant = false;
        g_variant_get (m_gVariant, "b", &bool_variant);
        ret = bool_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type BOOL, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toChar(uint8_t &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_BYTE)) {
        guchar char_variant = '\0';
        g_variant_get(m_gVariant, "y", &char_variant);
        ret = char_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type CHAR, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toInt(int &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_INT32)) {
        gint32 int_variant = 0;
        g_variant_get(m_gVariant, "i", &int_variant);
        ret = int_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type INT32, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toInt16(int16_t &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_INT16)) {
        gint16 int_variant = 0;
        g_variant_get(m_gVariant, "n", &int_variant);
        ret = int_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type INT16, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toUInt(uint32_t &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_UINT32)) {
        guint32 uint_variant = 0;
        g_variant_get(m_gVariant, "u", &uint_variant);
        ret = uint_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type UINT32, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::toUInt16(uint16_t &ret) const
{
    if (!m_gVariant) {
        XLOGD_DEBUG("m_gVariant is NULL!!");
        return false;
    }
    if (g_variant_is_of_type(m_gVariant, G_VARIANT_TYPE_UINT16)) {
        guint16 uint_variant = 0;
        g_variant_get(m_gVariant, "q", &uint_variant);
        ret = uint_variant;
    } else {
        XLOGD_ERROR("Variant <%s> is not of type UINT16, its type <%s>", 
                dumpToString().c_str(), g_variant_get_type_string(m_gVariant));
        return false;
    }
    return true;
}

bool DBusVariant::getFd(int index, int &retFd) const
{
    if (!m_gFdList) {
        XLOGD_ERROR("m_gFdList is NULL!!");
        return false;
    }

    GError *error = NULL;
    int fd = g_unix_fd_list_get (m_gFdList, index, &error);

    if (fd < 0) {
        XLOGD_ERROR("Received invalid file descriptor from GUnixFDList <-1>, error = <%s>", 
                (error == NULL) ? "" : error->message);
        g_clear_error (&error);
        return false;
    }

    retFd = fd;

    return true;
}
