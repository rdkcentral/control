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
//  configmodelsettings_p.h
//

#ifndef CONFIGMODELSETTINGS_P_H
#define CONFIGMODELSETTINGS_P_H

// #include "utils/bleconnectionparameters.h"

#include <string>
#include <jansson.h>

class ConfigModelSettingsData
{
public:
    ConfigModelSettingsData();
    ConfigModelSettingsData(json_t *json);
    ConfigModelSettingsData(const ConfigModelSettingsData &other);
    ~ConfigModelSettingsData();

public:
    bool m_valid;

    std::string m_name;
    bool m_disabled;
    std::string m_pairingNameFormat;
    std::regex m_scanNameMatcher;
    std::regex m_connectNameMatcher;
    std::string m_otaProductName;
    std::string m_standbyMode;
    uint16_t m_voiceKeyCode;
    bool m_voiceKeyCodePresent;

    bool m_typeZ;
    std::string m_connParamUpdateBeforeOtaVersion;
    std::string m_upgradePauseVersion;
    std::string m_upgradeStuckVersion;

    bool m_hasConnParams;
    // BleConnectionParameters m_connParams;

    ConfigModelSettings::ServicesType m_servicesType;
    std::set<std::string> m_servicesRequired;
    std::set<std::string> m_servicesOptional;
};


#endif // !defined(CONFIGMODELSETTINGS_P_H)
