#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <semaphore.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlmf_ws.h>
#include <ctrlmf_audio_capture.h>
#include <ctrlmf_iarm_control_manager.h>

#define CTRLMF_WS_HOST     "127.0.0.1"
#define CTRLMF_WS_PORT_STR "9880"
#define CTRLMF_WS_PORT_INT (9880)

#ifdef CTRLMF_WSS_ENABLED
#define CTRLMF_WS_URL_BASE "aowss://"
#else
#define CTRLMF_WS_URL_BASE "aows://"
#endif

#define CTRLMF_WS_URL CTRLMF_WS_URL_BASE CTRLMF_WS_HOST ":" CTRLMF_WS_PORT_STR "/mic_test"

typedef struct {
   sem_t semaphore_connected;
   sem_t semaphore_disconnected;
} ctrlmf_test_callback_data_t;

typedef struct {
   Iarm::ControlManager::ctrlm_iarm_client_control_manager_t *obj_ctrlm;
   ctrlmf_test_callback_data_t callback_data;
   ctrlmf_ws_callbacks_t       callbacks;
   std::vector<std::string>    request_types;
   std::string                 session_id;
   bool                        use_mic_tap;
   bool                        url_enabled;
   std::string                 url;
} ctrlmf_audio_cap_global_t;

static void ctrlmf_ws_connected(void *data);
static void ctrlmf_ws_disconnected(void *data);
static bool ctrlmf_ws_wait(sem_t *semaphore, uint32_t timeout);
static bool ctrlm_audio_capture_request_type_supported(std::string &type);
static void ctrlmf_audio_capture_event_handler(control_manager_event_t event, void *user_data);

static ctrlmf_audio_cap_global_t g_audio_cap;

// Capture an audio clip using websocket

bool ctrlmf_audio_capture_init(uint32_t audio_frame_size, bool use_mic_tap) {

   g_audio_cap.obj_ctrlm = new Iarm::ControlManager::ctrlm_iarm_client_control_manager_t;

   if(g_audio_cap.obj_ctrlm == NULL) {
      XLOGD_ERROR("out of memory");
      return(false);
   }

   g_audio_cap.obj_ctrlm->add_event_handler(ctrlmf_audio_capture_event_handler, NULL);
   g_audio_cap.use_mic_tap = use_mic_tap;

   // Get the current url
   if(g_audio_cap.use_mic_tap) {
      if(!g_audio_cap.obj_ctrlm->status_voice_mic_tap(g_audio_cap.url, &g_audio_cap.url_enabled)) {
         XLOGD_ERROR("unable to get mic tap status");
         return(false);
      }
   } else {
      if(!g_audio_cap.obj_ctrlm->status_voice_hf(g_audio_cap.url, &g_audio_cap.url_enabled)) {
         XLOGD_ERROR("unable to get hf status");
         return(false);
      }
   }
   if(!g_audio_cap.obj_ctrlm->voice_session_types(g_audio_cap.request_types)) {
      XLOGD_ERROR("unable to get voice session request types");
      return(false);
   }

   std::ostringstream request_types;
   bool first = true;

   for(std::vector<std::string>::iterator it = g_audio_cap.request_types.begin(); it != g_audio_cap.request_types.end(); it++) {
      if(!first) {
         request_types << ", ";
      }
      request_types << *it;
      first = false;
   }

   XLOGD_INFO("get url <%s> enabled <%s> request types <%s>", g_audio_cap.url.c_str(), g_audio_cap.url_enabled ? "YES" : "NO", request_types.str().c_str());

   // Set the FFV url to our websocket
   std::string url_new = CTRLMF_WS_URL;

   if(g_audio_cap.use_mic_tap) {
      if(!g_audio_cap.obj_ctrlm->configure_voice_mic_tap(url_new, true)) {
         XLOGD_ERROR("unable to set mic tap url");
         return(false);
      }
   } else {
      if(!g_audio_cap.obj_ctrlm->configure_voice_hf(url_new, true)) {
         XLOGD_ERROR("unable to set hf url");
         return(false);
      }
   }

   // Start websocket server
   sem_init(&g_audio_cap.callback_data.semaphore_connected, 0, 0);
   sem_init(&g_audio_cap.callback_data.semaphore_disconnected, 0, 0);

   g_audio_cap.callbacks.connected    = ctrlmf_ws_connected;
   g_audio_cap.callbacks.disconnected = ctrlmf_ws_disconnected;
   g_audio_cap.callbacks.data         = &g_audio_cap.callback_data;
   g_audio_cap.session_id             = "";
   ctrlmf_ws_init(audio_frame_size, CTRLMF_WS_PORT_INT, true, &g_audio_cap.callbacks);
   return(true);
}

bool ctrlmf_audio_capture_term(void) {
   // Stop websocket server
   ctrlmf_ws_term();

   sem_destroy(&g_audio_cap.callback_data.semaphore_connected);
   sem_destroy(&g_audio_cap.callback_data.semaphore_disconnected);

   // Restore the previous url
   XLOGD_INFO("restore url <%s> enabled <%s>", g_audio_cap.url.c_str(), g_audio_cap.url_enabled ? "YES" : "NO");

   if(g_audio_cap.use_mic_tap) {
      if(!g_audio_cap.obj_ctrlm->configure_voice_mic_tap(g_audio_cap.url, g_audio_cap.url_enabled)) {
         XLOGD_ERROR("unable to restore mic tap url");
         return(false);
      }
   } else {
      if(!g_audio_cap.obj_ctrlm->configure_voice_hf(g_audio_cap.url, g_audio_cap.url_enabled)) {
         XLOGD_ERROR("unable to restore hf url");
         return(false);
      }
   }

   if(g_audio_cap.obj_ctrlm != NULL) {
      delete g_audio_cap.obj_ctrlm;
      g_audio_cap.obj_ctrlm = NULL;
   }

   return(true);
}

bool ctrlmf_audio_capture_start(const char *request_type, ctrlmf_audio_frame_t audio_frames, uint32_t audio_frame_qty, uint32_t duration) {
   bool        result        = true;
   std::string type          = request_type;
   std::string transcription = "";
   std::string audio_file    = "";

   XLOGD_INFO("request type <%s> audio frame qty <%u>", type.c_str(), audio_frame_qty);

   // Initiate a voice session from the FF microphone
   if(!ctrlm_audio_capture_request_type_supported(type)) { // the platform does not support this request type
      XLOGD_ERROR("platform does not support request type <%s>", type.c_str());
      return(false);
   }

   ctrlmf_ws_capture_set(audio_frames, audio_frame_qty);

   if(!g_audio_cap.obj_ctrlm->voice_session_request(type, transcription, audio_file)) {
      XLOGD_ERROR("voice session request failed");
      result = false;
   } else if(!ctrlmf_ws_wait(&g_audio_cap.callback_data.semaphore_connected, 5)) { // Wait for connection begin or timeout
      XLOGD_ERROR("connection begin timeout");
      result = false;
   } else {
      XLOGD_INFO("connected");

      if(!ctrlmf_ws_wait(&g_audio_cap.callback_data.semaphore_disconnected, ((duration + 5999) / 1000))) { // Wait for connection end or timeout
         XLOGD_ERROR("connection end timeout");
         result = false;
      } else {
         XLOGD_INFO("disconnected");
      }
   }

   return(result);
}

bool ctrlmf_audio_capture_stop() {
   if(g_audio_cap.session_id.empty()) {
      XLOGD_ERROR("session not in progress");
   } else {
      if(!g_audio_cap.obj_ctrlm->voice_session_terminate(g_audio_cap.session_id)) {
         XLOGD_ERROR("voice session terminate failed");
      } else {
         g_audio_cap.session_id.clear();
         return(true);
      }
   }
   return(false);
}

void ctrlmf_ws_connected(void *data) {
   ctrlmf_test_callback_data_t *callback_data = (ctrlmf_test_callback_data_t *)data;
   if(callback_data == NULL) {
      XLOGD_ERROR("invalid callback data");
   } else {
      sem_post(&callback_data->semaphore_connected);
   }
}

void ctrlmf_ws_disconnected(void *data) {
   ctrlmf_test_callback_data_t *callback_data = (ctrlmf_test_callback_data_t *)data;
   if(callback_data == NULL) {
      XLOGD_ERROR("invalid callback data");
   } else {
      sem_post(&callback_data->semaphore_disconnected);
   }
}

bool ctrlmf_ws_wait(sem_t *semaphore, uint32_t timeout) {
   struct timespec end_time;
   int rc = -1;
   if(clock_gettime(CLOCK_REALTIME, &end_time) != 0) {
      XLOGD_ERROR("unable to get time");
      return(false);
   } else {
      end_time.tv_sec += timeout;
      do {
         errno = 0;
         rc = sem_timedwait(semaphore, &end_time);
         if(rc == -1 && errno == EINTR) {
            XLOGD_INFO("interrupted");
         } else {
            break;
         }
      } while(1);
   }

   if(rc != 0) { // no response received
      XLOGD_INFO("timeout");
      return(false);
   }
   return(true);
}

bool ctrlm_audio_capture_request_type_supported(std::string &type) {
   for(std::vector<std::string>::iterator it = g_audio_cap.request_types.begin(); it != g_audio_cap.request_types.end(); it++) {
      if(type == *it) {
         return(true);
      }
   }
   return(false);
}

void ctrlmf_audio_capture_event_handler(control_manager_event_t event, void *user_data) {
   if(user_data == NULL) {
      XLOGD_ERROR("Invalid Params");
      return;
   }
   switch(event) {
      case CONTROL_MANAGER_EVENT_SESSION_BEGIN: {
         control_manager_event_session_begin_t *session_begin = (control_manager_event_session_begin_t *)user_data;

         const char *session_id  = (session_begin->session_id  == NULL) ? "NULL" : session_begin->session_id;
         const char *device_type = (session_begin->device_type == NULL) ? "NULL" : session_begin->device_type;

         XLOGD_INFO("SESSION_BEGIN - remote id <%d> session id <%s> device type <%s> keyword verification <%s>", session_begin->remote_id, session_id, device_type, session_begin->keyword_verification ? "YES" : "NO");

         g_audio_cap.session_id = session_id;
         break;
      }
      case CONTROL_MANAGER_EVENT_SESSION_END: {
         control_manager_event_session_end_t *session_end = (control_manager_event_session_end_t *)user_data;

         const char *session_id = (session_end->session_id  == NULL) ? "NULL" : session_end->session_id;

         XLOGD_INFO("SESSION_END - remote id <%d> session id <%s>", session_end->remote_id, session_id);

         if(0 != g_audio_cap.session_id.compare(session_id)) {
            XLOGD_WARN("SESSION_END - session id mismatch rxd <%s> exp <%s>", session_id, g_audio_cap.session_id.c_str());
         } else {
            g_audio_cap.session_id.clear();
         }
         break;
      }
      case CONTROL_MANAGER_EVENT_STREAM_BEGIN: {
         control_manager_event_stream_begin_t *stream_begin = (control_manager_event_stream_begin_t *)user_data;

         const char *session_id = (stream_begin->session_id  == NULL) ? "NULL" : stream_begin->session_id;

         XLOGD_INFO("STREAM_BEGIN - remote id <%d> session id <%s>", stream_begin->remote_id, session_id);
         break;
      }
      case CONTROL_MANAGER_EVENT_STREAM_END: {
         control_manager_event_stream_end_t *stream_end = (control_manager_event_stream_end_t *)user_data;

         const char *session_id = (stream_end->session_id  == NULL) ? "NULL" : stream_end->session_id;

         XLOGD_INFO("STREAM_END - remote id <%d> session id <%s>", stream_end->remote_id, session_id);
         break;
      }
      case CONTROL_MANAGER_EVENT_SERVER_MESSAGE: {
         control_manager_event_server_message_t *server_message = (control_manager_event_server_message_t *)user_data;

         const char *session_id = (server_message->session_id  == NULL) ? "NULL" : server_message->session_id;

         XLOGD_INFO("SERVER_MESSAGE - session id <%s>", session_id);
         break;
      }
      case CONTROL_MANAGER_EVENT_KEYWORD_VERIFICATION: {
         control_manager_event_keyword_verification_t *keyword_verification = (control_manager_event_keyword_verification_t *)user_data;

         const char *session_id = (keyword_verification->session_id  == NULL) ? "NULL" : keyword_verification->session_id;

         XLOGD_INFO("KEYWORD_VERIFICATION - remote id <%d> session id <%s>", keyword_verification->remote_id, session_id);
         break;
      }
      case CONTROL_MANAGER_EVENT_INVALID: {
         XLOGD_ERROR("INVALID");
         break;
      }
      default: {
         XLOGD_ERROR("INVALID EVENT");
         break;
      }
   }
}
