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
#include "ctrlm_voice_endpoint_http.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "ctrlm_log.h"

// Structures
typedef struct {
    uuid_t                          uuid;
    xrsr_src_t                      src;
    xrsr_session_config_out_t       configuration;
    xrsr_callback_session_config_t  callback;
    rdkx_timestamp_t                timestamp;
} ctrlm_voice_session_begin_cb_http_t;

// Timestamps and stats are not pointers to avoid corruption
typedef struct {
    uuid_t                uuid;
    xrsr_src_t            src;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_stream_begin_cb_http_t;

typedef struct {
    uuid_t               uuid;
    xrsr_stream_stats_t  stats;
    rdkx_timestamp_t     timestamp;
} ctrlm_voice_stream_end_cb_http_t;

typedef struct {
    uuid_t                uuid;
    xrsr_session_stats_t  stats;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_session_end_cb_http_t;
// End Structures

// Function Implementations
ctrlm_voice_endpoint_http_t::ctrlm_voice_endpoint_http_t(ctrlm_voice_t *voice_obj) : ctrlm_voice_endpoint_t(voice_obj) {
    server_ret_code = 0;  //CID:155208 - Uninit_ctor
    this->xrsv_obj_http = NULL;
}

ctrlm_voice_endpoint_http_t::~ctrlm_voice_endpoint_http_t() {
    if(this->xrsv_obj_http) {
        xrsv_http_destroy(this->xrsv_obj_http);
        this->xrsv_obj_http = NULL;
    }
}

bool ctrlm_voice_endpoint_http_t::open() {
    if(this->voice_obj == NULL) {
        XLOGD_ERROR("Voice object NULL");
        return(false);
    }

    std::string device_id      = this->voice_obj->voice_stb_data_device_id_get();
    std::string receiver_id    = this->voice_obj->voice_stb_data_receiver_id_get();
    std::string partner_id     = this->voice_obj->voice_stb_data_partner_id_get();
    std::string experience     = this->voice_obj->voice_stb_data_experience_get();
    std::string app_id         = this->voice_obj->voice_stb_data_app_id_http_get();
    std::string language       = this->voice_obj->voice_stb_data_guide_language_get().c_str();

    xrsv_http_params_t    params_http = {
       .device_id        = device_id.c_str(),
       .receiver_id      = receiver_id.c_str(),
       .partner_id       = partner_id.c_str(),
       .experience       = experience.c_str(),
       .app_id           = app_id.c_str(),
       .language         = language.c_str(),
       .test_flag        = this->voice_obj->voice_stb_data_test_get(),
       .mask_pii         = ctrlm_is_pii_mask_enabled(),
       .user_data        = (void *)this
   };

   if((this->xrsv_obj_http = xrsv_http_create(&params_http)) == NULL) {
      XLOGD_ERROR("Failed to open speech vrex HTTP");
      return(false);
   }

   return(true);
}

bool ctrlm_voice_endpoint_http_t::get_handlers(xrsr_handlers_t *handlers) {
    if(handlers == NULL) {
        XLOGD_ERROR("Handler struct NULL");
        return(false);
    }

    if(this->voice_obj == NULL || this->xrsv_obj_http == NULL) {
        XLOGD_ERROR("Voice obj or Xrsv obj NULL");
        return(false);
    }

    xrsv_http_handlers_t handlers_xrsv = {0};
    errno_t safec_rc = memset_s(handlers, sizeof(xrsr_handlers_t), 0, sizeof(xrsr_handlers_t));
    ERR_CHK(safec_rc);

    // Set up handlers
    handlers_xrsv.session_begin = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_session_begin;
    handlers_xrsv.session_end   = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_session_end;
    handlers_xrsv.stream_begin  = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_stream_begin;
    handlers_xrsv.stream_end    = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_stream_end;
    handlers_xrsv.connected     = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_connected;
    handlers_xrsv.disconnected  = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_disconnected;
    handlers_xrsv.recv_msg      = &ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_recv_msg;

    if(!xrsv_http_handlers(this->xrsv_obj_http, &handlers_xrsv, handlers)) {
        XLOGD_ERROR("failed to get handlers http");
        return(false);
    }
    return(true);
}

void ctrlm_voice_endpoint_http_t::voice_stb_data_receiver_id_set(std::string &receiver_id) {
    if(this->xrsv_obj_http) {
        xrsv_http_update_receiver_id(this->xrsv_obj_http, receiver_id.c_str());
    }
}

void ctrlm_voice_endpoint_http_t::voice_stb_data_device_id_set(std::string &device_id) {
    if(this->xrsv_obj_http) {
        xrsv_http_update_device_id(this->xrsv_obj_http, device_id.c_str());
    }
}

void ctrlm_voice_endpoint_http_t::voice_stb_data_partner_id_set(std::string &partner_id) {
    if(this->xrsv_obj_http) {
        xrsv_http_update_partner_id(this->xrsv_obj_http, partner_id.c_str());
    }
}
 
void ctrlm_voice_endpoint_http_t::voice_stb_data_experience_set(std::string &experience) {
    if(this->xrsv_obj_http) {
        xrsv_http_update_experience(this->xrsv_obj_http, experience.c_str());
    }
}

void ctrlm_voice_endpoint_http_t::voice_stb_data_guide_language_set(const char *language) {
   if(this->xrsv_obj_http) {
       xrsv_http_update_language(this->xrsv_obj_http, language);
   }
}

void ctrlm_voice_endpoint_http_t::voice_stb_data_pii_mask_set(bool enable) {
   if(this->xrsv_obj_http) {
       xrsv_http_update_mask_pii(this->xrsv_obj_http, enable);
   }
}

// Class Callbacks
void ctrlm_voice_endpoint_http_t::voice_session_begin_callback_http(void *data, int size) {
    ctrlm_voice_session_begin_cb_http_t *dqm = (ctrlm_voice_session_begin_cb_http_t *)data;
    if(NULL == data) {
        XLOGD_ERROR("NULL data");
        return;
    }
    ctrlm_voice_session_info_t info;
    std::ostringstream user_agent;
    xrsr_session_config_in_t config_in;
    memset(&config_in, 0, sizeof(config_in));

    this->voice_obj->voice_session_info(dqm->src, &info);
    const char *sat = this->voice_obj->voice_stb_data_sat_get();
    user_agent << "rmtType=" << info.controller_name.c_str() << "; rmtSVer=" << info.controller_version_sw.c_str() << "; rmtHVer=" << info.controller_version_hw.c_str() << "; stbName=" << info.stb_name.c_str() << "; rmtVolt=" << std::setprecision(3) << info.controller_voltage << "V";

    // Source
    config_in.src = dqm->src;

    ctrlm_voice_device_t source = xrsr_to_voice_device(dqm->src);
    bool has_sat  = false;

    // Device Certificate
    bool ocsp_verify_stapling = false;
    bool ocsp_verify_ca       = false;
    xrsr_cert_t *client_cert = &config_in.http.client_cert;
    bool use_mtls = this->voice_stb_data_client_certificate_get(client_cert, ocsp_verify_stapling, ocsp_verify_ca);

    config_in.http.host_verify          = true;
    config_in.http.ocsp_verify_stapling = ocsp_verify_stapling;
    config_in.http.ocsp_verify_ca       = ocsp_verify_ca;

    // User agent and SAT Token
    if(sat[0] != '\0') {
        config_in.http.sat_token = sat;
        has_sat = true;
    }

    XLOGD_TELEMETRY("session begin - src <%s> h_SAT <%s> h_MTLS <%s> h_OCSPst <%s> h_OCSPca <%s>", ctrlm_voice_device_str(source), has_sat ? "YES" : "NO", use_mtls ? "YES" : "NO", ocsp_verify_stapling ? "YES" : "NO", ocsp_verify_ca ? "YES" : "NO");

    errno_t safec_rc = strcpy_s(this->user_agent, sizeof(this->user_agent), user_agent.str().c_str());
    ERR_CHK(safec_rc);

    config_in.http.user_agent = this->user_agent;

    if(dqm->src == XRSR_SRC_RCU_PTT && this->query_str_qty > 0) { // Add the application defined query string parameters
        uint32_t i = 0;
        const char **query_strs = this->query_strs;

        while(*query_strs != NULL) {
            if(i >= XRSR_QUERY_STRING_QTY_MAX) {
               XLOGD_WARN("maximum query string elements reached");
               break;
            }
            config_in.http.query_strs[i++] = *query_strs;
            query_strs++;
        }

        config_in.http.query_strs[i] = NULL;
    }

    this->server_ret_code = 0;

    ctrlm_voice_session_begin_cb_t session_begin;
    uuid_copy(session_begin.header.uuid, dqm->uuid);
    uuid_copy(this->uuid, dqm->uuid);
    session_begin.header.timestamp     = dqm->timestamp;
    session_begin.src                  = dqm->src;
    session_begin.configuration        = dqm->configuration;
    session_begin.endpoint             = this;
    session_begin.keyword_verification = false; // Possibly could change in future, but meets current use cases.

    this->voice_obj->voice_session_begin_callback(&session_begin);

    if(dqm->configuration.cb_session_config != NULL) {
        (*dqm->configuration.cb_session_config)(dqm->uuid, &config_in);
    }
}

void ctrlm_voice_endpoint_http_t::voice_stream_begin_callback_http(void *data, int size) {
    ctrlm_voice_stream_begin_cb_http_t *dqm = (ctrlm_voice_stream_begin_cb_http_t *)data;
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

void ctrlm_voice_endpoint_http_t::voice_stream_end_callback_http(void *data, int size) {
    ctrlm_voice_stream_end_cb_http_t *dqm = (ctrlm_voice_stream_end_cb_http_t *)data;
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

void ctrlm_voice_endpoint_http_t::voice_session_end_callback_http(void *data, int size) {
    ctrlm_voice_session_end_cb_http_t *dqm = (ctrlm_voice_session_end_cb_http_t *)data;
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

    // Check if HTTP was successful
    if((stats->reason != XRSR_SESSION_END_REASON_EOS && stats->reason != XRSR_SESSION_END_REASON_TERMINATE && stats->reason != XRSR_SESSION_END_REASON_EOT) 
            || (stats->ret_code_library != 0)  || (stats->ret_code_protocol != 200) || (this->server_ret_code != 0)) {
        success = false;
    }

    // Check for SAT errors
    if(this->server_ret_code == 116 || this->server_ret_code == 117 || this->server_ret_code == 118 ||
       this->server_ret_code == 119 || this->server_ret_code == 120 || this->server_ret_code == 121 ||
       this->server_ret_code == 123) {
        XLOGD_INFO("SAT Error Code from server <%ld>", this->server_ret_code);
        ctrlm_main_invalidate_service_access_token();
    }

    ctrlm_voice_session_end_cb_t session_end;
    uuid_copy(session_end.header.uuid, dqm->uuid);
    uuid_clear(this->uuid);
    session_end.header.timestamp = dqm->timestamp;
    session_end.success          = success;
    session_end.stats            = dqm->stats;
    this->voice_obj->voice_session_end_callback(&session_end);
}
void ctrlm_voice_endpoint_http_t::voice_session_recv_msg_http(const char *transcription, long ret_code) {
    this->voice_obj->voice_session_transcription_callback(this->uuid, transcription);
    this->voice_obj->voice_server_return_code_callback(this->uuid, NULL, ret_code);
    this->server_ret_code = ret_code;
}
// End Class Callbacks

// Static Callbacks
void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_session_begin(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_session_begin_cb_http_t msg = {0};

    bool is_mic = ctrlm_voice_xrsr_src_is_mic(src);

    if(!is_mic) {
        // This is a controller, make sure session request / controller info is satisfied
        XLOGD_DEBUG("Checking if VSR is done");
        sem_wait(endpoint->voice_session_vsr_semaphore_get());
    }

    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.configuration = *configuration;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_session_begin_callback_http(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_session_end(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_session_end_cb_http_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_session_end_callback_http(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_stream_begin(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_stream_begin_cb_http_t msg;
    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_begin_callback_http(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_stream_end(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_stream_end_cb_http_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_end_callback_http(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_connected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_connected_callback(&data);
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_disconnected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    ctrlm_voice_disconnected_cb_t data;
    uuid_copy(data.header.uuid, uuid);
    data.retry            = false;
    data.header.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_disconnected_callback(&data);
}

void ctrlm_voice_endpoint_http_t::ctrlm_voice_handler_http_recv_msg(xrsv_http_recv_msg_t *msg, void *user_data) {
    ctrlm_voice_endpoint_http_t *endpoint = (ctrlm_voice_endpoint_http_t *)user_data;
    if(msg) {
        endpoint->voice_session_recv_msg_http(msg->transcription, msg->ret_code);
    }
}
// End Static Callbacks

// End Function Implementations

