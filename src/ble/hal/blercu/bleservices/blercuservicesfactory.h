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
//  blercuservicesfactory.h
//

#ifndef BLERCUSERVICESFACTORY_H
#define BLERCUSERVICESFACTORY_H

#include "utils/bleaddress.h"

#include <string>
#include <memory>
#include <glib.h>


class ConfigSettings;

class BleRcuServices;
class BleGattProfile;


class BleRcuServicesFactory
{

public:
    BleRcuServicesFactory(const std::shared_ptr<const ConfigSettings> &config);
    ~BleRcuServicesFactory() = default;

public:
    std::shared_ptr<BleRcuServices>
        createServices(const BleAddress &address,
                       const std::shared_ptr<BleGattProfile> &gattProfile,
                       const std::string &name="",
                       GMainLoop* mainLoop = NULL);

private:
    const std::shared_ptr<const ConfigSettings> m_config;
};


#endif // !defined(BLERCUSERVICESFACTORY_H)
