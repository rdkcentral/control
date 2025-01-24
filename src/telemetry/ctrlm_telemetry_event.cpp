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

#include "ctrlm_telemetry_event.h"
#include "ctrlm_log.h"

template <>
bool ctrlm_telemetry_event_t<int>::event() const {
    XLOGD_TELEMETRY("telemetry event <%s, %d>", this->marker.c_str(), this->value);
    return(t2_event_d((char *)this->marker.c_str(), this->value) == T2ERROR_SUCCESS);
}

template <>
bool ctrlm_telemetry_event_t<double>::event() const {
    XLOGD_TELEMETRY("telemetry event <%s, %f>", this->marker.c_str(), this->value);
    return(t2_event_f((char *)this->marker.c_str(), this->value) == T2ERROR_SUCCESS);
}

template <>
bool ctrlm_telemetry_event_t<std::string>::event() const {
    XLOGD_TELEMETRY("telemetry event <%s, %s>", this->marker.c_str(), this->value.c_str());
    return(t2_event_s((char *)this->marker.c_str(), (char *)this->value.c_str()) == T2ERROR_SUCCESS);
}
