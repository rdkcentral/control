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
#include "ctrlm_voice_ipc_iarm_thunder.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_voice.h"
#include <functional>
#include "jansson.h"
#include "ctrlm_log.h"
#include "ctrlm_voice_obj.h"
#include "ctrlm_voice_ipc_request_type.h"
#include <fcntl.h>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "xraudio.h"

#define JSON_ENCODE_FLAGS                             (JSON_COMPACT)
#define JSON_DECODE_FLAGS                             (JSON_DECODE_ANY)

#define JSON_DEREFERENCE(x)                         if(x) {             \
                                                        json_decref(x); \
                                                        x = NULL;       \
                                                    }

#define JSON_REMOTE_ID                                "remoteId"
#define JSON_SESSION_ID                               "sessionId"
#define JSON_DEVICE_TYPE                              "deviceType"
#define JSON_KEYWORD_VERIFICATION                     "keywordVerification"
#define JSON_KEYWORD_VERIFIED                         "verified"
#define JSON_THUNDER_RESULT                           "success" // Thunder uses success key value to determine whether the command was succcessful
#define JSON_STATUS                                   "status"
#define JSON_URL_PTT                                  "urlPtt"
#define JSON_URL_HF                                   "urlHf"
#define JSON_URL_MIC_TAP                              "urlMicTap"
#define JSON_WW_FEEDBACK                              "wwFeedback"
#define JSON_PRV                                      "prv"
#define JSON_CAPABILITIES                             "capabilities"
#define JSON_TYPES                                    "types"
#define JSON_MESSAGE                                  "message"
#define JSON_MASK_PII                                 "maskPii"
#define JSON_STREAM_END_REASON                        "reason"
#define JSON_SESSION_END_RESULT                       "result"
#define JSON_SESSION_END_RESULT_SUCCESS               "success"
#define JSON_SESSION_END_RESULT_ERROR                 "error"
#define JSON_SESSION_END_RESULT_ABORT                 "abort"
#define JSON_SESSION_END_RESULT_SHORT                 "shortUtterance"
#define JSON_SESSION_END_TRANSCRIPTION                "transcription"
#define JSON_SESSION_END_PROTOCOL_ERROR               "protocolErrorCode"
#define JSON_SESSION_END_PROTOCOL_LIBRARY_ERROR       "protocolLibraryErrorCode"
#define JSON_SESSION_END_SERVER_ERROR                 "serverErrorCode"
#define JSON_SESSION_END_SERVER_STR                   "serverErrorString"
#define JSON_SESSION_END_INTERNAL_ERROR               "internalErrorCode"
#define JSON_SESSION_END_ERROR_REASON                 "reason"
#define JSON_SESSION_END_ABORT_REASON                 "reason"
#define JSON_SESSION_END_SHORT_REASON                 "reason"
#define JSON_SESSION_END_STB_STATS                    "stbStats"
#define JSON_SESSION_END_STB_STATS_TYPE               "type"
#define JSON_SESSION_END_STB_STATS_FIRMWARE           "firmware"
#define JSON_SESSION_END_STB_STATS_DEVICE_ID          "deviceId"
#define JSON_SESSION_END_STB_STATS_CTRLM_VERSION      "ctrlmVersion"
#define JSON_SESSION_END_STB_STATS_CONTROLLER_VERSION "controllerVersion"
#define JSON_SESSION_END_STB_STATS_CONTROLLER_TYPE    "controllerType"
#define JSON_SESSION_END_SERVER_STATS                 "serverStats"
#define JSON_SESSION_END_SERVER_STATS_IP              "serverIp"
#define JSON_SESSION_END_SERVER_STATS_DNS_TIME        "dnsTime"
#define JSON_SESSION_END_SERVER_STATS_CONNECT_TIME    "connectTime"

static bool broadcast_event(const char *bus_name, int event, const char *str);
static const char *voice_device_str(ctrlm_voice_device_t device);
static const char *voice_device_status_str(uint8_t status);

ctrlm_voice_ipc_iarm_thunder_t::ctrlm_voice_ipc_iarm_thunder_t(ctrlm_voice_t *obj_voice): ctrlm_voice_ipc_t(obj_voice) {

}

bool ctrlm_voice_ipc_iarm_thunder_t::register_ipc() const {
    bool ret = true;
    IARM_Result_t rc;
    XLOGD_INFO("Thunder");
    // NOTE: The IARM events are registered in ctrlm_main.cpp

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_STATUS);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_STATUS, &ctrlm_voice_ipc_iarm_thunder_t::status);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE, &ctrlm_voice_ipc_iarm_thunder_t::configure_voice);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SET_VOICE_INIT);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SET_VOICE_INIT, &ctrlm_voice_ipc_iarm_thunder_t::set_voice_init);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SEND_VOICE_MESSAGE);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SEND_VOICE_MESSAGE, &ctrlm_voice_ipc_iarm_thunder_t::send_voice_message);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SESSION_TYPES);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SESSION_TYPES, &ctrlm_voice_ipc_iarm_thunder_t::voice_session_types);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SESSION_REQUEST);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SESSION_REQUEST, &ctrlm_voice_ipc_iarm_thunder_t::voice_session_request);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SESSION_TERMINATE);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SESSION_TERMINATE, &ctrlm_voice_ipc_iarm_thunder_t::voice_session_terminate);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    XLOGD_INFO("Registering for %s IARM call", CTRLM_VOICE_IARM_CALL_SESSION_AUDIO_STREAM_START);
    rc = IARM_Bus_RegisterCall(CTRLM_VOICE_IARM_CALL_SESSION_AUDIO_STREAM_START, &ctrlm_voice_ipc_iarm_thunder_t::voice_session_audio_stream_start);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register %d", rc);
        ret = false;
    }

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::session_begin(const ctrlm_voice_ipc_event_session_begin_t &session_begin) {
    bool    ret   = false;
    json_t *event_data = json_object();
    int rc;

    // Assemble event data
    rc  = json_object_set_new_nocheck(event_data, JSON_REMOTE_ID, json_integer(session_begin.common.controller_id));
    rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_ID, json_string(session_begin.common.session_id_server.c_str()));
    rc |= json_object_set_new_nocheck(event_data, JSON_DEVICE_TYPE, json_string(voice_device_str(session_begin.common.device_type)));
    rc |= json_object_set_new_nocheck(event_data, JSON_KEYWORD_VERIFICATION, (session_begin.keyword_verification ? json_true() : json_false()));

    if(0 != rc) {
        XLOGD_TELEMETRY("Error creating JSON payload");
        JSON_DEREFERENCE(event_data);
    } else {
        char *json_str = json_dumps(event_data, JSON_ENCODE_FLAGS);
        if(json_str) {
            //TODO: surface the event through IARM
            XLOGD_INFO("%s", json_str);
            ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SESSION_BEGIN, json_str);
            free(json_str);
        } else {
            XLOGD_ERROR("Failed to encode JSON string");
        }
    }

    JSON_DEREFERENCE(event_data);

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::stream_begin(const ctrlm_voice_ipc_event_stream_begin_t &stream_begin) {
    bool    ret   = false;
    json_t *event_data = json_object();
    int rc;

    // Assemble event data
    rc  = json_object_set_new_nocheck(event_data, JSON_REMOTE_ID, json_integer(stream_begin.common.controller_id));
    rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_ID, json_string(stream_begin.common.session_id_server.c_str()));

    if(0 != rc) {
        XLOGD_ERROR("Error creating JSON payload");
        JSON_DEREFERENCE(event_data);
    } else {
        char *json_str = json_dumps(event_data, JSON_ENCODE_FLAGS);
        if(json_str) {
            //TODO: surface the event through IARM
            XLOGD_INFO("%s", json_str);
            ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_STREAM_BEGIN, json_str);
            free(json_str);
        } else {
            XLOGD_ERROR("Failed to encode JSON string");
        }
    }

    JSON_DEREFERENCE(event_data);

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::stream_end(const ctrlm_voice_ipc_event_stream_end_t &stream_end) {
    bool    ret   = false;
    json_t *event_data = json_object();
    int rc;

    // Assemble event data
    rc  = json_object_set_new_nocheck(event_data, JSON_REMOTE_ID, json_integer(stream_end.common.controller_id));
    rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_ID, json_string(stream_end.common.session_id_server.c_str()));
    rc |= json_object_set_new_nocheck(event_data, JSON_STREAM_END_REASON, json_integer(stream_end.reason));

    if(0 != rc) {
        XLOGD_ERROR("Error creating JSON payload");
        JSON_DEREFERENCE(event_data);
    } else {
        char *json_str = json_dumps(event_data, JSON_ENCODE_FLAGS);
        if(json_str) {
            //TODO: surface the event through IARM
            XLOGD_INFO("%s", json_str);
            ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_STREAM_END, json_str);
            free(json_str);
        } else {
            XLOGD_ERROR("Failed to encode JSON string");
        }
    }

    JSON_DEREFERENCE(event_data);

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::session_end(const ctrlm_voice_ipc_event_session_end_t &session_end) {
    bool    ret   = false;
    json_t *event_data = json_object();
    json_t *event_result = json_object();
    int rc;

    // Assemble event data
    rc  = json_object_set_new_nocheck(event_data, JSON_REMOTE_ID, json_integer(session_end.common.controller_id));
    rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_ID, json_string(session_end.common.session_id_server.c_str()));
    switch(session_end.result) {
        case SESSION_END_SUCCESS: {
            int rc_success;
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT, json_string(JSON_SESSION_END_RESULT_SUCCESS));

            // Add Success Data to result object
            rc_success  = json_object_set_new_nocheck(event_result, JSON_SESSION_END_TRANSCRIPTION, json_string(session_end.transcription.c_str()));
            if(0 != rc_success) {
                XLOGD_ERROR("Error creating success JSON subobject");
                JSON_DEREFERENCE(event_result);
            } else {
                rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT_SUCCESS, event_result);
            }
            break;
        }
        case SESSION_END_FAILURE: {
            int rc_failure;
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT, json_string(JSON_SESSION_END_RESULT_ERROR));

            // Add Failure Data to result object
            rc_failure  = json_object_set_new_nocheck(event_result, JSON_SESSION_END_ERROR_REASON, json_integer(session_end.reason));
            rc_failure |= json_object_set_new_nocheck(event_result, JSON_SESSION_END_PROTOCOL_ERROR, json_integer(session_end.return_code_protocol));
            rc_failure |= json_object_set_new_nocheck(event_result, JSON_SESSION_END_PROTOCOL_LIBRARY_ERROR, json_integer(session_end.return_code_protocol_library));
            rc_failure |= json_object_set_new_nocheck(event_result, JSON_SESSION_END_SERVER_ERROR, json_integer(session_end.return_code_server));
            rc_failure |= json_object_set_new_nocheck(event_result, JSON_SESSION_END_SERVER_STR, json_string(session_end.return_code_server_str.c_str()));
            rc_failure |= json_object_set_new_nocheck(event_result, JSON_SESSION_END_INTERNAL_ERROR, json_integer(session_end.return_code_internal));
            if(0 != rc_failure) {
                XLOGD_ERROR("Error creating failure JSON subobject");
                JSON_DEREFERENCE(event_result);
            } else {
                rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT_ERROR, event_result);
            }
            break;
        }
        case SESSION_END_ABORT: {
            int rc_abort;
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT, json_string(JSON_SESSION_END_RESULT_ABORT));

            // Add Abort Data to result object
            rc_abort  = json_object_set_new_nocheck(event_result, JSON_SESSION_END_ABORT_REASON, json_integer(session_end.reason));
            if(0 != rc_abort) {
                XLOGD_ERROR("Error creating abort JSON subobject");
                JSON_DEREFERENCE(event_result);
            } else {
                rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT_ABORT, event_result);
            }
            break;
        }
        case SESSION_END_SHORT_UTTERANCE: {
            int rc_short;
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT, json_string(JSON_SESSION_END_RESULT_SHORT));

            // Add Short Utterance Data to result object
            rc_short  = json_object_set_new_nocheck(event_result, JSON_SESSION_END_SHORT_REASON, json_integer(session_end.reason));
            if(0 != rc_short) {
                XLOGD_ERROR("Error creating short utterance JSON subobject");
                JSON_DEREFERENCE(event_result);
            } else {
                rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_RESULT_SHORT, event_result);
            }
            break;
        }
    }
    if(session_end.stb_stats) {
        int stats_rc;
        json_t *event_stb_stats = json_object();

        stats_rc  = json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_TYPE, json_string(session_end.stb_stats->type.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_FIRMWARE, json_string(session_end.stb_stats->firmware.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_DEVICE_ID, json_string(session_end.stb_stats->device_id.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_CTRLM_VERSION, json_string(session_end.stb_stats->ctrlm_version.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_CONTROLLER_VERSION, json_string(session_end.stb_stats->controller_version.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_stb_stats, JSON_SESSION_END_STB_STATS_CONTROLLER_TYPE, json_string(session_end.stb_stats->controller_type.c_str()));
        if(0 != stats_rc) {
            XLOGD_WARN("STB Stats corrupted.. Removing..");
            JSON_DEREFERENCE(event_stb_stats);
        } else {
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_STB_STATS, event_stb_stats);
        }
    }
    if(session_end.server_stats) {
        int stats_rc;
        json_t *event_server_stats = json_object();

        stats_rc  = json_object_set_new_nocheck(event_server_stats, JSON_SESSION_END_SERVER_STATS_IP, json_string(session_end.server_stats->server_ip.c_str()));
        stats_rc |= json_object_set_new_nocheck(event_server_stats, JSON_SESSION_END_SERVER_STATS_DNS_TIME, json_real(session_end.server_stats->dns_time));
        stats_rc |= json_object_set_new_nocheck(event_server_stats, JSON_SESSION_END_SERVER_STATS_CONNECT_TIME, json_real(session_end.server_stats->connect_time));
        if(0 != stats_rc) {
            XLOGD_WARN("Server Stats corrupted.. Removing..");
            JSON_DEREFERENCE(event_server_stats);
        } else {
            rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_END_SERVER_STATS, event_server_stats);
        }
    }

    if(0 != rc) {
        XLOGD_ERROR("Error creating JSON payload");
        JSON_DEREFERENCE(event_data);
    } else {
        char *json_str = json_dumps(event_data, JSON_ENCODE_FLAGS);
        if(json_str) {
            //TODO: surface the event through IARM
            XLOGD_INFO("<%s>", this->obj_voice->voice_stb_data_pii_mask_get() ? "***" : json_str);
            ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SESSION_END, json_str);
            free(json_str);
        } else {
            XLOGD_ERROR("Failed to encode JSON string");
        }
    }

    JSON_DEREFERENCE(event_data);

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::server_message(const char *message, unsigned long size) {
    bool    ret   = false;
    if(message) {
        XLOGD_INFO("%ul : <%s>", size, this->obj_voice->voice_stb_data_pii_mask_get() ? "***" : message);  //CID -160950 - Printargs
        ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_SERVER_MESSAGE, message);
    }
    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::keyword_verification(const ctrlm_voice_ipc_event_keyword_verification_t &keyword_verification) {
    bool    ret   = false;
    json_t *event_data = json_object();
    int rc;

    // Assemble event data
    rc  = json_object_set_new_nocheck(event_data, JSON_REMOTE_ID, json_integer(keyword_verification.common.controller_id));
    rc |= json_object_set_new_nocheck(event_data, JSON_SESSION_ID, json_string(keyword_verification.common.session_id_server.c_str()));
    rc |= json_object_set_new_nocheck(event_data, JSON_KEYWORD_VERIFIED, (keyword_verification.verified ? json_true() : json_false()));

    if(0 != rc) {
        XLOGD_ERROR("Error creating JSON payload");
        JSON_DEREFERENCE(event_data);
    } else {
        char *json_str = json_dumps(event_data, JSON_ENCODE_FLAGS);
        if(json_str) {
            //TODO: surface the event through IARM
            XLOGD_INFO("%s", json_str);
            ret = broadcast_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_JSON_KEYWORD_VERIFICATION, json_str);
            free(json_str);
        } else {
            XLOGD_ERROR("Failed to encode JSON string");
        }
    }

    JSON_DEREFERENCE(event_data);

    return(ret);
}

bool ctrlm_voice_ipc_iarm_thunder_t::session_statistics(const ctrlm_voice_ipc_event_session_statistics_t &session_stats) {
    // Not supported
    return(true);
}

void ctrlm_voice_ipc_iarm_thunder_t::deregister_ipc() const {
    XLOGD_INFO("Thunder");
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::status(void *data) {
    IARM_Result_t ret        = IARM_RESULT_SUCCESS;
    bool result              = false;
    json_t *obj              = NULL;
    json_t *obj_capabilities = NULL;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_status_t status;
        result = ctrlm_get_voice_obj()->voice_status(&status);
        if(result) {
            obj = json_object();
            obj_capabilities = json_array();
            int rc = 0;

            for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
                json_t *temp = json_object();
                rc |= json_object_set_new_nocheck(temp, JSON_STATUS, json_string(voice_device_status_str(status.status[i])));
                rc |= json_object_set_new_nocheck(obj, voice_device_str((ctrlm_voice_device_t)i), temp);
            }

            rc |= json_object_set_new_nocheck(obj, JSON_URL_PTT, json_string(status.urlPtt.c_str()));
            rc |= json_object_set_new_nocheck(obj, JSON_URL_HF, json_string(status.urlHf.c_str()));
            #ifdef CTRLM_LOCAL_MIC_TAP
            rc |= json_object_set_new_nocheck(obj, JSON_URL_MIC_TAP, json_string(status.urlMicTap.c_str()));
            #endif
            rc |= json_object_set_new_nocheck(obj, JSON_WW_FEEDBACK, status.wwFeedback ? json_true() : json_false());
            rc |= json_object_set_new_nocheck(obj, JSON_PRV, status.prv_enabled ? json_true() : json_false());
            rc |= json_object_set_new_nocheck(obj, JSON_THUNDER_RESULT, json_true());
            rc |= json_object_set_new_nocheck(obj, JSON_MASK_PII, (ctrlm_get_voice_obj()->voice_stb_data_pii_mask_get() ? json_true() : json_false()) );

            if (status.capabilities.prv) {
               rc |= json_array_append_new(obj_capabilities, json_string("PRV"));
            }
            if (status.capabilities.wwFeedback) {
               rc |= json_array_append_new(obj_capabilities, json_string("WWFEEDBACK"));
            }
            rc |= json_object_set_new_nocheck(obj, JSON_CAPABILITIES, obj_capabilities);

            if(rc) {
                XLOGD_WARN("JSON error..");
            }
        }
    }

    if(result) {
        json_result(obj, call_data->result, sizeof(call_data->result));
    } else {
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }

    if(obj) {
        json_decref(obj);
        obj = NULL;
    }
    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::configure_voice(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = false;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        json_t *obj = json_loads(call_data->payload, JSON_DECODE_FLAGS, NULL);
        if(obj) {
            ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
            if(voice_obj) {
                result = voice_obj->voice_configure(obj, true);
            } else {
                XLOGD_ERROR("Voice Object is NULL!");
            }
            json_decref(obj);
        } else {
            XLOGD_ERROR("Invalid JSON");
        }
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }
    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::set_voice_init(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = false;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
        if(voice_obj) {
            result = voice_obj->voice_init_set(call_data->payload);
        } else {
            XLOGD_ERROR("Voice Object is NULL!");
        }
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }

    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::send_voice_message(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = false;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
        if(voice_obj) {

           json_error_t error;
           json_t *obj = json_loads((const char *)call_data->payload, JSON_REJECT_DUPLICATES, &error);
           if(obj == NULL) {
               XLOGD_ERROR("invalid json");
               result = false;
           } else if(!json_is_object(obj)) {
               XLOGD_ERROR("json object not found");
               json_decref(obj);
               result = false;
           } else {
               json_t *obj_session_id     = json_object_get(obj, "trx");
               std::string str_session_id = "";
               if(obj_session_id == NULL || !json_is_string(obj_session_id)) {
                  XLOGD_WARN("sessionId not found");
               } else {
                  str_session_id = std::string(json_string_value(obj_session_id));
               }

               result = voice_obj->voice_message(str_session_id, call_data->payload);
               json_decref(obj);
           }
        } else {
            XLOGD_ERROR("Voice Object is NULL!");
        }
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }

    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::voice_session_types(void *data) {
    json_t *obj_result = NULL;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        return(IARM_RESULT_INVALID_PARAM);
    }

    ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
    if(voice_obj) {
        XLOGD_INFO("payload = <%s>", call_data->payload);

        obj_result         = json_object();
        json_t *obj_types  = json_array();

        int rc = json_array_append_new(obj_types, json_string("ptt_transcription"));
        rc |= json_array_append_new(obj_types, json_string("ptt_audio_file"));

        #ifdef CTRLM_LOCAL_MIC
        rc |= json_array_append_new(obj_types, json_string("mic_audio_file"));
        rc |= json_array_append_new(obj_types, json_string("mic_stream_single"));
        rc |= json_array_append_new(obj_types, json_string("mic_stream_multi"));
        rc |= json_array_append_new(obj_types, json_string("mic_tap_stream_single"));
        rc |= json_array_append_new(obj_types, json_string("mic_tap_stream_multi"));
        rc |= json_array_append_new(obj_types, json_string("mic_factory_test"));
        #endif

        rc |= json_object_set_new_nocheck(obj_result, JSON_TYPES, obj_types);
        rc |= json_object_set_new_nocheck(obj_result, JSON_THUNDER_RESULT, json_true());

        if(rc) {
            XLOGD_WARN("JSON error..");
        }
    } else {
        XLOGD_ERROR("Voice Object is NULL!");
    }
    if(obj_result != NULL) {
        json_result(obj_result, call_data->result, sizeof(call_data->result));
        json_decref(obj_result);
        obj_result = NULL;
    } else {
        json_result_bool(false, call_data->result, sizeof(call_data->result));
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::voice_session_request(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = true;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
        uuid_t request_uuid;

        if(voice_obj) {
            XLOGD_INFO("payload = <%s>", call_data->payload);

            json_error_t error;
            json_t *obj = json_loads((const char *)call_data->payload, JSON_REJECT_DUPLICATES, &error);
            if(obj == NULL) {
                XLOGD_ERROR("invalid json");
                result = false;
            } else if(!json_is_object(obj)) {
                XLOGD_ERROR("json object not found");
                json_decref(obj);
                result = false;
            } else {
                ctrlm_voice_ipc_request_config_t request_config;
                request_config.low_latency            = false;
                request_config.low_cpu_util           = false;
                request_config.requires_transcription = false;
                request_config.requires_audio_file    = false;
                request_config.supports_named_pipe    = false;
                request_config.format                 = { .type = CTRLM_VOICE_FORMAT_PCM };
                request_config.device                 = CTRLM_VOICE_DEVICE_PTT;
                
                uuid_generate(request_uuid);

                json_t *obj_type = json_object_get(obj, "type");
                std::string str_type = "";
                std::string str_transcription = "";
                std::string str_audio_file    = "";
                int fd = -1;
                if(obj_type == NULL || !json_is_string(obj_type)) {
                    XLOGD_ERROR("request type parameter not present");
                    result = false;
                } else {
                    str_type = std::string(json_string_value(obj_type));
                    transform(str_type.begin(), str_type.end(), str_type.begin(), ::tolower);

                    const char *str_request_type = str_type.c_str();
                    voice_session_request_type_handler_t *handler = voice_session_request_type_handler_get(str_request_type, strlen(str_request_type));

                    if(handler == NULL) {
                        XLOGD_ERROR("request type <%s> is invalid.", str_request_type);
                        result = false;
                    } else {
                        if(!(*handler->func)(&request_config)) {
                            XLOGD_ERROR("request type <%s> is not supported.", str_request_type);
                            result = false;
                        } else { // Process optional parameters
                            if(request_config.requires_transcription) {
                                json_t *obj_transcription = json_object_get(obj, "transcription");
                                if(obj_transcription == NULL || !json_is_string(obj_transcription)) {
                                    XLOGD_ERROR("invalid transcription parameter.");
                                    result = false;
                                } else {
                                    str_transcription = std::string(json_string_value(obj_transcription));
                                    if(str_transcription.empty()) {
                                        XLOGD_ERROR("Empty transcription.");
                                        result = false;
                                    }
                                }
                            }
                            if(request_config.requires_audio_file) {
                                request_config.format.type = CTRLM_VOICE_FORMAT_PCM;

                                // Optional audio format
                                json_t *obj_audio_format = json_object_get(obj, "audio_format");
                                if(obj_audio_format != NULL && json_is_string(obj_audio_format)) {
                                    std::string str_audio_format = std::string(json_string_value(obj_audio_format));
                                    transform(str_audio_format.begin(), str_audio_format.end(), str_audio_format.begin(), ::tolower);

                                    if(0 == str_audio_format.compare("opus")) {
                                        request_config.format.type = CTRLM_VOICE_FORMAT_OPUS;
                                    } else if(0 == str_audio_format.compare("pcm")) {
                                        request_config.format.type = CTRLM_VOICE_FORMAT_PCM;
                                    } else {
                                        XLOGD_ERROR("Invalid audio format <%s>.", str_audio_format.c_str());
                                        result = false;
                                    }
                                }

                                if(result) {
                                    json_t *obj_audio_file = json_object_get(obj, "audio_file");
                                    if(obj_audio_file == NULL || !json_is_string(obj_audio_file)) {
                                        XLOGD_ERROR("invalid audio file parameter.");
                                        result = false;
                                    } else {
                                        str_audio_file = std::string(json_string_value(obj_audio_file));
                                        if(str_audio_file.empty()) {
                                            XLOGD_ERROR("Empty audio file.");
                                            result = false;
                                        } else {
                                            struct stat buf;

                                            XLOGD_INFO("stat file <%s>", str_audio_file.c_str());

                                            errno = 0;
                                            if(stat(str_audio_file.c_str(), &buf)) {
                                                int errsv = errno;
                                                XLOGD_ERROR("Unable to stat file <%s> <%s>", str_audio_file.c_str(), strerror(errsv));
                                                result = false;
                                            } else {
                                                bool is_named_pipe = S_ISFIFO(buf.st_mode);

                                                if(is_named_pipe) {
                                                    if(!request_config.supports_named_pipe) {
                                                        XLOGD_ERROR("named pipe is not supported for request type <%s>", str_request_type);
                                                        result = false;
                                                    } else {
                                                        XLOGD_INFO("open named pipe <%s>", str_audio_file.c_str());
                                                        errno = 0;
                                                        fd = open(str_audio_file.c_str(), O_RDONLY | O_NONBLOCK);
                                                        if(fd < 0) {
                                                            int errsv = errno;
                                                            XLOGD_ERROR("Unable to open named pipe <%s> <%s>", str_audio_file.c_str(), strerror(errsv));
                                                            result = false;
                                                        } else {
                                                            str_audio_file.clear();
                                                        }
                                                    }
                                                } else { // Validate file format
                                                    // open the audio input file
                                                    errno = 0;
                                                    int audio_fd = open(str_audio_file.c_str(), O_RDONLY);
                                                    if(audio_fd < 0) {
                                                        int errsv = errno;
                                                        XLOGD_ERROR("Unable to open file <%s> <%s>", str_audio_file.c_str(), strerror(errsv));
                                                        result = false;
                                                    } else if(request_config.format.type == CTRLM_VOICE_FORMAT_PCM) {
                                                       // verify the audio format
                                                       uint32_t data_length = 0;
                                                       xraudio_output_format_t format;
                                                       int32_t offset =  xraudio_container_header_parse_wave(audio_fd, NULL, 0, &format, &data_length);
                                                       if(offset < 0) {
                                                           XLOGD_ERROR("failed to parse wave header <%s>", str_audio_file.c_str());
                                                           result = false;
                                                       } else if(format.channel_qty != 1 || format.sample_rate != 16000 || format.sample_size != 2 || format.encoding.type != XRAUDIO_ENCODING_PCM) {
                                                           XLOGD_ERROR("unsupported wave file format - channel qty <%u> sample rate <%d> sample size <%u> encoding <%d>", format.channel_qty, format.sample_rate, format.sample_size, format.encoding.type);
                                                           result = false;
                                                       } else if(data_length == 0) {
                                                           XLOGD_ERROR("zero length audio data <%s>", str_audio_file.c_str());
                                                           result = false;
                                                       }
                                                       close(audio_fd);
                                                   }
                                               }
                                           }
                                       }
                                   }
                                }
                            }
                        }
                    }
                }

                if (true == result) {
                    ctrlm_voice_session_response_status_t voice_status = voice_obj->voice_session_req(
                            CTRLM_MAIN_NETWORK_ID_INVALID, CTRLM_MAIN_CONTROLLER_ID_INVALID, 
                            request_config.device, request_config.format, NULL, "APPLICATION", "0.0.0.0", "0.0.0.0", 0.0,
                            false, NULL, NULL, NULL, (fd >= 0) ? true : false, str_transcription.empty() ? NULL : str_transcription.c_str(), str_audio_file.empty() ? NULL : str_audio_file.c_str(), &request_uuid, request_config.low_latency, request_config.low_cpu_util, fd);
                    if (voice_status != VOICE_SESSION_RESPONSE_AVAILABLE && 
                        voice_status != VOICE_SESSION_RESPONSE_AVAILABLE_PAR_VOICE) {
                        XLOGD_ERROR("Failed opening voice session <%s>", ctrlm_voice_session_response_status_str(voice_status));
                        if(fd >= 0) {
                            close(fd);
                        }
                        result = false;
                    }
                }
                json_decref(obj);
            }
        } else {
            XLOGD_ERROR("Voice Object is NULL!");
            result = false;
        }
        json_t *obj_result = json_object();

        int rc = 0;
        if(!result) {
            rc |= json_object_set_new_nocheck(obj_result, JSON_THUNDER_RESULT, json_false());
        } else {
            char uuid_str[37] = {'\0'};
            uuid_unparse_lower(request_uuid, uuid_str);
            rc |= json_object_set_new_nocheck(obj_result, JSON_THUNDER_RESULT, json_true());
            rc |= json_object_set_new_nocheck(obj_result, JSON_SESSION_ID, json_string(uuid_str));
        }

        if(rc) {
            XLOGD_WARN("JSON error..");
        }

        json_result(obj_result, call_data->result, sizeof(call_data->result));
        json_decref(obj_result);
        obj_result = NULL;
    }

    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::voice_session_terminate(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = true;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
        if(voice_obj) {
            XLOGD_INFO("payload = <%s>", call_data->payload);

            json_error_t error;
            json_t *obj = json_loads((const char *)call_data->payload, JSON_REJECT_DUPLICATES, &error);
            if(obj == NULL) {
                XLOGD_ERROR("invalid json");
                result = false;
            } else if(!json_is_object(obj)) {
                XLOGD_ERROR("json object not found");
                json_decref(obj);
                result = false;
            } else {
                json_t *obj_session_id = json_object_get(obj, "sessionId");
                if (obj_session_id == NULL) {
                    XLOGD_ERROR("obj_session_id is NULL");
                    result = false;
                } else {
                    uuid_t uuid;
                    const char *uuid_str = json_string_value(obj_session_id);
                    if(uuid_str == NULL || uuid_parse(uuid_str, uuid) != 0) {
                        XLOGD_ERROR("invalid session id");
                        result = false;
                    } else {
                        std::string str_session_id = std::string(uuid_str);
                        result = voice_obj->voice_session_term(str_session_id);
                        json_decref(obj);
                    }
                }
            }
        } else {
            XLOGD_ERROR("Voice Object is NULL!");
            result = false;
        }
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }

    return(ret);
}

IARM_Result_t ctrlm_voice_ipc_iarm_thunder_t::voice_session_audio_stream_start(void *data) {
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    bool result       = true;
    ctrlm_voice_iarm_call_json_t *call_data = (ctrlm_voice_iarm_call_json_t *)data;

    if(call_data == NULL || CTRLM_VOICE_IARM_BUS_API_REVISION != call_data->api_revision) {
        result = false;
        ret = IARM_RESULT_INVALID_PARAM;
    } else {
        ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
        if(voice_obj) {
            XLOGD_INFO("payload = <%s>", call_data->payload);

            json_error_t error;
            json_t *obj = json_loads((const char *)call_data->payload, JSON_REJECT_DUPLICATES, &error);
            if(obj == NULL) {
                XLOGD_ERROR("invalid json");
                result = false;
            } else if(!json_is_object(obj)) {
                XLOGD_ERROR("json object not found");
                json_decref(obj);
                result = false;
            } else {
                json_t *obj_session_id = json_object_get(obj, "sessionId");
                std::string str_session_id = std::string(json_string_value(obj_session_id));

                result = voice_obj->voice_session_audio_stream_start(str_session_id);
                json_decref(obj);
            }
        } else {
            XLOGD_ERROR("Voice Object is NULL!");
            result = false;
        }
        json_result_bool(result, call_data->result, sizeof(call_data->result));
    }

    return(ret);
}

void ctrlm_voice_ipc_iarm_thunder_t::json_result_bool(bool result, char *result_str, size_t result_str_len) {
    if(result_str != NULL) {
        int rc      = 0;
        json_t *obj = json_object();

        rc = json_object_set_new_nocheck(obj, JSON_THUNDER_RESULT, (result ? json_true() : json_false()));
        if(rc) {
            XLOGD_ERROR("failed to add success var to JSON");
        } else {
            json_result(obj, result_str, result_str_len);
        }

        if(obj) {
            json_decref(obj);
            obj = NULL;
        }
    }
}

void ctrlm_voice_ipc_iarm_thunder_t::json_result(json_t *obj, char *result_str, size_t result_str_len) {
    if(obj != NULL && result_str != NULL) {
        char   *obj_str = NULL;
        errno_t safec_rc = memset_s(result_str, result_str_len * sizeof(char), 0, result_str_len * sizeof(char));
        ERR_CHK(safec_rc);

        obj_str = json_dumps(obj, JSON_COMPACT);
        if(obj_str) {
            if(strlen(obj_str) >= result_str_len) {
                XLOGD_ERROR("result string buffer not big enough!");
            }
            safec_rc = strcpy_s(result_str, result_str_len, obj_str);
            ERR_CHK(safec_rc);
            free(obj_str);
            obj_str = NULL;
        }
    }
}

const char *voice_device_str(ctrlm_voice_device_t device) {
    switch(device) {
        case CTRLM_VOICE_DEVICE_PTT:            return("ptt");
        case CTRLM_VOICE_DEVICE_FF:             return("ff");
        #ifdef CTRLM_LOCAL_MIC
        case CTRLM_VOICE_DEVICE_MICROPHONE:     return("mic");
        #ifdef CTRLM_LOCAL_MIC_TAP
        case CTRLM_VOICE_DEVICE_MICROPHONE_TAP: return("mic_tap");
        #endif
        #endif
        default: break;
    }
    return("invalid");
}

const char *voice_device_status_str(uint8_t status) {
    if(status == CTRLM_VOICE_DEVICE_STATUS_NONE)          { return("ready");         }
    if(status & CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE) { return("opened");        }
    if(status & CTRLM_VOICE_DEVICE_STATUS_DISABLED)       { return("disabled");      }
    if(status & CTRLM_VOICE_DEVICE_STATUS_PRIVACY)        { return("privacy");       }
    if(status & CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED)  { return("not supported"); }
    return("invalid");
}

bool broadcast_event(const char *bus_name, int event, const char *str) {
    bool ret = false;
    size_t str_size = strlen(str) + 1;
    size_t size = sizeof(ctrlm_voice_iarm_event_json_t) + str_size;
    ctrlm_voice_iarm_event_json_t *data = (ctrlm_voice_iarm_event_json_t *)malloc(size);
    if(data) {
        IARM_Result_t result;

        //Can't be replaced with safeC version of this
        memset(data, 0, size);

        data->api_revision = CTRLM_VOICE_IARM_BUS_API_REVISION;
        //Can't be replaced with safeC version of this, as safeC string functions doesn't allow string size more than 4K
        snprintf(data->payload, str_size, "%s", str);
        result = IARM_Bus_BroadcastEvent(bus_name, event, data, size);
        if(IARM_RESULT_SUCCESS != result) {
            XLOGD_ERROR("IARM Bus Error!");
        } else {
            ret = true;
        }
        if(data) {
            free(data);
        }
    } else {
        XLOGD_ERROR("Failed to allocate data for IARM event");
    }
    return(ret);
}

bool ctrlm_voice_ipc_request_supported_ptt_transcription(ctrlm_voice_ipc_request_config_t *config) {
   config->requires_transcription = true;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_PTT;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_INVALID };
   config->low_latency            = false;
   config->low_cpu_util           = false;
   return(true);
}

bool ctrlm_voice_ipc_request_supported_ptt_audio_file(ctrlm_voice_ipc_request_config_t *config) {
   config->requires_transcription = false;
   config->requires_audio_file    = true;
   config->supports_named_pipe    = true;
   config->device                 = CTRLM_VOICE_DEVICE_PTT;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM };
   config->low_latency            = false;
   config->low_cpu_util           = false;
   return(true);
}

bool ctrlm_voice_ipc_request_supported_mic_transcription(ctrlm_voice_ipc_request_config_t *config) {
   #if 1
   return(false);
   #else
   config->requires_transcription = true;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_INVALID };
   config->low_latency            = false;
   config->low_cpu_util           = false;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_audio_file(ctrlm_voice_ipc_request_config_t *config) {
   #ifndef CTRLM_LOCAL_MIC
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = true;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM };
   config->low_latency            = false;
   config->low_cpu_util           = false;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_stream_default(ctrlm_voice_ipc_request_config_t *config) {
   #if 1
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM };
   config->low_latency            = false;
   config->low_cpu_util           = false;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_stream_single(ctrlm_voice_ipc_request_config_t *config) {
   #ifndef CTRLM_LOCAL_MIC
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_32_BIT };
   config->low_latency            = true;
   config->low_cpu_util           = false;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_stream_multi(ctrlm_voice_ipc_request_config_t *config) {
   #ifndef CTRLM_LOCAL_MIC
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI };
   config->low_latency            = true;
   config->low_cpu_util           = false;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_tap_stream_single(ctrlm_voice_ipc_request_config_t *config) {
   #ifndef CTRLM_LOCAL_MIC_TAP
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE_TAP;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_32_BIT };
   config->low_latency            = true;
   config->low_cpu_util           = true;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_tap_stream_multi(ctrlm_voice_ipc_request_config_t *config) {
   #ifndef CTRLM_LOCAL_MIC_TAP
   return(false);
   #else
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE_TAP;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI };
   config->low_latency            = true;
   config->low_cpu_util           = true;
   return(true);
   #endif
}

bool ctrlm_voice_ipc_request_supported_mic_factory_test(ctrlm_voice_ipc_request_config_t *config) {
   #ifdef CTRLM_LOCAL_MIC_TAP
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE_TAP;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_RAW };
   config->low_latency            = true;
   config->low_cpu_util           = true;
   return(true);
   #elif defined(CTRLM_LOCAL_MIC)
   config->requires_transcription = false;
   config->requires_audio_file    = false;
   config->supports_named_pipe    = false;
   config->device                 = CTRLM_VOICE_DEVICE_MICROPHONE;
   config->format                 = { .type = CTRLM_VOICE_FORMAT_PCM_RAW };
   config->low_latency            = true;
   config->low_cpu_util           = false;
   return(true);
   #else
   return(false);
   #endif
}
