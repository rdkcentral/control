#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>

#ifdef CTRLMF_THUNDER
#include "thunder/ctrlmf_thunder_plugin_display_settings.h"
#endif

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
#ifdef CTRLMF_THUNDER
   auto *ds = Thunder::DisplaySettings::ctrlmf_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   bool ret = ds->set_audio_ducking(mute, false, mute ? 0 : 100);
   if(ret) {
      XLOGD_INFO("Audio is %smuted", mute ? "" : "un-");
   } else {
      XLOGD_WARN("Muting sound error");
   }
   return(ret);
#else
   XLOGD_WARN("DisplaySettings not available (THUNDER disabled)");
   return(true);
#endif
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
#ifdef CTRLMF_THUNDER
   unsigned char level = (unsigned char)((vol * 100) + 0.5);
   auto *ds = Thunder::DisplaySettings::ctrlmf_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   bool ret = ds->set_audio_ducking(enable, relative, level);
   if(ret) {
      if(enable) {
         XLOGD_INFO("Audio ducking enabled - type <%s> level <%u%%>", relative ? "RELATIVE" : "ABSOLUTE", level);
      } else {
         XLOGD_INFO("Audio ducking disabled");
      }
   } else {
      XLOGD_WARN("Ducking sound error");
   }
   return(ret);
#else
   XLOGD_WARN("DisplaySettings not available (THUNDER disabled)");
   return(true);
#endif
}
