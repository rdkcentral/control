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

class ctrlm_powermanager_t {
public:
   virtual ~ctrlm_powermanager_t();
   
   static ctrlm_powermanager_t *get_instance();
   static void destroy_instance();
   virtual ctrlm_power_state_t get_power_state() = 0;
   #ifdef NETWORKED_STANDBY_MODE_ENABLED
   virtual bool get_networked_standby_mode() = 0;
   virtual bool get_wakeup_reason_voice() = 0;
   #endif
};

#endif
