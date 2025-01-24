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

#include "ctrlm_voice_telemetry_events.h"
#include <sstream>
#include "ctrlm_log.h"


ctrlm_voice_telemetry_vsr_error_t::ctrlm_voice_telemetry_vsr_error_t(const std::string &id) : ctrlm_telemetry_event_t<std::string>(std::string(MARKER_VOICE_VSR_FAIL_PREFIX)+id, "") {
    this->reset();
}

ctrlm_voice_telemetry_vsr_error_t::~ctrlm_voice_telemetry_vsr_error_t() {

}

bool ctrlm_voice_telemetry_vsr_error_t::event() const {
    bool ret = true;
    std::stringstream ss;
    std::string val_marker;

    ss << "telemetry event ";
    
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_TOTAL);
    ss << "<" << val_marker << "," << this->total << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->total) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_W_VOICE);
    ss << "<" << val_marker << "," << this->data << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->data) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_WO_VOICE);
    ss << "<" << val_marker << "," << this->no_data << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->no_data) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_IN_WIN);
    ss << "<" << val_marker << "," << this->within_window << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->within_window) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_OUT_WIN);
    ss << "<" << val_marker << "," << this->outside_window << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->outside_window) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_RSPTIME_ZERO);
    ss << "<" << val_marker << "," << this->zero_rsp << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->zero_rsp) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_RSP_WINDOW);
    ss << "<" << val_marker << "," << this->rsp_window << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->rsp_window) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_MIN_RSPTIME);
    ss << "<" << val_marker << "," << this->min_rsp << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->min_rsp) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_MAX_RSPTIME);
    ss << "<" << val_marker << "," << this->max_rsp << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->max_rsp) != T2ERROR_SUCCESS) {
        ret = false;
    }
    val_marker = this->marker + std::string(MARKER_VOICE_VSR_FAIL_SUB_AVG_RSPTIME);
    ss << "<" << val_marker << "," << this->avg_rsp << ">, ";
    if(t2_event_d((char *)val_marker.c_str(), this->avg_rsp) != T2ERROR_SUCCESS) {
        ret = false;
    }
    XLOGD_TELEMETRY("%s", ss.str().c_str());
    return(ret);
}

void ctrlm_voice_telemetry_vsr_error_t::update(bool data_sent, unsigned int rsp_window, signed long long rsp_time) {
    this->total++;
    if(data_sent) {
        this->data++;
    } else {
        this->no_data++;
    }
    if(rsp_time < rsp_window) {
        this->within_window++;
    } else {
        this->outside_window++;
    }
    this->rsp_window = rsp_window;
    if(rsp_time == 0) {
        this->zero_rsp++;
    }
    if(rsp_time < this->min_rsp) {
        this->min_rsp = rsp_time;
    }
    if(rsp_time > this->max_rsp) {
        this->max_rsp = rsp_time;
    }
    this->avg_rsp = ((this->avg_rsp * this->avg_rsp_count) + rsp_time) / (this->avg_rsp_count + 1);
    this->avg_rsp_count++;

    // // set string and event
    // std::stringstream ss;
    // ss << this->total << "," << this->data << "," << this->no_data << "," << this->within_window << "," << this->outside_window << "," << this->zero_rsp << ","  << this->rsp_window << "," << this->min_rsp << "," << this->avg_rsp << "," << this->max_rsp;
    // this->value = ss.str();
}

void ctrlm_voice_telemetry_vsr_error_t::reset() {
    this->total          = 0;
    this->no_data        = 0;
    this->data           = 0;
    this->within_window  = 0;
    this->outside_window = 0;
    this->zero_rsp       = 0;
    this->min_rsp        = 0;
    this->avg_rsp        = 0;
    this->avg_rsp_count  = 0;
    this->max_rsp        = 0;
    this->rsp_window     = 0;
}

ctrlm_voice_telemetry_vsr_error_map_t::ctrlm_voice_telemetry_vsr_error_map_t() {

}

ctrlm_voice_telemetry_vsr_error_map_t::~ctrlm_voice_telemetry_vsr_error_map_t() {
    
}

ctrlm_voice_telemetry_vsr_error_t *ctrlm_voice_telemetry_vsr_error_map_t::get(const std::string &id) {
    ctrlm_voice_telemetry_vsr_error_t *ret = NULL;
    if(id != "") {
        if(this->data.count(id) == 0) {
            this->data.emplace(id, ctrlm_voice_telemetry_vsr_error_t(id));
        }
        auto vsr_error = this->data.find(id);
        if(vsr_error != this->data.end()) {
            ret = &vsr_error->second;
        }
    }
    return(ret);
}

void ctrlm_voice_telemetry_vsr_error_map_t::clear() {
    this->data.clear();
}
