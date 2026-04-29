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
    , m_voiceKeyCodePresent(false)
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
    , m_standbyMode(other.m_standbyMode)
    , m_voiceKeyCode(other.m_voiceKeyCode)
    , m_voiceKeyCodePresent(other.m_voiceKeyCodePresent)
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
 */
ConfigModelSettingsData::ConfigModelSettingsData(json_t *json)
    : m_valid(false)
    , m_disabled(false)
    , m_pairingNameFormat("")
    , m_otaProductName("")
    , m_standbyMode("")
    , m_voiceKeyCodePresent(false)
    , m_typeZ(false)
    , m_connParamUpdateBeforeOtaVersion("")
    , m_upgradePauseVersion("")
    , m_upgradeStuckVersion("")
    , m_hasConnParams(false)
    , m_servicesType(ConfigModelSettings::GattServiceType)
{
    XLOGD_INFO("--------------------------------------");

    // name field
    json_t *obj = json_object_get(json, "name");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("Required field 'name' INVALID, aborting...");
        return;
    }
    m_name = json_string_value(obj);

    XLOGD_INFO("Name = <%s>", m_name.c_str());

    // (optional) disabled field
    obj = json_object_get(json, "disabled");
    if (obj) {
        if (!json_is_boolean(obj)) {
            XLOGD_WARN("Optional field 'disabled' invalid, continuing...");
        } else {
            m_disabled = json_is_true(obj);
            XLOGD_INFO("parsed 'disabled' field value <%s>", m_disabled ? "TRUE" : "FALSE");
        }
    }

    // Advertising names field
    json_t *advertisingNamesObj = json_object_get(json, "advertisingNames");
    if (!advertisingNamesObj || !json_is_object(advertisingNamesObj)) {
        XLOGD_ERROR("Required field 'advertisingNames' INVALID, aborting...");
        return;
    }
    // advertisingNames.regexPairing field
    obj = json_object_get(advertisingNamesObj, "regexPairing");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("Required field 'advertisingNames.regexPairing' INVALID, aborting...");
        return;
    }
    m_scanNameMatcher = std::regex(json_string_value(obj), std::regex_constants::ECMAScript);
    m_connectNameMatcher = m_scanNameMatcher;
    XLOGD_INFO("Pairing and reconnect advertising name regex <%s>", json_string_value(obj));


    // (optional) advertisingNames.optional object
    json_t *optionalNames = json_object_get(advertisingNamesObj, "optional");
    if (optionalNames) {
        if (!json_is_object(optionalNames)) {
            XLOGD_WARN("Optional field 'advertisingNames.optional' invalid, continuing...");
        } else {
            // (optional) formatSpecifierTargetedPairing field
            obj = json_object_get(optionalNames, "formatSpecifierTargetedPairing");
            if (obj) {
                if (!json_is_string(obj)) {
                    XLOGD_WARN("Optional field 'advertisingNames.optional.formatSpecifierTargetedPairing' invalid, continuing...");
                } else {
                    m_pairingNameFormat = json_string_value(obj);
                    XLOGD_INFO("Pairing name printf format specifier for targeted pairing <%s>", m_pairingNameFormat.c_str());
                }
            }
        
            // (optional) regexReconnect field
            obj = json_object_get(optionalNames, "regexReconnect");
            if (obj) {
                if (!json_is_string(obj)) {
                    XLOGD_WARN("Optional field 'advertisingNames.optional.regexReconnect' invalid, continuing...");
                } else {
                    m_connectNameMatcher = std::regex(json_string_value(obj), std::regex_constants::ECMAScript);
                    XLOGD_INFO("Reconnect advertising name regex overridden with <%s>", json_string_value(obj));
                }
            }
        }
    }

    // (optional) otaProductName field
    obj = json_object_get(json, "otaProductName");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("Optional field 'otaProductName' invalid, continuing...");
        } else {
            m_otaProductName = json_string_value(obj);
            XLOGD_INFO("parsed 'otaProductName' field value <%s>", m_otaProductName.c_str());
        }
    }

    // (optional) typeZ field
    obj = json_object_get(json, "typeZ");
    if (obj) {
        if (!json_is_boolean(obj)) {
            XLOGD_WARN("Optional field 'typeZ' invalid, continuing...");
        } else {
            m_typeZ = json_is_true(obj);
            XLOGD_INFO("parsed 'typeZ' field value <%s>", m_typeZ ? "true" : "false");
        }
    }

    // (optional) connParamUpdateBeforeOtaVersion field
    obj = json_object_get(json, "connParamUpdateBeforeOtaVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("Optional field 'connParamUpdateBeforeOtaVersion' invalid, continuing...");
        } else {
            m_connParamUpdateBeforeOtaVersion = json_string_value(obj);
            XLOGD_INFO("parsed 'connParamUpdateBeforeOtaVersion' field value <%s>", m_connParamUpdateBeforeOtaVersion.c_str());
        }
    }

    // (optional) upgradePauseVersion field
    obj = json_object_get(json, "upgradePauseVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("Optional field 'upgradePauseVersion' invalid, continuing...");
        } else {
            m_upgradePauseVersion = json_string_value(obj);
            XLOGD_INFO("parsed 'upgradePauseVersion' field value <%s>", m_upgradePauseVersion.c_str());
        }
    }

    // (optional) upgradeStuckVersion field
    obj = json_object_get(json, "upgradeStuckVersion");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("Optional field 'upgradeStuckVersion' invalid, continuing...");
        } else {
            m_upgradeStuckVersion = json_string_value(obj);
            XLOGD_INFO("parsed 'upgradeStuckVersion' field value <%s>", m_upgradeStuckVersion.c_str());
        }
    }

    // (optional) standbyMode field
    obj = json_object_get(json, "standbyMode");
    if (obj) {
        if (!json_is_string(obj)) {
            XLOGD_WARN("Optional field 'standbyMode' invalid, continuing...");
        } else {
            m_standbyMode = json_string_value(obj);
            XLOGD_INFO("parsed 'standbyMode' field value <%s>", m_standbyMode.c_str());
        }
    }

    // (optional) voiceKeyCode field
    obj = json_object_get(json, "voiceKeyCode");
    if (obj) {
        if(!json_is_integer(obj)) {
            XLOGD_WARN("Optional field 'voiceKeyCode' invalid, continuing...");
        } else {
            json_int_t keyCode = json_integer_value(obj);
            if(keyCode < 0 || keyCode > 0xFFFF) {
                XLOGD_WARN("Optional field 'voiceKeyCode' out of range, continuing...");
            } else {
                m_voiceKeyCode        = keyCode;
                m_voiceKeyCodePresent = true;
                XLOGD_INFO("parsed 'voiceKeyCode' field value <%u>", m_voiceKeyCode);
            }
        }
    }

    // services field
    json_t *servicesObj = json_object_get(json, "services");
    if (!servicesObj || !json_is_object(servicesObj)) {
        XLOGD_ERROR("Required field 'services' INVALID, aborting...");
        return;
    }
    // services.type field
    obj = json_object_get(servicesObj, "type");
    if (!obj || !json_is_string(obj)) {
        XLOGD_ERROR("Required field 'service.type' INVALID, aborting...");
        return;
    }
    string typeStr = json_string_value(obj);
    XLOGD_INFO("parsed 'service.type' field value <%s>", typeStr.c_str());

    if (typeStr.compare("gatt") == 0) {
        m_servicesType = ConfigModelSettings::GattServiceType;
    } else {
        XLOGD_ERROR("Required field 'service.type' not supported, aborting...");
        return;
    }

    // services.required array
    json_t *requiredArray = json_object_get(servicesObj, "required");
    if (!requiredArray || !json_is_array(requiredArray)) {
        XLOGD_ERROR("Required field 'services.required' INVALID, aborting...");
        return;
    }
    size_t array_size = json_array_size(requiredArray);

    for (unsigned int i = 0; i < array_size; i++) {
        obj = json_array_get(requiredArray, i);
        if (!obj || !json_is_string(obj)) {
            XLOGD_ERROR("Invalid 'services.required' array entry, aborting...");
            return;
        }
        string serviceStr = json_string_value(obj);

        if (!BleUuid().doesServiceExist(serviceStr)) {
            XLOGD_ERROR("invalid service name <%s>, aborting...", serviceStr.c_str());
            return;
        }
        m_servicesRequired.insert(serviceStr);
    }

    std::string servicesRequiredStr;
    for (const auto& service : m_servicesRequired) {
        servicesRequiredStr += service + ", ";
    }
    if (!m_servicesRequired.empty()) {
        servicesRequiredStr = servicesRequiredStr.substr(0, servicesRequiredStr.length() - 2); // Remove the trailing comma and space
    }

    XLOGD_INFO("Services required = <%s>", servicesRequiredStr.c_str());

    // services.optional array
    json_t *optionalArray = json_object_get(servicesObj, "optional");
    if (optionalArray) {
        if (!json_is_array(optionalArray)) {
            XLOGD_WARN("Optional field 'services.optional' invalid, continuing...");
        } else {
            array_size = json_array_size(optionalArray);

            for (unsigned int i = 0; i < array_size; i++) {
                obj = json_array_get(optionalArray, i);
                if (!obj || !json_is_string(obj)) {
                    XLOGD_ERROR("Invalid 'services.optional' array entry, aborting...");
                    return;
                }
                string serviceStr = json_string_value(obj);

                if (!BleUuid().doesServiceExist(serviceStr)) {
                    XLOGD_ERROR("invalid service name <%s>, aborting...", serviceStr.c_str());
                    return;
                }
                m_servicesOptional.insert(serviceStr);
            }

            std::string servicesOptionalStr;
            for (const auto& service : m_servicesOptional) {
                servicesOptionalStr += service + ", ";
            }
            if (!m_servicesOptional.empty()) {
                servicesOptionalStr = servicesOptionalStr.substr(0, servicesOptionalStr.length() - 2); // Remove the trailing comma and space
            }

            XLOGD_INFO("Services optional = <%s>", servicesOptionalStr.c_str());
        }
    }
    XLOGD_INFO("--------------------------------------");

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
