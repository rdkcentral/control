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
    TimeOuts timeouts = { 15000, 15000, 60000, 20000, 20000, 65000 };

    struct {
        const char *name;
        int *storage;
    } fields[6] = {
        { "discovery",      &timeouts.discoveryMSecs        },
        { "pair",           &timeouts.pairingMSecs          },
        { "setup",          &timeouts.setupMSecs            },
        { "unpair",         &timeouts.upairingMSecs         },
        { "hidrawPoll",     &timeouts.hidrawWaitPollMSecs   },
        { "hidrawLimit",    &timeouts.hidrawWaitLimitMSecs  },
    };

    // process the fields
    for (unsigned int i = 0; i < (sizeof(fields) / sizeof(fields[0])); i++) {
       json_t *timeout = json_object_get(json, fields[i].name);
        if (!timeout || !json_is_integer(timeout)) {
            if(timeout) {
                XLOGD_WARN("invalid '%s' field, reverting to default", fields[i].name);
            }
        } else {
               *(fields[i].storage) = (int)json_integer_value(timeout);
            XLOGD_DEBUG("'%s' field = %d", fields[i].name, *(fields[i].storage));
        }
    }

    return timeouts;
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
    // find the timeout params
    json_t *timeoutsObj = json_object_get(jsonConfig, "timeouts");
    if (!timeoutsObj || !json_is_object(timeoutsObj)) {
        XLOGD_ERROR( "missing or invalid 'timeouts' field in config");
        return std::shared_ptr<ConfigSettings>();
    }

    TimeOuts timeouts = parseTimeouts(timeoutsObj);

    
    // find the vendor details array
    json_t *modelArray = json_object_get(jsonConfig, "models");
    if (!modelArray || !json_is_array(modelArray)) {
        XLOGD_ERROR( "missing or invalid 'models' field in config");
        return std::shared_ptr<ConfigSettings>();
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

    // finally return the config
    return std::make_shared<ConfigSettings>(timeouts, std::move(models));
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
                               std::vector<ConfigModelSettings> &&modelDetails)
    : m_timeOuts(timeouts)
    , m_modelDetails(std::move(modelDetails))
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

