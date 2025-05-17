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
#include "ctrlm_log.h"
#include "ctrlm_main_powermanager.h"

static ctrlm_main_powermanager_t *instance = NULL;

ctrlm_main_powermanager_t *ctrlm_main_powermanager_create() {
   if(instance == NULL) {
      instance = new ctrlm_main_powermanager_t();
   }

   #ifdef USE_IARM_POWER_MANAGER
   instance->iarm_powermanager = ctrlm_ipc_iarm_powermanager_create();
   if(instance->iarm_powermanager == NULL) {
      XLOGD_ERROR("failed to create IARM Power Manager instance");
   } else {
      instance->iarm_powermanager->running_set(true);
   }
   #else
   instance->thunder_powermanager = ctrlm_thunder_powermanager_create();
   if(instance->thunder_powermanager == NULL) {
      XLOGD_ERROR("failed to create Thunder Power Manager instance");
   } else {
      intance->thunder_powermanager->running_set(true);
   }
   #endif

   return(instance);
}

ctrlm_main_powermanager_t::ctrlm_main_powermanager_t() {
}

ctrlm_main_powermanager_t::~ctrlm_main_powermanager_t() {
}


void ctrlm_main_powermanager_t::get_power_state(ctrlm_power_state_t &current_state) {

   #ifdef USE_IARM_POWER_MANAGER
   this->iarm_powermanager->get_power_state(current_state);
   #else
   this->thunder_powermanager->get_power_state(current_state);
   #endif
   
   return;
}

#ifdef NETWORKED_STANDBY_MODE_ENABLED
void ctrlm_main_powermanager_t::get_networked_standby_mode(bool &networked_standby_mode) {

   #ifdef USE_IARM_POWER_MANAGER
   this->iarm_powermanager->get_networked_standby_mode(networked_standby_mode);
   #else
   this->thunder_powermanager->get_networked_standby_mode(networked_standby_mode);
   #endif

   return;
}

void ctrlm_main_powermanager_t::get_wakeup_reason_voice(bool &wakeup_reason_voice) {

   #ifdef USE_IARM_POWER_MANAGER
   this->iarm_powermanager->get_wakeup_reason_voice(wakeup_reason_voice);
   #else
   this->thunder_powermanager->get_wakeup_reason_voice(wakeup_reason_voice);
   #endif

   return;
}
#endif
