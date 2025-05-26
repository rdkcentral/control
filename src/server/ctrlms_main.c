#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <argp.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlms_version.h>
#include <ctrlms_ws.h>

#define CTRLMS_VERSION "1.0"

#define CTRLMS_WS_PORT_INT (9881)

typedef struct {
   bool     silent;
   bool     verbose;
} ctrlms_options_t;

static bool          ctrlms_cmdline_args(int argc, char *argv[]);
static error_t       ctrlms_parse_opt(int key, char *arg, struct argp_state *state);

const char *argp_program_version     =  "controlServer " CTRLMS_VERSION;
const char *argp_program_bug_address = "<david_wolaver@cable.comcast.com>";

static char doc[] = "controlServer -- a server application";

static char args_doc[] = "";

static struct argp_option options[] = {
   {"verbose",           'v', 0,            0,  "Produce verbose output" },
   {"quiet",             'q', 0,            0,  "Don't produce any output" },
   { 0 }
};

static struct argp argp = { options, ctrlms_parse_opt, args_doc, doc };

static ctrlms_options_t g_ctrlms_opts = { .silent            = false,
                                          .verbose           = false
                                        };

int main(int argc, char* argv[]) {
   // Parse command line arguments
   if(!ctrlms_cmdline_args(argc, argv)) {
      return(-1);
   }

   xlog_level_t level = XLOG_LEVEL_WARN;
   if(g_ctrlms_opts.silent) {
      level = XLOG_LEVEL_ERROR;
   } else if(g_ctrlms_opts.verbose) {
      level = XLOG_LEVEL_INFO;
   // TODO Add option to allow debug level logging
   //} else if() {
   //   level = XLOG_LEVEL_DEBUG;
   }

   if(!ctrlms_init(level)) {
      XLOGD_ERROR("ctrlms_main: init failed");
   } else {

      // TODO Start listening for connections
      if(!ctrlms_ws_init(CTRLMS_WS_PORT_INT, true)) {
         XLOGD_ERROR("ctrlms_main: ws init failed");
      } else {
         ctrlms_ws_listen();
         ctrlms_ws_term();
      }
      XLOGD_INFO("ctrlms_main: main loop ended");
   }
   ctrlms_term();
   XLOGD_INFO("ctrlms_main: return");

   return(0);
}

error_t ctrlms_parse_opt(int key, char *arg, struct argp_state *state) {
   // Get the input argument from argp_parse, which we know is a pointer to our arguments structure.
   ctrlms_options_t *arguments = state->input;

   switch(key) {
      case 'q': {
         arguments->silent  = true;
         break;
      }
      case 'v': {
         arguments->verbose = true;
         break;
      }
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

bool ctrlms_cmdline_args(int argc, char *argv[]) {
   argp_parse(&argp, argc, argv, 0, 0, &g_ctrlms_opts);

   #if 0
   if() { // Nothing to do
      printf("Invalid options specified. Try 'controlServer --help' or 'controlServer --usage' for more information.\n");
      return(false);
   }
   #endif

   XLOGD_INFO("verbose          <%s>", g_ctrlms_opts.verbose          ? "YES" : "NO");
   XLOGD_INFO("silent           <%s>", g_ctrlms_opts.silent           ? "YES" : "NO");

   return(true);
}
