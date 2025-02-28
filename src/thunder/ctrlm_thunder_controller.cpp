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
#include "ctrlm_thunder_controller.h"
#include "ctrlm_thunder_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>
#include <secure_wrapper.h>
#include "ctrlm_thunder_helpers.h"
#include <glib.h>
#include "safec_lib.h"
#ifdef THUNDER_SECURITY
#include <securityagent/SecurityTokenUtil.h>
#endif

#define THUNDER_CONTROLLER_OPEN_RETRY_MAX      (15)
#define THUNDER_CONTROLLER_OPEN_RETRY_INTERVAL (5000)
#define THUNDER_SECURITY_TOKEN_MAX_LENGTH      (2048)

using namespace Thunder;
using namespace Controller;
using namespace WPEFramework;

static ctrlm_thunder_controller_t *_instance = NULL;

static void _on_activation_change(ctrlm_thunder_controller_t *controller, JsonObject params) {
    plugin_state_t state = PLUGIN_INVALID;
    if(params.HasLabel("callsign")) {
        if(params.HasLabel("state")) {
            if("Activated" == params["state"].String()) {
            state = PLUGIN_ACTIVATED;
            } else if("Activation" == params["state"].String()) {
                state = PLUGIN_ACTIVATING;
            } else if("Deactivated" == params["state"].String()) {
                state = PLUGIN_DEACTIVATED;
            } else if("Deactivation" == params["state"].String()) {
                state = PLUGIN_DEACTIVATING;
            }
        } else {
            XLOGD_WARN("state field doesn't exist");
        }
        controller->on_activation_change(params["callsign"].String(), state);
    } else {
        XLOGD_WARN("callsign field doesn't exist");
    }
}

ctrlm_thunder_controller_t::ctrlm_thunder_controller_t() {
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
    
    this->ready = false;
    this->open_controller_retry_count = 0;
    this->controller_client = NULL;
    if(!this->open_controller()) {
        g_timeout_add(THUNDER_CONTROLLER_OPEN_RETRY_INTERVAL, &ctrlm_thunder_controller_t::open_controller_retry, (void *)this);
    }
}

ctrlm_thunder_controller_t::~ctrlm_thunder_controller_t() {
     auto controllerClient = (JSONRPC::LinkType<Core::JSON::IElement>*)this->controller_client;
     if(controllerClient) {
         delete controllerClient;
         this->controller_client = NULL;
     }
}

ctrlm_thunder_controller_t *ctrlm_thunder_controller_t::getInstance() {
    if(_instance == NULL) {
        _instance = new ctrlm_thunder_controller_t();
    }
    return(_instance);
}

bool ctrlm_thunder_controller_t::is_ready() {
    return(this->ready);
}

bool ctrlm_thunder_controller_t::call(std::string method, void *params, void *response) {
    bool ret = false;
    auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->controller_client;
    JsonObject *jsonParams = (JsonObject *)params;
    JsonObject *jsonResponse = (JsonObject *)response;
    if(clientObject) {
        if(!method.empty() && jsonParams && jsonResponse) {
            uint32_t thunderRet = clientObject->Invoke<JsonObject, JsonObject>(CALL_TIMEOUT, _T(method), *jsonParams, *jsonResponse);
            if(thunderRet == Core::ERROR_NONE) {
                ret = true;
            } else {
                XLOGD_ERROR("Thunder call failed <%s> <%d>", method.c_str(), thunderRet);
            }
        } else {
            XLOGD_ERROR("Invalid parameters");
        }
    } else {
        XLOGD_ERROR("Client is NULL");
    }
    return(ret);
}

plugin_state_t ctrlm_thunder_controller_t::get_plugin_state(std::string callsign) {
    plugin_state_t ret = PLUGIN_INVALID;
    auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)controller_client;
    if(clientObject) {
        Core::JSON::ArrayType<PluginHost::MetaData::Service> response;
        uint32_t thunderRet = clientObject->Get<Core::JSON::ArrayType<PluginHost::MetaData::Service> >(CALL_TIMEOUT, std::string("status@") + callsign, response);
        if(thunderRet == Core::ERROR_NONE) {
            if(response[0].JSONState == PluginHost::IShell::ACTIVATED) {
                ret = PLUGIN_ACTIVATED;
            } else if(response[0].JSONState == PluginHost::IShell::ACTIVATION) {
                ret = PLUGIN_ACTIVATING;
            } else if(response[0].JSONState == PluginHost::IShell::DEACTIVATION) {
                ret = PLUGIN_DEACTIVATING;
            } else if(response[0].JSONState == PluginHost::IShell::DEACTIVATED) {
                ret = PLUGIN_DEACTIVATED;
            }
        } else {
            XLOGD_ERROR("Thunder call failed <%u>", thunderRet);
        }
    } else {
        XLOGD_ERROR("Controller client is NULL!");
    }
    return(ret);
}

void ctrlm_thunder_controller_t::on_activation_change(std::string callsign, plugin_state_t state) {
    for(auto &itr : this->activation_callbacks) {
        if(itr.first.find(callsign) != std::string::npos) {
            for(auto &csitr : itr.second) {
                csitr.first(state, csitr.second);
            }
        }
    }
}

bool ctrlm_thunder_controller_t::add_activation_handler(std::string callsign, plugin_activation_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler) {
        this->activation_callbacks[callsign].push_back(std::make_pair(handler, user_data));
        ret = true;
    } else {
        XLOGD_WARN("Handler is NULL");
    }
    return(ret);
}

void ctrlm_thunder_controller_t::remove_activation_handler(std::string callsign, plugin_activation_handler_t handler) {
    for(auto &itr : this->activation_callbacks) {
        if(itr.first.find(callsign) != std::string::npos) {
            auto csitr = itr.second.begin();
            while (csitr != itr.second.end()) {
                if(csitr->first == handler) {
                    csitr = itr.second.erase(csitr);
                } else {
                    csitr++;
                }
            }
        }
    }
}

bool ctrlm_thunder_controller_t::add_ready_handler(thunder_ready_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler) {
        this->ready_callbacks.push_back(std::make_pair(handler, user_data));
        ret = true;
    } else {
        XLOGD_WARN("Handler is NULL");
    }
    return(ret);
}

void ctrlm_thunder_controller_t::remove_ready_handler(thunder_ready_handler_t handler) {
    auto itr = this->ready_callbacks.begin();
    while (itr != this->ready_callbacks.end()) {
        if(itr->first == handler) {
            itr = this->ready_callbacks.erase(itr);
        } else {
            itr++;
        }
    }
}

std::string ctrlm_thunder_controller_t::get_security_token() {
    std::string sToken = "";
#ifdef THUNDER_SECURITY
    unsigned char security_buffer[THUNDER_SECURITY_TOKEN_MAX_LENGTH+1] = {0};
    int security_len = 0;
    errno_t safec_rc = memset_s(security_buffer, sizeof(security_buffer), 0, sizeof(security_buffer));
    ERR_CHK(safec_rc);
    if((security_len = GetSecurityToken(THUNDER_SECURITY_TOKEN_MAX_LENGTH, security_buffer)) > 0) {
        sToken = (char *)security_buffer;
        sToken = "token=" + sToken;
        XLOGD_INFO("Security Token retrieved successfully", sToken.c_str());
    } else {
        if(security_len == 0) {
            XLOGD_WARN("0 length token");
        }
        XLOGD_WARN("Security Token retrieval failed!");
    }
#endif
   return sToken;
}

bool ctrlm_thunder_controller_t::is_thunder_active() {
   return Thunder::Helpers::is_systemd_process_active("wpeframework.service");
}

bool ctrlm_thunder_controller_t::open_controller() {
    bool ret = false;
    if(ctrlm_thunder_controller_t::is_thunder_active()) {
        std::string sToken = ctrlm_thunder_controller_t::get_security_token();
        if(sToken.empty()) {
            XLOGD_WARN("Thunder security token is empty..");
        }
        auto controllerClient = new JSONRPC::LinkType<Core::JSON::IElement>(_T(""), _T(""), false, sToken);
        this->controller_client = controllerClient;
        if(controllerClient == NULL) {
            XLOGD_ERROR("Controller creation failed...");
        } else {
            uint32_t thunderRet = controllerClient->Subscribe<JsonObject>(CALL_TIMEOUT, _T("statechange"), &_on_activation_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("Thunder subscribe failed <statechange> <%u>", thunderRet);
            } else {
                ret = true;
            }
        }
        if(ret && this->ready == false) {
            this->ready = true;
            for(auto &itr : this->ready_callbacks) {
                itr.first(itr.second);
            }
        } else if(ret == false) {
            delete controllerClient;
            this->controller_client = NULL;
        }
    }
    return(ret);
}

int ctrlm_thunder_controller_t::open_controller_retry(void *data) {
    ctrlm_thunder_controller_t *controller = (ctrlm_thunder_controller_t *)data;
    if(controller) {
        if(!controller->open_controller()) {
            controller->open_controller_retry_count++;
            if(controller->open_controller_retry_count > THUNDER_CONTROLLER_OPEN_RETRY_MAX) {
                XLOGD_FATAL("Retried %d times and still failing, giving up", THUNDER_CONTROLLER_OPEN_RETRY_MAX);
                return(0);
            }
            return(1);
        }
    }
    return(0);
}

const char *Thunder::plugin_state_str(plugin_state_t state) {
    switch(state) {
        case PLUGIN_BOOT_ACTIVATED: return("ACTIVATED (BOOT)");
        case PLUGIN_ACTIVATING:     return("ACTIVATING");
        case PLUGIN_ACTIVATED:      return("ACTIVATED");
        case PLUGIN_DEACTIVATING:   return("DEACTIVATING");
        case PLUGIN_DEACTIVATED:    return("DEACTIVATED");
        default: break;
    }
    return("INVALID");
}
