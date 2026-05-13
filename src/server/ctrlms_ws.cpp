#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <nopoll.h>
#include <dlfcn.h>
#include <jansson.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <safec_lib.h>
#include <ctrlms_ws.h>
#include <ctrlm_server_app.h>

#ifdef CTRLMS_WSS_ENABLED
#include "rdkcertselector.h"
#include <openssl/ssl.h>
#include <openssl/pkcs12.h>
#define CTRLMS_WS_TLS_CERT_KEY_FILE    "/tmp/serverXXXXXX"
#define CTRLMS_WS_CERT_FILENAME_PREFIX "file://"
#endif

typedef struct {
   volatile sig_atomic_t    term_requested;
   noPollCtx *              nopoll_ctx;
   noPollConnOpts *         opts;
   char                     tmp_cert[32];
   bool                     tmp_cert_created;
   bool                     cert_valid;
   noPollConn *             nopoll_conn;
   void *                   app_handle;
   ctrlms_app_interface_t  *app_interface;
} ctrlms_ws_global_t;

static bool  ctrlms_ws_load_app(ctrlms_ws_global_t *state, bool use_stub, void **handle);

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
   errno_t safec_rc = -1;

   g_ctrlms_ws.term_requested      = 0;
   g_ctrlms_ws.nopoll_ctx          = NULL;
   g_ctrlms_ws.opts                = NULL;
   g_ctrlms_ws.cert_valid          = false;
   g_ctrlms_ws.tmp_cert_created    = false;
   memset(g_ctrlms_ws.tmp_cert, 0, sizeof(g_ctrlms_ws.tmp_cert));
   g_ctrlms_ws.app_interface       = NULL;
   g_ctrlms_ws.app_handle          = NULL;
   g_ctrlms_ws.nopoll_conn         = NULL;

   bool result = false;
   do {
      if(!ctrlms_ws_load_app(&g_ctrlms_ws, false, &g_ctrlms_ws.app_handle)) {
         XLOGD_INFO("exiting due to app load failure");
         break;
      }

      g_ctrlms_ws.nopoll_ctx = nopoll_ctx_new();
      if(g_ctrlms_ws.nopoll_ctx == NULL) {
         XLOGD_ERROR("nopoll context create");
         break;
      }

      #ifdef CTRLMS_WSS_ENABLED
      do {
         int cert_key_fd   = -1;
         FILE *cert_key_fp = NULL;
         int errsv;

         safec_rc = sprintf_s(g_ctrlms_ws.tmp_cert, sizeof(g_ctrlms_ws.tmp_cert), "%s", CTRLMS_WS_TLS_CERT_KEY_FILE);
         if(safec_rc < EOK) {
            ERR_CHK(safec_rc);
         }

         cert_key_fd = mkstemp(g_ctrlms_ws.tmp_cert);
         if(cert_key_fd == -1) {
            errsv = errno;
            XLOGD_ERROR("mkstemp failed: <%s>", strerror(errsv));
            break;
         }
         g_ctrlms_ws.tmp_cert_created = true;

         cert_key_fp = fdopen(cert_key_fd, "w");
         if(cert_key_fp == NULL) {
            errsv = errno;
            XLOGD_ERROR("fdopen failed: <%s>", strerror(errsv));
            if(0 != close(cert_key_fd)) {
               errsv = errno;
               XLOGD_ERROR("failed to close cert/key file descriptor <%s>", strerror(errsv));
            }
            break;
         }

         if(!ctrlms_ws_cert_config(cert_key_fp)) {
            XLOGD_ERROR("failed to set cert or key");
            fclose(cert_key_fp);
            break;
         }
         fclose(cert_key_fp);

         // Init OpenSSL
         SSL_library_init();
         SSL_load_error_strings();
         OpenSSL_add_all_algorithms();
         g_ctrlms_ws.cert_valid = true;
      } while(0);

      if(!g_ctrlms_ws.cert_valid) {
         XLOGD_ERROR("no valid cert/key available");
         break;
      }
      #endif

      if(log_enable) {
         nopoll_log_enable(g_ctrlms_ws.nopoll_ctx, nopoll_true);
         nopoll_log_set_handler(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_nopoll_log, NULL);
      }
      g_ctrlms_ws.opts = nopoll_conn_opts_new();
      if(g_ctrlms_ws.opts == NULL) {
         XLOGD_ERROR("nopoll connection options create");
         nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
         g_ctrlms_ws.nopoll_ctx = NULL;
         break;
      }

      nopoll_ctx_set_on_accept(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_accept, &g_ctrlms_ws);
      nopoll_ctx_set_on_ready(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_ready, &g_ctrlms_ws);
      nopoll_ctx_set_on_msg(g_ctrlms_ws.nopoll_ctx, ctrlms_ws_on_message, &g_ctrlms_ws);

      if(g_ctrlms_ws.cert_valid) {
         nopoll_conn_opts_set_ssl_protocol(g_ctrlms_ws.opts, NOPOLL_METHOD_TLSV1_2);
         nopoll_conn_opts_ssl_host_verify(g_ctrlms_ws.opts, nopoll_false); //localhost will not match host specified in certificate

         if(!nopoll_conn_opts_set_ssl_certs(g_ctrlms_ws.opts, g_ctrlms_ws.tmp_cert, g_ctrlms_ws.tmp_cert, NULL, NULL)) {
            XLOGD_ERROR("Failed to add cert/key files to nopoll_conn");
            nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
            g_ctrlms_ws.nopoll_ctx = NULL;
            nopoll_conn_opts_free(g_ctrlms_ws.opts);
            g_ctrlms_ws.opts = NULL;
            break;
         }
      }

      char port_str[6];
      safec_rc = sprintf_s(port_str, sizeof(port_str), "%u", port);
      if(safec_rc < EOK) {
         ERR_CHK(safec_rc);
      }

      // Start loopback-only IPv6 listener
      if(g_ctrlms_ws.cert_valid) {
         g_ctrlms_ws.nopoll_conn = nopoll_listener_tls_new_opts6(g_ctrlms_ws.nopoll_ctx, g_ctrlms_ws.opts, "::1", port_str);
      } else {
         g_ctrlms_ws.nopoll_conn = nopoll_listener_new_opts6(g_ctrlms_ws.nopoll_ctx, g_ctrlms_ws.opts, "::1", port_str);
      }
      if(!nopoll_conn_is_ok(g_ctrlms_ws.nopoll_conn)) {
         XLOGD_ERROR("Listener connection IPv6 NOT ok");
         nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
         g_ctrlms_ws.nopoll_ctx = NULL;
         nopoll_conn_opts_free(g_ctrlms_ws.opts);
         g_ctrlms_ws.opts = NULL;
         break;
      }

      result = true;
   } while(0);

   if(!result) {
      if(g_ctrlms_ws.tmp_cert_created) {
         if(0 != unlink(g_ctrlms_ws.tmp_cert)) {
            int errsv = errno;
            XLOGD_ERROR("failed to remove temp cert <%s>", strerror(errsv));
         }
      }
      if(g_ctrlms_ws.app_handle != NULL) {
         dlclose(g_ctrlms_ws.app_handle);
         g_ctrlms_ws.app_handle = NULL;
      }
      if(g_ctrlms_ws.app_interface != NULL) {
         delete g_ctrlms_ws.app_interface;
         g_ctrlms_ws.app_interface = NULL;
      }
   }

   return(result);
}

bool ctrlms_ws_listen(void) {
   XLOGD_INFO("Enter main loop");
   // Poll with a 100 ms timeout so the loop can observe term_requested, which
   // is set exclusively by ctrlms_ws_term() — an async-signal-safe operation.
   while(!g_ctrlms_ws.term_requested) {
      nopoll_loop_wait(g_ctrlms_ws.nopoll_ctx, 100000);
   }

   nopoll_conn_opts_unref(g_ctrlms_ws.opts);
   nopoll_conn_unref(g_ctrlms_ws.nopoll_conn);
   nopoll_ctx_unref(g_ctrlms_ws.nopoll_ctx);
   g_ctrlms_ws.nopoll_ctx = NULL;

   if(g_ctrlms_ws.cert_valid) {
      if(0 != unlink(g_ctrlms_ws.tmp_cert)) {
         int err_store = errno;
         XLOGD_ERROR("failed to remove temp cert <%s>", strerror(err_store));
      }
   }

   if(g_ctrlms_ws.app_handle != NULL) {
      dlclose(g_ctrlms_ws.app_handle);
      g_ctrlms_ws.app_handle = NULL;
   }
   if(g_ctrlms_ws.app_interface != NULL) {
      delete g_ctrlms_ws.app_interface;
      g_ctrlms_ws.app_interface = NULL;
   }
   return(true);
}

void ctrlms_ws_term(void) {
   g_ctrlms_ws.term_requested = 1;
}

nopoll_bool ctrlms_ws_on_accept(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_global_t *state = (ctrlms_ws_global_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return(nopoll_false);
   }

   // Set ping handler
   nopoll_conn_set_on_ping_msg(conn, ctrlms_ws_on_ping, user_data);

   return(nopoll_true);
}

nopoll_bool ctrlms_ws_on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_global_t *state = (ctrlms_ws_global_t *)user_data;

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
   ctrlms_ws_global_t *state = (ctrlms_ws_global_t *)user_data;

   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   bool close_conn = false;
   int payload_size = nopoll_msg_get_payload_size(msg);
   const unsigned char *payload = nopoll_msg_get_payload(msg);
   
   switch(nopoll_msg_opcode(msg)) {
      case NOPOLL_TEXT_FRAME: {
         XLOGD_DEBUG("NOPOLL_TEXT_FRAME size <%d>", payload_size);

         json_t *json_obj = json_loadb((const char *)payload, payload_size, 0, NULL);

         if(json_obj == NULL) {
            XLOGD_ERROR("Failed to parse JSON object");
            break;
         } else {
            // Pass the incoming payload to the application as a borrowed reference.
            // The json_obj pointer is only valid for the duration of this call and
            // must not be stored by the callee unless it takes its own reference.
            close_conn = state->app_interface->ws_receive_json(json_obj);
            json_decref(json_obj);
         }
         break;
      }
      case NOPOLL_BINARY_FRAME: {
         XLOGD_DEBUG("NOPOLL_BINARY_FRAME size <%d>", payload_size);

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
   ctrlms_ws_global_t *state = (ctrlms_ws_global_t *)user_data;
   if(state == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }

   XLOGD_INFO("Ping received");
   // Do nothing, we don't care about this event
}

void ctrlms_ws_on_close(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data) {
   ctrlms_ws_global_t *state = (ctrlms_ws_global_t *)user_data;
   
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
   
   rdkcertselector_h cert_selector  = NULL;
   PKCS12 *p12_cert                 = NULL;
   EVP_PKEY *pkey                   = NULL;
   X509 *x509_cert                  = NULL;
   STACK_OF(X509) *additional_certs = NULL;
   do {
      FILE *device_cert_fp             = NULL;

      char *cert_path     = NULL;
      char *cert_password = NULL;
      cert_selector = rdkcertselector_new(NULL, NULL, "FBK_MTLS");

      if(cert_selector == NULL){
         XLOGD_TELEMETRY("cert selector init failed");
         break;
      }

      rdkcertselectorStatus_t cert_status = rdkcertselector_getCert(cert_selector, &cert_path, &cert_password);

      if(cert_status != certselectorOk) {
         XLOGD_TELEMETRY("cert selector retrieval failed");
         break;
      }
      
      if(cert_path == NULL || cert_password == NULL) {
         XLOGD_TELEMETRY("cert selector get failed");
         break;
      }

      char *local_path = cert_path;
      if(strncmp(local_path, CTRLMS_WS_CERT_FILENAME_PREFIX, strlen(CTRLMS_WS_CERT_FILENAME_PREFIX)) == 0) {
         local_path += strlen(CTRLMS_WS_CERT_FILENAME_PREFIX);
      }

      device_cert_fp = fopen(local_path, "rb");
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

      if(1 != PKCS12_parse(p12_cert, cert_password, &pkey, &x509_cert, &additional_certs)) {
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

      if(1 != PEM_write_PrivateKey(cert_key_fp, pkey, NULL, (unsigned char*)cert_password, strlen(cert_password), NULL, NULL)) {
         XLOGD_ERROR("failed to write temp key");
         break;
       }

      ret = true;
   }while(0);

   if(cert_selector != NULL) {
      rdkcertselector_free(&cert_selector);
   }

   if(p12_cert != NULL) {
      PKCS12_free(p12_cert);
   }

   if(pkey != NULL) {
      EVP_PKEY_free(pkey);
   }

   if(x509_cert != NULL) {
      X509_free(x509_cert);
   }

   if(additional_certs != NULL) {
      sk_X509_pop_free(additional_certs, X509_free);
   }

   return ret;
}

bool ctrlms_ws_add_chain(FILE *cert_key_fp, STACK_OF(X509) *additional_certs) {
   if(cert_key_fp == NULL) {
      XLOGD_ERROR("null file pointer");
      return false;
   }
   if(additional_certs == NULL) {
      XLOGD_WARN("no additional certs");
      return true;
   }

   for(int32_t index = 0; index < sk_X509_num(additional_certs); index++) {
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

bool ctrlms_ws_load_app(ctrlms_ws_global_t *state, bool use_stub, void **handle) {
   if(handle == NULL) {
      XLOGD_ERROR("invalid params");
      return(false);
   }
   *handle = dlopen("libctrlm-server-app.so", RTLD_NOW);
   if(NULL == *handle) {
      XLOGD_WARN("failed to load server app plugin <%s>", dlerror());

      if(use_stub) {
         XLOGD_INFO("Using stub implementation of app interface");
         state->app_interface = new ctrlms_app_interface_t();
         return(true);
      }
      return(false);
   }

   dlerror(); // Clear any existing error
   ctrlms_app_interface_create_t app_interface = (ctrlms_app_interface_create_t)dlsym(*handle, "ctrlms_app_interface_create");
   char *error = dlerror();

   if(error != NULL) {
      XLOGD_ERROR("failed to find plugin interface, error <%s>", error);
      dlclose(*handle);
      *handle = NULL;

      if(use_stub) {
         XLOGD_INFO("Using stub implementation of app interface");
         state->app_interface = new ctrlms_app_interface_t();
         return(true);
      }
      return(false);
   }

   XLOGD_INFO("successfully loaded plugin interface");
   state->app_interface = (*app_interface)();

   if(NULL == state->app_interface) {
      XLOGD_ERROR("failed to create plugin app interface");
      dlclose(*handle);
      *handle = NULL;
      if(use_stub) {
         XLOGD_INFO("Using stub implementation of app interface");
         state->app_interface = new ctrlms_app_interface_t();
         return(true);
      }
      return(false);
   }

   return(true);
}

void ctrlms_app_interface_t::ws_connected(void) {
   XLOGD_INFO("STUB: implement ws_connected");
}

void ctrlms_app_interface_t::ws_disconnected(void) {
   XLOGD_INFO("STUB: implement ws_disconnected");
}

// Return true to request that the WebSocket connection be closed.
// Return false to keep the connection open after handling/ignoring the frame.
bool ctrlms_app_interface_t::ws_receive_audio(const unsigned char *payload, int payload_size) {
   XLOGD_INFO("STUB: audio received size <%d>", payload_size);
   return(false);
};
bool ctrlms_app_interface_t::ws_receive_json(const json_t *json_obj) {
   XLOGD_INFO("STUB: json object received");
   return(false);
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
