#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <argp.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlm_fta_lib.h>

#define CTRLMF_VERSION "1.0"

typedef struct {
   bool     silent;
   bool     verbose;
   bool     factory_reset;
   bool     ctrlm_restart;
   char *   audio_file_path;
   char *   output_file_path;
   bool     mic_test_factory;
   uint32_t mic_test_duration;
   uint32_t mic_test_level;
   double  *mic_test_snr_min;
   double  *mic_test_snr_max;
   double  *mic_test_snr_var;
   double   snr_min;
   double   snr_max;
   double   snr_var;
   bool     mute_main_audio;
} ctrlmf_options_t;

static bool          ctrlmf_cmdline_args(int argc, char *argv[]);
static error_t       ctrlmf_parse_opt(int key, char *arg, struct argp_state *state);

const char *argp_program_version     =  "controlFactory " CTRLMF_VERSION;
const char *argp_program_bug_address = "<david_wolaver@cable.comcast.com>";

static char doc[] = "controlFactory -- a factory test application";

static char args_doc[] = "";

static struct argp_option options[] = {
   {"verbose",           'v', 0,            0,  "Produce verbose output" },
   {"quiet",             'q', 0,            0,  "Don't produce any output" },
   {"factory-reset",     'F', 0,            0,  "Perform control manager factory reset and restart the application." },
   {"soft-factory-reset",'f', 0,            0,  "Perform control manager factory reset" },
   {"ctrlm-restart",     'r', 0,            0,  "Restart the control manager application" },
   #ifdef CTRLMF_LOCAL_MIC
   #ifdef CTRLMF_AUDIO_PLAYBACK
   {"mic-test-audio",    'a', "<filename>", 0,  "Microphone test audio file" },
   #endif
   {"mic-test-factory",  'd', 0,            0,  "Factory microphone test" },
   {"mic-test-duration", 'u', "<N>",        0,  "Microphone test duration in milliseconds" },
   {"mic-test-snr-min",  'x', "<F>",        0,  "Microphone test SNR minimum value" },
   {"mic-test-snr-max",  'y', "<F>",        0,  "Microphone test SNR maximum value" },
   {"mic-test-snr-var",  'z', "<F>",        0,  "Microphone test SNR maximum variance" },
   {"mic-test-output",   'g', "<filename>", 0,  "Microphone test output filename" },
   {"mic-test-level",    'l', "<N>",        0,  "Microphone test level" },
   #endif
   #ifdef CTRLMF_AUDIO_CONTROL
   {"mute-main-audio",   'm', 0,            0,  "Mute the main audio output" },
   #endif
   { 0 }
};

static struct argp argp = { options, ctrlmf_parse_opt, args_doc, doc };

static ctrlmf_options_t g_ctrlmf_opts = { .silent            = false,
                                          .verbose           = false,
                                          .factory_reset     = false,
                                          .ctrlm_restart     = false,
                                          .audio_file_path   = NULL,
                                          .output_file_path  = NULL,
                                          .mic_test_factory  = false,
                                          .mic_test_duration = CTRLMF_DURATION,
                                          .mic_test_snr_min  = &g_ctrlmf_opts.snr_min,
                                          .mic_test_snr_max  = &g_ctrlmf_opts.snr_max,
                                          .mic_test_snr_var  = &g_ctrlmf_opts.snr_var,
                                          .mic_test_level    = 0,
                                          .snr_min           = CTRLMF_SNR_MIN,
                                          .snr_max           = CTRLMF_SNR_MAX,
                                          .snr_var           = CTRLMF_SNR_VAR,
                                          .mute_main_audio   = false
                                        };

int main(int argc, char* argv[]) {
   // Parse command line arguments
   if(!ctrlmf_cmdline_args(argc, argv)) {
      return(-1);
   }

   xlog_level_t level = XLOG_LEVEL_WARN;
   if(g_ctrlmf_opts.silent) {
      level = XLOG_LEVEL_ERROR;
   } else if(g_ctrlmf_opts.verbose) {
      level = XLOG_LEVEL_INFO;
   // TODO Add option to allow debug level logging
   //} else if() {
   //   level = XLOG_LEVEL_DEBUG;
   }

   bool requires_audio_playback = (g_ctrlmf_opts.audio_file_path != NULL) ? true : false;

   if(!ctrlmf_init(level, requires_audio_playback)) {
      XLOGD_ERROR("ctrlmf_main: init failed");
   } else {
      XLOGD_INFO("ctrlmf_main: Run main loop");
      if(g_ctrlmf_opts.factory_reset) {
         ctrlmf_factory_reset();
      }
      if(g_ctrlmf_opts.ctrlm_restart) {
         ctrlmf_systemd_service_exec("ctrlm-main.service", CTRLMF_SYSTEMD_METHOD_RESTART);
      }
      #ifdef CTRLMF_AUDIO_CONTROL
      if(g_ctrlmf_opts.mute_main_audio) {
         ctrlmf_audio_control_mute(true);
      }
      #endif
      if(g_ctrlmf_opts.mic_test_factory) {
         #ifdef CTRLMF_LOCAL_MIC
         ctrlmf_test_result_t test_result;
         if(!ctrlmf_mic_test_factory(g_ctrlmf_opts.mic_test_duration, g_ctrlmf_opts.output_file_path, g_ctrlmf_opts.mic_test_level, g_ctrlmf_opts.audio_file_path, g_ctrlmf_opts.mic_test_snr_min, g_ctrlmf_opts.mic_test_snr_max, g_ctrlmf_opts.mic_test_snr_var, &test_result)) {
            XLOGD_ERROR("ctrlmf_main: mic test failed");
         } else {
            XLOGD_INFO("ctrlmf_main: test result <%s>", test_result.pass ? "PASS" : "FAIL");
         }
         #endif
      } else if(g_ctrlmf_opts.audio_file_path != NULL) {
         #ifdef CTRLMF_AUDIO_PLAYBACK
         ctrlmf_audio_playback_start(g_ctrlmf_opts.audio_file_path);
         #endif
      }
      #ifdef CTRLMF_AUDIO_CONTROL
      if(g_ctrlmf_opts.mute_main_audio) {
         ctrlmf_audio_control_mute(false);
      }
      #endif

      XLOGD_INFO("ctrlmf_main: main loop ended");
   }
   ctrlmf_term();
   XLOGD_INFO("ctrlmf_main: return");

   return(0);
}

error_t ctrlmf_parse_opt(int key, char *arg, struct argp_state *state) {
   // Get the input argument from argp_parse, which we know is a pointer to our arguments structure.
   ctrlmf_options_t *arguments = state->input;

   switch(key) {
      case 'q': {
         arguments->silent  = true;
         break;
      }
      case 'v': {
         arguments->verbose = true;
         break;
      }
      case 'F': {
         arguments->factory_reset = true;
         arguments->ctrlm_restart = true;
         break;
      }
      case 'f': {
         arguments->factory_reset = true;
         break;
      }
      case 'r': {
         arguments->ctrlm_restart = true;
         break;
      }
      #ifdef CTRLMF_LOCAL_MIC
      #ifdef CTRLMF_AUDIO_PLAYBACK
      case 'a': {
         XLOGD_INFO("mic test audio file <%s>", arg);
         arguments->audio_file_path = arg;
         break;
      }
      #endif
      case 'g': {
         XLOGD_INFO("output file path <%s>", arg);
         arguments->output_file_path = arg;
         break;
      }
      case 'd': {
         arguments->mic_test_factory = true;
         break;
      }
      case 'u': {
         int duration = atoi(arg);
         if(duration <= 0) {
            XLOGD_ERROR("mic test duration invalid <%s>", arg);
            return(ARGP_ERR_UNKNOWN);
         } else {
            arguments->mic_test_duration = duration;
         }
         break;
      }
      case 'l': {
         int level = atoi(arg);
         if(level < 0) {
            XLOGD_ERROR("mic test level invalid <%s>", arg);
            return(ARGP_ERR_UNKNOWN);
         } else {
            arguments->mic_test_level = level;
         }
         break;
      }
      case 'x': {
         arguments->snr_min = atof(arg);
         arguments->mic_test_snr_min = &arguments->snr_min;
         break;
      }
      case 'y': {
         arguments->snr_max = atof(arg);
         arguments->mic_test_snr_max = &arguments->snr_max;
         break;
      }
      case 'z': {
         arguments->snr_var = atof(arg);
         arguments->mic_test_snr_var = &arguments->snr_var;
         break;
      }
      #endif
      #ifdef CTRLMF_AUDIO_CONTROL
      case 'm': {
         arguments->mute_main_audio = true;
         break;
      }
      #endif
      case ARGP_KEY_ARG: {
         argp_usage(state);
         return(ARGP_ERR_UNKNOWN);
      }
      case ARGP_KEY_END: {
         break;
      }
      default: {
         return(ARGP_ERR_UNKNOWN);
      }
   }

   return(0);
}

bool ctrlmf_cmdline_args(int argc, char *argv[]) {
   argp_parse(&argp, argc, argv, 0, 0, &g_ctrlmf_opts);

   if(!g_ctrlmf_opts.factory_reset && !g_ctrlmf_opts.ctrlm_restart && !g_ctrlmf_opts.mic_test_factory) { // Nothing to do
      printf("Invalid options specified. Try 'controlFactory --help' or 'controlFactory --usage' for more information.\n");
      return(false);
   }

   XLOGD_INFO("verbose          <%s>", g_ctrlmf_opts.verbose          ? "YES" : "NO");
   XLOGD_INFO("silent           <%s>", g_ctrlmf_opts.silent           ? "YES" : "NO");
   XLOGD_INFO("factory reset    <%s>", g_ctrlmf_opts.factory_reset    ? "YES" : "NO");
   XLOGD_INFO("ctrlm restart    <%s>", g_ctrlmf_opts.ctrlm_restart    ? "YES" : "NO");
   XLOGD_INFO("audio file       <%s>", g_ctrlmf_opts.audio_file_path  ? g_ctrlmf_opts.audio_file_path  : "NULL");
   XLOGD_INFO("output file path <%s>", g_ctrlmf_opts.output_file_path ? g_ctrlmf_opts.output_file_path : "NULL");
   #ifdef CTRLMF_LOCAL_MIC
   if(g_ctrlmf_opts.mic_test_factory) {
      XLOGD_INFO("mic test duration <%d ms> snr min <%f> max <%f> var <%f>", g_ctrlmf_opts.mic_test_duration, *g_ctrlmf_opts.mic_test_snr_min, *g_ctrlmf_opts.mic_test_snr_max, *g_ctrlmf_opts.mic_test_snr_var);
   }
   #endif
   #ifdef CTRLMF_AUDIO_CONTROL
   XLOGD_INFO("mute main audio <%s>", g_ctrlmf_opts.mute_main_audio   ? "YES" : "NO");
   #endif

   return(true);
}
