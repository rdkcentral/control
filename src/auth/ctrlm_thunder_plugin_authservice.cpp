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
#include "ctrlm_thunder_plugin_authservice.h"
#include "ctrlm_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace AuthService;
using namespace WPEFramework;

static void _on_sat_change(ctrlm_thunder_plugin_authservice_t *plugin, JsonObject params) {
    plugin->on_sat_change();
}

static void _on_account_id_change(ctrlm_thunder_plugin_authservice_t *plugin, JsonObject params) {
    plugin->on_account_id_change(params["newServiceAccountId"].String());
}

static void _on_activation_status_change(ctrlm_thunder_plugin_authservice_t *plugin, JsonObject params) {
    plugin->on_activation_status_change(params["newActivationStatus"].String());
}

ctrlm_thunder_plugin_authservice_t::ctrlm_thunder_plugin_authservice_t() : ctrlm_thunder_plugin_t("Authservice", "org.rdk.AuthService", 1) {
    this->registered_events = false;
}

ctrlm_thunder_plugin_authservice_t::~ctrlm_thunder_plugin_authservice_t() {

}

ctrlm_thunder_plugin_authservice_t *ctrlm_thunder_plugin_authservice_t::getInstance() {
    XLOGD_INFO("authservice\n");
    static ctrlm_thunder_plugin_authservice_t instance;
    return &instance;
}

bool ctrlm_thunder_plugin_authservice_t::is_device_activated() {
    bool ret = false;
    JsonObject params, response;
    if(this->call_plugin("getActivationStatus", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            if(response["status"].String() == "activated") {
                ret = true;
            } else {
                XLOGD_WARN("Device activatation status is not activated <%s>", response["status"].String().c_str());
            }
        } else {
            XLOGD_WARN("Success for getActivationStatus was false");
        }
    } else {
        XLOGD_WARN("Call for getActivationStatus failed");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_authservice_t::get_device_id(std::string &device_id) {
    bool ret = false;
    JsonObject params, response;
    if(this->call_plugin("getXDeviceId", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            device_id = response["xDeviceId"].String();
            if(!device_id.empty()) {
                ret = true;
            }
        } else {
            XLOGD_WARN("Success for getXDeviceId was false");
        }
    } else {
        XLOGD_WARN("Call for getXDeviceId failed");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_authservice_t::get_partner_id(std::string &partner_id) {
    bool ret = false;
    JsonObject params, response;
    if(this->call_plugin("getDeviceId", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            partner_id = response["partnerId"].String();
            if(!partner_id.empty()) {
                ret = true;
            }
        } else {
            XLOGD_WARN("Success for getPartnerId was false");
        }
    } else {
        XLOGD_WARN("Call for getPartnerId failed");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_authservice_t::get_account_id(std::string &account_id) {
    bool ret = false;
    JsonObject params, response;
    if(this->call_plugin("getServiceAccountId", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            account_id = response["serviceAccountId"].String();
            if(!account_id.empty()) {
                ret = true;
            }
        } else {
            XLOGD_WARN("Success for getServiceAccountId was false");
        }
    } else {
        XLOGD_WARN("Call for getServiceAccountId failed");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_authservice_t::get_sat(std::string &sat, time_t &expiration) {
    bool ret = false;
    JsonObject params, response;
    if(this->call_plugin("getServiceAccessToken", (void *)&params, (void *)&response)) {
        if(response["success"].Boolean()) { // If success doesn't exist, it defaults to false which is fine.
            if(response["status"].Number() == 0) {
               std::string temp = response["token"].String();
               if(temp.length() > 0) {
                  sat        = std::move(temp);
                  expiration = time(NULL) + response["expires"].Number();
                  ret = true;
               } else {
                   XLOGD_WARN("SAT Token length 0");
               }
            } else {
                XLOGD_WARN("SAT Token status not 0");
            }
        } else {
            XLOGD_WARN("Success for getServiceAccessToken was false");
        }
    } else {
        XLOGD_WARN("Call for getServiceAccessToken failed");
    }
    return(ret);
}

bool ctrlm_thunder_plugin_authservice_t::add_event_handler(authservice_event_handler_t handler, void *user_data) {
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

void ctrlm_thunder_plugin_authservice_t::remove_event_handler(authservice_event_handler_t handler) {
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


void ctrlm_thunder_plugin_authservice_t::on_sat_change() {
    for(auto &itr : this->event_callbacks) {
        itr.first(EVENT_SAT_CHANGED, NULL, itr.second);
    }
}

void ctrlm_thunder_plugin_authservice_t::on_account_id_change(std::string new_account_id) {
    for(auto &itr : this->event_callbacks) {
        itr.first(EVENT_ACCOUNT_ID_CHANGED, (void *)new_account_id.c_str(), itr.second);
    }
}

void ctrlm_thunder_plugin_authservice_t::on_activation_status_change(std::string status) {
    if(status == "activated") {
        for(auto &itr : this->event_callbacks) {
            itr.first(EVENT_ACTIVATION_STATUS_ACTIVATED, NULL, itr.second);
        }
    }
}

bool ctrlm_thunder_plugin_authservice_t::register_events() {
    bool ret = this->registered_events;
    if(ret == false) {
        auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
        if(clientObject) {
            ret = true;
            uint32_t thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("serviceAccessTokenChanged"), &_on_sat_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("Thunder subscribe failed <serviceAccessTokenChanged>");
                ret = false;
            }

            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onServiceAccountIdChanged"), &_on_account_id_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("Thunder subscribe failed <onServiceAccountIdChanged>");
                ret = false;
            }

            thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onActivationStatusChanged"), &_on_activation_status_change, this);
            if(thunderRet != Core::ERROR_NONE) {
                XLOGD_ERROR("Thunder subscribe failed <onActivationStatusChanged>");
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
