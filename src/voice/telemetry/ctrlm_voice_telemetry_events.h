/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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
#ifndef __CTRLM_VOICE_TELEMETRY_EVENTS_H__
#define __CTRLM_VOICE_TELEMETRY_EVENTS_H__
#include "ctrlm_telemetry.h"
#include "xr_timestamp.h"
#include "stdint.h"
#include <vector>
#include <string>
#include <map>
#include <mutex>

class ctrlm_voice_telemetry_vsr_error_t : public ctrlm_telemetry_event_t<std::string> {
public:
    ctrlm_voice_telemetry_vsr_error_t(const std::string &id);
    ~ctrlm_voice_telemetry_vsr_error_t();

public:
    bool event() const;
    void update(bool data_sent, unsigned int rsp_window, signed long long rsp_time);
    void reset();

protected:
    unsigned int     total;
    unsigned int     no_data;
    unsigned int     data;
    unsigned int     within_window;
    unsigned int     outside_window;
    unsigned int     zero_rsp;
    signed long long min_rsp;
    signed long long avg_rsp;
    unsigned int     avg_rsp_count;
    signed long long max_rsp;
    unsigned int     rsp_window;
};

class ctrlm_voice_telemetry_vsr_error_map_t {
public:
    ctrlm_voice_telemetry_vsr_error_map_t();
    virtual ~ctrlm_voice_telemetry_vsr_error_map_t();

public:
    ctrlm_voice_telemetry_vsr_error_t *get(const std::string &id);
    void clear();

protected:
    std::map<std::string, ctrlm_voice_telemetry_vsr_error_t> data;

public:
    ctrlm_voice_telemetry_vsr_error_t* operator[](const std::string &id) {return(this->get(id));}
};

class ctrlm_voice_telemetry_session_t : public ctrlm_telemetry_event_t<std::string> {
public:
    ctrlm_voice_telemetry_session_t();
    ~ctrlm_voice_telemetry_session_t();

public:
    void reset_events();
    void update_on_session_begin(const std::string &device_type, const std::string &device_version, const std::string &encoding, bool press_and_release, bool end_of_speech_detection);
    void update_on_key_release(int32_t time_start_lag, int32_t time_stream_len_exp, rdkx_timestamp_t *time_key_down, rdkx_timestamp_t *time_key_up);
    void update_on_stream_end(uint32_t time_stream_len_act, uint32_t packets_total, uint32_t packets_lost, uint32_t samples_total, uint32_t samples_lost, uint32_t decoder_failures, uint32_t samples_buffered_max);
    bool update_on_session_end(bool result, int32_t end_reason_rcu, int32_t end_reason_session, int32_t server_return_code, const std::string &server_message, int32_t time_stream_len_exp, int32_t ret_code_protocol, int32_t stream_end_reason);

private:
   static const uint32_t  m_event_list_max_size = CTRLM_TELEMETRY_MAX_EVENT_SIZE_BYTES;

    void reset_stats();
    bool event();

    std::string m_device_type;          // controller type
    std::string m_device_version;       // controller version
    std::string m_encoding;             // encoding (PCM, ADPCM, OPUS)
    uint32_t    m_interaction_mode;     // interaction mode (0 = press and hold, 1 = press and release, 2 - end of speech detection)

    rdkx_timestamp_t m_time_prev_session_end; // timestamp of previous session end

    int64_t     m_time_prev_session;    // amount of elapsed time in ms since the previous voice session (amount of time elapsed from voice session end to session begin)
    int32_t     m_time_start_lag;       // start lag time in ms (amount of time elapsed from voice key down reception to first audio packet received)
    int32_t     m_time_stream_len_exp;  // expected stream length in ms (amount of time elapsed from voice key down to key up)
    uint32_t    m_time_stream_len_act;  // actual stream length in ms
    int32_t     m_time_stream_delta;    // delta stream length in ms (expected - actual)
    uint32_t    m_packets_total;        // total packets (received + lost)
    uint32_t    m_packets_lost;         // lost packets
    uint32_t    m_samples_total;        // total samples (received + lost)
    uint32_t    m_samples_lost;         // lost samples
    uint32_t    m_decoder_failures;     // decoder failure count
    uint32_t    m_samples_buffered_max; // sample buffer high watermark

    int32_t     m_end_reason_rcu;       // reason for ending the stream as reported by RCU
    int32_t     m_end_reason_session;   // reason for ending the session
    int32_t     m_end_reason_server;    // server response success/error code
    std::string m_server_message;       // server message
    bool        m_result;               // flag to indicate if session was successful
    int32_t     m_ret_code_protocol;    // protocol return code
    int32_t     m_end_reason_stream;    // reason for ending the stream as reported by speech router

    bool        m_has_key_release;      // flag to indicate if key release event has been received

    std::mutex  m_mutex_event_list;     // mutex to protect event list
    std::string m_event_list;           // list of events that occurred during the reporting window
};

#endif
