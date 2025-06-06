#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <rdkversion.h>
#include <ctrlms_utils.h>

typedef struct {
   bool initialized;
   bool is_production;
} ctrlms_global_t;

ctrlms_global_t g_ctrlms = {
   .initialized         = false,
   .is_production       = true
};

bool ctrlms_init(xlog_level_t level) {
   rdk_version_info_t info;
   int ret_val = rdk_version_parse_version(&info);

   if(ret_val != 0) {
      XLOGD_ERROR("parse error <%s>\n", info.parse_error == NULL ? "" : info.parse_error);
      return(false);
   }
   
   g_ctrlms.is_production = info.production_build;

   rdk_version_object_free(&info);

   int rc = xlog_init(XLOG_MODULE_ID, NULL, 0);
   xlog_level_set_all(level);

   if(rc != 0) {
      XLOGD_ERROR("failed to init xlog");
      return(false);
   }

   g_ctrlms.initialized         = true;
   return(true);
}

void ctrlms_term(void) {
   g_ctrlms.initialized         = false;
   xlog_term();
}

bool ctrlms_is_initialized(void) {
   return(g_ctrlms.initialized);
}

bool ctrlms_is_production(void) {
   return(g_ctrlms.is_production);
}

