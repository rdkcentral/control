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
#include "ctrlm_thunder_plugin_system.h"
#include "ctrlm_thunder_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace System;
using namespace WPEFramework;

static void _on_firmware_update_state_change(ctrlm_thunder_plugin_system_t *plugin, JsonObject params) {
    plugin->on_firmware_update_state_change((firmware_update_state_t)params["firmwareUpdateStateChange"].Number());
}

ctrlm_thunder_plugin_system_t::ctrlm_thunder_plugin_system_t() : ctrlm_thunder_plugin_t("System", "org.rdk.System", 1) {
    this->registered_events = false;
}

ctrlm_thunder_plugin_system_t::~ctrlm_thunder_plugin_system_t() {

}

ctrlm_thunder_plugin_system_t *ctrlm_thunder_plugin_system_t::getInstance() {
    XLOGD_INFO("system");
    static ctrlm_thunder_plugin_system_t instance;
    return &instance;
}

bool ctrlm_thunder_plugin_system_t::add_event_handler(system_event_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        XLOGD_INFO("%s Event handler added", this->name.c_str());
        this->event_callbacks.push_back(std::make_pair(handler, user_data));
        ret = true;
    } else {
        XLOGD_INFO("%s Event handler is NULL", this->name.c_str());
    }
    return(ret);
}

void ctrlm_thunder_plugin_system_t::remove_event_handler(system_event_handler_t handler) {
    auto itr = this->event_callbacks.begin();
    while(itr != this->event_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("%s Event handler removed", this->name.c_str());
            itr = this->event_callbacks.erase(itr);
        } else {
            ++itr;
        }
    }
}


void ctrlm_thunder_plugin_system_t::on_firmware_update_state_change(firmware_update_state_t state) {
    XLOGD_INFO("state: %s", system_firmware_update_state_str(state));
    for(auto &itr : this->event_callbacks) {
        itr.first(EVENT_FIRMWARE_UPDATE_STATE_CHANGED, (void *)state, itr.second);
    }
}

bool ctrlm_thunder_plugin_system_t::register_events() {
    bool ret = this->registered_events;
    if(ret == false) {
        auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
        if(clientObject) {
            ret = true;
            uint32_t thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onFirmwareUpdateStateChange"), &_on_firmware_update_state_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("Thunder subscribe failed <onFirmwareUpdateStateChange>");
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

const char *Thunder::System::system_event_str(system_event_t event) {
    switch(event) {
        case EVENT_FIRMWARE_UPDATE_STATE_CHANGED: return("FIRMWARE_UPDATE_STATE_CHANGED");
        default: break;
    }
    return("UNKNOWN");
}

const char *Thunder::System::system_firmware_update_state_str(firmware_update_state_t state) {
    switch(state) {
        case STATE_UNINITIALIZED:      return("UNINTIALIZED");
        case STATE_REQUESTING:         return("REQUESTING");
        case STATE_DOWNLOADING:        return("DOWNLOADING");
        case STATE_FAILED:             return("FAILED");
        case STATE_DOWNLOAD_COMPLETE:  return("DOWNLOAD_COMPLETE");
        case STATE_VALIDATION_COMPLETE:return("VALIDATION_COMPLETE");
        case STATE_PREPARING_REBOOT:   return("PREPARING_REBOOT");
        default:break;
    }
    return("UNKNOWN");
}
