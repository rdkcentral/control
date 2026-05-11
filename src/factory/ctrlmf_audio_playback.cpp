#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>
#include <ctrlmf_audio_playback.h>
#include <ctrlmf_thunder_plugin_system_audio_player.h>
#include <unistd.h>


typedef struct {
   Thunder::SystemAudioPlayer::ctrlm_thunder_plugin_system_audio_player_t *obj_sap;
   bool                        initialized;
} ctrlmf_audio_play_global_t;

static void ctrlmf_audio_playback_event_handler(system_audio_player_event_t event, void *user_data);

static ctrlmf_audio_play_global_t g_audio_play;

bool ctrlmf_audio_playback_init(void) {
   g_audio_play.obj_sap = new Thunder::SystemAudioPlayer::ctrlm_thunder_plugin_system_audio_player_t;

   if(g_audio_play.obj_sap == NULL) {
      XLOGD_ERROR("out of memory");
      return(false);
   }

   g_audio_play.obj_sap->add_event_handler(ctrlmf_audio_playback_event_handler, NULL);

   bool sap_opened = g_audio_play.obj_sap->open(SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_WAV, SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_FILE, SYSTEM_AUDIO_PLAYER_PLAY_MODE_SYSTEM);
   if(!sap_opened) {
      XLOGD_ERROR("unable to open system audio player");
      delete g_audio_play.obj_sap;
      g_audio_play.obj_sap = NULL;
      return(false);
   }
   g_audio_play.initialized = true;
   return(true);
}

bool ctrlmf_audio_playback_term(void) {
   if(g_audio_play.obj_sap != NULL) {
      if(!g_audio_play.obj_sap->close()) {
          XLOGD_WARN("unable to close system audio player");
      }

      delete g_audio_play.obj_sap;
      g_audio_play.obj_sap = NULL;
   }
   g_audio_play.initialized = false;
   return(true);
}

// Play an audio file using system audio player
bool ctrlmf_audio_playback_start(const char *filename) {
   if(!g_audio_play.initialized) {
      XLOGD_ERROR("not initialized");
      return(false);
   }

   // TODO check to make sure the file exists
   XLOGD_INFO("filename <%s>", filename);

   // Set to a pre-defined volume level
   const char *volume_primary = "80";
   const char *volume_player  = "100";
   if(!g_audio_play.obj_sap->setMixerLevels(volume_primary, volume_player)) {
      XLOGD_ERROR("unable to set mixer levels - primary <%s> player <%s>", volume_primary, volume_player);
   }
   
   char url[256];
   snprintf(url, sizeof(url), "file://%s", filename);
   
   if(!g_audio_play.obj_sap->play(url)) {
      XLOGD_ERROR("unable to play file <%s>", url);
      return(false);
   }
   
   return(true);
}

void ctrlmf_audio_playback_event_handler(system_audio_player_event_t event, void *user_data) {
   switch(event) {
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_STARTED: {
         XLOGD_INFO("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_STARTED");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_FINISHED: {
         XLOGD_INFO("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_FINISHED");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_PAUSED: {
         XLOGD_INFO("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_PAUSED");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_RESUMED: {
         XLOGD_INFO("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_RESUMED");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA");
         break;
      }
      default: {
         XLOGD_ERROR("INVALID EVENT");
         break;
      }
   }
}
