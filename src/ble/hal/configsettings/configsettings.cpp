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
//  configsettings.cpp
//

#include "configsettings.h"
#include "ctrlm_log_ble.h"
#include "config.h"
#include "ctrlm_hal_ble.h"

#include <cstring>
#include <cstdint>

using namespace std;



// -----------------------------------------------------------------------------
/*!
    \internal

    Parses the \a json object to extract the timeout values (in milliseconds).
    The object must be formatted like the following

    \code{.json}
        {
            "discovery": 15000,
            "pair": 15000,
            "setup": 60000,
            "unpair": 20000,

            "hidrawPoll": 20000,
            "hidrawLimit": 65000
        }
    \endcode

    All values should be in milliseconds and correspond to the timeout used
    during the various stages of the pairing process.

    \see fromJsonFile()
 */
ConfigSettings::TimeOuts ConfigSettings::parseTimeouts(json_t *json)
{
    TimeOuts timeouts = {
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_DISCOVERY,
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_PAIR,
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_SETUP,
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_UNPAIR,
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_HIDRAWPOLL,
        JSON_INT_VALUE_NETWORK_BLE_TIMEOUTS_HIDRAWLIMIT
    };

    if(json == NULL) { // return default values
        return timeouts;
    }

    struct {
        const char *name;
        int *storage;
        int minValue;
        int maxValue;
    } fields[6] = {
        { "discovery",      &timeouts.discoveryMSecs,       0, 600000 },
        { "pair",           &timeouts.pairingMSecs,         0, 600000 },
        { "setup",          &timeouts.setupMSecs,           0, 600000 },
        { "unpair",         &timeouts.upairingMSecs,        0, 600000 },
        { "hidrawPoll",     &timeouts.hidrawWaitPollMSecs,  0, 600000 },
        { "hidrawLimit",    &timeouts.hidrawWaitLimitMSecs, 0, 600000 },
    };

    // process the fields
    for (unsigned int i = 0; i < (sizeof(fields) / sizeof(fields[0])); i++) {
       json_t *timeout = json_object_get(json, fields[i].name);
        if (!timeout || !json_is_integer(timeout)) {
            if(timeout) {
                XLOGD_WARN("invalid '%s' field, reverting to default", fields[i].name);
            }
        } else {
            json_int_t value = json_integer_value(timeout);
            if(value < fields[i].minValue || value > fields[i].maxValue) {
                XLOGD_WARN("'%s' field value %d out of range (%d - %d), using default", fields[i].name, (int)value, fields[i].minValue, fields[i].maxValue);
            } else {
               *(fields[i].storage) = (int)value;
            }
            XLOGD_INFO("'%s' field = %d ms", fields[i].name, *(fields[i].storage));
        }
    }

    return timeouts;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Parses the \a json "voice_key_mode" array which lists the audio mode to
    program into the RCU for each voice key.  The array must be formatted like
    the following

    \code{.json}
        [
            { "hid_key" : 65,  "mode": "disabled" },
            { "hid_key" : 113, "mode": "hold" },
            { "hid_key" : 112, "mode": "hybrid", "threshold": 300 },
            { "hid_key" : 78,  "mode": "timeout", "timeout": 4000 },
            { "hid_key" : 30,  "mode": "continuous" },
            { "hid_key" : 65,  "mode": "legacy" }
        ]
    \endcode

    "hid_key" is the HID keycode of the voice key and "mode" is one of
    "disabled", "hold", "hybrid", "timeout", "continuous" or "legacy".  The optional
    "threshold" and "timeout" fields are in milliseconds.  Up to 5 keys can be
    listed.  If any entry is invalid the whole array is ignored and an empty
    list is returned, so the RCU model's default is used.

    \see parseJson()
 */
std::vector<ConfigVoiceKeyMode> ConfigSettings::parseVoiceKeyModes(json_t *json)
{
    static const size_t MAX_VOICE_KEYS = CTRLM_HAL_BLE_AUDIO_MODE_CONTROL_MAX_KEYS;
    static const struct {
        const char *name;
        uint8_t     mode;
    } modes[] = {
        { "disabled",   CTRLM_HAL_BLE_AUDIO_MODE_VOICE_DISABLED },
        { "hold",       CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_HOLD },
        { "hybrid",     CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_AND_HOLD_THRESHOLD },
        { "timeout",    CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_TIMEOUT },
        { "continuous", CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_CONTINUOUS },
        { "legacy",     CTRLM_HAL_BLE_AUDIO_MODE_LEGACY },
    };

    std::vector<ConfigVoiceKeyMode> voiceKeyModes;

    if (json == NULL) {
        return voiceKeyModes;
    }
    if (!json_is_array(json)) {
        XLOGD_ERROR("invalid 'voice_key_mode' field, must be an array (using RCU model defaults)");
        return voiceKeyModes;
    }
    if (json_array_size(json) > MAX_VOICE_KEYS) {
        XLOGD_ERROR("'voice_key_mode' has %u entries, max is %u (using RCU model defaults)", (unsigned int)json_array_size(json), (unsigned int)MAX_VOICE_KEYS);
        return voiceKeyModes;
    }

    auto parseMs = [](json_t *entry, const char *name, unsigned int index, bool &present, uint32_t &value) -> bool {
        present = false;
        value   = 0;
        json_t *obj = json_object_get(entry, name);
        if (obj == NULL) {
            return true;
        }
        if (!json_is_integer(obj) || json_integer_value(obj) < 0 || json_integer_value(obj) > (json_int_t)UINT32_MAX) {
            XLOGD_ERROR("'voice_key_mode' entry %u has invalid '%s' field", index, name);
            return false;
        }
        present = true;
        value   = (uint32_t)json_integer_value(obj);
        return true;
    };

    size_t index;
    json_t *entry;
    json_array_foreach(json, index, entry) {
        ConfigVoiceKeyMode keyMode = {};

        json_t *hidKey = json_object_get(entry, "hid_key");
        if (!json_is_object(entry) || !hidKey || !json_is_integer(hidKey) ||
            json_integer_value(hidKey) <= 0 || json_integer_value(hidKey) >= CTRLM_HAL_BLE_AUDIO_MODE_CONTROL_UNUSED_KEY) {
            XLOGD_ERROR("'voice_key_mode' entry %u has missing or invalid 'hid_key' field (using RCU model defaults)", (unsigned int)index);
            return std::vector<ConfigVoiceKeyMode>();
        }
        keyMode.hidKey = (uint8_t)json_integer_value(hidKey);

        json_t *mode = json_object_get(entry, "mode");
        const char *modeStr = (mode && json_is_string(mode)) ? json_string_value(mode) : NULL;
        bool modeFound = false;
        for (const auto &m : modes) {
            if (modeStr && strcmp(modeStr, m.name) == 0) {
                keyMode.audioMode = m.mode;
                modeFound = true;
                break;
            }
        }
        if (!modeFound) {
            XLOGD_ERROR("'voice_key_mode' entry %u has missing or invalid 'mode' field (using RCU model defaults)", (unsigned int)index);
            return std::vector<ConfigVoiceKeyMode>();
        }

        if (!parseMs(entry, "threshold", index, keyMode.thresholdPresent, keyMode.thresholdMs) ||
            !parseMs(entry, "timeout",   index, keyMode.timeoutPresent,   keyMode.timeoutMs)) {
            XLOGD_ERROR("using RCU model defaults for voice key modes");
            return std::vector<ConfigVoiceKeyMode>();
        }

        for (const auto &existing : voiceKeyModes) {
            if (existing.hidKey == keyMode.hidKey) {
                XLOGD_ERROR("'voice_key_mode' entry %u has duplicate 'hid_key' %u (using RCU model defaults)", (unsigned int)index, keyMode.hidKey);
                return std::vector<ConfigVoiceKeyMode>();
            }
        }

        const std::string thresholdStr = keyMode.thresholdPresent ? " threshold <" + std::to_string(keyMode.thresholdMs) + " ms>" : "";
        const std::string timeoutStr   = keyMode.timeoutPresent   ? " timeout <"   + std::to_string(keyMode.timeoutMs)   + " ms>" : "";
        XLOGD_INFO("voice_key_mode: hid_key <0x%02X> mode <%s>%s%s", keyMode.hidKey, modeStr, thresholdStr.c_str(), timeoutStr.c_str());

        voiceKeyModes.push_back(keyMode);
    }

    return voiceKeyModes;
}

// -----------------------------------------------------------------------------
/*!
    Returns the default config settings.

    \see fromJsonBuffer()
 */
std::shared_ptr<ConfigSettings> ConfigSettings::defaults()
{
    return fromJsonBuffer(DEFAULT_CONFIG::json());
}

// -----------------------------------------------------------------------------
/*!
    Returns the config settings using provided json object.

    \see parseJson()
 */
std::shared_ptr<ConfigSettings> ConfigSettings::fromJsonObj(json_t *jsonConfig) {
    return parseJson(jsonConfig);
}

// -----------------------------------------------------------------------------
/*!
    Parses a json config file and returns a \l{std::shared_ptr} to a
    \l{ConfigSettings} object.  If the json is not valid or one or more of
    mandatory fields is missing or malformed then a null shared pointer is
    returned.

    \see defaults()
 */
std::shared_ptr<ConfigSettings> ConfigSettings::parseJson(json_t *jsonConfig)
{
    TimeOuts timeouts;
    // find the timeout params
    json_t *timeoutsObj = json_object_get(jsonConfig, "timeouts");
    if (!timeoutsObj || !json_is_object(timeoutsObj)) {
        XLOGD_WARN( "missing or invalid 'timeouts' field in config (using defaults)");
        timeouts = parseTimeouts(NULL);
    } else {
        timeouts = parseTimeouts(timeoutsObj);
    }

    json_t *obj_network_ble = NULL;

    // find the vendor details array
    json_t *modelArray = json_object_get(jsonConfig, "models");
    if (!modelArray || !json_is_array(modelArray)) {
        XLOGD_WARN( "missing or invalid 'models' field in config (using defaults)");
        
        obj_network_ble = json_loads(DEFAULT_CONFIG::json(), JSON_DECODE_ANY, NULL);
        if (!obj_network_ble || !json_is_object(obj_network_ble)) {
            if(!obj_network_ble) {
                XLOGD_ERROR("invalid default config buffer");
            } else {
                XLOGD_ERROR("default config buffer is not a json object");
                json_decref(obj_network_ble);
            }
            return std::shared_ptr<ConfigSettings>();
        }
    
        modelArray = json_object_get(obj_network_ble, "models");
        if((!modelArray || !json_is_array(modelArray))) {
            XLOGD_ERROR("missing or invalid 'models' field in default config");
            json_decref(obj_network_ble);
            return std::shared_ptr<ConfigSettings>();
        }
    }
    size_t array_size = json_array_size(modelArray);

    vector<ConfigModelSettings> models;

    for (unsigned int i = 0; i < array_size; i++) {
        json_t *modelObject = json_array_get(modelArray, i);
        if (modelObject != NULL) {
            ConfigModelSettings modelSettings(modelObject);
            if (!modelSettings.isValid()) {
                XLOGD_ERROR( "modelSettings invalid, continue....");
                continue;
            }
            models.push_back(std::move(modelSettings));
        }
    }

    if(obj_network_ble != NULL) {
        json_decref(obj_network_ble);
    }

    std::vector<ConfigVoiceKeyMode> voiceKeyModes = parseVoiceKeyModes(json_object_get(jsonConfig, "voice_key_mode"));

    // finally return the config
    return std::make_shared<ConfigSettings>(timeouts, std::move(models), std::move(voiceKeyModes));
}

// -----------------------------------------------------------------------------
/*!
    Parses a json config file and updates the existing object.  Any unrecognized fields are ignored.

    \see defaults()
 */
bool ConfigSettings::updateJson(json_t *jsonConfig)
{
    bool updated = false;

    if (jsonConfig == nullptr || !json_is_object(jsonConfig)) {
        XLOGD_WARN("invalid json config");
        return(updated);
    }

    // find the timeout params
    json_t *timeoutsObj = json_object_get(jsonConfig, "timeouts");
    if (timeoutsObj && json_is_object(timeoutsObj)) {
        m_timeOuts = parseTimeouts(timeoutsObj);
        updated = true;
    }

    // find the model details array
    json_t *modelArray = json_object_get(jsonConfig, "models");
    if (modelArray && json_is_array(modelArray)) {
        XLOGD_WARN("models cannot be updated");
    }

    if (json_object_get(jsonConfig, "voice_key_mode") != NULL) {
        XLOGD_WARN("voice_key_mode cannot be updated");
    }

    return(updated);
}

// -----------------------------------------------------------------------------
/*!
    Parses a json config file and returns a \l{std::shared_ptr} to a
    \l{ConfigSettings} object.  If the json is not valid or one or more of
    mandatory fields is missing or malformed then a null shared pointer is
    returned.

    \see defaults()
 */
std::shared_ptr<ConfigSettings> ConfigSettings::fromJsonFile(const std::string &filePath)
{
    json_t *json = json_load_file(filePath.c_str(), JSON_REJECT_DUPLICATES, NULL);
    if (!json) {
        XLOGD_INFO("Cannot open %-25s for read", filePath.c_str());
        return std::shared_ptr<ConfigSettings>();
    }
    std::shared_ptr<ConfigSettings> ptr = parseJson(json);
    json_decref(json);
    return ptr;
}

// -----------------------------------------------------------------------------
/*!
    Parses a json config file and returns a \l{std::shared_ptr} to a
    \l{ConfigSettings} object.  If the json is not valid or one or more of
    mandatory fields is missing or malformed then a null shared pointer is
    returned.

    \see defaults()
 */
std::shared_ptr<ConfigSettings> ConfigSettings::fromJsonBuffer(const char *jsonBuffer)
{
    json_t *payload = json_loads(jsonBuffer, JSON_DECODE_ANY, NULL);
    if (!payload) {
        XLOGD_ERROR("Bad JSON buffer: %s", jsonBuffer);
        return std::shared_ptr<ConfigSettings>();
    }

    std::shared_ptr<ConfigSettings> ptr = parseJson(payload);
    json_decref(payload);
    return ptr;
}



// -----------------------------------------------------------------------------
/*!
    \internal

    Internal constructor used when config is read from a json document.

    \see fromJsonFile()
 */
ConfigSettings::ConfigSettings(const TimeOuts &timeouts,
                               std::vector<ConfigModelSettings> &&modelDetails,
                               std::vector<ConfigVoiceKeyMode> &&voiceKeyModes)
    : m_timeOuts(timeouts)
    , m_modelDetails(std::move(modelDetails))
    , m_voiceKeyModes(std::move(voiceKeyModes))
{
}

// -----------------------------------------------------------------------------
/*!
    Deletes the settings.

 */
ConfigSettings::~ConfigSettings()
{
}

// -----------------------------------------------------------------------------
/*!
    Returns the settings for the model with the given \a name value.  If no
     matching model is found then an invalid ConfigModelSettings is returned.

 */
ConfigModelSettings ConfigSettings::modelSettings(std::string name) const
{
    for (const ConfigModelSettings &settings : m_modelDetails) {
        if (std::regex_match(name.c_str(), settings.connectNameMatcher())) {
            return settings;
        }
    }

    return ConfigModelSettings();
}

// -----------------------------------------------------------------------------
/*!
    Returns list of model settings from the config params.

 */
std::vector<ConfigModelSettings> ConfigSettings::modelSettings() const
{
    return m_modelDetails;
}

// -----------------------------------------------------------------------------
/*!
    Returns the voice key audio modes to program on all RCUs.  An empty list
    means the built-in default for the RCU model is used.

 */
std::vector<ConfigVoiceKeyMode> ConfigSettings::voiceKeyModes() const
{
    return m_voiceKeyModes;
}

// -----------------------------------------------------------------------------
/*!
    Returns the discovery timeout in milliseconds to use when attempting to
    pair to a new RCU.

    The default value is 15000ms.
 */
int ConfigSettings::discoveryTimeout() const
{
    return m_timeOuts.discoveryMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns the pairing timeout in milliseconds to use when attempting to
    pair to a new RCU.  This is the time from when the pair request was made to
    bluez and the time that an acknowledgement was received.

    The default value is 15000ms.
 */
int ConfigSettings::pairingTimeout() const
{
    return m_timeOuts.pairingMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns the setup timeout in milliseconds to use when attempting to
    pair to a new RCU.  This is the time from when the device is connected and
    paired but we are fetching the details about the device from the vendor
    daemon.

    The default value is 60000ms.
 */
int ConfigSettings::setupTimeout() const
{
    return m_timeOuts.setupMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns the un-pairing timeout in milliseconds to use when attempting to
    pair to a new RCU.  This timeout is used on a failed pairing attempting and
    we want to unpair the device that we tried to pair with.

    The default value is 20000ms.
 */
int ConfigSettings::upairingTimeout() const
{
    return m_timeOuts.upairingMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns the interval in milliseconds on which to poll for the hidraw
    device to be present whilst configuring the bluetooth device.

    The default value is 20000ms.

    \see hidrawWaitLimitTimeout()
 */
int ConfigSettings::hidrawWaitPollTimeout() const
{
    return m_timeOuts.hidrawWaitPollMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns the timeout in milliseconds in which to wait for the hidraw device
    to show up once bluez tells us that the device is connected and paired.
    If this limit is exceeded we (typically) kick off some recovery mechanism
    as it indicates something has gone wrong in the kernel / daemon.

    The default value is 65000ms.

    \see hidrawWaitLimitTimeout()
 */
int ConfigSettings::hidrawWaitLimitTimeout() const
{
    return m_timeOuts.hidrawWaitLimitMSecs;
}

// -----------------------------------------------------------------------------
/*!
    Returns true if any of the config fields are updated. Otherwise, returns false.
 */
bool ConfigSettings::updateFromJson(json_t *jsonConfig)
{
    return updateJson(jsonConfig);
}
