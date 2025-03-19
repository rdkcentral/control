#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <libIBus.h>
#include <ctrlmf_utils.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_iarm_client.h>
#include <jansson.h>

#define FACTORY_RESET_TIMEOUT (15 * 1000) // 15 seconds, in milliseconds

bool ctrlmf_factory_reset(void) {
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }

   Iarm::Client::ctrlm_iarm_client_t obj_ctrlm;

   ctrlm_main_iarm_call_json_t reset;
   memset(&reset, 0, sizeof(reset));
   
   reset.api_revision = CTRLM_MAIN_IARM_BUS_API_REVISION;
   
   IARM_Result_t result = IARM_Bus_Call_with_IPCTimeout(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_MAIN_IARM_CALL_FACTORY_RESET, &reset, sizeof(reset), FACTORY_RESET_TIMEOUT);
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("CTRLM_MAIN_IARM_CALL_FACTORY_RESET call <%s>", iarm_result_str(result));
      return(false);
   }

   json_t *obj = json_loads(reset.result, JSON_DECODE_ANY, NULL);
   if (obj == NULL) {
       XLOGD_ERROR("Invalid JSON");
       return(false);
   }

   json_t *value = json_object_get(obj, "success");
   if (value == NULL || !json_is_boolean(value)) {
       XLOGD_ERROR("Invalid JSON");
       json_decref(obj);
       return(false);
   }
   bool ret = json_is_true(value);
   json_decref(obj);

   return(ret);
}
