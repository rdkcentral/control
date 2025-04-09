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
#include "ctrlm_utils.h"


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

ctrlm_voice_telemetry_session_t::ctrlm_voice_telemetry_session_t() : ctrlm_telemetry_event_t<std::string>(std::string(MARKER_VOICE_SESSION_STATS), "") {
    m_time_prev_session_end = {0, 0};
    m_time_prev_session     = -1;
    reset_stats();
}

ctrlm_voice_telemetry_session_t::~ctrlm_voice_telemetry_session_t() {

}

void ctrlm_voice_telemetry_session_t::reset_events() {
    std::unique_lock<std::mutex> guard(m_mutex_event_list);
    m_event_list.clear();
}

bool ctrlm_voice_telemetry_session_t::event() {
    std::unique_lock<std::mutex> guard(m_mutex_event_list);
    bool ret = true;
    std::stringstream ss;
    std::string val_marker = this->marker;
    
    if(m_event_list.empty()) {
        ss << "[";  // Add opening bracket
    } else {
        char &last_char = m_event_list.back();
        last_char = ','; // Replace the closing bracket with a comma
    }
    ss << "[" MARKER_VOICE_SESSION_STATS_VERSION ",";
    ss << "\"" << m_device_type << "\",";
    ss << "\"" << m_device_version << "\",";
    ss << "\"" << m_encoding << "\",";
    ss << m_interaction_mode << ",";
    ss << m_time_prev_session << ",";
    ss << m_time_start_lag << ",";
    ss << m_time_stream_len_exp << ",";
    ss << m_time_stream_len_act << ",";
    ss << m_time_stream_delta << ",";
    ss << m_packets_total << ",";
    ss << m_packets_lost << ",";
    ss << m_samples_total << ",";
    ss << m_samples_lost << ",";
    ss << m_decoder_failures << ",";
    ss << m_samples_buffered_max << ",";
    ss << m_end_reason_stream << ",";
    ss << m_end_reason_protocol << ",";
    ss << m_end_reason_server << ",";
    ss << "\"" << m_server_message << "\",";
    ss << m_result << "]]";

    if(m_event_list.length() + ss.str().length() > m_event_list_max_size) { // Maximum data size exceeded
        XLOGD_WARN("telemetry event exceeds max size <%s,%s>", val_marker.c_str(), ss.str().c_str());

        // Add back the closing bracket
        char &last_char = m_event_list.back();
        last_char = ']'; // Replace the comma with a closing bracket
        ret = false;
    } else {
        // Append the event to the list
        m_event_list.append(ss.str());

        T2ERROR t2_error = t2_event_s((char *)val_marker.c_str(), m_event_list.c_str());
        if(t2_error != T2ERROR_SUCCESS) {
            XLOGD_WARN("telemetry t2_event_s error <%s>", ctrlm_t2_error_str(t2_error));
            ret = false;
        } else {
            XLOGD_TELEMETRY("telemetry event <%s,%s> size <%u>", val_marker.c_str(), ss.str().c_str(), m_event_list.length());
        }
    }
    return(ret);
}

void ctrlm_voice_telemetry_session_t::update_on_session_begin(const std::string &device_type, const std::string &device_version, const std::string &encoding, bool press_and_release, bool end_of_speech_detection) {
    reset_stats();
    m_device_type         = device_type;
    m_device_version      = device_version;
    m_encoding            = encoding;
    m_interaction_mode    = (end_of_speech_detection) ? 2 : (press_and_release == false) ? 0 : 1;
}

void ctrlm_voice_telemetry_session_t::update_on_key_release(int32_t time_start_lag, int32_t time_stream_len_exp, rdkx_timestamp_t *time_key_down, rdkx_timestamp_t *time_key_up) {
    m_time_start_lag        = time_start_lag;
    m_time_stream_len_exp   = time_stream_len_exp;

    if(time_key_down != NULL) {
        if(m_time_prev_session_end.tv_sec == 0 && m_time_prev_session_end.tv_nsec == 0) { // no sessions have reported end time yet
           m_time_prev_session = -1; 
        } else {
            m_time_prev_session = rdkx_timestamp_subtract_ms(m_time_prev_session_end, *time_key_down);

            if(m_time_prev_session > 15 * 60 * 1000) { // the time between the previous session and the current session is greater than 15 minutes
                m_time_prev_session = -3;
            } else if(m_time_prev_session < 0) { // the time between the previous session and the current session is negative (something went wrong)
                m_time_prev_session = -4;
            }
        }
    }
    if(time_key_up != NULL) {
        m_time_prev_session_end = *time_key_up;
    }
    m_has_key_release       = true;
}

void ctrlm_voice_telemetry_session_t::update_on_stream_end(uint32_t time_stream_len_act, uint32_t packets_total, uint32_t packets_lost, uint32_t samples_total, uint32_t samples_lost, uint32_t decoder_failures, uint32_t samples_buffered_max) {
    m_time_stream_len_act  = time_stream_len_act;
    m_packets_total        = packets_total;
    m_packets_lost         = packets_lost;
    m_samples_total        = samples_total;
    m_samples_lost         = samples_lost;
    m_decoder_failures     = decoder_failures;
    m_samples_buffered_max = samples_buffered_max;

    if(m_has_key_release) {
        m_time_stream_delta = m_time_stream_len_act - m_time_stream_len_exp;
    }
}

bool ctrlm_voice_telemetry_session_t::update_on_session_end(bool result, int32_t end_reason_stream, int32_t end_reason_protocol, int32_t end_reason_server, const std::string &server_message, int32_t time_stream_len_exp) {
    m_result               = result;
    m_end_reason_stream    = end_reason_stream;
    m_end_reason_protocol  = end_reason_protocol;
    m_end_reason_server    = end_reason_server;
    m_server_message       = server_message;
    
    if(!m_has_key_release) { // if there is no key release, the start time and end time are not known
        rdkx_timestamp_get(&m_time_prev_session_end);
        m_time_prev_session = -2; // the time between previous session is not known because there is no start time reported.
    }
    if(time_stream_len_exp >= 0) { // if the expected stream length was reported, overwrite the value calculated from key down and key up timestamps
        m_time_stream_len_exp = time_stream_len_exp;
        m_time_stream_delta   = m_time_stream_len_act - m_time_stream_len_exp;
    }

    return(event());
}

void ctrlm_voice_telemetry_session_t::reset_stats() {
    m_time_start_lag       = -1;
    m_time_stream_len_exp  = -1;
    m_time_stream_len_act  = 0;
    m_time_stream_delta    = 0;
    m_packets_total        = 0;
    m_packets_lost         = 0;
    m_samples_total        = 0;
    m_samples_lost         = 0;
    m_decoder_failures     = 0;
    m_samples_buffered_max = 0;

    m_end_reason_stream    = 0;
    m_end_reason_protocol  = 0;
    m_end_reason_server    = 0;
    m_result               = false;

    m_server_message.clear();
    m_device_type.clear();
    m_device_version.clear();
    m_encoding.clear();
    m_interaction_mode     = 0;

    m_has_key_release      = false;
}
