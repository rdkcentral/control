/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "ctrlm_voice_ipc_iarm_legacy.h"
#include "include/ctrlm_ipc_voice.h"
#include "ctrlm_utils.h"
#include "ctrlm_log.h"
#include "ctrlm_voice_obj.h"

ctrlm_voice_ipc_iarm_legacy_t::ctrlm_voice_ipc_iarm_legacy_t(ctrlm_voice_t *obj_voice) : ctrlm_voice_ipc_t(obj_voice) {
    this->state = EVENT_ALL;
}

bool ctrlm_voice_ipc_iarm_legacy_t::register_ipc() const {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_begin(const ctrlm_voice_ipc_event_session_begin_t &session_begin) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::stream_begin(const ctrlm_voice_ipc_event_stream_begin_t &stream_begin) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::stream_end(const ctrlm_voice_ipc_event_stream_end_t &stream_end) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_end(const ctrlm_voice_ipc_event_session_end_t &session_end) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::server_message(const char *message, unsigned long size) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::keyword_verification(const ctrlm_voice_ipc_event_keyword_verification_t &keyword_verification) {
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_statistics(const ctrlm_voice_ipc_event_session_statistics_t &session_stats) {
    return(true);
}

void ctrlm_voice_ipc_iarm_legacy_t::deregister_ipc() const {
}
