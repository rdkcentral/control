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
#ifndef __CTRLM_POWERMANAGER_H__
#define __CTRLM_POWERMANAGER_H__

#include "ctrlm_ipc.h"
#ifdef USE_IARM_POWER_MANAGER
#include "ctrlm_ipc_iarm_powermanager.h"
#else
#include "ctrlm_thunder_powermanager.h"
#endif
class ctrlm_main_powermanager_t {
public:
   ctrlm_main_powermanager_t();
   virtual ~ctrlm_main_powermanager_t();
   
   void get_power_state(ctrlm_power_state_t &power_state);
   #ifdef NETWORKED_STANDBY_MODE_ENABLED
   void get_networked_standby_mode(bool &networked_standby_mode);
   void get_wakeup_reason_voice(bool &wakeup_reason_voice);
   #endif
   #ifdef USE_IARM_POWER_MANAGER
   ctrlm_ipc_iarm_powermanager_t *iarm_powermanager;
   #else
   ctrlm_thunder_powermanager_t  *thunder_powermanager;
   #endif
};

ctrlm_main_powermanager_t *ctrlm_main_powermanager_create();

#endif