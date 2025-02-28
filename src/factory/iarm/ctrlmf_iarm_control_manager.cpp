/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <ctrlmf_iarm_control_manager.h>
#include <jansson.h>
#include <string.h>
#include <ctrlm_ipc_voice.h>

#define JSON_ENCODE_FLAGS (JSON_COMPACT)
#define JSON_DECODE_FLAGS (JSON_DECODE_ANY)

#define JSON_REMOTE_ID                                "remoteId"
#define JSON_SESSION_ID                               "sessionId"
#define JSON_DEVICE_TYPE                              "deviceType"
#define JSON_KEYWORD_VERIFICATION                     "keywordVerification"
#define JSON_TRX                                      "trx"
#define JSON_URL_PTT                                  "urlPtt"
#define JSON_URL_HF                                   "urlHf"
#define JSON_URL_MIC_TAP                              "urlMicTap"
#define JSON_ENABLE                                   "enable"
#define JSON_MIC                                      "mic"
#define JSON_MIC_TAP                                  "micTap"
#define JSON_THUNDER_RESULT                           "success"
#define JSON_TYPES                                    "types"
#define JSON_TYPE                                     "type"
#define JSON_TRANSCRIPTION                            "transcription"
#define JSON_AUDIO_FILE                               "audio_file"
#define JSON_READY                                    "ready"
#define JSON_STATUS                                   "status"

using namespace Iarm;
using namespace ControlManager;

Iarm::ControlManager::ctrlm_iarm_client_control_manager_t *g_obj_iarm_client = NULL;

static void _on_session_begin(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_REMOTE_ID);
      if(value == NULL || !json_is_integer(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_REMOTE_ID);
         break;
      }
      int32_t remote_id = json_integer_value(value);

      value = json_object_get(obj, JSON_SESSION_ID);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_SESSION_ID);
         break;
      }
      std::string session_id = json_string_value(value);

      value = json_object_get(obj, JSON_DEVICE_TYPE);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_DEVICE_TYPE);
         break;
      }
      std::string device_type = json_string_value(value);

      value = json_object_get(obj, JSON_KEYWORD_VERIFICATION);
      if(value == NULL || !json_is_boolean(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_KEYWORD_VERIFICATION);
         break;
      }
      bool keyword_verification = json_is_true(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_session_begin(remote_id, session_id, device_type, keyword_verification);
      }
   } while(0);

   json_decref(obj);
}

static void _on_session_end(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);

   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_REMOTE_ID);
      if(value == NULL || !json_is_integer(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_REMOTE_ID);
         break;
      }
      int32_t remote_id = json_integer_value(value);

      value = json_object_get(obj, JSON_SESSION_ID);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_SESSION_ID);
         break;
      }
      std::string session_id = json_string_value(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_session_end(remote_id, session_id);
      }
   } while(0);

   json_decref(obj);
}

static void _on_stream_begin(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_REMOTE_ID);
      if(value == NULL || !json_is_integer(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_REMOTE_ID);
         break;
      }
      int32_t remote_id = json_integer_value(value);

      value = json_object_get(obj, JSON_SESSION_ID);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_SESSION_ID);
         break;
      }
      std::string session_id = json_string_value(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_stream_begin(remote_id, session_id);
      }
   } while(0);

   json_decref(obj);
}

static void _on_stream_end(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_REMOTE_ID);
      if(value == NULL || !json_is_integer(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_REMOTE_ID);
         break;
      }
      int32_t remote_id = json_integer_value(value);

      value = json_object_get(obj, JSON_SESSION_ID);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_SESSION_ID);
         break;
      }
      std::string session_id = json_string_value(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_stream_end(remote_id, session_id);
      }
   } while(0);

   json_decref(obj);
}

static void _on_server_message(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_TRX);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_TRX);
         break;
      }
      std::string trx = json_string_value(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_server_message(trx);
      }
   } while(0);

   json_decref(obj);
}

static void _on_keyword_verification(const char *owner, IARM_EventId_t event_id, void *data, size_t len) {
   char *json_str = (char *)data;

   XLOGD_INFO("len <%u> json <%s>", len, json_str);
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return;
   }
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_REMOTE_ID);
      if(value == NULL || !json_is_integer(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_REMOTE_ID);
         break;
      }
      int32_t remote_id = json_integer_value(value);

      value = json_object_get(obj, JSON_SESSION_ID);
      if(value == NULL || !json_is_string(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_SESSION_ID);
         break;
      }
      std::string session_id = json_string_value(value);

      if(g_obj_iarm_client != NULL) {
         g_obj_iarm_client->on_keyword_verification(remote_id, session_id);
      }
   } while(0);

   json_decref(obj);
}

static bool ctrlm_iarm_json_parse_result(const char *json_str) {
   json_t *obj = json_loads(json_str, JSON_DECODE_FLAGS, NULL);
   if(obj == NULL) {
      XLOGD_ERROR("Invalid JSON");
      return(false);
   }
   bool ret = false;
   do {
      if(!json_is_object(obj)) {
         XLOGD_ERROR("not a JSON object");
         break;
      }

      json_t *value = json_object_get(obj, JSON_THUNDER_RESULT);
      if(value == NULL || !json_is_boolean(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_THUNDER_RESULT);
         break;
      }
      ret = json_is_true(value);
   } while(0);

   json_decref(obj);
   return(ret);
}

ctrlm_iarm_client_control_manager_t::ctrlm_iarm_client_control_manager_t() : ctrlm_iarm_client_t() {
   this->registered_events = false;

   // Only a single object is supported
   g_obj_iarm_client = this;
}

ctrlm_iarm_client_control_manager_t::~ctrlm_iarm_client_control_manager_t() {
   g_obj_iarm_client = NULL;
}

// configureVoice    Configures the RDK’s voice stack
bool ctrlm_iarm_client_control_manager_t::configure_voice_hf(std::string &url_hf, bool enable) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      json_t *obj_mic = json_object();
      if(obj_mic == NULL) {
         XLOGD_ERROR("create JSON mic obj");
         break;
      }

      int rc = 0;
      rc |= json_object_set_new_nocheck(obj_mic, JSON_ENABLE, json_boolean(enable));
      rc |= json_object_set_new_nocheck(obj,     JSON_URL_HF, json_string(url_hf.c_str()));
      rc |= json_object_set_new_nocheck(obj,     JSON_MIC,    obj_mic);

      if(rc) {
         XLOGD_WARN("JSON error..");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      if(!ctrlm_iarm_json_parse_result(call_json->result)) {
         XLOGD_WARN("iarm call result failure");
         break;
      }
      ret = true;
   } while(0);

   json_decref(obj);

   return(ret);
}

bool ctrlm_iarm_client_control_manager_t::configure_voice_mic_tap(std::string &url_mic_tap, bool enable) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      json_t *obj_mic = json_object();
      if(obj_mic == NULL) {
         XLOGD_ERROR("create JSON mic obj");
         break;
      }

      int rc = 0;
      rc |= json_object_set_new_nocheck(obj_mic, JSON_ENABLE,      json_boolean(enable));
      rc |= json_object_set_new_nocheck(obj,     JSON_URL_MIC_TAP, json_string(url_mic_tap.c_str()));
      rc |= json_object_set_new_nocheck(obj,     JSON_MIC,         obj_mic);

      if(rc) {
         XLOGD_WARN("JSON error..");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      if(!ctrlm_iarm_json_parse_result(call_json->result)) {
         XLOGD_WARN("iarm call result failure");
         break;
      }
      ret = true;
   } while(0);

   json_decref(obj);

   return(ret);
}

// sendVoiceMessage  Sends a message to the Voice Server
bool ctrlm_iarm_client_control_manager_t::send_voice_message(void) {
   bool ret = false;
   return(ret);
}

// setVoiceInit   Sets the application metadata in the INIT message that gets sent to the Voice Server
bool ctrlm_iarm_client_control_manager_t::set_voice_init(void) {
   bool ret = false;
   return(ret);
}

// voiceSessionTypes   Retrieves the types of voice sessions which are supported by the platform.
bool ctrlm_iarm_client_control_manager_t::voice_session_types(std::vector<std::string> &types) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      json_t *obj_mic_tap = json_object();
      if(obj_mic_tap == NULL) {
         XLOGD_ERROR("create JSON mic obj");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_SESSION_TYPES, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      XLOGD_INFO("JSON result <%s>", call_json->result);

      json_t *obj_result = json_loads(call_json->result, JSON_DECODE_FLAGS, NULL);
      if(obj_result == NULL) {
         XLOGD_ERROR("Invalid JSON result");
         break;
      }
      
      if(!json_is_object(obj_result)) {
         XLOGD_ERROR("result not a JSON object");
         json_decref(obj_result);
         break;
      }

      json_t *value = json_object_get(obj_result, JSON_THUNDER_RESULT);
      if(value == NULL || !json_is_boolean(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_THUNDER_RESULT);
         json_decref(obj_result);
         break;
      }
      ret = json_is_true(value);
      
      if(ret) {
         // Get the types array and return to the caller as a vector of strings
         json_t *obj_types = json_object_get(obj_result, JSON_TYPES);
         
         if(obj_types == NULL || !json_is_array(obj_types)) {
            XLOGD_ERROR("invalid <%s> value", JSON_TYPES);
         } else {
            size_t index;
            
            json_array_foreach(obj_types, index, value) {
               if(!json_is_string(value)) {
                  XLOGD_WARN("skipping invalid value at index <%u>", index);
               } else {
                   types.push_back(json_string_value(value));
                }
            }
         }
      }
      json_decref(obj_result);
   } while(0);

   json_decref(obj);

   return(ret);
}

// voiceSessionRequest   Requests a voice session using the specified request type and optional parameters.
bool ctrlm_iarm_client_control_manager_t::voice_session_request(std::string &type, std::string &transcription, std::string &audio_file) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      int rc = 0;
      
      if(!transcription.empty()) {
         rc |= json_object_set_new_nocheck(obj, JSON_TRANSCRIPTION, json_string(transcription.c_str()));
      }
      if(!type.empty()) {
         rc |= json_object_set_new_nocheck(obj, JSON_TYPE, json_string(type.c_str()));
      }
      if(!audio_file.empty()) {
         rc |= json_object_set_new_nocheck(obj, JSON_AUDIO_FILE, json_string(audio_file.c_str()));
      }

      if(rc) {
         XLOGD_WARN("JSON error..");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_SESSION_REQUEST, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      if(!ctrlm_iarm_json_parse_result(call_json->result)) {
         XLOGD_WARN("iarm call result failure");
         break;
      }
      ret = true;
   } while(0);

   json_decref(obj);

   return(ret);
}

// voiceSessionTerminate   Terminates a voice session using the specified session identifier.
bool ctrlm_iarm_client_control_manager_t::voice_session_terminate(std::string &session_id) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      int rc = 0;
      
      if(!session_id.empty()) {
         rc |= json_object_set_new_nocheck(obj, JSON_SESSION_ID, json_string(session_id.c_str()));
      }

      if(rc) {
         XLOGD_WARN("JSON error..");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_SESSION_TERMINATE, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      if(!ctrlm_iarm_json_parse_result(call_json->result)) {
         XLOGD_WARN("iarm call result failure");
         break;
      }
      ret = true;
   } while(0);

   json_decref(obj);

   return(ret);
}

// voiceStatus    Returns the current status of the hands free voice config
bool ctrlm_iarm_client_control_manager_t::status_voice_hf(std::string &url_hf, bool *url_hf_enabled) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      json_t *obj_mic_tap = json_object();
      if(obj_mic_tap == NULL) {
         XLOGD_ERROR("create JSON mic obj");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_STATUS, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      XLOGD_INFO("JSON result <%s>", call_json->result);

      json_t *obj_result = json_loads(call_json->result, JSON_DECODE_FLAGS, NULL);
      if(obj_result == NULL) {
         XLOGD_ERROR("Invalid JSON result");
         break;
      }
      
      if(!json_is_object(obj_result)) {
         XLOGD_ERROR("result not a JSON object");
         json_decref(obj_result);
         break;
      }

      json_t *value = json_object_get(obj_result, JSON_THUNDER_RESULT);
      if(value == NULL || !json_is_boolean(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_THUNDER_RESULT);
         json_decref(obj_result);
         break;
      }
      ret = json_is_true(value);
      
      if(ret) {
         json_t *obj_mic = json_object_get(obj_result, JSON_MIC);
         
         if(obj_mic == NULL || !json_is_object(obj_mic)) {
            XLOGD_ERROR("invalid <%s> value", JSON_MIC);
            json_decref(obj_result);
            break;
         }
         value = json_object_get(obj_result, JSON_URL_HF);
         
         if(value == NULL || !json_is_string(value)) {
            XLOGD_ERROR("invalid <%s> value", JSON_URL_HF);
            json_decref(obj_result);
            break;
         }
         
         url_hf = json_string_value(value);
         
         value = json_object_get(obj_mic, JSON_STATUS);
         
         if(value == NULL || !json_is_string(value)) {
            XLOGD_ERROR("invalid <%s> value", JSON_STATUS);
            json_decref(obj_result);
            break;
         }
         
         const char *status = json_string_value(value);
         if(url_hf_enabled != NULL) {
            if(0 == strcmp(status, JSON_READY)) {
               *url_hf_enabled = true;
            } else {
               *url_hf_enabled = false;
            }
         }
         XLOGD_DEBUG("Status <%s> url <%s>", status, url_hf.c_str());
      }
      json_decref(obj_result);
   } while(0);

   json_decref(obj);

   return(ret);
}

// voiceStatus    Returns the current status of the microphone tap voice config
bool ctrlm_iarm_client_control_manager_t::status_voice_mic_tap(std::string &url_mic_tap, bool *url_mic_tap_enabled) {
   bool ret = false;

   json_t *obj = json_object();
   if(obj == NULL) {
      XLOGD_ERROR("create JSON obj");
      return(false);
   }

   do {
      json_t *obj_mic_tap = json_object();
      if(obj_mic_tap == NULL) {
         XLOGD_ERROR("create JSON mic obj");
         break;
      }

      char *json_str = json_dumps(obj, JSON_ENCODE_FLAGS);
      if(json_str == NULL) {
         XLOGD_WARN("Failed to encode JSON string");
         break;
      }

      XLOGD_INFO("<%s>", json_str);

      size_t json_len = strlen(json_str) + 1;

      char msg[sizeof(ctrlm_voice_iarm_call_json_t) + json_len];
      memset(msg, 0, sizeof(msg));

      ctrlm_voice_iarm_call_json_t *call_json = (ctrlm_voice_iarm_call_json_t *)&msg[0];
      call_json->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;

      memcpy(call_json->payload, json_str, json_len);
      free(json_str);

      IARM_Result_t result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_STATUS, &msg[0], sizeof(msg));

      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_WARN("iarm call failed <%s>", ctrlm_iarm_result_str(result));
         break;
      }

      XLOGD_INFO("JSON result <%s>", call_json->result);

      json_t *obj_result = json_loads(call_json->result, JSON_DECODE_FLAGS, NULL);
      if(obj_result == NULL) {
         XLOGD_ERROR("Invalid JSON result");
         break;
      }
      
      if(!json_is_object(obj_result)) {
         XLOGD_ERROR("result not a JSON object");
         json_decref(obj_result);
         break;
      }

      json_t *value = json_object_get(obj_result, JSON_THUNDER_RESULT);
      if(value == NULL || !json_is_boolean(value)) {
         XLOGD_ERROR("invalid <%s> value", JSON_THUNDER_RESULT);
         json_decref(obj_result);
         break;
      }
      ret = json_is_true(value);
      
      if(ret) {
         json_t *obj_mic = json_object_get(obj_result, JSON_MIC);
         
         if(obj_mic == NULL || !json_is_object(obj_mic)) {
            XLOGD_ERROR("invalid <%s> value", JSON_MIC);
            json_decref(obj_result);
            break;
         }
         value = json_object_get(obj_result, JSON_URL_MIC_TAP);
         
         if(value == NULL || !json_is_string(value)) {
            XLOGD_ERROR("invalid <%s> value", JSON_URL_MIC_TAP);
            json_decref(obj_result);
            break;
         }
         
         url_mic_tap = json_string_value(value);
         
         value = json_object_get(obj_mic, JSON_STATUS);
         
         if(value == NULL || !json_is_string(value)) {
            XLOGD_ERROR("invalid <%s> value", JSON_STATUS);
            json_decref(obj_result);
            break;
         }
         
         const char *status = json_string_value(value);
         if(url_mic_tap_enabled != NULL) {
            if(0 == strcmp(status, JSON_READY)) {
               *url_mic_tap_enabled = true;
            } else {
               *url_mic_tap_enabled = false;
            }
         }
         XLOGD_DEBUG("Status <%s> url <%s>", status, url_mic_tap.c_str());
      }
      json_decref(obj_result);
   } while(0);

   json_decref(obj);

   return(ret);
}

bool ctrlm_iarm_client_control_manager_t::add_event_handler(control_manager_event_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        XLOGD_INFO("Event handler added");
        this->event_callbacks.push_back(std::make_pair(handler, user_data));
        if(!this->registered_events) {
           if(!this->register_events()) {
              XLOGD_WARN("unable to register for control manager events");
           }
        }

        ret = true;
    } else {
        XLOGD_INFO("Event handler is NULL");
    }
    return(ret);
}

void ctrlm_iarm_client_control_manager_t::remove_event_handler(control_manager_event_handler_t handler) {
    auto itr = this->event_callbacks.begin();
    while(itr != this->event_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("Event handler removed");
            itr = this->event_callbacks.erase(itr);
        } else {
            ++itr;
        }
    }
}

bool ctrlm_iarm_client_control_manager_t::register_events() {
    bool ret = this->registered_events;
    if(ret == false) {
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SESSION_BEGIN,        &_on_session_begin);
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SESSION_END,          &_on_session_end);
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_STREAM_BEGIN,         &_on_stream_begin);
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_STREAM_END,           &_on_stream_end);
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SERVER_MESSAGE,       &_on_server_message);
        IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_KEYWORD_VERIFICATION, &_on_keyword_verification);

        this->registered_events = true;
        ret = true;
    }
    Iarm::Client::ctrlm_iarm_client_t::register_events();
    return(ret);
}

void ctrlm_iarm_client_control_manager_t::on_session_begin(int32_t remote_id, const std::string &session_id, const std::string &device_type, bool keyword_verification) {
   control_manager_event_session_begin_t session_begin;
   session_begin.remote_id            = remote_id;
   session_begin.session_id           = session_id.c_str();
   session_begin.device_type          = device_type.c_str();
   session_begin.keyword_verification = keyword_verification;

   XLOGD_INFO("remote id <%d> session id <%s> device type <%s> keyword verification <%s>", session_begin.remote_id, session_begin.session_id, session_begin.device_type, session_begin.keyword_verification ? "YES" : "NO");
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SESSION_BEGIN, &session_begin);
   }
}

void ctrlm_iarm_client_control_manager_t::on_session_end(int32_t remote_id, const std::string &session_id) {
   control_manager_event_session_end_t session_end;
   session_end.remote_id  = remote_id;
   session_end.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", session_end.remote_id, session_end.session_id);
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SESSION_END, &session_end);
   }
}

void ctrlm_iarm_client_control_manager_t::on_stream_begin(int32_t remote_id, const std::string &session_id) {
   control_manager_event_session_end_t stream_begin;
   stream_begin.remote_id  = remote_id;
   stream_begin.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", stream_begin.remote_id, stream_begin.session_id);
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_STREAM_BEGIN, &stream_begin);
   }
}

void ctrlm_iarm_client_control_manager_t::on_stream_end(int32_t remote_id, const std::string &session_id) {
   control_manager_event_session_end_t stream_end;
   stream_end.remote_id  = remote_id;
   stream_end.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", stream_end.remote_id, stream_end.session_id);
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_STREAM_END, &stream_end);
   }
}

void ctrlm_iarm_client_control_manager_t::on_server_message(const std::string &session_id) {
   control_manager_event_server_message_t server_message;
   server_message.session_id = session_id.c_str();

   XLOGD_INFO("session id <%s>", session_id.c_str());
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SERVER_MESSAGE, &server_message);
   }
}

void ctrlm_iarm_client_control_manager_t::on_keyword_verification(int32_t remote_id, const std::string &session_id) {
   control_manager_event_keyword_verification_t keyword_verification;
   keyword_verification.remote_id  = remote_id;
   keyword_verification.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", keyword_verification.remote_id, keyword_verification.session_id);
   for(auto &itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_KEYWORD_VERIFICATION, &keyword_verification);
   }
}
