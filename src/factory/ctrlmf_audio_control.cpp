#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>

#ifdef CTRLM_USE_THUNDER_FR_DS
#include "thunder/ctrlmf_thunder_plugin_display_settings.h"
#else
#include "host.hpp"
#include "exception.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioOutputPort.hpp"
#include "frontPanelIndicator.hpp"
#include "manager.hpp"
#include "dsMgr.h"
#include "dsRpc.h"
#include "dsDisplay.h"
#include <dsAudio.h>
#endif

bool ctrlmf_audio_control_init(void) {
#ifdef CTRLM_USE_THUNDER_FR_DS
   // DSMgr initialization is no longer needed; Thunder plugins self-initialise.
   return(true);
#else
   if(device::Manager::IsInitialized) {
      XLOGD_INFO("DSMgr already initialized");
      return(true);
   }
   try {
      device::Manager::Initialize();
      XLOGD_INFO("DSMgr is initialized");
   }
   catch (...) {
      XLOGD_WARN("Failed to initialize DSMgr");
      return(false);
   }
   return(true);
#endif
}

bool ctrlmf_audio_control_term(void) {
#ifdef CTRLM_USE_THUNDER_FR_DS
   // DSMgr deinitialization is no longer needed; Thunder plugins self-manage.
   return(true);
#else
   try {
      if(device::Manager::IsInitialized) {
         device::Manager::DeInitialize();
      }
   }
   catch(...) {
      XLOGD_WARN("Failed to deinitialize DSMgr");
      return(false);
   }
   return(true);
#endif
}

bool ctrlmf_audio_control_mute(bool mute) {
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Function called: mute=%d", mute);
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }
#ifdef CTRLM_USE_THUNDER_FR_DS
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Using CTRLM_USE_THUNDER_FR_DS path");
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Getting DisplaySettings instance...");

   auto *ds = Thunder::DisplaySettings::ctrlmf_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] DisplaySettings instance obtained");
   bool action = mute;        // true = start ducking (mute), false = stop ducking (unmute)
   bool type   = false;       // false = absolute ducking
   unsigned char level = mute ? 0 : 100;
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Calculated: action=%d, type=%d, level=%u", action, type, level);
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Calling set_audio_ducking...");
   bool ret = ds->set_audio_ducking(action, type, level);
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] set_audio_ducking returned: %d", ret);

   if(ret) {
      XLOGD_INFO("Audio is %smuted", mute ? "" : "un-");
   } else {
      XLOGD_WARN("Muting sound error");
   }
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Returning: %d", ret);
   return(ret);
#else
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Using device::Host (non-Thunder) path");
   try {
      dsAudioDuckingAction_t action = mute ? dsAUDIO_DUCKINGACTION_START : dsAUDIO_DUCKINGACTION_STOP;
      unsigned char level = mute ? 0 : 100;
      XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Calculated: action=%d, level=%u", action, level);
      XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Calling device::Host::getInstance().getAudioOutputPort(SPEAKER0).setAudioDucking...");
      device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, dsAUDIO_DUCKINGTYPE_ABSOLUTE, level);
      XLOGD_INFO("[CTRLMF_AUDIO_MUTE] setAudioDucking call completed successfully");

      XLOGD_INFO("Audio is %smuted", mute?"":"un-");
   }
   catch(std::exception& error) {
      XLOGD_WARN("Muting sound error : %s", error.what());
      return(false);
   }
   XLOGD_INFO("[CTRLMF_AUDIO_MUTE] Returning: true");
   return(true);
#endif
}

bool ctrlmf_audio_control_attenuate(bool enable, bool relative, double vol) {
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Function called: enable=%d, relative=%d, vol=%f", enable, relative, vol);
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }
   if(vol < 0 || vol > 1) {
      XLOGD_ERROR("Invalid volume");
      return(false);
   }
#ifdef CTRLM_USE_THUNDER_FR_DS
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Using CTRLM_USE_THUNDER_FR_DS path");
   unsigned char level = (unsigned char)((vol * 100) + 0.5);
   bool action = enable;      // true = start ducking, false = stop ducking
   bool type   = relative;    // true = relative, false = absolute
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Calculated: action=%d, type=%d, level=%u", action, type, level);

   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Getting DisplaySettings instance...");
   auto *ds = Thunder::DisplaySettings::ctrlmf_thunder_plugin_display_settings_t::getInstance();
   if(!ds) {
      XLOGD_ERROR("DisplaySettings plugin not available");
      return(false);
   }
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] DisplaySettings instance obtained successfully");
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Calling set_audio_ducking...");
   bool ret = ds->set_audio_ducking(action, type, level);
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] set_audio_ducking returned: %d", ret);
   if(ret) {
      if(enable) {
         XLOGD_INFO("Audio ducking enabled - type <%s> level <%u%%>", relative ? "RELATIVE" : "ABSOLUTE", level);
      } else {
         XLOGD_INFO("Audio ducking disabled");
      }
   } else {
      XLOGD_WARN("Ducking sound error");
   }
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Returning: %d", ret);
   return(ret);
#else
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Using device::Host (non-Thunder) path");
   try {
      unsigned char level = (unsigned char)((vol * 100) + 0.5);

      dsAudioDuckingAction_t action = enable   ? dsAUDIO_DUCKINGACTION_START  : dsAUDIO_DUCKINGACTION_STOP;
      dsAudioDuckingType_t   type   = relative ? dsAUDIO_DUCKINGTYPE_RELATIVE : dsAUDIO_DUCKINGTYPE_ABSOLUTE;
      XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Calculated: action=%d, type=%d, level=%u", action, type, level);

      XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Calling device::Host::getInstance().getAudioOutputPort(SPEAKER0).setAudioDucking...");
      device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, type, level);
      XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] setAudioDucking call completed successfully");

      if(enable) {
         XLOGD_INFO("Audio ducking enabled - type <%s> level <%u%%>", relative ? "RELATIVE" : "ABSOLUTE", level);
      } else {
         XLOGD_INFO("Audio ducking disabled");
      }
   }
   catch(std::exception& error) {
      XLOGD_WARN("Ducking sound error : %s", error.what());
      return(false);
   }
   XLOGD_INFO("[CTRLMF_AUDIO_ATTENUATE] Returning: true");
   return(true);
#endif
}
