#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>
// dsMgr includes
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
// end dsMgr includes

bool ctrlmf_audio_control_init(void) {
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
}

bool ctrlmf_audio_control_term(void) {
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
}

bool ctrlmf_audio_control_mute(bool mute) {
   if(!ctrlmf_is_initialized()) {
      XLOGD_ERROR("not initialized");
      return(false);
   }

   try {
      dsAudioDuckingAction_t action = mute ? dsAUDIO_DUCKINGACTION_START : dsAUDIO_DUCKINGACTION_STOP;
      device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, dsAUDIO_DUCKINGTYPE_ABSOLUTE, mute ? 0 : 100);
      XLOGD_INFO("Audio is %smuted", mute?"":"un-");
   }
   catch(std::exception& error) {
     XLOGD_WARN("Muting sound error : %s", error.what());
     return(false);
   }
   return(true);
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
   try {
      unsigned char level = (unsigned char)((vol * 100) + 0.5);

      dsAudioDuckingAction_t action = enable   ? dsAUDIO_DUCKINGACTION_START  : dsAUDIO_DUCKINGACTION_STOP;
      dsAudioDuckingType_t   type   = relative ? dsAUDIO_DUCKINGTYPE_RELATIVE : dsAUDIO_DUCKINGTYPE_ABSOLUTE;

      device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, type, level);

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
   return(true);
}
