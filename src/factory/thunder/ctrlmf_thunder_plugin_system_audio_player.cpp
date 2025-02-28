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
#include "ctrlmf_thunder_plugin_system_audio_player.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace SystemAudioPlayer;
using namespace WPEFramework;

static void _on_sap_events(ctrlm_thunder_plugin_system_audio_player_t *plugin, JsonObject params) {
    plugin->on_sap_events(params["id"].String(), params["event"].String());
}

ctrlm_thunder_plugin_system_audio_player_t::ctrlm_thunder_plugin_system_audio_player_t() : ctrlm_thunder_plugin_t("SystemAudioPlayer", "org.rdk.SystemAudioPlayer", 1) {
    this->registered_events = false;
}

ctrlm_thunder_plugin_system_audio_player_t::~ctrlm_thunder_plugin_system_audio_player_t() {

}

bool ctrlm_thunder_plugin_system_audio_player_t::open(const char *audio_type, const char *source_type, const char *play_mode) {
    bool ret = false;
    JsonObject params, response;
    params["audiotype"]  = audio_type;
    params["sourcetype"] = source_type;
    params["playmode"]   = play_mode;
    if(this->call_plugin("open", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            this->id_active = response["id"].String();
            ret = true;
            XLOGD_DEBUG("%s: id <%s>\n", __FUNCTION__, this->id_active.c_str());
        } else {
            XLOGD_WARN("%s: Success for open was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for open failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::close() {
    bool ret = false;
    JsonObject params, response;
    params["id"] = this->id_active;
    if(this->call_plugin("close", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for close was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for close failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::play(std::string url) {
    bool ret = false;
    JsonObject params, response;
    params["id"]  = this->id_active;
    params["url"] = url;
    if(this->call_plugin("play", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for play was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for play failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::pause() {
    bool ret = false;
    JsonObject params, response;
    params["id"] = this->id_active;
    if(this->call_plugin("pause", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for pause was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for pause failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::resume() {
    bool ret = false;
    JsonObject params, response;
    params["id"] = this->id_active;
    if(this->call_plugin("resume", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for resume was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for resume failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::setMixerLevels(std::string volume_primary, std::string volume_player) {
    bool ret = false;
    JsonObject params, response;
    params["id"]            = this->id_active;
    params["primaryVolume"] = volume_primary;
    params["playerVolume"]  = volume_player;

    if(this->call_plugin("setMixerLevels", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for setMixerLevels was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for setMixerLevels failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::config(std::string format, std::string rate, std::string channels, std::string layout) {
    bool ret = false;
    JsonObject params, response;
    params["id"] = this->id_active;
    if(this->call_plugin("config", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            ret = true;
        } else {
            XLOGD_WARN("%s: Success for config was false\n", __FUNCTION__);
        }
    } else {
        XLOGD_WARN("%s: Call for config failed\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_system_audio_player_t::add_event_handler(system_audio_player_event_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        XLOGD_INFO("%s: %s Event handler added\n", __FUNCTION__, this->name.c_str());
        this->event_callbacks.push_back(std::make_pair(handler, user_data));
        if(!this->registered_events) {
           if(!this->register_events()) {
              XLOGD_WARN("%s: unable to register for system audio player events\n", __FUNCTION__);
           }
        }

        ret = true;
    } else {
        XLOGD_INFO("%s: %s Event handler is NULL\n", __FUNCTION__, this->name.c_str());
    }
    return(ret);
}

void ctrlm_thunder_plugin_system_audio_player_t::remove_event_handler(system_audio_player_event_handler_t handler) {
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


void ctrlm_thunder_plugin_system_audio_player_t::on_sap_events(std::string str_id, std::string str_event) {
    XLOGD_DEBUG("%s: Event received <%s> event <%s>\n", __FUNCTION__, str_id.c_str(), str_event.c_str());
    if(str_id != this->id_active) {
       XLOGD_WARN("%s: ignoring event id <%s> != active <%s>\n", __FUNCTION__, str_id.c_str(), this->id_active.c_str());
       return;
    }
    system_audio_player_event_t event = SYSTEM_AUDIO_PLAYER_EVENT_INVALID;
    
    // This is really bad.  Do not reuse this code.
    if(str_event == "PLAYBACK_STARTED") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_STARTED;
    } else if(str_event == "PLAYBACK_FINISHED") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_FINISHED;
    } else if(str_event == "PLAYBACK_PAUSED") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_PAUSED;
    } else if(str_event == "PLAYBACK_RESUMED") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_RESUMED;
    } else if(str_event == "NETWORK_ERROR") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR;
    } else if(str_event == "PLAYBACK_ERROR") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR;
    } else if(str_event == "NEED_DATA") {
       event = SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA;
    }
    
    if(event == SYSTEM_AUDIO_PLAYER_EVENT_INVALID) {
       XLOGD_ERROR("%s: invalid event received <%s> id <%s>\n", __FUNCTION__, str_event.c_str(), str_id.c_str());
       return;
    }
    for(auto itr : this->event_callbacks) {
        itr.first(event, itr.second);
    }
}

bool ctrlm_thunder_plugin_system_audio_player_t::register_events() {
    bool ret = this->registered_events;
    if(ret == false) {
        auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
        if(clientObject) {
            ret = true;
            uint32_t thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onsapevents"), &_on_sap_events, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <onsapevents>\n", __FUNCTION__);
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
