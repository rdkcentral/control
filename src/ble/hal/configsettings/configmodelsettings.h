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
//  configmodelsettings.h
//

#ifndef CONFIGMODELSETTINGS_H
#define CONFIGMODELSETTINGS_H

// #include "utils/bleconnectionparameters.h"

#include <memory>
#include <string>
#include <regex>
#include <set>
#include <jansson.h>


class ConfigModelSettingsData;


class ConfigModelSettings
{
public:
    ConfigModelSettings();
    ConfigModelSettings(const ConfigModelSettings &other);
    ~ConfigModelSettings();

private:
    friend class ConfigSettings;
    explicit ConfigModelSettings(json_t *json);

public:
    bool isValid() const;

    std::string name() const;
    uint32_t oui() const;

    bool disabled() const;

    std::string pairingNameFormat() const;
    std::regex scanNameMatcher() const;
    std::regex connectNameMatcher() const;
    std::string otaProductName() const;

    bool typeZ() const;

    std::string connParamUpdateBeforeOtaVersion() const;
    std::string upgradePauseVersion() const;
    std::string upgradeStuckVersion() const;

    // QSet<quint8> irFilterBytes() const;

    std::string standbyMode() const;
    bool voiceKeyCode(uint16_t &keyCode) const;

public:
    enum ServicesType {
        GattServiceType
    };

    ServicesType servicesType() const;

public:

    std::set<std::string> servicesRequired() const;
    std::set<std::string> servicesOptional() const;

public:
    bool hasBleConnParams() const;
    // BleConnectionParameters bleConnParams() const;

private:
    std::shared_ptr<ConfigModelSettingsData> d;
};


#endif // !defined(CONFIGMODELSETTINGS_H)
