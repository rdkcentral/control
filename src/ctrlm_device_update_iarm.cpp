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
#include <glib.h>
#include "libIBus.h"
#include "ctrlm.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "rf4ce/ctrlm_rf4ce_network.h"
#include "ctrlm_device_update.h"
#include <rdkx_logger.h>

static IARM_Result_t ctrlm_device_update_update_available(void *arg);

gboolean ctrlm_device_update_init_iarm() {
   IARM_Result_t rc;
   XLOGD_INFO("");

   rc = IARM_Bus_RegisterCall(CTRLM_DEVICE_UPDATE_IARM_CALL_UPDATE_AVAILABLE, ctrlm_device_update_update_available);
   if(rc != IARM_RESULT_SUCCESS) {
      XLOGD_ERROR("CTRLM_DEVICE_UPDATE_IARM_CALL_UPDATE_AVAILABLE %d", rc);
      return(false);
   }

   return(true);
}

IARM_Result_t ctrlm_device_update_update_available(void *arg) {
   ctrlm_device_update_iarm_call_update_available_t *params = (ctrlm_device_update_iarm_call_update_available_t *) arg;
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(params->api_revision != CTRLM_DEVICE_UPDATE_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", params->api_revision, CTRLM_DEVICE_UPDATE_IARM_BUS_API_REVISION);
      params->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("got location '%s' and filenames '%s'",params->firmwareLocation, params->firmwareNames);

   //format from script utilty will be:
   // firmwareLocation - "/opt/CDL"
   // firmwareNames - "test.bin,test1.bin,test2.bin"

   gchar *tok = NULL;
   gchar *saveptr = NULL;
   size_t  len = 0;
   len = strlen(params->firmwareNames);
   if(len != 0){
      tok  = strtok_s(params->firmwareNames, &len, ",", &saveptr);
   }
   while(tok!=NULL){
      std::string filename=params->firmwareLocation;
      filename+="/";
      filename+=tok;
      XLOGD_INFO("filename %s", filename.c_str());

      //start processing new update file via message to background task
      if(ctrlm_device_update_process_xconf_update(filename.c_str())==false) {
         params->result = CTRLM_IARM_CALL_RESULT_ERROR;
      }
      else {
         // something went wrong so send error back
         params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
      }
      tok = strtok_s(NULL, &len, ",", &saveptr);

   }
   return IARM_RESULT_SUCCESS;

}
