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
//  configmodelsettings.cpp
//

#include "configmodelsettings.h"
#include "configmodelsettings_p.h"
#include "bleuuid.h"
#include "ctrlm_log_ble.h"

using namespace std;

ConfigModelSettingsData::ConfigModelSettingsData()
    : m_valid(false)
    , m_disabled(false)
    , m_typeZ(false)
    , m_hasConnParams(false)
    , m_servicesType(ConfigModelSettings::GattServiceType)
{
}

ConfigModelSettingsData::ConfigModelSettingsData(const ConfigModelSettingsData &other)
    : m_valid(other.m_valid)
    , m_name(other.m_name)
    , m_disabled(other.m_disabled)
    , m_pairingNameFormat(other.m_pairingNameFormat)
    , m_otaProductName(other.m_otaProductName)
    // , m_filterBytes(other.m_filterBytes)
    , m_standbyMode(other.m_standbyMode)
    , m_typeZ(other.m_typeZ)
    , m_connParamUpdateBeforeOtaVersion(other.m_connParamUpdateBeforeOtaVersion)
    , m_upgradePauseVersion(other.m_upgradePauseVersion)
    , m_upgradeStuckVersion(other.m_upgradeStuckVersion)
    , m_hasConnParams(other.m_hasConnParams)
    // , m_connParams(other.m_connParams)
    , m_servicesType(other.m_servicesType)
    , m_servicesRequired(other.m_servicesRequired)
    , m_servicesOptional(other.m_servicesOptional)
{
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Constructs some vendor settings from the supplied json object, if the json
    object has errors then an invalid object is created.

    The object should be formatted like the following:

    \code
        [
            {
                "name": "EC05x",
                "manufacturer": "Ruwido",
                "oui": "1C:A2:B1",
                "pairingNameFormat": "U%03hhu*",
                "connectionParams": {
                    "maxInterval": 15.0,
                    "minInterval": 15.0,
                    "latency": 332,
                    "supervisionTimeout": 15000
                },
                "services": {
                    "type": "dbus",
                    "dbusServiceName": "com.ruwido.rcu",
                    "dbusObjectPath": "/com/ruwido/rcu"
                    "supported": [
                        "audio",
                        "battery",
                        "deviceInfo",
                        "findMe",
                        "infrared",
                        "touch"
                    ]
                }
            },
            ...
        ]
    \endcode

 */
ConfigModelSettingsData::ConfigModelSettingsData(json_t *json)
    : m_valid(false)
    , m_disabled(false)
    , m_typeZ(false)
    , m_hasConnParams(false)
    , m_servicesType(ConfigModelSettings::GattServiceType)
{
    // name field
    json_t *obj = json_object_get(json, "name");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("invalid 'name' field");
        return;
    }
    m_name = json_string_value(obj);

    XLOGD_DEBUG("m_name = <%s>", m_name.c_str());

    // (optional) disabled field
    obj = json_object_get(json, "disabled");
    if (obj) {
        if (!json_is_boolean(obj)) {
            XLOGD_ERROR("invalid 'disabled' field");
            return;
        } else {
            m_disabled = json_is_true(obj);
        }
    }

    // pairingFormat field
    obj = json_object_get(json, "pairingNameFormat");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("invalid 'pairingNameFormat' field");
        return;
    }
    m_pairingNameFormat = json_string_value(obj);

    // scanNameFormat field
    obj = json_object_get(json, "scanNameFormat");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("invalid 'scanNameFormat' field");
        return;
    }
    m_scanNameMatcher = std::regex(json_string_value(obj), std::regex_constants::ECMAScript);
    
    // connectNameFormat field
    obj = json_object_get(json, "connectNameFormat");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("invalid 'connectNameFormat' field");
        return;
    }
    m_connectNameMatcher = std::regex(json_string_value(obj), std::regex_constants::ECMAScript);

    // otaProductName field
    obj = json_object_get(json, "otaProductName");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("invalid 'otaProductName' field");
        return;
    }
    m_otaProductName = json_string_value(obj);

    // (optional) typeZ field
    obj = json_object_get(json, "typeZ");
    if (obj) {
        if (!json_is_boolean(obj)) {
            XLOGD_WARN("invalid 'typeZ' field, not fatal, continuing to parse info");
        } else {
            m_typeZ = json_is_true(obj);
        }
    }

    // (optional) connParamUpdateBeforeOtaVersion field
    obj = json_object_get(json, "connParamUpdateBeforeOtaVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("invalid 'connParamUpdateBeforeOtaVersion' field, not fatal, continuing to parse info");
        } else {
            m_connParamUpdateBeforeOtaVersion = json_string_value(obj);
        }
    }

    // (optional) upgradePauseVersion field
    obj = json_object_get(json, "upgradePauseVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("invalid 'upgradePauseVersion' field, not fatal, continuing to parse info");
        } else {
            m_upgradePauseVersion = json_string_value(obj);
        }
    }

    // (optional) upgradeStuckVersion field
    obj = json_object_get(json, "upgradeStuckVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("invalid 'upgradeStuckVersion' field, not fatal, continuing to parse info");
        } else {
            m_upgradeStuckVersion = json_string_value(obj);
        }
    }

    // filterByte field
    // {
    //  const QJsonValue filterBytes = json["filterBytes"];
    //  if (!filterBytes.isArray()) {
    //      qWarning("invalid 'filterBytes' field");
    //      return;
    //  }

    //  const QJsonArray filterByteValues = filterBytes.toArray();
    //  for (const auto &filterByte : filterByteValues) {
    //      if (!filterByte.isDouble())
    //          qWarning("invalid entry in 'filterBytes' array");
    //      else
    //          m_filterBytes.insert(static_cast<quint8>(filterByte.toInt()));
    //  }
    // }

    // standbyMode field
    obj = json_object_get(json, "standbyMode");
    if (!obj || !json_is_string(obj)) {
        XLOGD_WARN("invalid or missing 'standbyMode' field, not fatal, continuing to parse info");
    } else {
        m_standbyMode = json_string_value(obj);
    }

    // voiceKeyCode field
    obj = json_object_get(json, "voiceKeyCode");
    if (obj) {
        if(!json_is_integer(obj)) {
            XLOGD_WARN("invalid 'voiceKeyCode' field, not fatal, continuing to parse info");
        } else {
            json_int_t keyCode = json_integer_value(obj);
            if(keyCode < 0 || keyCode > 0xFFFF) {
                XLOGD_WARN("out of range 'voiceKeyCode' field, not fatal, continuing to parse info");
            } else {
                m_voiceKeyCode        = keyCode;
                m_voiceKeyCodePresent = true;
            }
        }
    }

    // services field
    json_t *servicesObj = json_object_get(json, "services");
    if (!servicesObj || !json_is_object(servicesObj)) {
        XLOGD_ERROR("missing or invalid 'services' field");
        return;
    }
    // services.type field
    obj = json_object_get(servicesObj, "type");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("missing 'service.type' field");
        return;
    }
    string typeStr = json_string_value(obj);

    if (typeStr.compare("gatt") == 0) {
        m_servicesType = ConfigModelSettings::GattServiceType;
    } else {
        XLOGD_ERROR("invalid 'service.type' field value");
        return;
    }

    // services.required array
    json_t *requiredArray = json_object_get(servicesObj, "required");
    if (!requiredArray || !json_is_array(requiredArray)) {
        XLOGD_ERROR( "missing or invalid 'services.required' field");
        return;
    }
    size_t array_size = json_array_size(requiredArray);

    for (unsigned int i = 0; i < array_size; i++) {
        obj = json_array_get(requiredArray, i);
        if (!obj || !json_is_string(obj)) {
            XLOGD_ERROR("invalid 'services.required' array entry");
            return;
        }
        string serviceStr = json_string_value(obj);

        if (!BleUuid().doesServiceExist(serviceStr)) {
            XLOGD_ERROR("invalid service name <%s>", serviceStr.c_str());
            return;
        }
        m_servicesRequired.insert(serviceStr);
    }

    std::string servicesRequiredStr;
    for (const auto& service : m_servicesRequired) {
        servicesRequiredStr += service + ", ";
    }
    servicesRequiredStr = servicesRequiredStr.substr(0, servicesRequiredStr.length() - 2); // Remove the trailing comma and space

    XLOGD_DEBUG("m_servicesRequired = <%s>", servicesRequiredStr.c_str());

    // services.optional array
    json_t *optionalArray = json_object_get(servicesObj, "optional");
    if (!optionalArray || !json_is_array(optionalArray)) {
        XLOGD_ERROR( "missing or invalid 'services.optional' field");
        return;
    }
    array_size = json_array_size(optionalArray);

    for (unsigned int i = 0; i < array_size; i++) {
        obj = json_array_get(optionalArray, i);
        if (!obj || !json_is_string(obj)) {
            XLOGD_ERROR("invalid 'services.optional' array entry");
            return;
        }
        string serviceStr = json_string_value(obj);

        if (!BleUuid().doesServiceExist(serviceStr)) {
            XLOGD_ERROR("invalid service name <%s>", serviceStr.c_str());
            return;
        }
        m_servicesOptional.insert(serviceStr);
    }

    std::string servicesOptionalStr;
    for (const auto& service : m_servicesOptional) {
        servicesOptionalStr += service + ", ";
    }
    servicesOptionalStr = servicesOptionalStr.substr(0, servicesOptionalStr.length() - 2); // Remove the trailing comma and space

    XLOGD_DEBUG("m_servicesOptional = <%s>\n", servicesOptionalStr.c_str());

    // (optional) connectionParams
    // if (json.contains("connectionParams")) {

    //  const QJsonValue connParams = json["connectionParams"];
    //  if (!connParams.isObject()) {
    //         qWarning("invalid 'connectionParams' field");
    //         return;
    //     }

    //     const QJsonObject connParamsObj = connParams.toObject();
    //     const QJsonValue maxInterval = connParamsObj["maxInterval"];
    //     const QJsonValue minInterval = connParamsObj["minInterval"];

    //     if (maxInterval.isDouble() && minInterval.isDouble()) {
    //         m_connParams.setIntervalRange(minInterval.toDouble(),
    //                                       maxInterval.toDouble());
    //     } else if (maxInterval.type() != minInterval.type()) {
    //         qWarning("both 'maxInterval' and 'minInterval' must be set to set "
    //                  "connection interval");
    //     }

    //     const QJsonValue latency = connParamsObj["latency"];

    //     if (latency.isDouble())
    //         m_connParams.setLatency(latency.toInt(m_connParams.latency()));
    //     else if (!latency.isUndefined())
    //         qWarning("invalid type for latency setting");

    //     const QJsonValue supervisionTimeout = connParamsObj["supervisionTimeout"];

    //     if (supervisionTimeout.isDouble())
    //         m_connParams.setSupervisionTimeout(supervisionTimeout.toInt(m_connParams.supervisionTimeout()));
    //     else if (!supervisionTimeout.isUndefined())
    //         qWarning("invalid type for supervisionTimeout setting");

    //     m_hasConnParams = true;
    // }

    m_valid = true;
}

ConfigModelSettingsData::~ConfigModelSettingsData()
{
}


ConfigModelSettings::ConfigModelSettings()
    : d(make_shared<ConfigModelSettingsData>())
{
}

ConfigModelSettings::ConfigModelSettings(const ConfigModelSettings &other)
    : d(other.d)
{
}

ConfigModelSettings::ConfigModelSettings(json_t *json)
    : d(make_shared<ConfigModelSettingsData>(json))
{
}

ConfigModelSettings::~ConfigModelSettings()
{
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the settings are valid.  If the object is invalid then
    all the getters will return undefined results.

 */
bool ConfigModelSettings::isValid() const
{
    return d && d->m_valid;
}

// -----------------------------------------------------------------------------
/*!
    Returns the model name of the RCU.

 */
std::string ConfigModelSettings::name() const
{
    return d->m_name;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the model is disable, i.e. any of these types of RCU that
    try to pair will be rejected and any that are already paired won't be
    managed by this code.

    This was added for the Amidala RCU (EC080), and is better than just removing
    them from the config as it allows us to clean up any boxes that may have
    already paired RCUs in the past.

 */
bool ConfigModelSettings::disabled() const
{
    return d->m_disabled;
}

// -----------------------------------------------------------------------------
/*!
    Printf style format for a regex / wildcard pattern for matching against
    the vendor devices during pairing.

 */
std::string ConfigModelSettings::pairingNameFormat() const
{
    return d->m_pairingNameFormat;
}

// -----------------------------------------------------------------------------
/*!
    Returns a regex that can be used to match a RCU device in pairing
    mode during a scan.

    This is different from the \a pairingNameFormat() in that is a printf
    style format that expects a pairing byte value to be applied to it to create
    matcher for a single device.  This is a matcher for any device in pairing
    mode.

 */
std::regex ConfigModelSettings::scanNameMatcher() const
{
    return d->m_scanNameMatcher;
}

// -----------------------------------------------------------------------------
/*!

 */
std::regex ConfigModelSettings::connectNameMatcher() const
{
    return d->m_connectNameMatcher;
}

// -----------------------------------------------------------------------------
/*!
    Returns the OTA product name of the RCU.

 */
std::string ConfigModelSettings::otaProductName() const
{
    return d->m_otaProductName;
}

bool ConfigModelSettings::typeZ() const
{
    return d->m_typeZ;
}

std::string ConfigModelSettings::connParamUpdateBeforeOtaVersion() const
{
    return d->m_connParamUpdateBeforeOtaVersion;
}

std::string ConfigModelSettings::upgradePauseVersion() const
{
    return d->m_upgradePauseVersion;
}

std::string ConfigModelSettings::upgradeStuckVersion() const
{
    return d->m_upgradeStuckVersion;
}

// -----------------------------------------------------------------------------
/*!
    Returns the IR filter byte value that the RCU model will send when pairing.

 */
// QSet<quint8> ConfigModelSettings::irFilterBytes() const
// {
//  return d->m_filterBytes;
// }

// -----------------------------------------------------------------------------
/*!
    The standby mode used in the IR service.
 */
std::string ConfigModelSettings::standbyMode() const
{
    return d->m_standbyMode;
}

// -----------------------------------------------------------------------------
/*!
    The key code used for voice sessions.
 */
bool ConfigModelSettings::voiceKeyCode(uint16_t &keyCode) const
{
    if(d->m_voiceKeyCodePresent) {
        keyCode = d->m_voiceKeyCode;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the special connection parameters should be set for
    the vendor's devices.  Currently this only applies to Ruwido RCUs that
    have voice search ability.

    \see bleConnParams()
 */
bool ConfigModelSettings::hasBleConnParams() const
{
    return d->m_hasConnParams;
}

// -----------------------------------------------------------------------------
/*!
    Returns the connection parameters that should be used for the vendor's
    device.  If hasBleConnParams() returns \c false the returned value is
    undefined.

    \see hasBleConnParams()
 */
// BleConnectionParameters ConfigModelSettings::bleConnParams() const
// {
//  return d->m_connParams;
// }

// -----------------------------------------------------------------------------
/*!
    Returns the service type used for the model

 */
ConfigModelSettings::ServicesType ConfigModelSettings::servicesType() const
{
    return d->m_servicesType;
}

// -----------------------------------------------------------------------------
/*!
    Returns a mask of the required services supported by the vendor daemon.

 */
std::set<std::string> ConfigModelSettings::servicesRequired() const
{
    return d->m_servicesRequired;
}

// -----------------------------------------------------------------------------
/*!
    Returns a mask of the optional services supported by the vendor daemon.

 */
std::set<std::string> ConfigModelSettings::servicesOptional() const
{
    return d->m_servicesOptional;
}
