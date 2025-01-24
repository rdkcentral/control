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
//  blercuservicesfactory.cpp
//

#include "blercuservicesfactory.h"
#include "gatt/gatt_services.h"
#include "configsettings/configsettings.h"
#include "ctrlm_log_ble.h"

// -----------------------------------------------------------------------------
/*!`
    \class BleRcuServicesFactory
    \brief Factory class for creating BLE RCU service objects.

    The returned services object stores shared pointers to all the individual
    service objects attached to the given gatt profile.



 */
BleRcuServicesFactory::BleRcuServicesFactory(const std::shared_ptr<const ConfigSettings> &config)
    : m_config(config)
{
}

// -----------------------------------------------------------------------------
/*!
    Creates a \l{BleRcuServices} object and returns it.

    The \a address is the bluetooth mac address and the OUI of that is used to
    determine the services implementation to use.  For EC10x RCUs we use the
    GATT services, for the Ruwido RCUs we tunnel messages over the HID
    descriptors.


 */
std::shared_ptr<BleRcuServices> BleRcuServicesFactory::createServices(const BleAddress &address,
                                                                      const std::shared_ptr<BleGattProfile> &gattProfile,
                                                                      const std::string &name,
                                                                      GMainLoop* mainLoop)
{
    ConfigModelSettings settings = m_config->modelSettings(name);
    if (!settings.isValid()) {
        XLOGD_ERROR("no model settings for device %s with name %s", address.toString().c_str(), name.c_str());
        return std::shared_ptr<BleRcuServices>(nullptr);
    }

    // create the service type based on the communication method
    switch (settings.servicesType()) {
        case ConfigModelSettings::GattServiceType:
            return std::make_shared<GattServices>(address, gattProfile, settings, mainLoop);
        default:
            XLOGD_ERROR("service interface not supported");
            return std::shared_ptr<BleRcuServices>(nullptr);
    }
}

