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
#include "ctrlm_rf4ce_network_attr_config.h"
#include "ctrlm_config_types.h"
#include "ctrlm_config_default.h"
#include "ctrlm_tr181.h" // TODO: remove once generic RFC is approved for use
#include "ctrlm.h"
#include "ctrlm_log.h"
#include <functional>
#include <sstream>

// ctrlm_rf4ce_rsp_time_t
#define RESPONSE_TIME_ID             (0x0D)
#define RESPONSE_TIME_PROFILE_XRC    (0xC0)
#define RESPONSE_TIME_PROFILE_XVP    (0xC1)
#define RESPONSE_TIME_PROFILE_XDIU   (0xC2)
#define RESPONSE_TIME_MIN            (100)  // 100ms
#define RESPONSE_TIME_MAX            (1000) // 1 sec
#define RESPONSE_TIME_CONFIG_VBN_MIN (0)    // 0ms
#define RESPONSE_TIME_CONFIG_VBN_MAX (3000) // 3 sec
ctrlm_rf4ce_rsp_time_t::ctrlm_rf4ce_rsp_time_t() :
ctrlm_attr_t("Controller Response Time"),
ctrlm_rf4ce_rib_attr_t(RESPONSE_TIME_ID, RIB_ATTR_INDEX_ALL, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET),
ctrlm_config_attr_t(JSON_OBJ_NAME_NETWORK_RF4CE JSON_PATH_SEPERATOR JSON_OBJ_NAME_NETWORK_RF4CE_RSP_TIME) {
    this->rsp_times[RESPONSE_TIME_PROFILE_XRC]  = JSON_INT_VALUE_NETWORK_RF4CE_RSP_TIME_XRC;
    this->rsp_times[RESPONSE_TIME_PROFILE_XVP]  = JSON_INT_VALUE_NETWORK_RF4CE_RSP_TIME_XVP;
    this->rsp_times[RESPONSE_TIME_PROFILE_XDIU] = JSON_INT_VALUE_NETWORK_RF4CE_RSP_TIME_XDIU;
    ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
    if(rfc) {
        rfc->add_changed_listener(ctrlm_rfc_t::attrs::RF4CE, std::bind(&ctrlm_rf4ce_rsp_time_t::rfc_value_retrieved, this, std::placeholders::_1));
    }
}

ctrlm_rf4ce_rsp_time_t::~ctrlm_rf4ce_rsp_time_t() {
}

unsigned int ctrlm_rf4ce_rsp_time_t::get_ms(uint8_t profile_id) const {
    unsigned int ret = 0;
    if(this->rsp_times.count(profile_id) > 0) {
        ret = this->rsp_times.at(profile_id);
    }
    return(ret);
}

unsigned int ctrlm_rf4ce_rsp_time_t::get_us(uint8_t profile_id) const {
    return(1000 * this->get_ms(profile_id));
}

// TODO: remove once generic RFC is approved for use
void ctrlm_rf4ce_rsp_time_t::legacy_rfc() {
    unsigned int value = 0;
    bool changed = false;
    int rsp_time_min = (ctrlm_is_production_build() ? RESPONSE_TIME_MIN : RESPONSE_TIME_CONFIG_VBN_MIN);
    int rsp_time_max = (ctrlm_is_production_build() ? RESPONSE_TIME_MAX : RESPONSE_TIME_CONFIG_VBN_MAX);
    ctrlm_tr181_result_t result = ctrlm_tr181_int_get(CTRLM_RF4CE_TR181_RSP_TIME_XRC, &value, rsp_time_min, rsp_time_max);
    if(result != CTRLM_TR181_RESULT_SUCCESS) {
        XLOGD_INFO("XRC Response Time not present");
    } else {
        if(value != this->rsp_times[RESPONSE_TIME_PROFILE_XRC]) {
            changed = true;
        }
        this->rsp_times[RESPONSE_TIME_PROFILE_XRC] = value;
        XLOGD_INFO("XRC Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XRC]);
    }

    result = ctrlm_tr181_int_get(CTRLM_RF4CE_TR181_RSP_TIME_XVP, &value, rsp_time_min, rsp_time_max);
    if(result != CTRLM_TR181_RESULT_SUCCESS) {
        XLOGD_INFO("XVP Response Time not present");
    } else {
        if(value != this->rsp_times[RESPONSE_TIME_PROFILE_XVP]) {
            changed = true;
        }
        this->rsp_times[RESPONSE_TIME_PROFILE_XVP] = value;
        XLOGD_INFO("XVP Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XVP]);
    }

    result = ctrlm_tr181_int_get(CTRLM_RF4CE_TR181_RSP_TIME_XDIU, &value, rsp_time_min, rsp_time_max);
    if(result != CTRLM_TR181_RESULT_SUCCESS) {
        XLOGD_INFO("XDIU Response Time not present");
    } else {
        if(value != this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]) {
            changed = true;
        }
        this->rsp_times[RESPONSE_TIME_PROFILE_XDIU] = value;
        XLOGD_INFO("XDIU Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]);
    }

    if(changed && this->updated_listener) {
        XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
        this->updated_listener(*this);
    }
}

std::string ctrlm_rf4ce_rsp_time_t::to_string() const {
    std::stringstream ss;
    ss << "XRC <"  << this->rsp_times.at(RESPONSE_TIME_PROFILE_XRC)   << "ms> ";
    ss << "XVP <"  << this->rsp_times.at(RESPONSE_TIME_PROFILE_XVP)   << "ms> ";
    ss << "XDIU <" << this->rsp_times.at(RESPONSE_TIME_PROFILE_XDIU)  << "ms>";
    return(ss.str());
}

bool ctrlm_rf4ce_rsp_time_t::read_config() {
    bool ret = false;
    unsigned int temp = 0;
    int rsp_time_min = (ctrlm_is_production_build() ? RESPONSE_TIME_MIN : RESPONSE_TIME_CONFIG_VBN_MIN);
    int rsp_time_max = (ctrlm_is_production_build() ? RESPONSE_TIME_MAX : RESPONSE_TIME_CONFIG_VBN_MAX);
    ctrlm_config_int_t xrc(this->path + JSON_PATH_SEPERATOR "xrc");
    ctrlm_config_int_t xvp(this->path + JSON_PATH_SEPERATOR "xvp");
    ctrlm_config_int_t xdiu(this->path + JSON_PATH_SEPERATOR "xdiu");

    if(xrc.get_config_value(temp, rsp_time_min, rsp_time_max)) {
        this->rsp_times[RESPONSE_TIME_PROFILE_XRC] = temp;
        XLOGD_INFO("XRC Response Time from config file: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XRC]);
        ret = true;
    } else {
        XLOGD_INFO("XRC Response Time default: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XRC]);
    }
    if(xvp.get_config_value(temp, rsp_time_min, rsp_time_max)) {
        this->rsp_times[RESPONSE_TIME_PROFILE_XVP] = temp;
        XLOGD_INFO("XVP Response Time from config file: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XVP]);
        ret = true;
    } else {
        XLOGD_INFO("XVP Response Time default: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XVP]);
    }
    if(xdiu.get_config_value(temp, rsp_time_min, rsp_time_max)) {
        this->rsp_times[RESPONSE_TIME_PROFILE_XDIU] = temp;
        XLOGD_INFO("XDIU Response Time from config file: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]);
        ret = true;
    } else {
        XLOGD_INFO("XDIU Response Time default: %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]);
    }

    if(ret && this->updated_listener) {
        XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
        this->updated_listener(*this);
    }

    return(ret);
}

void ctrlm_rf4ce_rsp_time_t::rfc_value_retrieved(const ctrlm_rfc_attr_t &attr) {
    bool changed      = false;
    int rsp_time_min = (ctrlm_is_production_build() ? RESPONSE_TIME_MIN : RESPONSE_TIME_CONFIG_VBN_MIN);
    int rsp_time_max = (ctrlm_is_production_build() ? RESPONSE_TIME_MAX : RESPONSE_TIME_CONFIG_VBN_MAX);

    unsigned int temp = this->rsp_times[RESPONSE_TIME_PROFILE_XRC];
    if(attr.get_rfc_value(JSON_OBJ_NAME_NETWORK_RF4CE_RSP_TIME JSON_PATH_SEPERATOR "xrc",  this->rsp_times[RESPONSE_TIME_PROFILE_XRC],  rsp_time_min, rsp_time_max)) {
        XLOGD_INFO("XRC Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XRC]);
        if(temp != this->rsp_times[RESPONSE_TIME_PROFILE_XRC]) {
            changed = true;
        }
    }
    temp = this->rsp_times[RESPONSE_TIME_PROFILE_XVP];
    if(attr.get_rfc_value(JSON_OBJ_NAME_NETWORK_RF4CE_RSP_TIME JSON_PATH_SEPERATOR "xvp",  this->rsp_times[RESPONSE_TIME_PROFILE_XVP],  rsp_time_min, rsp_time_max)) {
        XLOGD_INFO("XVP Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XVP]);
        if(temp != this->rsp_times[RESPONSE_TIME_PROFILE_XVP]) {
            changed = true;
        }
    }
    temp = this->rsp_times[RESPONSE_TIME_PROFILE_XDIU];
    if(attr.get_rfc_value(JSON_OBJ_NAME_NETWORK_RF4CE_RSP_TIME JSON_PATH_SEPERATOR "xdiu", this->rsp_times[RESPONSE_TIME_PROFILE_XDIU], rsp_time_min, rsp_time_max)) {
        XLOGD_INFO("XDIU Response Time %ums", this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]);
        if(temp != this->rsp_times[RESPONSE_TIME_PROFILE_XDIU]) {
            changed = true;
        }
    }
    if(changed && this->updated_listener) {
        XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
        this->updated_listener(*this);
    }
}

void ctrlm_rf4ce_rsp_time_t::set_updated_listener(ctrlm_rf4ce_rsp_time_listener_t listener) {
    this->updated_listener = listener;
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_rsp_time_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(index == RESPONSE_TIME_PROFILE_XRC ||
           index == RESPONSE_TIME_PROFILE_XVP ||
           index == RESPONSE_TIME_PROFILE_XDIU) {
            data[0] =  this->rsp_times[index]       & 0xFF;
            data[1] = (this->rsp_times[index] >> 8) & 0xFF;
            *len = sizeof(uint16_t);
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: Profile <0x%02X> Time <%ums>", this->get_name().c_str(), index, this->rsp_times.at(index));
        } else {
            XLOGD_ERROR("invalid index <%02X>", index);
            ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_rsp_time_t
