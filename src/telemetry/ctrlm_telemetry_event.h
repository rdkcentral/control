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
#ifndef __CTRLM_TELEMETRY_EVENT_H__
#define __CTRLM_TELEMETRY_EVENT_H__
#include <type_traits>
#include <string>
#include "ctrlm_telemetry_markers.h"
#ifdef TELEMETRY_SUPPORT
#include <telemetry2_0.h>
#include <telemetry_busmessage_sender.h>
#endif

template <typename T>
class ctrlm_telemetry_event_t {
    static_assert(std::is_same<T, int>::value || std::is_same<T, double>::value || std::is_same<T, std::string>::value, "Invalid telemetry event type");
public:
    ctrlm_telemetry_event_t(const std::string &marker, const T &value) {
        this->marker = marker;
        this->value  = value;
    }
    ~ctrlm_telemetry_event_t() {};

public:
    virtual bool event() const;

protected:
    std::string marker;
    T value;
};

#endif
