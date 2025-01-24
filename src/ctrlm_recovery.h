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
#ifndef _CTRLM_RECOVERY_H_
#define _CTRLM_RECOVERY_H_

#include "ctrlm.h"

// Defines for crash recovery
#define CTRLM_NVM_BACKUP                  "/opt/ctrlm.back"
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
#define HAL_NVM_BACKUP                    "/opt/hal_nvm.back"
#endif

typedef enum {
   CTRLM_RECOVERY_CRASH_COUNT,
   CTRLM_RECOVERY_INVALID_HAL_NVM,
   CTRLM_RECOVERY_INVALID_CTRLM_DB
} ctrlm_recovery_property_t;

typedef enum {
   CTRLM_RECOVERY_TYPE_NONE   = 0,
   CTRLM_RECOVERY_TYPE_BACKUP = 1,
   CTRLM_RECOVERY_TYPE_RESET  = 2
} ctrlm_recovery_type_t;

typedef enum {
   CTRLM_RECOVERY_STATUS_SUCCESS,
   CTRLM_RECOVERY_STATUS_FAILURE
} ctrlm_recovery_status_t;

bool ctrlm_recovery_init();
void ctrlm_recovery_property_set(ctrlm_recovery_property_t property, void *value);
void ctrlm_recovery_property_get(ctrlm_recovery_property_t property, void *value);
void ctrlm_recovery_factory_reset();
void ctrlm_recovery_terminate(bool reset);

#endif
