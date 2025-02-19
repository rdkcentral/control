#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <rdkversion.h>
#include <ctrlmf_utils.h>
#include <ctrlmf_audio_playback.h>
#include <ctrlmf_audio_capture.h>
#include <ctrlm_fta_lib.h>

typedef struct {
   bool initialized;
   bool audio_control_init;
   bool audio_playback_init;
   bool is_production;
} ctrlmf_global_t;

ctrlmf_global_t g_ctrlmf = {
   .initialized         = false,
   .audio_control_init  = false,
   .audio_playback_init = false,
   .is_production       = true
};

bool ctrlmf_init(xlog_level_t level, bool requires_audio_playback) {
   rdk_version_info_t info;
   int ret_val = rdk_version_parse_version(&info);

   if(ret_val != 0) {
      XLOGD_ERROR("parse error <%s>\n", info.parse_error == NULL ? "" : info.parse_error);
      return(false);
   }
   
   g_ctrlmf.is_production = info.production_build;

   rdk_version_object_free(&info);

   int rc = xlog_init(XLOG_MODULE_ID, NULL, 0);
   xlog_level_set_all(level);

   if(rc != 0) {
      XLOGD_ERROR("failed to init xlog");
      return(false);
   }

   #ifdef CTRLMF_AUDIO_CONTROL
   if(!ctrlmf_audio_control_init()) {
      XLOGD_ERROR("failed to init audio control");
      return(false);
   }
   #endif

   #ifdef CTRLMF_AUDIO_PLAYBACK
   if(requires_audio_playback && !ctrlmf_audio_playback_init()) {
      XLOGD_ERROR("failed to init audio playback");
      #ifdef CTRLMF_AUDIO_CONTROL
      ctrlmf_audio_control_term();
      #endif
      return(false);
   }
   #endif

   g_ctrlmf.audio_control_init  = true;
   g_ctrlmf.audio_playback_init = requires_audio_playback;
   g_ctrlmf.initialized         = true;
   return(true);
}

void ctrlmf_term(void) {

   #ifdef CTRLMF_AUDIO_PLAYBACK
   if(g_ctrlmf.audio_playback_init) {
      ctrlmf_audio_playback_term();
   }
   #endif

   #ifdef CTRLMF_AUDIO_CONTROL
   if(g_ctrlmf.audio_control_init) {
      ctrlmf_audio_control_term();
   }
   #endif

   g_ctrlmf.audio_playback_init = false;
   g_ctrlmf.audio_control_init  = false;
   g_ctrlmf.initialized         = false;
   xlog_term();
}

bool ctrlmf_is_initialized(void) {
   return(g_ctrlmf.initialized);
}

bool ctrlmf_is_production(void) {
   return(g_ctrlmf.is_production);
}

