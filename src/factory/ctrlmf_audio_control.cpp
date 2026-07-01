#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>
#include <dsAudio.h>
#include "thunder/plugins/ctrlm_thunder_plugin_display_settings.h"

bool ctrlmf_audio_control_init(void) {
   // DSMgr initialization is no longer needed; Thunder plugins self-initialise.
   return(true);
}

bool ctrlmf_audio_control_term(void) {
   // DSMgr deinitialization is no longer needed; Thunder plugins self-manage.
   return(true);
}

bool ctrlmf_audio_control_mute(bool mute) {
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }
   dsAudioDuckingAction_t action = mute ? dsAUDIO_DUCKINGACTION_START : dsAUDIO_DUCKINGACTION_STOP;
   auto *ds = Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   bool ret = ds->set_audio_ducking(action, dsAUDIO_DUCKINGTYPE_ABSOLUTE, mute ? 0 : 100);
   if(ret) {
      XLOGD_INFO("Audio is %smuted", mute ? "" : "un-");
   } else {
      XLOGD_WARN("Muting sound via Thunder failed");
   }
   return(ret);
}

bool ctrlmf_audio_control_attenuate(bool enable, bool relative, double vol) {
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }
   if(vol < 0 || vol > 1) {
      XLOGD_ERROR("Invalid volume");
      return(false);
   }
   unsigned char level = (unsigned char)((vol * 100) + 0.5);
   dsAudioDuckingAction_t action = enable   ? dsAUDIO_DUCKINGACTION_START  : dsAUDIO_DUCKINGACTION_STOP;
   dsAudioDuckingType_t   type   = relative ? dsAUDIO_DUCKINGTYPE_RELATIVE : dsAUDIO_DUCKINGTYPE_ABSOLUTE;

   auto *ds = Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   bool ret = ds->set_audio_ducking(action, type, level);
   if(ret) {
      if(enable) {
         XLOGD_INFO("Audio ducking enabled - type <%s> level <%u%%>", relative ? "RELATIVE" : "ABSOLUTE", level);
      } else {
         XLOGD_INFO("Audio ducking disabled");
      }
   } else {
      XLOGD_WARN("Ducking sound via Thunder failed");
   }
   return(ret);
}
