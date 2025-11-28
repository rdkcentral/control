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

static IARM_Result_t update_settings(void *arg);
static bool broadcast_event(const char *bus_name, int event, void *data, size_t data_size);

ctrlm_voice_ipc_iarm_legacy_t::ctrlm_voice_ipc_iarm_legacy_t(ctrlm_voice_t *obj_voice) : ctrlm_voice_ipc_t(obj_voice) {
    this->state = EVENT_ALL;
}

bool ctrlm_voice_ipc_iarm_legacy_t::register_ipc() const {
    bool ret = true;
    IARM_Result_t rc;
    // NOTE: The IARM events are registered in ctrlm_main.cpp

    XLOGD_INFO("ServiceManager");

    XLOGD_INFO("Registering for voice update settings");
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_UPDATE_SETTINGS, update_settings);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }
    return(ret);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_begin(const ctrlm_voice_ipc_event_session_begin_t &session_begin) {
    bool ret = true;
    if(session_begin.keyword_verification == true && session_begin.keyword_verification_required) {
        this->state = AWAIT_KW_VERIFICATION;
        this->session_begin_cached = session_begin;
    } else {
        this->state = EVENT_ALL;
        ctrlm_voice_iarm_event_session_begin_t event = {0};
        XLOGD_INFO("(%u, %s, %u) Session id %lu Language <%s> mime type <%s> is voice assistant <%s>", session_begin.common.network_id, ctrlm_network_type_str(session_begin.common.network_type).c_str(), session_begin.common.controller_id, session_begin.common.session_id_ctrlm, session_begin.language.c_str(), session_begin.mime_type.c_str(), session_begin.common.voice_assistant  ? "true" : "false");
        event.api_revision       = CTRLM_VOICE_IARM_BUS_API_REVISION;
        event.network_id         = session_begin.common.network_id;
        event.network_type       = session_begin.common.network_type;
        event.controller_id      = session_begin.common.controller_id;
        event.session_id         = session_begin.common.session_id_ctrlm;
        errno_t safec_rc = strcpy_s((char *)event.mime_type, sizeof(event.mime_type),session_begin.mime_type.c_str());
        ERR_CHK(safec_rc);

        safec_rc = strcpy_s((char *)event.sub_type, sizeof(event.sub_type), session_begin.sub_type.c_str());
        ERR_CHK(safec_rc);

        safec_rc = strcpy_s((char *)event.language, sizeof(event.language), session_begin.language.c_str());
        ERR_CHK(safec_rc);
        event.is_voice_assistant = session_begin.common.voice_assistant ? 1 : 0;
        ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_BEGIN, &event, sizeof(event));
    }
    return(ret);
}

bool ctrlm_voice_ipc_iarm_legacy_t::stream_begin(const ctrlm_voice_ipc_event_stream_begin_t &stream_begin) {
    XLOGD_INFO("Not supported");
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::stream_end(const ctrlm_voice_ipc_event_stream_end_t &stream_end) {
    bool ret = true;
    if(this->state != IGNORE_ALL) {
        XLOGD_INFO("(%u, %s, %u) Session id %lu", stream_end.common.network_id, ctrlm_network_type_str(stream_end.common.network_type).c_str(), stream_end.common.controller_id, stream_end.common.session_id_ctrlm);
        ctrlm_voice_iarm_event_session_end_t event ={0};
        event.api_revision       = CTRLM_VOICE_IARM_BUS_API_REVISION;
        event.network_id         = stream_end.common.network_id;
        event.network_type       = stream_end.common.network_type;
        event.controller_id      = stream_end.common.controller_id;
        event.session_id         = stream_end.common.session_id_ctrlm;
        event.reason             = (ctrlm_voice_session_end_reason_t)stream_end.reason;
        event.is_voice_assistant = stream_end.common.voice_assistant ? 1 : 0;
        ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_END, &event, sizeof(event));
    }
    return(ret);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_end(const ctrlm_voice_ipc_event_session_end_t &session_end) {
    bool ret = true;
    errno_t safec_rc = -1;
    if(this->state != IGNORE_ALL) {
        XLOGD_INFO("(%u, %s, %u) Session id %lu", session_end.common.network_id, ctrlm_network_type_str(session_end.common.network_type).c_str(), session_end.common.controller_id, session_end.common.session_id_ctrlm);
        switch(session_end.result) {
            case SESSION_END_SUCCESS:
            case SESSION_END_FAILURE: {
                ctrlm_voice_iarm_event_session_result_t event = {0};
                event.api_revision         = CTRLM_VOICE_IARM_BUS_API_REVISION;
                event.network_id           = session_end.common.network_id;
                event.network_type         = session_end.common.network_type;
                event.controller_id        = session_end.common.controller_id;
                event.session_id           = session_end.common.session_id_ctrlm;
                event.result               = session_end.result == SESSION_END_SUCCESS ? CTRLM_VOICE_SESSION_RESULT_SUCCESS : CTRLM_VOICE_SESSION_RESULT_FAILURE;
                event.return_code_http     = session_end.return_code_protocol;
                event.return_code_curl     = session_end.return_code_protocol_library;
                event.return_code_vrex     = session_end.return_code_server;
                event.return_code_internal = session_end.return_code_internal;
                safec_rc = strcpy_s(event.vrex_session_id, sizeof(event.vrex_session_id), session_end.common.session_id_server.c_str());
                ERR_CHK(safec_rc);

                safec_rc = strcpy_s(event.vrex_session_text, sizeof(event.vrex_session_text), session_end.transcription.c_str());
                ERR_CHK(safec_rc);

                safec_rc = strcpy_s(event.vrex_session_message, sizeof(event.vrex_session_message), session_end.return_code_server_str.c_str());
                ERR_CHK(safec_rc);

                safec_rc = strcpy_s(event.session_uuid, sizeof(event.session_uuid), session_end.common.session_id_server.c_str());
                ERR_CHK(safec_rc);


                if(session_end.server_stats) {
                    safec_rc = strcpy_s(event.curl_request_ip, sizeof(event.curl_request_ip), session_end.server_stats->server_ip.c_str());
                    ERR_CHK(safec_rc);
                    event.curl_request_dns_time     = session_end.server_stats->dns_time;
                    event.curl_request_connect_time = session_end.server_stats->connect_time;
                }
                ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_RESULT, &event, sizeof(event));
                break;
            }
            case SESSION_END_ABORT: {
                ctrlm_voice_iarm_event_session_abort_t event = {0};
                event.api_revision         = CTRLM_VOICE_IARM_BUS_API_REVISION;
                event.network_id           = session_end.common.network_id;
                event.network_type         = session_end.common.network_type;
                event.controller_id        = session_end.common.controller_id;
                event.session_id           = session_end.common.session_id_ctrlm;
                event.reason               = (ctrlm_voice_session_abort_reason_t)session_end.reason;
                ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_ABORT, &event, sizeof(event));
                break;
            }
            case SESSION_END_SHORT_UTTERANCE: {
                ctrlm_voice_iarm_event_session_short_t event = {0};
                event.api_revision         = CTRLM_VOICE_IARM_BUS_API_REVISION;
                event.network_id           = session_end.common.network_id;
                event.network_type         = session_end.common.network_type;
                event.controller_id        = session_end.common.controller_id;
                event.session_id           = session_end.common.session_id_ctrlm;
                event.reason               = (ctrlm_voice_session_end_reason_t)session_end.reason;
                event.return_code_internal = session_end.return_code_internal;
                ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_SHORT, &event, sizeof(event));
                break;
            }
        }
    }
    // Reset state
    this->state = EVENT_ALL;
    return(ret);               
}

bool ctrlm_voice_ipc_iarm_legacy_t::server_message(const char *message, unsigned long size) {
    XLOGD_INFO("Not supported");
    return(true);
}

bool ctrlm_voice_ipc_iarm_legacy_t::keyword_verification(const ctrlm_voice_ipc_event_keyword_verification_t &keyword_verification) {
    bool ret = true;
    if(this->state == AWAIT_KW_VERIFICATION) {
        if(keyword_verification.verified) {
            this->state = EVENT_ALL;
            this->session_begin_cached.keyword_verification          = false;
            this->session_begin_cached.keyword_verification_required = false;
            ret = this->session_begin(this->session_begin_cached);
        } else {
            this->state = IGNORE_ALL;
        }
    }
    return(ret);
}

bool ctrlm_voice_ipc_iarm_legacy_t::session_statistics(const ctrlm_voice_ipc_event_session_statistics_t &session_stats) {
    ctrlm_voice_iarm_event_session_stats_t event = {0};
    event.api_revision         = CTRLM_VOICE_IARM_BUS_API_REVISION;
    event.network_id           = session_stats.common.network_id;
    event.network_type         = session_stats.common.network_type;
    event.controller_id        = session_stats.common.controller_id;
    event.session_id           = session_stats.common.session_id_ctrlm;
    event.session              = session_stats.session;
    event.reboot               = session_stats.reboot;
    return(broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_STATS, &event, sizeof(event)));
}

void ctrlm_voice_ipc_iarm_legacy_t::deregister_ipc() const {
    XLOGD_INFO("ServiceManager");
}

IARM_Result_t update_settings(void *arg) {
    ctrlm_voice_iarm_call_settings_t *voice_settings = (ctrlm_voice_iarm_call_settings_t *)arg;
    ctrlm_voice_t *voice_obj = NULL;

    if(voice_settings == NULL) {
        XLOGD_ERROR("voice settings NULL");
        return(IARM_RESULT_INVALID_PARAM);
    }

    XLOGD_INFO("Rxd CTRLM_VOICE_IARM_EVENT_VOICE_SETTINGS - API Revision %u", voice_settings->api_revision);

    if(voice_settings->api_revision != CTRLM_VOICE_IARM_BUS_API_REVISION) {
        XLOGD_INFO("Unsupported API Revision (%u, %u)", voice_settings->api_revision, CTRLM_VOICE_IARM_BUS_API_REVISION);
        voice_settings->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
        return(IARM_RESULT_INVALID_PARAM);
    }

    voice_obj = ctrlm_get_voice_obj();
    if(voice_obj) {
        if(!voice_obj->voice_configure(voice_settings, true)) {
            return(IARM_RESULT_INVALID_PARAM);
        }
    } else {
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

bool broadcast_event(const char *bus_name, int event, void *data, size_t data_size) {
    bool ret = true;
    IARM_Result_t result = IARM_Bus_BroadcastEvent(bus_name, event, data, data_size);
    if(IARM_RESULT_SUCCESS != result) {
        XLOGD_ERROR("IARM Bus Error!");
        ret = false;
    }
    return(ret);
}
