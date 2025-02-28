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
#include <ctrlm_fta_lib.h>
#include <secure_wrapper.h>
#include "ctrlmf_thunder_controller.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

#define THUNDER_CONTROLLER_OPEN_RETRY_MAX      (15)
#define THUNDER_CONTROLLER_OPEN_RETRY_INTERVAL (5000)

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
            XLOGD_WARN("%s: state field doesn't exist\n", __FUNCTION__);
        }
        controller->on_activation_change(params["callsign"].String(), state);
    } else {
        XLOGD_WARN("%s: callsign field doesn't exist\n", __FUNCTION__);
    }
}

ctrlm_thunder_controller_t::ctrlm_thunder_controller_t() {
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
    
    this->ready = false;
    this->open_controller_retry_count = 0;
    this->controller_client = NULL;
    if(!this->open_controller()) {
        //g_timeout_add(THUNDER_CONTROLLER_OPEN_RETRY_INTERVAL, &ctrlm_thunder_controller_t::open_controller_retry, (void *)this);
        // TODO
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
                XLOGD_ERROR("%s: Thunder call failed <%s> <%d>\n", __FUNCTION__, method.c_str(), thunderRet);
            }
        } else {
            XLOGD_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        }
    } else {
        XLOGD_ERROR("%s: Client is NULL\n", __FUNCTION__);
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
            XLOGD_ERROR("%s: Thunder call failed <%u>\n", __FUNCTION__, thunderRet);
        }
    } else {
        XLOGD_ERROR("%s: Controller client is NULL!\n", __FUNCTION__);
    }
    return(ret);
}

void ctrlm_thunder_controller_t::on_activation_change(std::string callsign, plugin_state_t state) {
    for(auto itr : this->activation_callbacks) {
        if(itr.first.find(callsign) != std::string::npos) {
            for(auto csitr : itr.second) {
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
        XLOGD_WARN("%s: Handler is NULL\n", __FUNCTION__);
    }
    return(ret);
}

void ctrlm_thunder_controller_t::remove_activation_handler(std::string callsign, plugin_activation_handler_t handler) {
    for(auto itr : this->activation_callbacks) {
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
        XLOGD_WARN("%s: Handler is NULL\n", __FUNCTION__);
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
    FILE *pSecurity = v_secure_popen("r", "/usr/bin/WPEFrameworkSecurityUtility");
    if(pSecurity) {
        JsonObject pSecurityJson;
        std::string pSecurityOutput;
        std::array<char, 256> pSecurityBuffer;

        while(fgets(pSecurityBuffer.data(), 256, pSecurity) != NULL) {
            pSecurityOutput += pSecurityBuffer.data();
        }
        v_secure_pclose(pSecurity);
        pSecurityJson.FromString(pSecurityOutput);
        if(pSecurityJson["success"].Boolean() == true) {
            XLOGD_INFO("%s: Security Token retrieved successfully\n", __FUNCTION__);
            sToken = "token=" + pSecurityJson["token"].String();
        } else {
            XLOGD_ERROR("%s: Security Token retrieval failed!\n", __FUNCTION__);
        }
    } else {
        XLOGD_ERROR("%s: Failed to open security utility\n", __FUNCTION__);
    }
#endif
   return sToken;
}

bool ctrlm_thunder_controller_t::is_thunder_active() {
   return(ctrlmf_systemd_is_service_active("wpeframework.service"));
}

bool ctrlm_thunder_controller_t::open_controller() {
    bool ret = false;
    if(ctrlm_thunder_controller_t::is_thunder_active()) {
        std::string sToken = ctrlm_thunder_controller_t::get_security_token();
        if(sToken.empty()) {
            XLOGD_WARN("%s: Thunder security token is empty..\n", __FUNCTION__);
        }
        auto controllerClient = new JSONRPC::LinkType<Core::JSON::IElement>(_T(""), _T(""), false, sToken);
        this->controller_client = controllerClient;
        if(controllerClient == NULL) {
            XLOGD_ERROR("%s: Controller creation failed...\n", __FUNCTION__);
        } else {
            uint32_t thunderRet = controllerClient->Subscribe<JsonObject>(CALL_TIMEOUT, _T("statechange"), &_on_activation_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("%s: Thunder subscribe failed <statechange> <%u>\n", __FUNCTION__, thunderRet);
            } else {
                ret = true;
            }
        }
        if(ret && this->ready == false) {
            this->ready = true;
            for(auto itr : this->ready_callbacks) {
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
                XLOGD_FATAL("%s: Retried %d times and still failing, giving up\n", __FUNCTION__, THUNDER_CONTROLLER_OPEN_RETRY_MAX);
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
