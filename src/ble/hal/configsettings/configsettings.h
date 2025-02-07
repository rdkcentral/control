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
//  configsettings.h
//

#ifndef CONFIGSETTINGS_H
#define CONFIGSETTINGS_H

#include "configmodelsettings.h"

#include <string>
#include <memory>


class ConfigSettings
{
public:
    ~ConfigSettings();

    static std::shared_ptr<ConfigSettings> defaults();
    static std::shared_ptr<ConfigSettings> fromJsonObj(json_t *jsonConfig);

private:
    static std::shared_ptr<ConfigSettings> fromJsonFile(const std::string &filePath);
    static std::shared_ptr<ConfigSettings> fromJsonBuffer(const char *jsonBuffer);
    static std::shared_ptr<ConfigSettings> parseJson(json_t *jsonConfig);

    struct TimeOuts {
        int discoveryMSecs;
        int pairingMSecs;
        int setupMSecs;
        int upairingMSecs;

        int hidrawWaitPollMSecs;
        int hidrawWaitLimitMSecs;
    };

public:
    // friend class std::shared_ptr<ConfigSettings>;
    ConfigSettings(const TimeOuts &timeouts,
                   std::vector<ConfigModelSettings> &&modelDetails);

public:
    int discoveryTimeout() const;
    int pairingTimeout() const;
    int setupTimeout() const;
    int upairingTimeout() const;

    int hidrawWaitPollTimeout() const;
    int hidrawWaitLimitTimeout() const;

    ConfigModelSettings modelSettings(std::string name) const;
    std::vector<ConfigModelSettings> modelSettings() const;

private:
    static TimeOuts parseTimeouts(json_t *json);

private:
    const TimeOuts m_timeOuts;
    const std::vector<ConfigModelSettings> m_modelDetails;
};

// QDebug operator<<(QDebug dbg, const ConfigSettings &settings);


#endif // !defined(CONFIGSETTINGS_H)
