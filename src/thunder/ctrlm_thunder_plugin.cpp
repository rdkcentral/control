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
#include "ctrlm_thunder_plugin.h"
#include "ctrlm_thunder_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>
#include <sstream>
#include <secure_wrapper.h>
#include <glib.h>

using namespace Thunder;
using namespace Plugin;
using namespace WPEFramework;

#define REGISTER_EVENTS_RETRY_MAX (5)

static void _on_activation_change(Thunder::plugin_state_t state, void *data) {
    ctrlm_thunder_plugin_t *plugin = (ctrlm_thunder_plugin_t *)data;
    if(plugin) {
        plugin->on_activation_change(state);
    } else {
        XLOGD_ERROR("Plugin is NULL");
    }
}

static void _on_thunder_ready(void *data) {
    ctrlm_thunder_plugin_t *plugin = (ctrlm_thunder_plugin_t *)data;
    if(plugin) {
        plugin->on_thunder_ready();
    } else {
        XLOGD_ERROR("Plugin is NULL");
    }
}

ctrlm_thunder_plugin_t::ctrlm_thunder_plugin_t(std::string name, std::string callsign, int api_version) {
    XLOGD_INFO("Plugin <%s> Callsign <%s> API Version <%d>", name.c_str(), callsign.c_str(), api_version);
    this->name     = std::move(name);
    this->callsign = std::move(callsign);
    this->api_version = api_version;
    this->register_events_retry = 0;
    this->controller    = Thunder::Controller::ctrlm_thunder_controller_t::getInstance();
    this->plugin_client = NULL;

    if(this->controller) {
        if(!this->controller->is_ready()) {
            XLOGD_INFO("Thunder is not ready, setting up thunder ready handler");
            this->controller->add_ready_handler(_on_thunder_ready, (void *)this);
        } else {
            this->on_thunder_ready(true);
        }
        this->controller->add_activation_handler(this->callsign, _on_activation_change, (void *)this);
    } else {
        XLOGD_FATAL("Controller is NULL, state eventing will not work");
    }
}

ctrlm_thunder_plugin_t::~ctrlm_thunder_plugin_t() {
    auto pluginClient = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;

    if(pluginClient){
        delete pluginClient;
        this->plugin_client = NULL;
    }

    if(this->controller) {
        this->controller->remove_activation_handler(this->callsign, _on_activation_change);
    }
}

std::string ctrlm_thunder_plugin_t::callsign_with_api() {
    std::stringstream ret;
    ret << this->callsign << "." << this->api_version;
    return(ret.str());
}

bool ctrlm_thunder_plugin_t::is_activated() {
    bool ret = false;
    if(this->controller) {
        ret = (this->controller->get_plugin_state(this->callsign) == PLUGIN_ACTIVATED);
    } else {
        XLOGD_ERROR("Controller is NULL!");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_t::activate() {
    JsonObject params, response;
    params["callsign"] = this->callsign;
    return(this->call_controller("activate", (void *)&params, (void *)&response));
}

bool ctrlm_thunder_plugin_t::deactivate() {
    JsonObject params, response;
    params["callsign"] = this->callsign;
    return(this->call_controller("deactivate", (void *)&params, (void *)&response));
}

bool ctrlm_thunder_plugin_t::add_activation_handler(plugin_activation_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        XLOGD_INFO("Activation handler added");
        this->activation_callbacks.push_back(std::make_pair(handler, user_data));
        ret = true;
    } else {
        XLOGD_WARN("Activation handler is NULL!");
    }
    return(ret);
}

void ctrlm_thunder_plugin_t::remove_activation_handler(plugin_activation_handler_t handler) {
    auto itr = this->activation_callbacks.begin();
    while(itr != this->activation_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("Activation handler removed");
            itr = this->activation_callbacks.erase(itr);
        } else {
            ++itr;
        }
    }
}

int ctrlm_thunder_plugin_t::on_plugin_activated(void *data) {
    bool ret = 0;
    ctrlm_thunder_plugin_t *plugin = (ctrlm_thunder_plugin_t *)data;
    if(plugin) {
        XLOGD_INFO("%s activated, registering events", plugin->name.c_str());
        ret = (plugin->register_events() ? 0 : 1); // If this fails, try to register again.
        if(ret == 1) {
            plugin->register_events_retry++;
            if(plugin->register_events_retry > REGISTER_EVENTS_RETRY_MAX) {
                XLOGD_FATAL("Failed to register events %d times, no longer trying again...", REGISTER_EVENTS_RETRY_MAX);
                ret = 0;
            }
        } else {
            plugin->on_initial_activation();
        }
    } else {
        XLOGD_ERROR("Plugin is NULL");
    }
    return(ret);
}

void ctrlm_thunder_plugin_t::on_activation_change(plugin_state_t state) {
    if(state == PLUGIN_ACTIVATED || state == PLUGIN_BOOT_ACTIVATED) {
        g_timeout_add(100, &ctrlm_thunder_plugin_t::on_plugin_activated, (void *)this);
    }
    for(auto &itr : this->activation_callbacks) {
        itr.first(state, itr.second);
    }
}

void ctrlm_thunder_plugin_t::on_thunder_ready(bool boot) {
    std::string callsign_api = this->callsign_with_api();
    XLOGD_INFO("Thunder is now ready, create plugin client for %s", callsign_api.c_str());
    auto pluginClient = new JSONRPC::LinkType<Core::JSON::IElement>(_T(callsign_api), _T(""), false, Thunder::Controller::ctrlm_thunder_controller_t::get_security_token());
    if(pluginClient == NULL) {
        XLOGD_ERROR("NULL pluginClient");
    }
    this->plugin_client = (void *)pluginClient;
    if(this->controller) {
        plugin_state_t state = this->controller->get_plugin_state(this->callsign);
        if(boot && state == PLUGIN_ACTIVATED) {
            state = PLUGIN_BOOT_ACTIVATED;
        }
        this->on_activation_change(state);
    }
}

bool ctrlm_thunder_plugin_t::property_get(std::string property, void *response, unsigned int retries) {
    bool ret = false;
    unsigned int attempts = 0;
    auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
    JsonObject *jsonResponse = (JsonObject *)response;
    if(clientObject) {
        if(!property.empty() && jsonResponse) {
            uint32_t thunderRet = Core::ERROR_TIMEDOUT;
            const char *method = property.c_str(); 
            while(thunderRet == Core::ERROR_TIMEDOUT && attempts <= retries) { // We only want to retry if return code is for TIMEDOUT
                thunderRet = clientObject->GetProperty(method, *jsonResponse, CALL_TIMEOUT);
                if(thunderRet == Core::ERROR_NONE) {
                    ret = true;
                } else {
                    attempts += 1;
                    XLOGD_ERROR("Thunder property get failed <%s> <%u>, attempt %u of %u", method, thunderRet, attempts, (thunderRet == Core::ERROR_TIMEDOUT ? retries : 0) + 1); // retries + initial attempt
                }
            }
        } else {
            XLOGD_ERROR("Invalid parameters");
        }
    } else {
        XLOGD_ERROR("Client is NULL");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_t::call_plugin(std::string method, void *params, void *response, unsigned int retries) {
    bool ret = false;
    unsigned int attempts = 0;
    auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
    JsonObject *jsonParams = (JsonObject *)params;
    JsonObject *jsonResponse = (JsonObject *)response;
    if(clientObject) {
        if(!method.empty() && jsonParams && jsonResponse) {
            uint32_t thunderRet = Core::ERROR_TIMEDOUT;
            while(thunderRet == Core::ERROR_TIMEDOUT && attempts <= retries) { // We only want to retry if return code is for TIMEDOUT
                thunderRet = clientObject->Invoke<JsonObject, JsonObject>(CALL_TIMEOUT, _T(method), *jsonParams, *jsonResponse);
                if(thunderRet == Core::ERROR_NONE) {
                    ret = true;
                } else {
                    attempts += 1;
                    XLOGD_ERROR("Thunder call failed <%s> <%u>, attempt %u of %u", method.c_str(), thunderRet, attempts, (thunderRet == Core::ERROR_TIMEDOUT ? retries : 0) + 1); // retries + initial attempt
                }
            }
        } else {
            XLOGD_ERROR("Invalid parameters");
        }
    } else {
        XLOGD_ERROR("Client is NULL");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_t::call_controller(std::string method, void *params, void *response) {
    bool ret = false;
    if(this->controller) {
        ret = this->controller->call(std::move(method), params, response);
    } else {
        XLOGD_ERROR("Controller is NULL");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_t::register_events() {
    bool ret = true;
    return(ret);
}

void ctrlm_thunder_plugin_t::on_initial_activation() {
    // Nothing needed here for now
}
