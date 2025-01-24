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
#include "ctrlm_thunder_plugin_device_info.h"
#include "ctrlm_thunder_log.h"
#include "ctrlm_utils.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>
#include <glib.h>

using namespace Thunder;
using namespace DeviceInfo;
using namespace WPEFramework;

ctrlm_thunder_plugin_device_info_t::ctrlm_thunder_plugin_device_info_t() : ctrlm_thunder_plugin_t("DeviceInfo", "DeviceInfo", 1) {
    sem_init(&this->semaphore, 0, 1);
    this->device_type = CTRLM_DEVICE_TYPE_INVALID;
    this->activated   = false;
}

ctrlm_thunder_plugin_device_info_t::~ctrlm_thunder_plugin_device_info_t() {

}

ctrlm_thunder_plugin_device_info_t *ctrlm_thunder_plugin_device_info_t::getInstance() {
    XLOGD_INFO("device info");
    static ctrlm_thunder_plugin_device_info_t instance;
    return &instance;
}

bool ctrlm_thunder_plugin_device_info_t::add_on_activation_handler(on_activation_handler_t handler, void *user_data) {
    bool ret = false;
    if(handler != NULL) {
        if(this->activated) {
            handler(user_data);
        } else {
            XLOGD_INFO("%s Event handler added", this->name.c_str());
            this->on_activation_callbacks.push_back(std::make_pair(handler, user_data));
        }
        ret = true;
    } else {
        XLOGD_ERROR("%s Event handler is NULL", this->name.c_str());
    }
    return(ret);
}

void ctrlm_thunder_plugin_device_info_t::remove_on_activation_handler(on_activation_handler_t handler) {
    auto itr = this->on_activation_callbacks.begin();
    while(itr != this->on_activation_callbacks.end()) {
        if(itr->first == handler) {
            XLOGD_INFO("%s Event handler removed", this->name.c_str());
            itr = this->on_activation_callbacks.erase(itr);
        } else {
            ++itr;
        }
    }
}

bool ctrlm_thunder_plugin_device_info_t::get_device_type(ctrlm_device_type_t &device_type) {
    bool ret = false;
    sem_wait(&this->semaphore);
    if(!this->activated) {
        XLOGD_INFO("%s not activated", this->name.c_str());
    } else {
        device_type = this->device_type;
        XLOGD_INFO("%s device type <%s>", this->name.c_str(), ctrlm_device_type_str(device_type));
        ret = true;
    }
    sem_post(&this->semaphore);
    return(ret);
}

bool ctrlm_thunder_plugin_device_info_t::_get_device_type() {
    bool ret = false;
    JsonObject response;

    XLOGD_INFO("Calling DeviceInfo for device type");

    sem_wait(&this->semaphore);
    const char *property = "devicetype";

    if(this->property_get(property, (void *)&response, 2)) {
        if(response.HasLabel(property)) {
            std::string device_type = response[property].String();
            XLOGD_INFO("device type <%s>", device_type.c_str());

            if(device_type == "IpStb") {
                this->device_type = CTRLM_DEVICE_TYPE_STB_IP;
            } else if(device_type == "QamIpStb") {
                this->device_type = CTRLM_DEVICE_TYPE_STB_QAM;
            } else if(device_type == "tv") {
                this->device_type = CTRLM_DEVICE_TYPE_TV;
            } else {
                XLOGD_ERROR("unknown device type <%s>", device_type.c_str());
            }
        } else {
            std::string response_str;
            response.ToString(response_str);
            XLOGD_ERROR("DeviceInfo device type response malformed: <%s>", response_str.c_str());
        }
    } else {
        XLOGD_ERROR("DeviceInfo device type call failed!");
    }

    sem_post(&this->semaphore);
    return(ret);
}


void ctrlm_thunder_plugin_device_info_t::on_initial_activation() {
    // Get initial device type
    this->_get_device_type();
    
    Thunder::Plugin::ctrlm_thunder_plugin_t::on_initial_activation();

    this->activated = true;

    // Call the on_activation callbacks
    for(auto &itr : this->on_activation_callbacks) {
        itr.first(itr.second);
    }

    // Remove all the on_activation callbacks
    this->on_activation_callbacks.clear();
}
