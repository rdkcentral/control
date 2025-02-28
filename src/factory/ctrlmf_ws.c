#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <nopoll.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <safec_lib.h>
#include <ctrlmf_ws.h>
#include <ctrlm_fta_platform.h>

#ifdef CTRLMF_WSS_ENABLED
#include <openssl/pkcs12.h>
#define CTRLMF_WS_CIPHER_LIST       "AES256-SHA256:AES128-GCM-SHA256:AES128-SHA256"
#define CTRLMF_WS_TLS_CERT_KEY_FILE "/tmp/serverXXXXXX"
#define CTRLMF_WS_CERT_NAME_LEN     (1024)
#define CTRLMF_WS_CERT_PW_LEN       (128)
#endif

typedef enum {
   CTRLMF_WS_MSG_TYPE_TEXT    = 0,
   CTRLMF_WS_MSG_TYPE_BINARY  = 1,
   CTRLMF_WS_MSG_TYPE_UNKNOWN = 2
} ctrlmf_ws_msg_type_t;

typedef struct {
   sem_t *                semaphore;
   uint16_t               port;
   bool                   log_enable;
   ctrlmf_audio_frame_t * audio_frames;
   uint32_t *             audio_frame_qty;
   uint32_t               audio_frame_size;
   ctrlmf_ws_callbacks_t *callbacks;
} ctrlmf_ws_thread_params_t;

typedef struct {
   noPollConn *          nopoll_conn;
   ctrlmf_audio_frame_t *audio_frames;
   uint32_t *            audio_frame_qty;
   uint32_t              audio_frame_size;
   uint32_t              audio_byte_cnt;
   uint32_t              audio_byte_total;
   uint8_t *             audio_byte_ptr;
   ctrlmf_ws_callbacks_t callbacks;
} ctrlmf_ws_thread_state_t;

typedef struct {
   pthread_t            thread_id;
   noPollCtx *          nopoll_ctx;
   ctrlmf_audio_frame_t audio_frames;
   uint32_t             audio_frame_qty;
} ctrlmf_ws_global_t;

static void *ctrlmf_ws_main(void *param);

static nopoll_bool ctrlmf_ws_on_accept(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);
static nopoll_bool ctrlmf_ws_on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);
static void        ctrlmf_ws_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data);
static void        ctrlmf_ws_on_ping(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data);
static void        ctrlmf_ws_on_close(noPollCtx  *ctx, noPollConn *conn, noPollPtr user_data);
static void        ctrlmf_ws_nopoll_log(noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);
#ifdef CTRLMF_WSS_ENABLED
static bool        ctrlmf_ws_cert_config(FILE *cert_key_fp);
static bool        ctrlmf_ws_add_chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs);
#endif

ctrlmf_ws_global_t g_ctrlmf_ws;

bool ctrlmf_ws_init(uint32_t audio_frame_size, uint16_t port, bool log_enable, ctrlmf_ws_callbacks_t *callbacks) {
   ctrlmf_ws_thread_params_t params;

   sem_t semaphore;
   sem_init(&semaphore, 0, 0);
   
   // Launch thread
   params.semaphore = &semaphore;

   params.port             = port;
   params.log_enable       = log_enable;
   params.audio_frames     = &g_ctrlmf_ws.audio_frames;
   params.audio_frame_qty  = &g_ctrlmf_ws.audio_frame_qty;
   params.audio_frame_size = audio_frame_size;
   params.callbacks        = callbacks;

   g_ctrlmf_ws.nopoll_ctx      = NULL;
   g_ctrlmf_ws.audio_frames    = NULL;
   g_ctrlmf_ws.audio_frame_qty = 0;
   
   if(0 != pthread_create(&g_ctrlmf_ws.thread_id, NULL, ctrlmf_ws_main, &params)) {
      XLOGD_ERROR("unable to launch thread");
      return(false);
   }

   // Block until initialization is complete or a timeout occurs
   XLOGD_INFO("Waiting for thread initialization...");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(true);
}

bool ctrlmf_ws_capture_set(ctrlmf_audio_frame_t audio_frames, uint32_t audio_frame_qty) {
   g_ctrlmf_ws.audio_frames    = audio_frames;
   g_ctrlmf_ws.audio_frame_qty = audio_frame_qty;
   return(true);
}

void ctrlmf_ws_term(void) {
   if(g_ctrlmf_ws.nopoll_ctx != NULL) {
      nopoll_loop_stop(g_ctrlmf_ws.nopoll_ctx);
   }

   // Wait for thread to exit
   XLOGD_INFO("Waiting for thread to exit");
   void *retval = NULL;
   pthread_join(g_ctrlmf_ws.thread_id, &retval);
   XLOGD_INFO("thread exited.");
}

void *ctrlmf_ws_main(void *param) {
   ctrlmf_ws_thread_params_t params = *((ctrlmf_ws_thread_params_t *)param);
   errno_t safec_rc = -1;

   if(params.audio_frames == NULL || params.audio_frame_qty == NULL || params.callbacks == NULL) {
      XLOGD_ERROR("invalid params");
      return(NULL);
   }
   ctrlmf_ws_thread_state_t state;
   
   state.audio_frames     = params.audio_frames;
   state.audio_frame_qty  = params.audio_frame_qty;
   state.audio_frame_size = params.audio_frame_size;
   state.audio_byte_cnt   = 0;
   state.audio_byte_ptr   = (uint8_t *)params.audio_frames;
   state.audio_byte_total = *params.audio_frame_qty * state.audio_frame_size;
   state.callbacks        = *params.callbacks;

   g_ctrlmf_ws.nopoll_ctx = nopoll_ctx_new();
   if(g_ctrlmf_ws.nopoll_ctx == NULL) {
      XLOGD_ERROR("nopoll context create");
      return(NULL);
   }

   #ifdef CTRLMF_WSS_ENABLED
   int cert_key_fd   = -1;
   FILE *cert_key_fp = NULL;
   char tmp_cert[32] = {0};
   int err_store;

   safec_rc = sprintf_s(tmp_cert, sizeof(tmp_cert), "%s", CTRLMF_WS_TLS_CERT_KEY_FILE);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   umask(0600);
   cert_key_fd = mkstemp(tmp_cert);
   if (cert_key_fd == -1)
   {
      err_store = errno;
      XLOGD_ERROR("mkstemp failed: <%s>", strerror(err_store));
      return(NULL);
   }

   cert_key_fp = fdopen(cert_key_fd, "w");
   if(cert_key_fp == NULL) {
      err_store = errno;
      XLOGD_ERROR("fdopen failed: <%s>", strerror(err_store));
      if(0 != unlink(&tmp_cert[0])) {
         err_store = errno;
         XLOGD_ERROR("failed to remove temp cert <%s>", strerror(err_store));
      }
      return(NULL);
   }

   if(!ctrlmf_ws_cert_config(cert_key_fp)) {
      XLOGD_ERROR("failed to set cert or key, exit");
      fclose(cert_key_fp);
      if(0 != unlink(&tmp_cert[0])) {
         err_store = errno;
         XLOGD_ERROR("failed to remove temp cert <%s>", strerror(err_store));
      }
      return(NULL);
   }
   fclose(cert_key_fp);

   // Init OpenSSL
   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();
   #endif
   
   if(params.log_enable) {
      nopoll_log_enable(g_ctrlmf_ws.nopoll_ctx, nopoll_true);
      nopoll_log_set_handler(g_ctrlmf_ws.nopoll_ctx, ctrlmf_ws_nopoll_log, NULL);
   }
   noPollConnOpts *opts = nopoll_conn_opts_new();
   nopoll_ctx_set_on_accept(g_ctrlmf_ws.nopoll_ctx, ctrlmf_ws_on_accept, &state);
   nopoll_ctx_set_on_ready(g_ctrlmf_ws.nopoll_ctx, ctrlmf_ws_on_ready, &state);
   nopoll_ctx_set_on_msg(g_ctrlmf_ws.nopoll_ctx, ctrlmf_ws_on_message, &state);
   
   #ifdef CTRLMF_WSS_ENABLED
   nopoll_conn_opts_set_ssl_protocol(opts, NOPOLL_METHOD_TLSV1_2);
   nopoll_conn_opts_ssl_host_verify(opts, nopoll_false); //localhost will not match host specified in certificate

   if(!nopoll_conn_opts_set_ssl_certs(opts, &tmp_cert[0], &tmp_cert[0], NULL, NULL)) {
      XLOGD_ERROR("Failed to add cert/key files to nopoll_conn");
      nopoll_ctx_unref(g_ctrlmf_ws.nopoll_ctx);
      nopoll_conn_opts_free(opts);
      return(NULL);
   }
   #endif

   char port[6];
   safec_rc = sprintf_s(port, sizeof(port), "%u", params.port);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   // Start IPv4/6 listener
   #ifdef CTRLMF_WSS_ENABLED
   state.nopoll_conn = nopoll_listener_tls_new_opts6(g_ctrlmf_ws.nopoll_ctx, opts, "::", port);
   #else
   state.nopoll_conn = nopoll_listener_new_opts6(g_ctrlmf_ws.nopoll_ctx, opts, "::", port);
   #endif
   if(!nopoll_conn_is_ok(state.nopoll_conn)) {
      XLOGD_ERROR("Listener connection IPv6 NOT ok");
      nopoll_ctx_unref(g_ctrlmf_ws.nopoll_ctx);
      g_ctrlmf_ws.nopoll_ctx = NULL;
      return(NULL);
   }

   // Unblock the caller that launched this thread
   sem_post(params.semaphore);
   params.semaphore = NULL;

   XLOGD_INFO("Enter main loop");

   nopoll_loop_wait(g_ctrlmf_ws.nopoll_ctx, 0);

   nopoll_conn_opts_unref(opts);
   nopoll_conn_unref(state.nopoll_conn);
   nopoll_ctx_unref(g_ctrlmf_ws.nopoll_ctx);
   g_ctrlmf_ws.nopoll_ctx = NULL;

   #ifdef CTRLMF_WSS_ENABLED
   if(0 != unlink(tmp_cert)) {
      int err_store = errno;
      XLOGD_ERROR("failed to remove temp cert <%s>", strerror(err_store));
   }
   #endif

   return(NULL);
}

nopoll_bool ctrlmf_ws_on_accept(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlmf_ws_thread_state_t *state = (ctrlmf_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return(nopoll_false);
   }

   state->audio_byte_cnt   = 0;
   state->audio_byte_ptr   = (uint8_t *)(*state->audio_frames);
   state->audio_byte_total = (*state->audio_frame_qty * state->audio_frame_size);

   // Set ping handler
   nopoll_conn_set_on_ping_msg(conn, ctrlmf_ws_on_ping, NULL);

   return(nopoll_true);
}

nopoll_bool ctrlmf_ws_on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlmf_ws_thread_state_t *state = (ctrlmf_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return(nopoll_false);
   }

   XLOGD_INFO("Connection established");
   
   if(state->callbacks.connected != NULL) {
      (*state->callbacks.connected)(state->callbacks.data);
   }
   
   nopoll_conn_set_on_close(conn, ctrlmf_ws_on_close, user_data);

   return(nopoll_true);
}

void ctrlmf_ws_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data) {
   ctrlmf_ws_thread_state_t *state = (ctrlmf_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   int payload_size = nopoll_msg_get_payload_size(msg);
   const unsigned char *payload = nopoll_msg_get_payload(msg);
   
   switch(nopoll_msg_opcode(msg)) {
      case NOPOLL_TEXT_FRAME: {
         XLOGD_INFO("NOPOLL_TEXT_FRAME");
         break;
      }
      case NOPOLL_BINARY_FRAME: {
         XLOGD_INFO("NOPOLL_BINARY_FRAME size <%d>", payload_size);
         if(state->audio_byte_cnt < state->audio_byte_total) {
            if(state->audio_byte_ptr != NULL) {
               if((state->audio_byte_cnt + payload_size) > state->audio_byte_total) {
                  memcpy(state->audio_byte_ptr, payload, (state->audio_byte_total - state->audio_byte_cnt));
                  state->audio_byte_cnt = state->audio_byte_total;
               } else {
                  memcpy(state->audio_byte_ptr, payload, payload_size);
                  state->audio_byte_ptr += payload_size;
                  state->audio_byte_cnt += payload_size;
               }
               if(state->audio_byte_cnt >= state->audio_byte_total) {
                  state->audio_byte_ptr = NULL;
               }
            }
         }
         if(state->audio_byte_cnt >= state->audio_byte_total) {
            const char *reason = "audio buffer filled";
            nopoll_conn_close_ext(conn, 1000, reason, strlen(reason));
         }
         break;
      }
      case NOPOLL_CONTINUATION_FRAME: {
         XLOGD_INFO("NOPOLL_CONTINUATION_FRAME");
         break;
      }
      default: {
         XLOGD_INFO("NOPOLL_UNKNOWN");
         break;
      }
   }
}

void ctrlmf_ws_on_ping(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data) {
   ctrlmf_ws_thread_state_t *state = (ctrlmf_ws_thread_state_t *)user_data;
   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   XLOGD_INFO("Ping received");
   // Do nothing, we don't care about this event
}

void ctrlmf_ws_on_close(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlmf_ws_thread_state_t *state = (ctrlmf_ws_thread_state_t *)user_data;
   
   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   XLOGD_INFO("");

   if(state->callbacks.disconnected != NULL) {
      (*state->callbacks.disconnected)(state->callbacks.data);
   }
}


#ifdef CTRLMF_WSS_ENABLED
bool ctrlmf_ws_cert_config(FILE* cert_key_fp) {

   bool ret = false;
   ctrlm_fta_platform_cert_info_t *cert_info = NULL;
   do {
      FILE *device_cert_fp             = NULL;
      PKCS12 *p12_cert                 = NULL;
      EVP_PKEY *pkey                   = NULL;
      X509 *x509_cert                  = NULL;
      STACK_OF(X509) *additional_certs = NULL;

      cert_info = ctrlm_fta_platform_cert_info_get(false);
      if(cert_info == NULL) {
         XLOGD_ERROR("unable to get certificate info");
         break;
      }

      if(cert_info->type != CTRLM_FTA_PLATFORM_VOICE_CERT_TYPE_P12) {
         XLOGD_ERROR("unable to parse certificates that are not of PKCS12 type");
         break;
      }

      device_cert_fp = fopen(cert_info->filename, "rb");
      if(device_cert_fp == NULL) {
         XLOGD_ERROR("unable to open P12 certificate");
         break;
      }

      d2i_PKCS12_fp(device_cert_fp, &p12_cert);
      fclose(device_cert_fp);
      device_cert_fp = NULL;

      if(p12_cert == NULL) {
         XLOGD_ERROR("unable to read P12 certificate");
         break;
      }

      if(1 != PKCS12_parse(p12_cert, cert_info->password, &pkey, &x509_cert, &additional_certs)) {
         XLOGD_ERROR("unable to parse P12 certificate");
         break;
      }

      if(1 != PEM_write_X509(cert_key_fp, x509_cert)) {
         XLOGD_ERROR("failed to write temp cert");
         break;
      }

      if(!ctrlmf_ws_add_chain(cert_key_fp, additional_certs)) {
         XLOGD_ERROR("failed to add chain certs");
         break;
      }

      if(1 != PEM_write_PrivateKey(cert_key_fp, pkey, NULL, (unsigned char*)cert_info->password, strlen(cert_info->password), NULL, NULL)) {
         XLOGD_ERROR("failed to write temp key");
         break;
       }

      ret = true;
   }while(0);

   if(cert_info != NULL) {
      ctrlm_fta_platform_cert_info_free(cert_info);
   }

   return ret;
}

bool ctrlmf_ws_add_chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs) {
   if(cert_key_fp == NULL) {
      XLOGD_ERROR("null file pointer");
      return false;
   }
   if(additional_certs == NULL) {
      XLOGD_ERROR("null certs");
      return false;
   }

   for(uint32_t index = 0; index < sk_X509_num(additional_certs); index++) {
      X509 *cert = sk_X509_value(additional_certs, index);
      if(1 != PEM_write_X509(cert_key_fp, cert)) {
         XLOGD_ERROR("failed to write temp cert index %d", index);
         return false;
      }
   }

   return true;
}
#endif

void ctrlmf_ws_nopoll_log(noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data) {
   xlog_args_t args;
   args.options  = XLOG_OPTS_DEFAULT;
   args.color    = XLOG_COLOR_NONE;
   args.function = XLOG_FUNCTION_NONE;
   args.line     = XLOG_LINE_NONE;
   args.id       = XLOG_MODULE_ID;
   args.size_max = XLOG_BUF_SIZE_DEFAULT;
   switch(level) {
      case NOPOLL_LEVEL_DEBUG:    { args.level = XLOG_LEVEL_DEBUG; break; }
      case NOPOLL_LEVEL_INFO:     { args.level = XLOG_LEVEL_INFO;  break; }
      case NOPOLL_LEVEL_WARNING:  { args.level = XLOG_LEVEL_WARN;  break; }
      case NOPOLL_LEVEL_CRITICAL: { args.level = XLOG_LEVEL_ERROR; break; }
      default:                    { args.level = XLOG_LEVEL_INFO;  break; }
   }
   int errsv = errno;
   xlog_printf(&args, "%s", log_msg);
   errno = errsv;
}
