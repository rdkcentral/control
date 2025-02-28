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
#include "ctrlmf_thunder_plugin.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>
#include <sstream>
#include <secure_wrapper.h>

using namespace Thunder;
using namespace Plugin;
using namespace WPEFramework;

#define REGISTER_EVENTS_RETRY_MAX (5)

static void _on_activation_change(Thunder::plugin_state_t state, void *data) {
    ctrlm_thunder_plugin_t *plugin = (ctrlm_thunder_plugin_t *)data;
    if(plugin) {
        plugin->on_activation_change(state);
    } else {
        XLOGD_ERROR("%s: Plugin is NULL\n", __FUNCTION__);
    }
}

static void _on_thunder_ready(void *data) {
    ctrlm_thunder_plugin_t *plugin = (ctrlm_thunder_plugin_t *)data;
    if(plugin) {
        plugin->on_thunder_ready();
    } else {
        XLOGD_ERROR("%s: Plugin is NULL\n", __FUNCTION__);
    }
}

ctrlm_thunder_plugin_t::ctrlm_thunder_plugin_t(std::string name, std::string callsign, int api_version) {
    XLOGD_INFO("%s: Plugin <%s> Callsign <%s> API Version <%d>\n", __FUNCTION__, name.c_str(), callsign.c_str(), api_version);
    std::string sToken;
    this->name     = name;
    this->callsign = callsign;
    this->api_version = api_version;
    this->register_events_retry = 0;
    this->controller    = Thunder::Controller::ctrlm_thunder_controller_t::getInstance();
    this->plugin_client = NULL;

    if(this->controller) {
        if(!this->controller->is_ready()) {
            XLOGD_INFO("%s: Thunder is not ready, setting up thunder ready handler\n", __FUNCTION__);
            this->controller->add_ready_handler(_on_thunder_ready, (void *)this);
        } else {
            this->on_thunder_ready(true);
        }
        this->controller->add_activation_handler(this->callsign, _on_activation_change, (void *)this);
    } else {
        XLOGD_FATAL("%s: Controller is NULL, state eventing will not work\n", __FUNCTION__);
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
        XLOGD_ERROR("%s: Controller is NULL!\n", __FUNCTION__);
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
        XLOGD_INFO("%s: Activation handler added\n", __FUNCTION__);
        this->activation_callbacks.push_back(std::make_pair(handler, user_data));
        ret = true;
    } else {
        XLOGD_WARN("%s: Activation handler is NULL!\n", __FUNCTION__);
    }
    return(ret);
}

void ctrlm_thunder_plugin_t::remove_activation_handler(plugin_activation_handler_t handler) {
    auto itr = this->activation_callbacks.begin();
    while(itr != this->activation_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("%s: Activation handler removed\n", __FUNCTION__);
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
        XLOGD_INFO("%s: %s activated, registering events\n", __FUNCTION__, plugin->name.c_str());
        ret = (plugin->register_events() ? 0 : 1); // If this fails, try to register again.
        if(ret == 1) {
            plugin->register_events_retry++;
            if(plugin->register_events_retry > REGISTER_EVENTS_RETRY_MAX) {
                XLOGD_FATAL("%s: Failed to register events %d times, no longer trying again...\n", __FUNCTION__, REGISTER_EVENTS_RETRY_MAX);
                ret = 0;
            }
        } else {
            plugin->on_initial_activation();
        }
    } else {
        XLOGD_ERROR("%s: Plugin is NULL\n", __FUNCTION__);
    }
    return(ret);
}

void ctrlm_thunder_plugin_t::on_activation_change(plugin_state_t state) {
    if(state == PLUGIN_ACTIVATED || state == PLUGIN_BOOT_ACTIVATED) {
        //g_timeout_add(100, &ctrlm_thunder_plugin_t::on_plugin_activated, (void *)this);
       // TODO
    }
    for(auto itr : this->activation_callbacks) {
        itr.first(state, itr.second);
    }
}

void ctrlm_thunder_plugin_t::on_thunder_ready(bool boot) {
    std::string callsign_api = this->callsign_with_api();
    XLOGD_INFO("%s: Thunder is now ready, create plugin client for %s\n", __FUNCTION__, callsign_api.c_str());
    auto pluginClient = new JSONRPC::LinkType<Core::JSON::IElement>(_T(callsign_api), _T(""), false, Thunder::Controller::ctrlm_thunder_controller_t::get_security_token());
    if(pluginClient == NULL) {
        XLOGD_ERROR("%s: NULL pluginClient\n", __FUNCTION__);
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

bool ctrlm_thunder_plugin_t::call_plugin(std::string method, void *params, void *response) {
    bool ret = false;
    auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
    JsonObject *jsonParams = (JsonObject *)params;
    JsonObject *jsonResponse = (JsonObject *)response;
    if(clientObject) {
        if(!method.empty() && jsonParams && jsonResponse) {
            uint32_t thunderRet = clientObject->Invoke<JsonObject, JsonObject>(CALL_TIMEOUT, _T(method), *jsonParams, *jsonResponse);
            if(thunderRet == Core::ERROR_NONE) {
                ret = true;
            } else {
                XLOGD_ERROR("%s: Thunder call failed <%s> <%u>\n", __FUNCTION__, method.c_str(), thunderRet);
            }
        } else {
            XLOGD_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        }
    } else {
        XLOGD_ERROR("%s: Client is NULL\n", __FUNCTION__);
    }
    return(ret);
}

bool ctrlm_thunder_plugin_t::call_controller(std::string method, void *params, void *response) {
    bool ret = false;
    if(this->controller) {
        ret = this->controller->call(method, params, response);
    } else {
        XLOGD_ERROR("%s: Controller is NULL\n", __FUNCTION__);
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
