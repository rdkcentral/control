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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_shared_memory.h"
#include "ctrlm_recovery.h"

static bool g_recovery_initialized = false;

bool ctrlm_recovery_init(void) {
   XLOGD_INFO("");

   if(g_recovery_initialized) {
      XLOGD_ERROR("already initialized");
      return(false);
   }
   if(!ctrlm_sm_init()) {
      XLOGD_ERROR("Failed to initialize shared memory");
      return(false);
   }
   g_recovery_initialized = true;
   return(true);
}

void ctrlm_recovery_property_set(ctrlm_recovery_property_t property, void *value) {
   if(!g_recovery_initialized) {
      XLOGD_ERROR("Recovery was not initialized properly.. Cannot set property %d", property);
      return;
   }
   if(value == NULL) {
      XLOGD_ERROR("invalid param");
      return;
   }

   switch(property) {
      case CTRLM_RECOVERY_CRASH_COUNT: {
         uint32_t *crash_count = (uint32_t *)value;
         ctrlm_sm_recovery_crash_count_write(*crash_count);
         break;
      }
      case CTRLM_RECOVERY_INVALID_HAL_NVM: {
         uint32_t *flag = (uint32_t *)value;
         ctrlm_sm_recovery_invalid_hal_nvm_write(*flag);
         break;
      }
      case CTRLM_RECOVERY_INVALID_CTRLM_DB: {
         uint32_t  *flag = (uint32_t *)value;
         ctrlm_sm_recovery_invalid_ctrlm_db_write(*flag);
         break;
      }
      default: {
         break;
      }
   }
}

void ctrlm_recovery_property_get(ctrlm_recovery_property_t property, void *value) {
   if(!g_recovery_initialized) {
      XLOGD_ERROR("Recovery was not initialized properly.. Cannot set property %d", property);
      return;
   }
   if(value == NULL) {
      XLOGD_ERROR("invalid param");
      return;
   }

   switch(property) {
      case CTRLM_RECOVERY_CRASH_COUNT:      { ctrlm_sm_recovery_crash_count_read((uint32_t *)value);      break; }
      case CTRLM_RECOVERY_INVALID_HAL_NVM:  { ctrlm_sm_recovery_invalid_hal_nvm_read((uint32_t *)value);  break; }
      case CTRLM_RECOVERY_INVALID_CTRLM_DB: { ctrlm_sm_recovery_invalid_ctrlm_db_read((uint32_t *)value); break; }
      default: { break; }
   }
}

void ctrlm_recovery_factory_reset() {
   if(!ctrlm_file_delete(CTRLM_NVM_BACKUP, true)) {
       XLOGD_ERROR("CTRLM_NVM_BACKUP failed");  //CID:87883 - checked return
   }
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
   if(remove(HAL_NVM_BACKUP) != 0) {
       XLOGD_ERROR("HAL_NVM_BACKUP failed");  //CID:87883 - checked return
   }
#endif
}

void ctrlm_recovery_terminate(bool reset) {
   if(reset) { // reset values
      ctrlm_sm_recovery_crash_count_write(0);
      ctrlm_sm_recovery_invalid_hal_nvm_write(0);
      ctrlm_sm_recovery_invalid_ctrlm_db_write(0);
   }
}
