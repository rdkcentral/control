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
#include "ctrlm_rfc.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include "rfcapi.h"
#include "ctrlm_log.h"
#include "ctrlm_config_default.h"

#define RFC_CALLER_ID "controlMgr"

static ctrlm_rfc_t *_instance = NULL;

ctrlm_rfc_t::ctrlm_rfc_t() {
    // Create the RFC Attr objects
    ctrlm_rfc_attr_t *vsdk           = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.vsdk", JSON_OBJ_NAME_VSDK);
    ctrlm_rfc_attr_t *voice          = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.voice", JSON_OBJ_NAME_VOICE);
    #ifdef CTRLM_NETWORK_RF4CE
    ctrlm_rfc_attr_t *rf4ce          = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.network_rf4ce", JSON_OBJ_NAME_NETWORK_RF4CE);
    #endif
    ctrlm_rfc_attr_t *ble            = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.network_ble", JSON_OBJ_NAME_NETWORK_BLE);
    ctrlm_rfc_attr_t *ip             = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.network_ip", JSON_OBJ_NAME_NETWORK_IP);
    ctrlm_rfc_attr_t *global         = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.global", JSON_OBJ_NAME_CTRLM_GLOBAL);
    ctrlm_rfc_attr_t *device_update  = new ctrlm_rfc_attr_t("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.device_update", JSON_OBJ_NAME_DEVICE_UPDATE);

    this->add_attribute(ctrlm_rfc_t::attrs::VSDK,          vsdk);
    this->add_attribute(ctrlm_rfc_t::attrs::VOICE,         voice);
    #ifdef CTRLM_NETWORK_RF4CE
    this->add_attribute(ctrlm_rfc_t::attrs::RF4CE,         rf4ce);
    #endif
    this->add_attribute(ctrlm_rfc_t::attrs::BLE,           ble);
    this->add_attribute(ctrlm_rfc_t::attrs::IP,            ip);
    this->add_attribute(ctrlm_rfc_t::attrs::GLOBAL,        global);
    this->add_attribute(ctrlm_rfc_t::attrs::DEVICE_UPDATE, device_update);
}

ctrlm_rfc_t::~ctrlm_rfc_t() {
    for(auto &itr : this->attributes) {
        if(itr.second) {
            delete(itr.second);
        }
    }
}

ctrlm_rfc_t *ctrlm_rfc_t::get_instance() {
    if(_instance == NULL) {
        _instance = new ctrlm_rfc_t();
    }
    return(_instance);
}

void ctrlm_rfc_t::destroy_instance() {
    if(_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}

void ctrlm_rfc_t::add_fetch_completed_listener(const ctrlm_rfc_fetch_completed_t &listener) {
   this->listeners.push_back(listener);
}

void ctrlm_rfc_t::remove_fetch_completed_listener(const ctrlm_rfc_fetch_completed_t &listener) {
   // TODO
}

void ctrlm_rfc_t::add_changed_listener(ctrlm_rfc_t::attrs type, const ctrlm_rfc_attr_changed_t &listener) {
    if(this->attributes.count(type) > 0) {
        this->attributes[type]->add_changed_listener(listener);
    } else {
        XLOGD_WARN("RFC attribute doesn't exist to add listener");
    }
}

void ctrlm_rfc_t::remove_changed_listener(ctrlm_rfc_t::attrs type, const ctrlm_rfc_attr_changed_t &listener) {
    if(this->attributes.count(type) > 0) {
        this->attributes[type]->remove_changed_listener(listener);
    } else {
        XLOGD_WARN("RFC attribute doesn't exist to remove listener");
    }
}

void ctrlm_rfc_t::add_attribute(ctrlm_rfc_t::attrs type, ctrlm_rfc_attr_t *attr) {
    if(this->attributes.count(type) > 0) {
        delete(this->attributes[type]);
    }
    this->attributes[type] = attr;
}

void ctrlm_rfc_t::remove_attribute(ctrlm_rfc_t::attrs type) {
    if(this->attributes.count(type) > 0) {
        delete(this->attributes[type]);
    }
    this->attributes.erase(type);
}

void ctrlm_rfc_t::fetch_attributes() {
    XLOGD_INFO("fetching RFC attributes");
    for(const auto &itr : this->attributes) {
        ctrlm_rfc_attr_t *attr = itr.second;
        if(attr->is_enabled()) {
            XLOGD_INFO("fetching <%s>", attr->get_tr181_string().c_str());
            std::string value = this->tr181_call(attr);
            if(!value.empty()) {
                attr->set_rfc_value(value);
            } else {
                XLOGD_DEBUG("tr181 value for <%s> is empty", attr->get_tr181_string().c_str());
            }
        } else {
            XLOGD_DEBUG("rfc is disabled for <%s>", attr->get_tr181_string().c_str());
        }
    }
    // Notify fetch completed listeners
    for(auto &itr : this->listeners) {
        itr();
    }
}

std::string ctrlm_rfc_t::tr181_call(ctrlm_rfc_attr_t *attr) {
    std::string ret;
    if(attr) {
        RFC_ParamData_t param;
        WDMP_STATUS rc = getRFCParameter((char *)RFC_CALLER_ID, attr->get_tr181_string().c_str(), &param);
        if(WDMP_SUCCESS != rc) {
            if(rc == WDMP_ERR_VALUE_IS_EMPTY) {
                XLOGD_WARN("rfc call failed for <%s> <%d, %s>", attr->get_tr181_string().c_str(), rc, getRFCErrorString(rc));
            } else {
                XLOGD_ERROR("rfc call failed for <%s> <%d, %s>", attr->get_tr181_string().c_str(), rc, getRFCErrorString(rc));
            }
        } else {
            ret = std::string(param.value);
        }
    }
    return(ret);
}
