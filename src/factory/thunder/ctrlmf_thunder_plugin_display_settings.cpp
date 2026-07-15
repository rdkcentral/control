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
#include "ctrlmf_thunder_plugin_display_settings.h"
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace DisplaySettings;
using namespace WPEFramework;

ctrlmf_thunder_plugin_display_settings_t::ctrlmf_thunder_plugin_display_settings_t() : ctrlm_thunder_plugin_t("DisplaySettings", "org.rdk.DisplaySettings", 1) {
}

ctrlmf_thunder_plugin_display_settings_t::~ctrlmf_thunder_plugin_display_settings_t() {
}

ctrlmf_thunder_plugin_display_settings_t *ctrlmf_thunder_plugin_display_settings_t::getInstance() {
    static ctrlmf_thunder_plugin_display_settings_t instance;
    return &instance;
}

bool ctrlmf_thunder_plugin_display_settings_t::set_audio_ducking(
        bool action, bool type, unsigned char level) {
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] Function called: action=%d (%s), type=%d (%s), level=%u", action, action ? "start" : "stop", type, type ? "relative" : "absolute", level);
    JsonObject params, response;
    params["audioPort"]   = "SPEAKER0";
    params["mode"]        = "raw";
    params["action"]      = action ? "start" : "stop";
    params["duckingType"] = type ? "relative" : "absolute";
    params["level"]       = (int)level;

    std::string params_str;
    params.ToString(params_str);
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] JSON params: %s", params_str.c_str());
    
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] Calling DisplaySettings plugin 'setAudioDucking'...");
    if(!this->call_plugin("setAudioDucking", (void *)&params, (void *)&response)) {
        XLOGD_ERROR("DisplaySettings setAudioDucking call failed");
        return false;
    }
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] call_plugin returned true, checking response...");
    
    std::string response_str;
    response.ToString(response_str);
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] Response JSON: %s", response_str.c_str());

    if(!response["success"].Boolean()) {
        XLOGD_ERROR("DisplaySettings setAudioDucking returned failure: %s", response_str.c_str());
        return false;
    }
    XLOGD_INFO("[CTRLMF_THUNDER_DS_DUCKING] SUCCESS: Returning true");
    return true;
}
