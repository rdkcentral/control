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
#include "ctrlm_voice_endpoint_ws_nsp.h"
#include "ctrlm_log.h"

// Structures
typedef struct {
    uuid_t                          uuid;
    xrsr_src_t                      src;
    xrsr_session_config_out_t       configuration;
    xrsr_callback_session_config_t  callback;
    rdkx_timestamp_t                timestamp;
} ctrlm_voice_session_begin_cb_ws_nsp_t;

// Timestamps and stats are not pointers to avoid corruption
typedef struct {
    uuid_t                uuid;
    xrsr_src_t            src;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_stream_begin_cb_ws_nsp_t;

typedef struct {
    uuid_t               uuid;
    xrsr_stream_stats_t  stats;
    rdkx_timestamp_t     timestamp;
} ctrlm_voice_stream_end_cb_ws_nsp_t;

typedef struct {
    uuid_t                uuid;
    xrsr_session_stats_t  stats;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_session_end_cb_ws_nsp_t;

typedef struct {
    sem_t                       *semaphore;
    const char *                 msg;
    bool *                       result;
} ctrlm_voice_message_send_ws_nsp_t;
// End Structures

// Static Functions
// End Static Functions

// Function Implementations
ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_endpoint_ws_nsp_t(ctrlm_voice_t *voice_obj) : ctrlm_voice_endpoint_t(voice_obj) {
    server_ret_code = 0;  //CID:157976 - Uninit-ctor
}

ctrlm_voice_endpoint_ws_nsp_t::~ctrlm_voice_endpoint_ws_nsp_t() {
}

bool ctrlm_voice_endpoint_ws_nsp_t::open() {
    if(this->voice_obj == NULL) {
        XLOGD_ERROR("Voice object NULL");
        return(false);
    }

   return(true);
}

bool ctrlm_voice_endpoint_ws_nsp_t::get_handlers(xrsr_handlers_t *handlers) {
    if(handlers == NULL) {
        XLOGD_ERROR("Handler struct NULL");
        return(false);
    }

    if(this->voice_obj == NULL) {
        XLOGD_ERROR("Voice obj is NULL");
        return(false);
    }

    errno_t safec_rc = memset_s(handlers, sizeof(xrsr_handlers_t), 0, sizeof(xrsr_handlers_t));
    ERR_CHK(safec_rc);

    // Set up handlers
    handlers->data              = this;
    handlers->session_begin     = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_session_begin;
    handlers->session_config    = NULL;
    handlers->session_end       = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_session_end;
    handlers->stream_begin      = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_stream_begin;
    handlers->stream_kwd        = NULL;
    handlers->stream_audio      = NULL;
    handlers->stream_end        = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_stream_end;
    handlers->source_error      = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_source_error;
    handlers->connected         = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_connected;
    handlers->disconnected      = &ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_disconnected;
    handlers->recv_msg          = NULL;

    return(true);
}

void ctrlm_voice_endpoint_ws_nsp_t::voice_session_begin_callback_ws_nsp(void *data, int size) {
    ctrlm_voice_session_begin_cb_ws_nsp_t *dqm = (ctrlm_voice_session_begin_cb_ws_nsp_t *)data;
    if(NULL == data) {
        XLOGD_ERROR("NULL data");
        return;
    }
    xrsr_session_config_in_t config_in;
    memset(&config_in, 0, sizeof(config_in));

    this->server_ret_code = 0;

    // Source
    config_in.src = dqm->src;

    ctrlm_voice_device_t source = xrsr_to_voice_device(dqm->src);

    bool ocsp_verify_stapling = false;
    bool ocsp_verify_ca       = false;
    xrsr_cert_t *client_cert = &config_in.ws.client_cert;
    bool use_mtls = this->voice_stb_data_client_certificate_get(client_cert, ocsp_verify_stapling, ocsp_verify_ca);

    config_in.ws.host_verify          = true;
    config_in.ws.ocsp_verify_stapling = ocsp_verify_stapling;
    config_in.ws.ocsp_verify_ca       = ocsp_verify_ca;
    config_in.ws.cert_expired_allow   = true;
    config_in.ws.cert_revoked_allow   = false;
    config_in.ws.ocsp_expired_allow   = false;

    XLOGD_TELEMETRY("session begin - src <%s> x_MTLS <%s> x_OCSPst <%s> x_OCSPca <%s>", ctrlm_voice_device_str(source), use_mtls ? "YES" : "NO", ocsp_verify_stapling ? "YES" : "NO", ocsp_verify_ca ? "YES" : "NO");

    ctrlm_voice_session_begin_cb_t session_begin;
    uuid_copy(session_begin.header.uuid, dqm->uuid);
    uuid_copy(this->uuid, dqm->uuid);
    session_begin.header.timestamp     = dqm->timestamp;
    session_begin.src                  = dqm->src;
    session_begin.configuration        = dqm->configuration;
    session_begin.endpoint             = this;
    session_begin.keyword_verification = false;

    this->voice_obj->voice_session_begin_callback(&session_begin);

    if(dqm->configuration.cb_session_config != NULL) {
        (*dqm->configuration.cb_session_config)(dqm->uuid, &config_in);
    }
}

void ctrlm_voice_endpoint_ws_nsp_t::voice_stream_begin_callback_ws_nsp(void *data, int size) {
    ctrlm_voice_stream_begin_cb_ws_nsp_t *dqm = (ctrlm_voice_stream_begin_cb_ws_nsp_t *)data;
    if(NULL == dqm) {
        XLOGD_ERROR("NULL data");
        return;
    }
    if(false == this->voice_obj->voice_session_id_is_current(dqm->uuid)) {
        return;
    }
    ctrlm_voice_stream_begin_cb_t stream_begin;
    uuid_copy(stream_begin.header.uuid, dqm->uuid);
    stream_begin.header.timestamp = dqm->timestamp;
    stream_begin.src              = dqm->src;
    this->voice_obj->voice_stream_begin_callback(&stream_begin);
}

void ctrlm_voice_endpoint_ws_nsp_t::voice_stream_end_callback_ws_nsp(void *data, int size) {
    ctrlm_voice_stream_end_cb_ws_nsp_t *dqm = (ctrlm_voice_stream_end_cb_ws_nsp_t *)data;
    if(NULL == dqm) {
        XLOGD_ERROR("NULL data");
        return;
    }
    if(false == this->voice_obj->voice_session_id_is_current(dqm->uuid)) {
        return;
    }

    ctrlm_voice_stream_end_cb_t stream_end;
    uuid_copy(stream_end.header.uuid, dqm->uuid);
    stream_end.header.timestamp = dqm->timestamp;
    stream_end.stats            = dqm->stats;
    this->voice_obj->voice_stream_end_callback(&stream_end);
}

void ctrlm_voice_endpoint_ws_nsp_t::voice_session_end_callback_ws_nsp(void *data, int size) {
    ctrlm_voice_session_end_cb_ws_nsp_t *dqm = (ctrlm_voice_session_end_cb_ws_nsp_t *)data;
    if(NULL == data) {
        XLOGD_ERROR("NULL data");
        return;
    }

    if(false == this->voice_obj->voice_session_id_is_current(dqm->uuid)) {
        return;
    }
    bool success = true;
    xrsr_session_stats_t *stats = &dqm->stats;
    
    if(stats == NULL) {
        XLOGD_ERROR("stats are NULL");
        return;
    }

    // Check if WS was successful
    if((stats->reason != XRSR_SESSION_END_REASON_DISCONNECT_REMOTE) || (this->server_ret_code != 0 && this->server_ret_code != 200)) {
        success = false;
    }

    ctrlm_voice_session_end_cb_t session_end;
    uuid_copy(session_end.header.uuid, dqm->uuid);
    uuid_clear(this->uuid);
    session_end.header.timestamp = dqm->timestamp;
    session_end.success          = success;
    session_end.stats            = dqm->stats;
    this->voice_obj->voice_session_end_callback(&session_end);
}

void ctrlm_voice_endpoint_ws_nsp_t::voice_session_server_return_code_ws_nsp(long ret_code) {
    this->server_ret_code = ret_code;
    this->voice_obj->voice_server_return_code_callback(this->uuid, NULL, ret_code);
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_session_begin(void *data, const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_keyword_detector_result_t *detector_result, xrsr_session_config_out_t *config_out, xrsr_session_config_in_t *config_in, rdkx_timestamp_t *timestamp, const char *transcription_in) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_session_begin_cb_ws_nsp_t msg = {0};

    if(!ctrlm_voice_xrsr_src_is_mic(src)) {
        // This is a controller, make sure session request / controller info is satisfied
        XLOGD_DEBUG("Checking if VSR is done");
        sem_wait(endpoint->voice_session_vsr_semaphore_get());
    }

    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.configuration = *config_out;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_session_begin_callback_ws_nsp(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_session_end(void *data, const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_session_end_cb_ws_nsp_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);

    endpoint->voice_session_end_callback_ws_nsp(&msg, sizeof(msg));
    endpoint->voice_obj->voice_status_set(uuid);
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_stream_begin(void *data, const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_stream_begin_cb_ws_nsp_t msg;
    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_begin_callback_ws_nsp(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_stream_end(void *data, const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_stream_end_cb_ws_nsp_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_end_callback_ws_nsp(&msg, sizeof(msg));
}

bool ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_connected(void *data, const uuid_t uuid, xrsr_handler_send_t send, void *param, rdkx_timestamp_t *timestamp, xrsr_session_config_update_t *session_config_update) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_cb_header_t msg;
    uuid_copy(msg.uuid, uuid);
    msg.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_connected_callback(&msg);
    return(true);
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_disconnected(void *data, const uuid_t uuid, xrsr_session_end_reason_t reason, bool retry, bool *detect_resume, rdkx_timestamp_t *timestamp) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)data;
    ctrlm_voice_disconnected_cb_t msg;
    uuid_copy(msg.header.uuid, uuid);
    msg.retry            = retry;
    msg.header.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_disconnected_callback(&msg);
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_conn_close(const char *reason, long ret_code, void *user_data) {
    ctrlm_voice_endpoint_ws_nsp_t *endpoint = (ctrlm_voice_endpoint_ws_nsp_t *)user_data;
    endpoint->voice_session_server_return_code_ws_nsp(ret_code);
}

void ctrlm_voice_endpoint_ws_nsp_t::ctrlm_voice_handler_ws_nsp_source_error(void *data, xrsr_src_t src) {
}

// End Function Implementations
