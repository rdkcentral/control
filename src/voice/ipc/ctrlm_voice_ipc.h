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
#ifndef __CTRLM_VOICE_IPC_H__
#define __CTRLM_VOICE_IPC_H__

#include "ctrlm.h"
#include "ctrlm_voice_types.h"
#include "ctrlm_ipc_voice.h"

// Classes for eventing

// Structure shared by all messages
class ctrlm_voice_ipc_event_common_t {
public:
    ctrlm_network_id_t    network_id;
    ctrlm_network_type_t  network_type;
    ctrlm_controller_id_t controller_id;
    bool                  voice_assistant;
    unsigned long         session_id_ctrlm;
    std::string           session_id_server;
    ctrlm_voice_device_t  device_type;

    ctrlm_voice_ipc_event_common_t() {
        this->reset();
    }

    void reset() {
        network_id           = CTRLM_MAIN_NETWORK_ID_INVALID;
        network_type         = CTRLM_NETWORK_TYPE_INVALID;
        controller_id        = CTRLM_MAIN_CONTROLLER_ID_INVALID;
        voice_assistant      = false;
        session_id_ctrlm     = 0;
        session_id_server    = "";
        device_type          = CTRLM_VOICE_DEVICE_INVALID;
    }
};

class ctrlm_voice_ipc_event_session_begin_t {
public:
    ctrlm_voice_ipc_event_common_t common;
    std::string                    mime_type;
    std::string                    sub_type;
    std::string                    language;
    bool                           keyword_verification;
    bool                           keyword_verification_required;

    ctrlm_voice_ipc_event_session_begin_t() {
        mime_type            = "";
        sub_type             = "";
        language             = "";
        keyword_verification = false;
        keyword_verification_required = false;  //CID:160949
    }
};

class ctrlm_voice_ipc_event_stream_begin_t {
public:
    ctrlm_voice_ipc_event_common_t common;

    ctrlm_voice_ipc_event_stream_begin_t() {
    }
};

class ctrlm_voice_ipc_event_stream_end_t {
public:
    ctrlm_voice_ipc_event_common_t common;
    int                            reason;

    ctrlm_voice_ipc_event_stream_end_t() {
        reason            = 0;
    }
};

typedef enum {
    SESSION_END_SUCCESS,
    SESSION_END_FAILURE,
    SESSION_END_ABORT,
    SESSION_END_SHORT_UTTERANCE
} ctrlm_voice_ipc_event_session_end_result_t;

typedef struct {
    std::string type;
    std::string firmware;
    std::string device_id;
    std::string ctrlm_version;
    std::string controller_version;
    std::string controller_type;
} ctrlm_voice_ipc_event_session_end_stb_stats_t;

typedef struct {
    std::string server_ip;
    double      dns_time;
    double      connect_time;
} ctrlm_voice_ipc_event_session_end_server_stats_t;

class ctrlm_voice_ipc_event_keyword_verification_t {
public:
    ctrlm_voice_ipc_event_common_t common;
    bool verified = false;

    ctrlm_voice_ipc_event_keyword_verification_t() {
    }
};

class ctrlm_voice_ipc_event_session_end_t {
public:
    ctrlm_voice_ipc_event_common_t             common;

    // Success
    std::string                                transcription;
    // End Success

    // Error
    ctrlm_voice_ipc_event_session_end_result_t result;
    long                                       return_code_protocol;
    long                                       return_code_protocol_library;
    long                                       return_code_server;
    std::string                                return_code_server_str;
    long                                       return_code_internal;
    // End Error

    // Error / Short Utterance / Abort
    int                                        reason;
    // End Error / Short Utterance / Abort

    // Stats
    ctrlm_voice_ipc_event_session_end_stb_stats_t    *stb_stats;
    ctrlm_voice_ipc_event_session_end_server_stats_t *server_stats;

    // End Stats

    ctrlm_voice_ipc_event_session_end_t() {
        result                       = SESSION_END_SUCCESS;
        transcription                = "";
        return_code_protocol         = 0;
        return_code_protocol_library = 0;
        return_code_server           = 0;
        return_code_server_str       = "";
        return_code_internal         = 0;
        reason                       = 0;
        stb_stats                    = NULL;
        server_stats                 = NULL;
    }
};

// This needs to be implemented
class ctrlm_voice_ipc_event_session_statistics_t {
public:
    ctrlm_voice_ipc_event_common_t common;
    ctrlm_voice_stats_session_t    session = { 0 }; ///< Statistics for the session
    ctrlm_voice_stats_reboot_t     reboot  = { 0 };  ///< Data associated with a reboot

    ctrlm_voice_ipc_event_session_statistics_t() {
    }
};
// End classes for eventing

class ctrlm_voice_ipc_t {
public:
    ctrlm_voice_ipc_t(ctrlm_voice_t *obj_voice) {
        this->obj_voice = obj_voice;
    }
    virtual ~ctrlm_voice_ipc_t() {};

    // Interface
    virtual bool register_ipc() const = 0;
    virtual bool session_begin(const ctrlm_voice_ipc_event_session_begin_t &session_begin) = 0;
    virtual bool stream_begin(const ctrlm_voice_ipc_event_stream_begin_t &stream_begin) = 0;
    virtual bool stream_end(const ctrlm_voice_ipc_event_stream_end_t &stream_end) = 0;
    virtual bool session_end(const ctrlm_voice_ipc_event_session_end_t &session_end) = 0;
    virtual bool server_message(const char *message, unsigned long size) = 0; // Pass a pointer to the message to avoid copying possible large chunks of data
    virtual bool keyword_verification(const ctrlm_voice_ipc_event_keyword_verification_t &keyword_verification) = 0;
    virtual bool session_statistics(const ctrlm_voice_ipc_event_session_statistics_t &session_stats) = 0;
    virtual void deregister_ipc() const = 0;
    // End Interface

protected:
    ctrlm_voice_t *obj_voice;

};

#endif
