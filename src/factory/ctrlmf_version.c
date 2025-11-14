#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ctrlm_log.h>
#include <dlfcn.h>
#include <rdkx_logger.h>
#include <rdkversion.h>
#include <ctrlmf_utils.h>
#include <ctrlmf_audio_playback.h>
#include <ctrlmf_audio_capture.h>
#include <ctrlm_fta_lib.h>

typedef struct {
   bool  initialized;
   void *handle_audio_analysis;
   bool  audio_control_init;
   bool  audio_playback_init;
   bool  is_production;
} ctrlmf_global_t;

ctrlmf_global_t g_ctrlmf = {
   .initialized           = false,
   .handle_audio_analysis = NULL,
   .audio_control_init    = false,
   .audio_playback_init   = false,
   .is_production         = true
};

static bool  ctrlmf_file_exists(const char *filename);
static void *ctrlmf_load_plugin_audio_analysis(ctrlmf_mic_test_audio_analyze_t *audio_analyze_func);

bool ctrlmf_init(xlog_level_t level, bool requires_audio_playback, ctrlmf_mic_test_audio_analyze_t *audio_analyze_func) {
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

   if(requires_audio_playback && !ctrlmf_audio_playback_init()) {
      XLOGD_ERROR("failed to init audio playback");
      #ifdef CTRLMF_AUDIO_CONTROL
      ctrlmf_audio_control_term();
      #endif
      return(false);
   }

   g_ctrlmf.handle_audio_analysis = ctrlmf_load_plugin_audio_analysis(&audio_analyze_func);
   g_ctrlmf.audio_control_init    = true;
   g_ctrlmf.audio_playback_init   = requires_audio_playback;
   g_ctrlmf.initialized           = true;
   return(true);
}

void ctrlmf_term(void) {

   if(g_ctrlmf.audio_playback_init) {
      ctrlmf_audio_playback_term();
   }

   #ifdef CTRLMF_AUDIO_CONTROL
   if(g_ctrlmf.audio_control_init) {
      ctrlmf_audio_control_term();
   }
   #endif

   if(g_ctrlmf.handle_audio_analysis != NULL) {
      dlclose(g_ctrlmf.handle_audio_analysis);
      g_ctrlmf.handle_audio_analysis = NULL;
   }

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

bool ctrlmf_file_exists(const char *filename) {
   if(filename == NULL) {
      return false;
   }
   struct stat buffer;
   if(stat(filename, &buffer) == 0) {
      return true;
   }
   return false;
}

void *ctrlmf_load_plugin_audio_analysis(ctrlmf_mic_test_audio_analyze_t *audio_analyze_func) {
   if(audio_analyze_func == NULL) {
      return(NULL);
   }
   void *handle = NULL;

   const char *so_path_vd = "/vendor/lib/libctrlmf_audio_analysis.so";
   const char *so_path_mw = "/usr/lib/libctrlmf_audio_analysis.so";
   if(ctrlmf_file_exists(so_path_vd)) {
      handle = dlopen(so_path_vd, RTLD_NOW);
   } else if(ctrlmf_file_exists(so_path_mw)) {
      handle = dlopen(so_path_mw, RTLD_NOW);
   } else {
      XLOGD_INFO("Audio Analysis plugin is not present.");
      return(NULL);
   }

   if(NULL == handle) {
      XLOGD_ERROR("Failed to load Audio Analysis plugin <%s>", dlerror());
      return(NULL);
   }

   dlerror();  // Clear any existing error

   *audio_analyze_func = (ctrlmf_mic_test_audio_analyze_t)dlsym(handle, "ctrlmf_mic_test_audio_analyze");
   char *error = dlerror();

   if(error != NULL) {
      XLOGD_ERROR("Failed to find plugin method (ctrlmf_mic_test_audio_analyze), error <%s>", error);
      dlclose(handle);
      return(NULL);
   }

   XLOGD_INFO("Audio Analysis plugin is loaded.");
   
   return(handle);
}
