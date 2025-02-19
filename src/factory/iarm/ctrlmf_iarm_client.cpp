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

#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include "ctrlmf_iarm_client.h"

using namespace Iarm;
using namespace Client;

static uint32_t g_user_count = 0;

ctrlm_iarm_client_t::ctrlm_iarm_client_t() {
   this->connected = false;

   if(g_user_count == 0) {
      IARM_Result_t result;

      XLOGD_INFO("initialize IARM bus");
      // Initialize the IARM Bus
      result = IARM_Bus_Init("CtrlmFactory");
      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_FATAL("Unable to initialize IARM bus!");
      } else {
         // Connect to IARM Bus
         result = IARM_Bus_Connect();
         if(IARM_RESULT_SUCCESS != result) {
            XLOGD_FATAL("Unable to connect IARM bus!");
         } else {
            XLOGD_INFO("connected to IARM bus");
         }
         this->connected = true;
         g_user_count++;
      }
   }

}

ctrlm_iarm_client_t::~ctrlm_iarm_client_t() {
   if(this->connected) {
      g_user_count--;
      if(g_user_count == 0) {
         IARM_Result_t result;

         // Disconnect from IARM Bus
         result = IARM_Bus_Disconnect();
         if(IARM_RESULT_SUCCESS != result) {
            XLOGD_FATAL("Unable to disconnect IARM bus!");
         }
      }
   }
}

bool ctrlm_iarm_client_t::register_events() {
    bool ret = true;
    return(ret);
}

const char *ctrlm_iarm_result_str(IARM_Result_t result) {
   switch(result) {
      case IARM_RESULT_SUCCESS:       return("IARM_CALL_SUCCESS");
      case IARM_RESULT_INVALID_PARAM: return("IARM_CALL_ERROR_INVALID_PARAMETER");
      case IARM_RESULT_INVALID_STATE: return("IARM_CALL_ERROR_INVALID_STATE");
      case IARM_RESULT_IPCCORE_FAIL:  return("IARM_CALL_ERROR_IPCCORE_FAIL");
      case IARM_RESULT_OOM:           return("IARM_CALL_ERROR_OOM");
      default:                        return("IARM_CALL_UNKNOWN");
   }
}

