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
#ifndef __CTRLM_THUNDER_PLUGIN_POWERMANAGER_H__
#define __CTRLM_THUNDER_PLUGIN_POWERMANAGER_H__
#include "ctrlm.h"
#include "ctrlm_thunder_plugin.h"
#include "ctrlm_utils.h"
#include "interfaces/IPowerManager.h"
#include <semaphore.h>
#include <map>

#define POWER_STATE_OFF                 WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF
#define POWER_STATE_STANDBY             WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY
#define POWER_STATE_ON                  WPEFramework::Exchange::IPowerManager::POWER_STATE_ON
#define POWER_STATE_STANDBY_LIGHT_SLEEP WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_LIGHT_SLEEP
#define POWER_STATE_STANDBY_DEEP_SLEEP  WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP

using PowerState   = WPEFramework::Exchange::IPowerManager::PowerState;
//using WakeupReason = WPEFramework::Exchange::IPowerManager::WakeupReason;

namespace Thunder {
namespace PowerManager {

/**
 * This class is used within ControlMgr to interact with the PowerManager Thunder Plugin.
 */
class ctrlm_thunder_plugin_powermanager_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder PowerManager instance, as it is a Singleton.
     * @return The instance of the Thunder PowerManager, or NULL on error.
     */
    static ctrlm_thunder_plugin_powermanager_t *get_instance();

    /**
     * PowerManager Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_powermanager_t();

    /**
     * This function is used to get the current power state from PowerManager plugin.
     * 
     */
    void get_power_state(ctrlm_power_state_t &power_state);

    /**
     * This function is used to get the networked standby mode from PowerManager plugin.
     * 
     */

    void get_networked_standby_mode(bool &networked_standby_mode);

    /**
     * This function is used to determine if the last wakeup was voice
     * 
     */
    void get_wakeup_reason_voice(bool &voice_wakeup);

    void on_power_state_changed(const ctrlm_power_state_t &current_state, const ctrlm_power_state_t &new_state);

protected:
    /**
     * PowerManager Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_powermanager_t();

    /**
     * This function is called when the initial activation of the plugin occurs.
     */
    virtual void on_initial_activation();

    /**
     * This function is called when the operational status of the plugin changes.
     */
    virtual void on_activation_change(Thunder::plugin_state_t state);

    bool register_events();

private:
    /* Check if they need to be stored here and cached or call the API every time - todo */
    ctrlm_power_state_t power_state;
    bool            networked_standby_mode;
    bool            registered_events;
    sem_t           semaphore; // check if it is needed
};
};
};
#endif