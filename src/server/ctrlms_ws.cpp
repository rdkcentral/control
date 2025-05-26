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
#include <dlfcn.h>
#include <jansson.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <safec_lib.h>
#include <ctrlms_ws.h>
#include <ctrlm_server_platform.h>
#include <ctrlm_server_app.h>

#ifdef CTRLMS_WSS_ENABLED
#include <openssl/pkcs12.h>
#define CTRLMS_WS_CIPHER_LIST       "AES256-SHA256:AES128-GCM-SHA256:AES128-SHA256"
#define CTRLMS_WS_TLS_CERT_KEY_FILE "/tmp/serverXXXXXX"
#define CTRLMS_WS_CERT_NAME_LEN     (1024)
#define CTRLMS_WS_CERT_PW_LEN       (128)
#endif

typedef enum {
   CTRLMS_WS_MSG_TYPE_TEXT    = 0,
   CTRLMS_WS_MSG_TYPE_BINARY  = 1,
   CTRLMS_WS_MSG_TYPE_UNKNOWN = 2
} ctrlms_ws_msg_type_t;

typedef struct {
   sem_t *                semaphore;
   uint16_t               port;
   bool                   log_enable;
} ctrlms_ws_thread_params_t;

typedef struct {
   noPollConn *              nopoll_conn;
   void *                    app_handle;
   ctrlms_app_interface_t   *app_interface;
} ctrlms_ws_thread_state_t;

typedef struct {
   pthread_t            thread_id;
   noPollCtx *          nopoll_ctx;
} ctrlms_ws_global_t;

static void *ctrlms_ws_main(void *param);
static void *ctrlms_ws_load_app(ctrlms_ws_thread_state_t *state);

static nopoll_bool ctrlms_ws_on_accept(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);
static nopoll_bool ctrlms_ws_on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);
static void        ctrlms_ws_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data);
static void        ctrlms_ws_on_ping(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data);
static void        ctrlms_ws_on_close(noPollCtx  *ctx, noPollConn *conn, noPollPtr user_data);
static void        ctrlms_ws_nopoll_log(noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);
#ifdef CTRLMS_WSS_ENABLED
static bool        ctrlms_ws_cert_config(FILE *cert_key_fp);
static bool        ctrlms_ws_add_chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs);
#endif

ctrlms_ws_global_t g_ctrlms_ws;

bool ctrlms_ws_init(uint16_t port, bool log_enable) {
   ctrlms_ws_thread_params_t params;

   sem_t semaphore;
   sem_init(&semaphore, 0, 0);
   
   // Launch thread
   params.semaphore = &semaphore;

   params.port             = port;
   params.log_enable       = log_enable;

   g_ctrlms_ws.nopoll_ctx      = NULL;
   
   if(0 != pthread_create(&g_ctrlms_ws.thread_id, NULL, ctrlms_ws_main, &params)) {
      XLOGD_ERROR("unable to launch thread");
      return(false);
   }

   // Block until initialization is complete or a timeout occurs
   XLOGD_INFO("Waiting for thread initialization...");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(true);
}

bool ctrlms_ws_listen(void) {
   // Wait for thread to exit
   XLOGD_INFO("Waiting until thread exits");
   void *retval = NULL;
   pthread_join(g_ctrlms_ws.thread_id, &retval);
   XLOGD_INFO("thread exited.");
   return(true);
}

void ctrlms_ws_term(void) {
   if(g_ctrlms_ws.nopoll_ctx != NULL) {
      nopoll_loop_stop(g_ctrlms_ws.nopoll_ctx);
   }

   // Wait for thread to exit
   XLOGD_INFO("Waiting for thread to exit");
   void *retval = NULL;
   pthread_join(g_ctrlms_ws.thread_id, &retval);
   XLOGD_INFO("thread exited.");
}

void *ctrlms_ws_main(void *param) {
   ctrlms_ws_thread_params_t params = *((ctrlms_ws_thread_params_t *)param);
   errno_t safec_rc = -1;

   ctrlms_ws_thread_state_t state;
   
   state.app_interface = NULL;
   state.app_handle    = ctrlms_ws_load_app(&state);

   g_ctrlms_ws.nopoll_ctx = nopoll_ctx_new();
   if(g_ctrlms_ws.nopoll_ctx == NULL) {
      XLOGD_ERROR("nopoll context create");
      return(NULL);
   }

   #ifdef CTRLMS_WSS_ENABLED
   int cert_key_fd   = -1;
   FILE *cert_key_fp = NULL;
   char tmp_cert[32] = {0};
   int err_store;

   safec_rc = sprintf_s(tmp_cert, sizeof(tmp_cert), "%s", CTRLMS_WS_TLS_CERT_KEY_FILE);
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

   if(!ctrlms_ws_cert_config(cert_key_fp)) {
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
      nopoll_log_enable(g_ctrlms_ws.nopoll_ctx, nopoll_true);
      nopoll_log_set_handler(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_nopoll_log, NULL);
   }
   noPollConnOpts *opts = nopoll_conn_opts_new();
   nopoll_ctx_set_on_accept(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_accept, &state);
   nopoll_ctx_set_on_ready(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_ready, &state);
   nopoll_ctx_set_on_msg(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_message, &state);
   
   #ifdef CTRLMS_WSS_ENABLED
   nopoll_conn_opts_set_ssl_protocol(opts, NOPOLL_METHOD_TLSV1_2);
   nopoll_conn_opts_ssl_host_verify(opts, nopoll_false); //localhost will not match host specified in certificate

   if(!nopoll_conn_opts_set_ssl_certs(opts, &tmp_cert[0], &tmp_cert[0], NULL, NULL)) {
      XLOGD_ERROR("Failed to add cert/key files to nopoll_conn");
      nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
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
   #ifdef CTRLMS_WSS_ENABLED
   state.nopoll_conn = nopoll_listener_tls_new_opts6(g_ctrlms_ws.nopoll_ctx, opts, "::", port);
   #else
   state.nopoll_conn = nopoll_listener_new_opts6(g_ctrlms_ws.nopoll_ctx, opts, "::", port);
   #endif
   if(!nopoll_conn_is_ok(state.nopoll_conn)) {
      XLOGD_ERROR("Listener connection IPv6 NOT ok");
      nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
      g_ctrlms_ws.nopoll_ctx = NULL;
      return(NULL);
   }

   // Unblock the caller that launched this thread
   sem_post(params.semaphore);
   params.semaphore = NULL;

   XLOGD_INFO("Enter main loop");

   nopoll_loop_wait(g_ctrlms_ws.nopoll_ctx, 0);

   nopoll_conn_opts_unref(opts);
   nopoll_conn_unref(state.nopoll_conn);
   nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
   g_ctrlms_ws.nopoll_ctx = NULL;

   #ifdef CTRLMS_WSS_ENABLED
   if(0 != unlink(tmp_cert)) {
      int err_store = errno;
      XLOGD_ERROR("failed to remove temp cert <%s>", strerror(err_store));
   }
   #endif

   if(state.app_handle != NULL) {
      dlclose(state.app_handle);
      state.app_handle = NULL;
   }
   if(state.app_interface != NULL) {
      delete state.app_interface;
      state.app_interface = NULL;
   }

   return(NULL);
}

nopoll_bool ctrlms_ws_on_accept(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_thread_state_t *state = (ctrlms_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return(nopoll_false);
   }

   // Set ping handler
   nopoll_conn_set_on_ping_msg(conn, ctrlms_ws_on_ping, NULL);

   return(nopoll_true);
}

nopoll_bool ctrlms_ws_on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_thread_state_t *state = (ctrlms_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return(nopoll_false);
   }

   XLOGD_INFO("Connection established");
   state->app_interface->ws_handle_set((void *)conn);
   state->app_interface->ws_connected();
   
   nopoll_conn_set_on_close(conn, ctrlms_ws_on_close, user_data);

   return(nopoll_true);
}

void ctrlms_ws_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data) {
   ctrlms_ws_thread_state_t *state = (ctrlms_ws_thread_state_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   bool close_conn = false;
   int payload_size = nopoll_msg_get_payload_size(msg);
   const unsigned char *payload = nopoll_msg_get_payload(msg);
   
   switch(nopoll_msg_opcode(msg)) {
      case NOPOLL_TEXT_FRAME: {
         XLOGD_INFO("NOPOLL_TEXT_FRAME size <%d>", payload_size);

         json_t *json_obj = json_loads((const char *)payload, 0, NULL);

         if(json_obj == NULL) {
            XLOGD_ERROR("Failed to parse JSON object");
            break;
         } else {
            // Pass the incoming payload to the application
            close_conn = state->app_interface->ws_receive_json(json_obj);
            json_decref(json_obj);
         }
         break;
      }
      case NOPOLL_BINARY_FRAME: {
         XLOGD_INFO("NOPOLL_BINARY_FRAME size <%d>", payload_size);

         // Pass the incoming payload to the application
         close_conn = state->app_interface->ws_receive_audio(payload, payload_size);
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

   if(close_conn) {
      const char *reason = "app closed";
      nopoll_conn_close_ext(conn, 1000, reason, strlen(reason));
   }
}

void ctrlms_ws_on_ping(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data) {
   ctrlms_ws_thread_state_t *state = (ctrlms_ws_thread_state_t *)user_data;
   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   XLOGD_INFO("Ping received");
   // Do nothing, we don't care about this event
}

void ctrlms_ws_on_close(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_thread_state_t *state = (ctrlms_ws_thread_state_t *)user_data;
   
   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   XLOGD_INFO("");

   state->app_interface->ws_disconnected();
   state->app_interface->ws_handle_set(NULL);
}


#ifdef CTRLMS_WSS_ENABLED
bool ctrlms_ws_cert_config(FILE* cert_key_fp) {

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

      if(!ctrlms_ws_add_chain(cert_key_fp, additional_certs)) {
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

bool ctrlms_ws_add_chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs) {
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

void ctrlms_ws_nopoll_log(noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data) {
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

typedef ctrlms_app_interface_t *(*ctrlms_app_interface_create_t)(void);

void *ctrlms_ws_load_app(ctrlms_ws_thread_state_t *state) {
   void *handle = dlopen("libctrlm_server_app.so", RTLD_NOW);
   if(NULL == handle) {
      XLOGD_WARN("failed to load server app plugin <%s>.  Using stub implementation.", dlerror());

      state->app_interface = new ctrlms_app_interface_t();
      return(NULL);
   }

   dlerror(); // Clear any existing error
   ctrlms_app_interface_create_t app_interface = (ctrlms_app_interface_create_t)dlsym(handle, "ctrlms_app_interface_create");
   char *error = dlerror();

   if(error != NULL) {
      XLOGD_ERROR("failed to find plugin interface, error <%s>", error);
      dlclose(handle);

      state->app_interface = new ctrlms_app_interface_t();
      return(NULL);
   }

   XLOGD_INFO("successfully loaded plugin interface");
   state->app_interface = (*app_interface)();

   return(handle);
}

void ctrlms_app_interface_t::ws_connected(void) {
   XLOGD_INFO("STUB: implement ws_connected");
}

void ctrlms_app_interface_t::ws_disconnected(void) {
   XLOGD_INFO("STUB: implement ws_disconnected");
}

bool ctrlms_app_interface_t::ws_receive_audio(const unsigned char *payload, int payload_size) {
   XLOGD_INFO("STUB: audio received size <%d>", payload_size);
   return(true);   
};
bool ctrlms_app_interface_t::ws_receive_json(const json_t *json_obj) {
   XLOGD_INFO("STUB: json object received");
   return(true);
}

void ctrlms_app_interface_t::ws_send_json(const json_t *json_obj) {
   if(json_obj == NULL) {
      XLOGD_ERROR("json object is NULL");
      return;
   }
   if(ws_handle == NULL) {
      XLOGD_ERROR("ws connection is not established");
      return;
   }
   char *payload = json_dumps(json_obj, JSON_COMPACT | JSON_ENSURE_ASCII);
   if(payload == NULL) {
      XLOGD_ERROR("failed to dump JSON object");
      return;
   }

   XLOGD_INFO("Sending <%s>", payload);
   int rc = nopoll_conn_send_text((noPollConn *)ws_handle, payload, strlen(payload));
   if(rc <= 0) {
      XLOGD_ERROR("failed to send message");
   }
   free(payload);
}

void ctrlms_app_interface_t::ws_handle_set(void *handle) {
   ws_handle = handle;
}
