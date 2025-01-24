/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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
#ifndef __CTRLM_THUNDER_PLUGIN_DEVICE_INFO_H__
#define __CTRLM_THUNDER_PLUGIN_DEVICE_INFO_H__
#include "ctrlm_thunder_plugin.h"
#include <semaphore.h>
#include <ctrlm.h>

namespace Thunder {
namespace DeviceInfo {

typedef void (*on_activation_handler_t)(void *user_data);

/**
 * This class is used within ControlMgr to interact with the DeviceInfo Thunder Plugin.
 */
class ctrlm_thunder_plugin_device_info_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder DeviceInfo instance, as it is a Singleton.
     * @return The instance of the Thunder DeviceInfo, or NULL on error.
     */
    static ctrlm_thunder_plugin_device_info_t *getInstance();

    /**
     * DeviceInfo Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_device_info_t();

    /**
     * This function is used to register a handler for the DeviceInfo Thunder Plugin events.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_on_activation_handler(on_activation_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the DeviceInfo Thunder Plugin events.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_on_activation_handler(on_activation_handler_t handler);

    /**
     * This function is used to get the device type.
     * @param device_type The type of device.
     */
    bool get_device_type(ctrlm_device_type_t &device_type);

protected:
    /**
     * DeviceInfo Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_device_info_t();

    /**
     * This function is called when the initial activation of the plugin occurs.
     */
    virtual void on_initial_activation();

    /**
     * This function is called to fetch the device type from the plugin.
     */
    bool _get_device_type();

private:
    ctrlm_device_type_t  device_type;
    bool                 activated;
    sem_t                semaphore;
    std::vector<std::pair<on_activation_handler_t, void *> > on_activation_callbacks;
};
};
};
#endif