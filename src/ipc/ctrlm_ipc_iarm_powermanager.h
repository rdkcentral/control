/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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
#include "ctrlm.h"

/**
 * This class is used within ControlMgr to interact with the IARM-based PowerManager 
 */
class ctrlm_ipc_iarm_powermanager_t {
public:
    /**
     * This function is used to get the IARM PowerManager instance, as it is a Singleton.
     * @return The instance of the IARM PowerManager, or NULL on error.
     */
    ctrlm_ipc_iarm_powermanager_t *get_instance();

    /**
     * IARM PowerManager Default Constructor
     */
    ctrlm_ipc_iarm_powermanager_t();

    /**
     * IARM PowerManager Destructor
     */
    virtual ~ctrlm_ipc_iarm_powermanager_t();

    void get_power_state(ctrlm_power_state_t &power_state);
    void running_set(bool running);
    #ifdef NETWORKED_STANDBY_MODE_ENABLED
    void get_networked_standby_mode(bool &networked_standby_mode);
    void get_wakeup_reason_voice(bool &wakeup_reason_voice);
    #endif


private:
    /* Check if they need to be stored here and cached or call the API every time - todo */
    ctrlm_power_state_t power_state;
    bool            networked_standby_mode;
    sem_t           semaphore;
};

ctrlm_ipc_iarm_powermanager_t *ctrlm_ipc_iarm_powermanager_create();
#if CTRLM_HAL_RF4CE_API_VERSION >= 10 && !defined(CTRLM_DPI_CONTROL_NOT_SUPPORTED)
IARM_Result_t ctrlm_iarm_powermanager_event_handler_power_pre_change(void* pArgs);
#endif
IARM_Result_t ctrlm_iarm_powermanager_call_power_state_change(void *arg);
