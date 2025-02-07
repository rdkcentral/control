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
#include "ctrlmf_thunder_plugin_control_manager.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace ControlManager;
using namespace WPEFramework;

static void _on_session_begin(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_session_begin(params["remoteId"].Number(), params["sessionId"].String(), params["deviceType"].String(), params["keywordVerification"].Boolean());
}

static void _on_session_end(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_session_end(params["remoteId"].Number(), params["sessionId"].String());
}

static void _on_stream_begin(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_stream_begin(params["remoteId"].Number(), params["sessionId"].String());
}

static void _on_stream_end(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_stream_end(params["remoteId"].Number(), params["sessionId"].String());
}

static void _on_server_message(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_server_message(params["trx"].String());
}

static void _on_keyword_verification(ctrlm_thunder_plugin_control_manager_t *plugin, JsonObject params) {
    plugin->on_keyword_verification(params["remoteId"].Number(), params["sessionId"].String());
}

ctrlm_thunder_plugin_control_manager_t::ctrlm_thunder_plugin_control_manager_t() : ctrlm_thunder_plugin_t("VoiceControl", "org.rdk.VoiceControl", 1) {
   this->registered_events = false;
}

ctrlm_thunder_plugin_control_manager_t::~ctrlm_thunder_plugin_control_manager_t() {

}

// configureVoice    Configures the RDK’s voice stack
bool ctrlm_thunder_plugin_control_manager_t::configure_voice_hf(std::string &url_hf, bool enable) {
   bool ret = false;
   JsonObject params, mic, response;
   params["urlHf"] = url_hf;
   mic["enable"]   = enable;
   params["mic"]   = mic;
   
   const char *method = "configureVoice";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

bool ctrlm_thunder_plugin_control_manager_t::configure_voice_mic_tap(std::string &url_mic_tap, bool enable) {
   bool ret = false;
   JsonObject params, mic, response;
   params["urlMicTap"] = url_mic_tap;
   mic["enable"]       = enable;
   params["mic"]       = mic;

   const char *method = "configureVoice";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

// sendVoiceMessage  Sends a message to the Voice Server
bool ctrlm_thunder_plugin_control_manager_t::send_voice_message(void) {
   bool ret = false;
   return(ret);
}

// setVoiceInit   Sets the application metadata in the INIT message that gets sent to the Voice Server
bool ctrlm_thunder_plugin_control_manager_t::set_voice_init(void) {
   bool ret = false;
   return(ret);
}

// voiceSessionTypes   Retrieves the types of voice sessions which are supported by the platform.
bool ctrlm_thunder_plugin_control_manager_t::voice_session_types(std::vector<std::string> &types) {
   bool ret = false;
   JsonObject params, response;

   const char *method = "voiceSessionTypes";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         // Get the types array and return to the caller as a vector of strings
         JsonArray obj_types = response["types"].Array();
         JsonArray::Iterator iter(obj_types.Elements());

         while(iter.Next()) {
            types.push_back(iter.Current().String().c_str());
         }
         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

// voiceSessionRequest   Requests a voice session using the specified request type and optional parameters.
bool ctrlm_thunder_plugin_control_manager_t::voice_session_request(std::string &type, std::string &transcription, std::string &audio_file) {
   bool ret = false;
   JsonObject params, response;

   if(!transcription.empty()) {
      params["transcription"] = transcription;
   }
   if(!type.empty()) {
      params["type"] = type;
   }
   if(!audio_file.empty()) {
      params["audio_file"] = audio_file;
   }

   const char *method = "voiceSessionRequest";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

// voiceSessionTerminate   Terminates a voice session using the specified session identifier.
bool ctrlm_thunder_plugin_control_manager_t::voice_session_terminate(std::string &session_id) {
   bool ret = false;
   JsonObject params, response;

   if(!session_id.empty()) {
      params["sessionId"] = session_id;
   }

   const char *method = "voiceSessionTerminate";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

// voiceStatus    Returns the current status of the hands free voice config
bool ctrlm_thunder_plugin_control_manager_t::status_voice_hf(std::string &url_hf, bool *url_hf_enabled) {
   bool ret = false;
   JsonObject params, response;

   const char *method = "voiceStatus";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         JsonObject mic = response["mic"].Object();
         url_hf = response["urlHf"].String();
         if(url_hf_enabled != NULL) {
            if(0 == strcmp(mic["status"].String().c_str(), "ready")) {
               *url_hf_enabled = true;
            } else {
               *url_hf_enabled = false;
            }
         }
         XLOGD_DEBUG("Status <%s> url <%s>", mic["status"].String().c_str(), url_hf.c_str());

         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

// voiceStatus    Returns the current status of the microphone tap voice config
bool ctrlm_thunder_plugin_control_manager_t::status_voice_mic_tap(std::string &url_mic_tap, bool *url_mic_tap_enabled) {
   bool ret = false;
   JsonObject params, response;

   const char *method = "voiceStatus";
   if(this->call_plugin(method, (void *)&params, (void *)&response)) {
      if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
         JsonObject mic = response["mic"].Object();
         url_mic_tap = response["urlMicTap"].String();
         if(url_mic_tap_enabled != NULL) {
            if(0 == strcmp(mic["status"].String().c_str(), "ready")) {
               *url_mic_tap_enabled = true;
            } else {
               *url_mic_tap_enabled = false;
            }
         }
         XLOGD_DEBUG("Status <%s> url <%s>", mic["status"].String().c_str(), url_mic_tap.c_str());

         ret = true;
      } else {
         XLOGD_WARN("Success for %s was false", method);
      }
   } else {
      XLOGD_WARN("Call for %s failed", method);
   }
   return(ret);
}

bool ctrlm_thunder_plugin_control_manager_t::add_event_handler(control_manager_event_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        XLOGD_INFO("%s: %s Event handler added\n", __FUNCTION__, this->name.c_str());
        this->event_callbacks.push_back(std::make_pair(handler, user_data));
        if(!this->registered_events) {
           if(!this->register_events()) {
              XLOGD_WARN("%s: unable to register for control manager events\n", __FUNCTION__);
           }
        }

        ret = true;
    } else {
        XLOGD_INFO("%s: %s Event handler is NULL\n", __FUNCTION__, this->name.c_str());
    }
    return(ret);
}

void ctrlm_thunder_plugin_control_manager_t::remove_event_handler(control_manager_event_handler_t handler) {
    auto itr = this->event_callbacks.begin();
    while(itr != this->event_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("%s: %s Event handler removed\n", __FUNCTION__, this->name.c_str());
            itr = this->event_callbacks.erase(itr);
        } else {
            ++itr;
        }
    }
}


bool ctrlm_thunder_plugin_control_manager_t::register_events() {
    bool ret = this->registered_events;
    if(ret == false) {
        auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
        if(clientObject) {
            ret = true;
            uint32_t thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onSessionBegin"), &_on_session_begin, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onSessionBegin>\n", __FUNCTION__);
                ret = false;
            }
            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onSessionEnd"), &_on_session_end, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onSessionEnd>\n", __FUNCTION__);
                ret = false;
            }
            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onStreamBegin"), &_on_stream_begin, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onStreamBegin>\n", __FUNCTION__);
                ret = false;
            }
            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onStreamEnd"), &_on_stream_end, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onStreamEnd>\n", __FUNCTION__);
                ret = false;
            }
            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onServerMessage"), &_on_server_message, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onServerMessage>\n", __FUNCTION__);
                ret = false;
            }
            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onKeywordVerification"), &_on_keyword_verification, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onKeywordVerification>\n", __FUNCTION__);
                ret = false;
            }

            if(ret) {
                this->registered_events = true;
            }
        }
    }
    Thunder::Plugin::ctrlm_thunder_plugin_t::register_events();
    return(ret);
}

void ctrlm_thunder_plugin_control_manager_t::on_session_begin(int32_t remote_id, std::string session_id, std::string device_type, bool keyword_verification) {
   control_manager_event_session_begin_t session_begin;
   session_begin.remote_id            = remote_id;
   session_begin.session_id           = session_id.c_str();
   session_begin.device_type          = device_type.c_str();
   session_begin.keyword_verification = keyword_verification;

   XLOGD_INFO("remote id <%d> session id <%s> device type <%s> keyword verification <%s>", session_begin.remote_id, session_begin.session_id, session_begin.device_type, session_begin.keyword_verification ? "YES" : "NO");
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SESSION_BEGIN, &session_begin);
   }
}

void ctrlm_thunder_plugin_control_manager_t::on_session_end(int32_t remote_id, std::string session_id) {
   control_manager_event_session_end_t session_end;
   session_end.remote_id  = remote_id;
   session_end.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", session_end.remote_id, session_end.session_id);
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SESSION_END, &session_end);
   }
}

void ctrlm_thunder_plugin_control_manager_t::on_stream_begin(int32_t remote_id, std::string session_id) {
   control_manager_event_session_end_t stream_begin;
   stream_begin.remote_id  = remote_id;
   stream_begin.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", stream_begin.remote_id, stream_begin.session_id);
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_STREAM_BEGIN, &stream_begin);
   }
}

void ctrlm_thunder_plugin_control_manager_t::on_stream_end(int32_t remote_id, std::string session_id) {
   control_manager_event_session_end_t stream_end;
   stream_end.remote_id  = remote_id;
   stream_end.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", stream_end.remote_id, stream_end.session_id);
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_STREAM_END, &stream_end);
   }
}

void ctrlm_thunder_plugin_control_manager_t::on_server_message(std::string session_id) {
   control_manager_event_server_message_t server_message;
   server_message.session_id = session_id.c_str();

   XLOGD_INFO("session id <%s>", session_id.c_str());
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_SERVER_MESSAGE, &server_message);
   }
}

void ctrlm_thunder_plugin_control_manager_t::on_keyword_verification(int32_t remote_id, std::string session_id) {
   control_manager_event_keyword_verification_t keyword_verification;
   keyword_verification.remote_id  = remote_id;
   keyword_verification.session_id = session_id.c_str();

   XLOGD_INFO("remote id <%d> session id <%s>", keyword_verification.remote_id, keyword_verification.session_id);
   for(auto itr : this->event_callbacks) {
       itr.first(CONTROL_MANAGER_EVENT_KEYWORD_VERIFICATION, &keyword_verification);
   }
}
