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
#include <vector>
#include <string>
#include <map>

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

#endif
