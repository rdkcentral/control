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
#ifndef __CTRLM_THUNDER_PLUGIN_CEC_SOURCE_H__
#define __CTRLM_THUNDER_PLUGIN_CEC_SOURCE_H__
#include "ctrlm_thunder_plugin.h"
#include "ctrlm_thunder_plugin_cec_common.h"
#include <semaphore.h>
#include <map>

namespace Thunder {
namespace CEC {
/**
 * This class is used within ControlMgr to interact with the CEC Source (HdmiCec_2) Thunder Plugin.
 */
class ctrlm_thunder_plugin_cec_source_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder CEC Source instance, as it is a Singleton.
     * @return The instance of the Thunder CEC Source, or NULL on error.
     */
    static ctrlm_thunder_plugin_cec_source_t *getInstance();

    /**
     * CEC Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_cec_source_t();

    /**
     * This function checks if CEC Daemon is started and CEC Plugin is started.
     * @return True if the CEC Daemon is running and the Plugin is activated, otherwise False.
     */
    bool is_activated();

    /**
     * This function can be used to activate a CEC Thunder Plugin.
     * @return True if the CEC Daemon is running and plugin is activated, otherwise False.
     */
    bool activate();

    /**
     * This function is used to call the enable API.
     * @param enable The boolean to enable/disable the CEC Thunder Plugin.
     * @return True if the call is successful, otherwise False.
     */
    bool enable(bool enable);

    /**
     * This function is used to get the HDMI CEC device data from cache.
     * @param cec_devices The vector that the resulting CEC device data will be placed.
     */
    void get_cec_devices(std::vector<Thunder::CEC::cec_device_t> &cec_devices);

public:
    /** 
     * This function is technically used internally but from static function. This is used to handle events in which devices are updated.
     */
    void on_device_updated();

protected:
    /**
     * CEC Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_cec_source_t();

    /**
     * This function is called when registering for Thunder events.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

    /**
     * This function is called when the initial activation of the plugin occurs.
     */
    virtual void on_initial_activation();

    /**
     * This function actually calls the CEC Thunder Plugin to get the CEC device data.
     * @return True if the call was successful and there is data available, otherwise False.
     */
    bool _update_cec_devices();

private:
    std::vector<Thunder::CEC::cec_device_t> devices;
    sem_t                                   semaphore;
    bool                                    registered_events;

public:
    int                                     device_add_timer; // needs to be modified by static functions

};
};
};
#endif