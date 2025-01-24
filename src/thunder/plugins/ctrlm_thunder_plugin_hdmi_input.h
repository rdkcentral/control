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
#ifndef __CTRLM_THUNDER_PLUGIN_HDMI_INPUT_H__
#define __CTRLM_THUNDER_PLUGIN_HDMI_INPUT_H__
#include "ctrlm_thunder_plugin.h"
#include <semaphore.h>
#include <map>

namespace Thunder {
namespace HDMIInput {

/**
 * This class is used within ControlMgr to interact with the HDMIInput Thunder Plugin.
 */
class ctrlm_thunder_plugin_hdmi_input_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder HDMI Input instance, as it is a Singleton.
     * @return The instance of the Thunder HDMI Input, or NULL on error.
     */
    static ctrlm_thunder_plugin_hdmi_input_t *getInstance();

    /**
     * HDMIInput Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_hdmi_input_t();

    /**
     * This function is used to get the HDMI Infoframe data from cache.
     * @param infoframe The vector that the resulting Infoframe data will be placed.
     */
    void get_infoframes(std::map<int, std::vector<uint8_t> > &infoframes);

public:
    /** 
     * This function is technically used internally but from static function. This is used to handle device status events.
     * @param port The HDMI port of the input device which has changed status.
     * @param active The boolean that states whether the source is active or not.
     */
    void on_device_status_change(int port, bool active);

    /**
     * This function is technically used internally but from static function. This function is used to parse device list from HdmiInput calls/events
     */
    void check_device_list(void *params);

protected:
    /**
     * HDMIInput Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_hdmi_input_t();

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
     * This function actually calls the HDMI Input Thunder Plugin to get the Infoframe data.
     * @return True if the call was successful and there is data available, otherwise False.
     */
    bool _get_infoframe(int port);

    /**
     * This function clears the current infoframe cache.
     */
    void _clear_infoframe(int port);

private:
    std::map<int, std::vector<uint8_t> > infoframes;
    sem_t                                semaphore;
    bool                                 registered_events;

};
};
};
#endif