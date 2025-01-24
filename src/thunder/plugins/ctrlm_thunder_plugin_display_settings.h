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
#ifndef __CTRLM_THUNDER_PLUGIN_DISPLAY_SETTINGS_H__
#define __CTRLM_THUNDER_PLUGIN_DISPLAY_SETTINGS_H__
#include "ctrlm_thunder_plugin.h"
#include <semaphore.h>

namespace Thunder {
namespace DisplaySettings {

/**
 * This class is used within ControlMgr to interact with the DisplaySettings Thunder Plugin.
 */
class ctrlm_thunder_plugin_display_settings_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder Display Settings instance, as it is a Singleton.
     * @return The instance of the Thunder Display Settings, or NULL on error.
     */
    static ctrlm_thunder_plugin_display_settings_t *getInstance();

    /**
     * Display Settings Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_display_settings_t();

    /**
     * This function is used to get the HDMI EDID data from cache.
     * @param edid The vector that the resulting EDID data will be placed.
     */
    void get_edid(std::vector<uint8_t> &edid);

public:
    /** 
     * This function is technically used internally but from static function. This is used to handle hotplug events.
     * @param connected The boolean that states whether the hotplug event is for a connected or disconnected device.
     */
    void on_hotplug(bool connected);

protected:
    /**
     * DisplaySettings Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_display_settings_t();

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
     * This function actually calls the Display Settings Thunder Plugin to get the EDID data.
     * @return True if the call was successful and there is data available, otherwise False.
     */
    bool _get_edid();

    /**
     * This function clears the current edid cache.
     */
    void _clear_edid();

private:
    std::vector<uint8_t> edid;
    sem_t                semaphore;
    bool                 registered_events;

};
};
};
#endif