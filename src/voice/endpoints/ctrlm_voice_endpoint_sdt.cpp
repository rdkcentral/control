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
#include "ctrlm_voice_endpoint_sdt.h"
#include "ctrlm_log.h"

// Structures
typedef struct {
    uuid_t                          uuid;
    xrsr_src_t                      src;
    xrsr_session_config_out_t       configuration;
    xrsr_callback_session_config_t  callback;
    bool                            has_stream_params;
    vmic_sdt_stream_params_t        stream_params;
    rdkx_timestamp_t                timestamp;
} ctrlm_voice_session_begin_cb_sdt_t;

// Timestamps and stats are not pointers to avoid corruption
typedef struct {
    uuid_t                uuid;
    xrsr_src_t            src;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_stream_begin_cb_sdt_t;

typedef struct {
    uuid_t               uuid;
    xrsr_stream_stats_t  stats;
    rdkx_timestamp_t     timestamp;
} ctrlm_voice_stream_end_cb_sdt_t;

typedef struct {
    uuid_t                uuid;
    xrsr_session_stats_t  stats;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_session_end_cb_sdt_t;

typedef struct {
    sem_t                       *semaphore;
    const char *                 msg;
    bool *                       result;
} ctrlm_voice_message_send_sdt_t;
// End Structures

// Function Implementations
ctrlm_voice_endpoint_sdt_t::ctrlm_voice_endpoint_sdt_t(ctrlm_voice_t *voice_obj) : ctrlm_voice_endpoint_t(voice_obj) {
    this->vmic_obj_sdt     = NULL;
    #ifdef SUPPORT_ASYNC_SRVR_MSG
    this->voice_message_available = true;
    #else
    this->voice_message_available = false;
    #endif
}

ctrlm_voice_endpoint_sdt_t::~ctrlm_voice_endpoint_sdt_t() {
    if(this->vmic_obj_sdt) {
        vmic_sdt_destroy(this->vmic_obj_sdt);
        this->vmic_obj_sdt = NULL;
    }
}

bool ctrlm_voice_endpoint_sdt_t::open() {

    if(this->voice_obj == NULL) {
        XLOGD_ERROR("Voice object NULL");
        return(false);
    }
  
    vmic_sdt_params_t      params_sdt = {
      .test_flag        = this->voice_obj->voice_stb_data_test_get(),
      .mask_pii         = ctrlm_is_pii_mask_enabled(),
      .user_data        = (void *)this
   };

   if((this->vmic_obj_sdt = vmic_sdt_create(&params_sdt)) == NULL) {
      XLOGD_ERROR("Failed to open speech vmic sdt");
      return(false);
   }

   return(true);
}

bool ctrlm_voice_endpoint_sdt_t::get_handlers(xrsr_handlers_t *handlers) {
    if(handlers == NULL) {
        XLOGD_ERROR("Handler struct NULL");
        return(false);
    }

   if(this->voice_obj == NULL || this->vmic_obj_sdt == NULL) {
        XLOGD_ERROR("Voice obj or Xrsv obj NULL");
        return(false);
    }

    vmic_sdt_handlers_t handlers_vmic;
    memset(handlers, 0, sizeof(xrsr_handlers_t));
    memset(&handlers_vmic, 0, sizeof(handlers_vmic));

    // Set up handlers
    handlers_vmic.session_begin     = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_session_begin;
    handlers_vmic.session_end       = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_session_end;
    handlers_vmic.stream_begin      = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_begin;
    handlers_vmic.stream_kwd        = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_kwd;
    handlers_vmic.stream_end        = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_end;
    handlers_vmic.connected         = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_connected;
    handlers_vmic.disconnected      = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_disconnected;
    handlers_vmic.msg               = &ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_server_message;

    if(!vmic_sdt_handlers(this->vmic_obj_sdt, &handlers_vmic, handlers)) {
        XLOGD_ERROR("failed to get handlers sdt");
        return(false);
    }

    return(true);
}

bool ctrlm_voice_endpoint_sdt_t::voice_init_set(const char *blob) const {
    if(this->voice_obj == NULL || this->vmic_obj_sdt == NULL) {
        XLOGD_ERROR("not ready for this request");
        return(false);
    }

    return(true);

}

bool ctrlm_voice_endpoint_sdt_t::voice_message(const char *msg) const {
    bool ret = false;
    sem_t semaphore;

    sem_init(&semaphore, 0, 0);

    ctrlm_voice_message_send_sdt_t dqm;
    memset(&dqm, 0, sizeof(dqm));
    dqm.semaphore = &semaphore;
    dqm.msg       = msg;
    dqm.result    = &ret;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_VOICE, (ctrlm_msg_handler_voice_t)&ctrlm_voice_endpoint_sdt_t::voice_message_send, &dqm, sizeof(dqm), (void *)this);
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return(ret);
}

void ctrlm_voice_endpoint_sdt_t::voice_stb_data_pii_mask_set(bool enable) {
    if(this->vmic_obj_sdt) {
        vmic_sdt_update_mask_pii(this->vmic_obj_sdt, enable);
    }
}

void ctrlm_voice_endpoint_sdt_t::voice_message_send(void *data, int size) {
    ctrlm_voice_message_send_sdt_t *dqm = (ctrlm_voice_message_send_sdt_t *)data;
    if(dqm == NULL || dqm->result == NULL || dqm->msg == NULL) {
        XLOGD_ERROR("Null data");
        return;
    } else if(this->voice_obj == NULL || this->vmic_obj_sdt == NULL) {
        XLOGD_ERROR("not ready for this request");
        *dqm->result = false;
    } else {
        if(this->voice_message_available == false) {
            this->voice_message_queue.push_back(dqm->msg);
            *dqm->result = true;
        } else {
            *dqm->result = vmic_sdt_send_msg(dqm->msg);
        }
    }
    if(dqm->semaphore) {
        sem_post(dqm->semaphore);
    }
}

void ctrlm_voice_endpoint_sdt_t::voice_session_begin_callback_sdt(void *data, int size) {
    ctrlm_voice_session_begin_cb_sdt_t *dqm = (ctrlm_voice_session_begin_cb_sdt_t *)data;
    if(NULL == data) {
        XLOGD_ERROR("NULL data");
        return;
    }
    ctrlm_voice_session_info_t info;
    bool keyword_verification = false;
    xrsr_session_config_in_t config_in;
    memset(&config_in, 0, sizeof(config_in));

    this->server_ret_code = 0;
    this->voice_message_available = false; // This is just for sanity
    this->voice_message_queue.clear();

    this->voice_obj->voice_session_info(dqm->src, &info);

    // Source
    config_in.src = dqm->src;

    // Handle stream parameters
    if(dqm->has_stream_params) {
       vmic_sdt_stream_params_t *stream_params = &dqm->stream_params;
       if(info.has_stream_params) {
          stream_params->keyword_sample_begin               = info.stream_params.pre_keyword_sample_qty; // in samples
          stream_params->keyword_sample_end                 = (info.stream_params.pre_keyword_sample_qty + info.stream_params.keyword_sample_qty); // in samples
          stream_params->keyword_doa                        = info.stream_params.doa;
          stream_params->keyword_sensitivity                = info.stream_params.standard_search_pt;
          stream_params->keyword_sensitivity_triggered      = info.stream_params.standard_search_pt_triggered;
          stream_params->keyword_sensitivity_high           = info.stream_params.high_search_pt;
          stream_params->keyword_sensitivity_high_support   = info.stream_params.high_search_pt_support;
          stream_params->keyword_sensitivity_high_triggered = info.stream_params.high_search_pt_triggered;
          stream_params->dynamic_gain                       = info.stream_params.dynamic_gain;
          for(int i = 0; i < CTRLM_FAR_FIELD_BEAMS_MAX; i++) {
              if(info.stream_params.beams[i].selected) {
                  //TODO figure out how to determine linear vs nonlinear
                  if(info.stream_params.beams[i].confidence_normalized) {
                      stream_params->linear_confidence       = info.stream_params.beams[i].confidence;
                      stream_params->nonlinear_confidence    = 0;
                  } else {
                      stream_params->nonlinear_confidence = info.stream_params.beams[i].confidence;
                      stream_params->linear_confidence    = 0;
                  }
                  stream_params->signal_noise_ratio   = info.stream_params.beams[i].snr;
              }
          }
          stream_params->push_to_talk         = info.stream_params.push_to_talk;
          if(stream_params->keyword_sample_begin != stream_params->keyword_sample_end) {
              keyword_verification = true;
          }
       } else {
          // TODO tie this together with xraudio_stream_to_pipe call in xrsr_xraudio.c
          if(stream_params->keyword_sample_begin > 1600) {
             uint32_t delta = stream_params->keyword_sample_begin - 1600;
             stream_params->keyword_sample_begin -= delta;
             stream_params->keyword_sample_end   -= delta;
          }

          stream_params->keyword_sample_begin = stream_params->keyword_sample_begin / 16; // in ms
          stream_params->keyword_sample_end   = stream_params->keyword_sample_end   / 16; // in ms
          stream_params->keyword_doa          = 0;
          stream_params->dynamic_gain         = 0;
          stream_params->push_to_talk         = false;
          if(stream_params->keyword_sample_begin != stream_params->keyword_sample_end) {
              keyword_verification = true;
          }
       }
       XLOGD_INFO("session begin - ptt <%s> keyword begin <%u> end <%u> doa <%u> gain <%4.1f> db", (stream_params->push_to_talk ? "TRUE" : "FALSE"), stream_params->keyword_sample_begin, stream_params->keyword_sample_end, stream_params->keyword_doa, stream_params->dynamic_gain);
    }
    // End handle stream parameters

    ctrlm_voice_session_begin_cb_t session_begin;
    uuid_copy(session_begin.header.uuid, dqm->uuid);
    uuid_copy(this->uuid, dqm->uuid);
    session_begin.header.timestamp     = dqm->timestamp;
    session_begin.src                  = dqm->src;
    session_begin.configuration        = dqm->configuration;
    session_begin.endpoint             = this;
    session_begin.keyword_verification = keyword_verification;

    this->voice_obj->voice_session_begin_callback(&session_begin);

    if(dqm->configuration.cb_session_config != NULL) {
        (*dqm->configuration.cb_session_config)(dqm->uuid, &config_in);
    }
}

void ctrlm_voice_endpoint_sdt_t::voice_stream_begin_callback_sdt(void *data, int size) {
    ctrlm_voice_stream_begin_cb_sdt_t *dqm = (ctrlm_voice_stream_begin_cb_sdt_t *)data;
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

    this->voice_message_queue.clear();
    this->voice_message_available = true;
}

void ctrlm_voice_endpoint_sdt_t::voice_stream_end_callback_sdt(void *data, int size) {
    ctrlm_voice_stream_end_cb_sdt_t *dqm = (ctrlm_voice_stream_end_cb_sdt_t *)data;
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

void ctrlm_voice_endpoint_sdt_t::voice_session_end_callback_sdt(void *data, int size) {
    ctrlm_voice_session_end_cb_sdt_t *dqm = (ctrlm_voice_session_end_cb_sdt_t *)data;
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

    ctrlm_voice_session_end_cb_t session_end;
    uuid_copy(session_end.header.uuid, dqm->uuid);
    session_end.header.timestamp = dqm->timestamp;
    session_end.success          = success;
    session_end.stats            = dqm->stats;
    this->voice_obj->voice_session_end_callback(&session_end);
}

void ctrlm_voice_endpoint_sdt_t::voice_session_recv_msg_sdt(const char *transcription) {
    this->voice_obj->voice_session_transcription_callback(this->uuid, transcription);
}

void ctrlm_voice_endpoint_sdt_t::voice_session_server_return_code_sdt(long ret_code) {
    this->server_ret_code = ret_code;
    this->voice_obj->voice_server_return_code_callback(this->uuid, NULL, ret_code);
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_session_begin(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, vmic_sdt_stream_params_t *stream_params, rdkx_timestamp_t *timestamp, void *user_data) {

ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_session_begin_cb_sdt_t msg;
    memset(&msg, 0, sizeof(msg));

    if(!ctrlm_voice_xrsr_src_is_mic(src)) {
        // This is a controller, make sure session request / controller info is satisfied
        XLOGD_DEBUG("Checking if VSR is done");
        sem_wait(endpoint->voice_session_vsr_semaphore_get());
    }
    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.configuration = *configuration;
    if(stream_params != NULL) {
       msg.has_stream_params = true;
       msg.stream_params     = *stream_params;
    }
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_session_begin_callback_sdt(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_session_end(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_session_end_cb_sdt_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_session_end_callback_sdt(&msg, sizeof(msg));
    endpoint->voice_obj->voice_status_set(uuid);
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_begin(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_stream_begin_cb_sdt_t msg;
    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_begin_callback_sdt(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_kwd(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_stream_kwd_callback(&data);
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_stream_end(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_stream_end_cb_sdt_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_end_callback_sdt(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_connected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_connected_callback(&data);
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_disconnected(const uuid_t uuid, bool retry, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    ctrlm_voice_disconnected_cb_t data;
    uuid_copy(data.header.uuid, uuid);
    data.retry            = retry;
    data.header.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_disconnected_callback(&data);
}

void ctrlm_voice_endpoint_sdt_t::ctrlm_voice_handler_sdt_server_message(const char *msg, unsigned long length, void *user_data) {
    ctrlm_voice_endpoint_sdt_t *endpoint = (ctrlm_voice_endpoint_sdt_t *)user_data;
    endpoint->voice_obj->server_message(msg, length);
}
