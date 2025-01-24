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
//  gatt_external_services.h
//

#ifndef GATT_EXTERNAL_SERVICES_H
#define GATT_EXTERNAL_SERVICES_H

#include <vector>
#include <memory>
#include <glib.h>
#include <blercu/bleservices/gatt/gatt_audioservice.h>
#include <blercu/bleservices/gatt/gatt_infraredservice.h>
#include <blercu/bleservices/gatt/gatt_upgradeservice.h>

void ctrlm_ble_uuid_names_install();
void ctrlm_ble_gatt_services_install(GMainLoop *mainLoop, const ConfigModelSettings &settings, const std::shared_ptr<const GattDeviceInfoService> &deviceInfo, std::vector<std::shared_ptr<GattAudioService>> &audioServices, std::vector<std::shared_ptr<GattInfraredService>> &infraredServices, std::vector<std::shared_ptr<GattUpgradeService>> &upgradeServices);

#endif // !defined(GATT_EXTERNAL_SERVICES_H)
