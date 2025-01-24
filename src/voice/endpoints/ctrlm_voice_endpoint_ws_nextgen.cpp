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
#include "ctrlm_voice_endpoint_ws_nextgen.h"
#include "ctrlm_log.h"

#define NEXTGEN_AUDIO_MODEL_PTT  "ptt"
#define NEXTGEN_AUDIO_MODEL_HF   "hf"
#define NEXTGEN_AUDIO_PROFILE_HF "FFV"

// Structures
typedef struct {
    uuid_t                          uuid;
    xrsr_src_t                      src;
    xrsr_session_config_out_t       configuration;
    xrsr_callback_session_config_t  callback;
    bool                            has_stream_params;
    xrsv_ws_nextgen_stream_params_t stream_params;
    rdkx_timestamp_t                timestamp;
} ctrlm_voice_session_begin_cb_ws_nextgen_t;

// Timestamps and stats are not pointers to avoid corruption
typedef struct {
    uuid_t                uuid;
    xrsr_src_t            src;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_stream_begin_cb_ws_nextgen_t;

typedef struct {
    uuid_t               uuid;
    xrsr_stream_stats_t  stats;
    rdkx_timestamp_t     timestamp;
} ctrlm_voice_stream_end_cb_ws_nextgen_t;

typedef struct {
    uuid_t                uuid;
    xrsr_session_stats_t  stats;
    rdkx_timestamp_t      timestamp;
} ctrlm_voice_session_end_cb_ws_nextgen_t;

typedef struct {
    sem_t                       *semaphore;
    const char *                 msg;
    bool *                       result;
} ctrlm_voice_message_send_ws_nextgen_t;
// End Structures

// Static Functions
static const char *controller_name_to_audio_profile(const char *controller);
static const char *controller_name_to_audio_model(const char *controller);
// End Static Functions

// Function Implementations
ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_endpoint_ws_nextgen_t(ctrlm_voice_t *voice_obj) : ctrlm_voice_endpoint_t(voice_obj) {
    this->xrsv_obj_ws_nextgen     = NULL;
    this->voice_message_available = false;
    server_ret_code = 0;  //CID:157976 - Uninit-ctor
}

ctrlm_voice_endpoint_ws_nextgen_t::~ctrlm_voice_endpoint_ws_nextgen_t() {
    if(this->xrsv_obj_ws_nextgen) {
        xrsv_ws_nextgen_destroy(this->xrsv_obj_ws_nextgen);
        this->xrsv_obj_ws_nextgen = NULL;
    }
}

bool ctrlm_voice_endpoint_ws_nextgen_t::open() {
    if(this->voice_obj == NULL) {
        XLOGD_ERROR("Voice object NULL");
        return(false);
    }
    std::string device_id      = this->voice_obj->voice_stb_data_device_id_get();
    std::string partner_id     = this->voice_obj->voice_stb_data_partner_id_get();
    std::string experience     = this->voice_obj->voice_stb_data_experience_get();
    std::string language       = this->voice_obj->voice_stb_data_guide_language_get().c_str();
    std::string account_number = this->voice_obj->voice_stb_data_account_number_get();
#ifdef VOICE_NEXTGEN_MAC
    std::string device_mac     = ctrlm_device_mac_get();
#else
    std::string device_mac     = "";
#endif
    std::string rf_protocol    = "";

    xrsv_ws_nextgen_params_t      params_ws = {
       .device_id                 = (device_id.empty() == false ? device_id.c_str() : NULL),
       .account_id                = (account_number.empty() == false ? account_number.c_str() : NULL),
       .partner_id                = (partner_id.empty() == false ? partner_id.c_str() : NULL),
       .experience                = (experience.empty() == false ? experience.c_str() : NULL),
       .audio_profile             = controller_name_to_audio_profile(""),
       .audio_model               = controller_name_to_audio_model(""),
       .language                  = language.c_str(),
       .device_mac                = (device_mac.empty() == false ? device_mac.c_str() : NULL),
       .rf_protocol               = (rf_protocol.empty() == false ? rf_protocol.c_str() : NULL),
       .test_flag                 = this->voice_obj->voice_stb_data_test_get(),
       .bypass_wuw_verify_success = this->voice_obj->voice_stb_data_bypass_wuw_verify_success_get(),
       .bypass_wuw_verify_failure = this->voice_obj->voice_stb_data_bypass_wuw_verify_failure_get(),
       .mask_pii                  = ctrlm_is_pii_mask_enabled(),
       .user_data                 = (void *)this
   };

   if((this->xrsv_obj_ws_nextgen = xrsv_ws_nextgen_create(&params_ws)) == NULL) {
      XLOGD_ERROR("Failed to open speech vrex WS");
      return(false);
   }

   return(true);
}

bool ctrlm_voice_endpoint_ws_nextgen_t::get_handlers(xrsr_handlers_t *handlers) {
    if(handlers == NULL) {
        XLOGD_ERROR("Handler struct NULL");
        return(false);
    }

    if(this->voice_obj == NULL || this->xrsv_obj_ws_nextgen == NULL) {
        XLOGD_ERROR("Voice obj or Xrsv obj NULL");
        return(false);
    }

    xrsv_ws_nextgen_handlers_t handlers_xrsv = {0};
    errno_t safec_rc = memset_s(handlers, sizeof(xrsr_handlers_t), 0, sizeof(xrsr_handlers_t));
    ERR_CHK(safec_rc);

    // Set up handlers
    handlers_xrsv.session_begin     = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_session_begin;
    handlers_xrsv.session_end       = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_session_end;
    handlers_xrsv.stream_begin      = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_begin;
    handlers_xrsv.stream_kwd        = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_kwd;
    handlers_xrsv.stream_end        = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_end;
    handlers_xrsv.connected         = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_connected;
    handlers_xrsv.disconnected      = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_disconnected;
    handlers_xrsv.sent_init         = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_sent_init;
    handlers_xrsv.listening         = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_listening;
    handlers_xrsv.asr               = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_asr;
    handlers_xrsv.conn_close        = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_conn_close;
    handlers_xrsv.response_vrex     = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_response_vrex;
    handlers_xrsv.wuw_verification  = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_wuw_verification;
    handlers_xrsv.source_error      = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_source_error;
    handlers_xrsv.tv_mute           = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_mute;
    handlers_xrsv.tv_power          = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_power;
    handlers_xrsv.tv_volume         = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_volume;
    handlers_xrsv.msg               = &ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_server_message;

    if(!xrsv_ws_nextgen_handlers(this->xrsv_obj_ws_nextgen, &handlers_xrsv, handlers)) {
        XLOGD_ERROR("failed to get handlers http");
        return(false);
    }
    return(true);
}

bool ctrlm_voice_endpoint_ws_nextgen_t::voice_init_set(const char *blob) const {
    if(this->voice_obj == NULL || this->xrsv_obj_ws_nextgen == NULL) {
        XLOGD_ERROR("not ready for this request");
        return(false);
    }

    return(xrsv_ws_nextgen_update_init_app(this->xrsv_obj_ws_nextgen, blob));
}

bool ctrlm_voice_endpoint_ws_nextgen_t::voice_message(const char *msg) const {
    bool ret = false;
    sem_t semaphore;

    sem_init(&semaphore, 0, 0);

    ctrlm_voice_message_send_ws_nextgen_t dqm = {0};
    dqm.semaphore = &semaphore;
    dqm.msg       = msg;
    dqm.result    = &ret;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_VOICE, (ctrlm_msg_handler_voice_t)&ctrlm_voice_endpoint_ws_nextgen_t::voice_message_send, &dqm, sizeof(dqm), (void *)this);
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return(ret);
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_message_send(void *data, int size) {
    ctrlm_voice_message_send_ws_nextgen_t *dqm = (ctrlm_voice_message_send_ws_nextgen_t *)data;
    if(dqm == NULL || dqm->result == NULL || dqm->msg == NULL) {
        XLOGD_ERROR("Null data");
        return;
    } else if(this->voice_obj == NULL || this->xrsv_obj_ws_nextgen == NULL) {
        XLOGD_ERROR("not ready for this request");
        *dqm->result = false;
    } else {
        if(this->voice_message_available == false) {
            this->voice_message_queue.push_back(dqm->msg);
            *dqm->result = true;
        } else {
            *dqm->result = xrsv_ws_nextgen_send_msg(this->xrsv_obj_ws_nextgen, dqm->msg);
        }
    }
    if(dqm->semaphore) {
        sem_post(dqm->semaphore);
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_account_number_set(std::string &account_number) {
    if(this->xrsv_obj_ws_nextgen) {
       xrsv_ws_nextgen_update_account_id(this->xrsv_obj_ws_nextgen, account_number.c_str());
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_device_id_set(std::string &device_id) {
    if(this->xrsv_obj_ws_nextgen) {
        xrsv_ws_nextgen_update_device_id(this->xrsv_obj_ws_nextgen, device_id.c_str());
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_device_type_set(ctrlm_device_type_t device_type) {
    if(this->xrsv_obj_ws_nextgen) {
        if(device_type == CTRLM_DEVICE_TYPE_TV) {
            xrsv_ws_nextgen_update_device_type(this->xrsv_obj_ws_nextgen, XRSV_WS_NEXTGEN_DEVICE_TYPE_TV);
        } else {
            xrsv_ws_nextgen_update_device_type(this->xrsv_obj_ws_nextgen, XRSV_WS_NEXTGEN_DEVICE_TYPE_STB);
        }
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_partner_id_set(std::string &partner_id) {
    if(this->xrsv_obj_ws_nextgen) {
        xrsv_ws_nextgen_update_partner_id(this->xrsv_obj_ws_nextgen, partner_id.c_str());
    }
}
 
void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_experience_set(std::string &experience) {
    if(this->xrsv_obj_ws_nextgen) {
        xrsv_ws_nextgen_update_experience(this->xrsv_obj_ws_nextgen, experience.c_str());
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_guide_language_set(const char *language) {
   if(this->xrsv_obj_ws_nextgen) {
       xrsv_ws_nextgen_update_language(this->xrsv_obj_ws_nextgen, language);
   }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stb_data_mask_pii_set(bool enable) {
   if(this->xrsv_obj_ws_nextgen) {
       xrsv_ws_nextgen_update_mask_pii(this->xrsv_obj_ws_nextgen, enable);
   }
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_session_begin_callback_ws_nextgen(void *data, int size) {
    ctrlm_voice_session_begin_cb_ws_nextgen_t *dqm = (ctrlm_voice_session_begin_cb_ws_nextgen_t *)data;
    if(NULL == data) {
        XLOGD_ERROR("NULL data");
        return;
    }
    ctrlm_voice_session_info_t info;
    voice_params_par_t params;
    bool keyword_verification = false;
    xrsr_session_config_in_t config_in;
    memset(&config_in, 0, sizeof(config_in));

    this->server_ret_code = 0;
    this->voice_message_available = false; // This is just for sanity
    this->voice_message_queue.clear();

    this->voice_obj->voice_session_info(dqm->src, &info);
    const char *sat = this->voice_obj->voice_stb_data_sat_get();
    this->voice_obj->voice_params_par_get(&params);

    // Source
    config_in.src = dqm->src;

    bool is_mic = ctrlm_voice_xrsr_src_is_mic(dqm->src);
    ctrlm_voice_device_t source = xrsr_to_voice_device(dqm->src);
    bool has_sat  = false;

    if(sat[0] != '\0') {
        config_in.ws.sat_token = sat;
        has_sat = true;
    }

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

    if(is_mic) {
        xrsv_ws_nextgen_update_audio_profile(this->xrsv_obj_ws_nextgen, NEXTGEN_AUDIO_PROFILE_HF);
        xrsv_ws_nextgen_update_audio_model(this->xrsv_obj_ws_nextgen, NEXTGEN_AUDIO_MODEL_HF);
    } else {
        // VREX wanted specific naming for the current XR lineup (possibly for stats?).. we need to send the product name if we don't have a mapping to audio_profile
        const char *audio_profile = controller_name_to_audio_profile(info.controller_name.c_str());
        xrsv_ws_nextgen_update_audio_profile(this->xrsv_obj_ws_nextgen, audio_profile != NULL ? audio_profile : info.controller_name.c_str());
        xrsv_ws_nextgen_update_audio_model(this->xrsv_obj_ws_nextgen, controller_name_to_audio_model(info.controller_name.c_str()));
    }
    xrsv_ws_nextgen_update_audio_rf_protocol(this->xrsv_obj_ws_nextgen, info.rf_protocol.c_str());

    // Handle stream parameters
    if(dqm->has_stream_params) {
       xrsv_ws_nextgen_stream_params_t *stream_params = &dqm->stream_params;
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
          if(stream_params->keyword_sample_begin > info.ffv_leading_samples) {
             uint32_t delta = stream_params->keyword_sample_begin - info.ffv_leading_samples;
             stream_params->keyword_sample_begin -= delta;
             stream_params->keyword_sample_end   -= delta;
          }

          if(is_mic) {
              stream_params->push_to_talk = false;
          } else {
              stream_params->push_to_talk = true;
          }
          if(stream_params->keyword_sample_begin != stream_params->keyword_sample_end) {
              keyword_verification = true;
          }
       }
       stream_params->par_eos_timeout = params.par_voice_enabled ? params.par_voice_eos_timeout : 0;
       config_in.ws.keyword_begin     = stream_params->keyword_sample_begin;
       config_in.ws.keyword_duration  = stream_params->keyword_sample_end - stream_params->keyword_sample_begin;

       xrsv_ws_nextgen_stream_params_t *stream_params_out = (xrsv_ws_nextgen_stream_params_t *)malloc(sizeof(xrsv_ws_nextgen_stream_params_t));

       if(stream_params_out != NULL) {
          *stream_params_out = *stream_params;
       }
       config_in.ws.app_config = stream_params_out;

       XLOGD_TELEMETRY("session begin - src <%s> ptt <%s> w_SAT <%s> w_MTLS <%s> w_OCSPst <%s> w_OCSPca <%s> keyword begin <%u> end <%u> doa <%u> gain <%4.1f> db", ctrlm_voice_device_str(source), (stream_params->push_to_talk ? "TRUE" : "FALSE"), has_sat ? "YES" : "NO", use_mtls ? "YES" : "NO", ocsp_verify_stapling ? "YES" : "NO", ocsp_verify_ca ? "YES" : "NO", stream_params->keyword_sample_begin, stream_params->keyword_sample_end, stream_params->keyword_doa, stream_params->dynamic_gain);
    } else if(!is_mic || dqm->configuration.user_initiated) {
       xrsv_ws_nextgen_stream_params_t *stream_params = (xrsv_ws_nextgen_stream_params_t *)malloc(sizeof(xrsv_ws_nextgen_stream_params_t));

       if(stream_params != NULL) {
          errno_t safec_rc = memset_s(stream_params, sizeof(*stream_params), 0, sizeof(*stream_params));
          ERR_CHK(safec_rc);

          stream_params->push_to_talk = true;
          stream_params->par_eos_timeout = params.par_voice_enabled ? params.par_voice_eos_timeout : 0;
       }
       config_in.ws.app_config = stream_params;

       XLOGD_TELEMETRY("session begin - src <%s> ptt <TRUE> w_SAT <%s> w_MTLS <%s> w_OCSPst <%s> w_OCSPca <%s>", ctrlm_voice_device_str(source), has_sat ? "YES" : "NO", use_mtls ? "YES" : "NO", ocsp_verify_stapling ? "YES" : "NO", ocsp_verify_ca ? "YES" : "NO");
    } else {
       XLOGD_ERROR("session begin - invalid params - src <%s>", ctrlm_voice_device_str(source));
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

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stream_begin_callback_ws_nextgen(void *data, int size) {
    ctrlm_voice_stream_begin_cb_ws_nextgen_t *dqm = (ctrlm_voice_stream_begin_cb_ws_nextgen_t *)data;
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

    // The init message is sent before this message is called, now we can send messages
    for(const auto &itr : this->voice_message_queue) {
        bool res = xrsv_ws_nextgen_send_msg(this->xrsv_obj_ws_nextgen, itr.c_str());
        XLOGD_INFO("Sent queued up voiceMessage <%s>",  res ? "SUCCESS" : "FAIL");
    }
    this->voice_message_queue.clear();
    this->voice_message_available = true;
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_stream_end_callback_ws_nextgen(void *data, int size) {
    ctrlm_voice_stream_end_cb_ws_nextgen_t *dqm = (ctrlm_voice_stream_end_cb_ws_nextgen_t *)data;
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

void ctrlm_voice_endpoint_ws_nextgen_t::voice_session_end_callback_ws_nextgen(void *data, int size) {
    ctrlm_voice_session_end_cb_ws_nextgen_t *dqm = (ctrlm_voice_session_end_cb_ws_nextgen_t *)data;
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
    if((stats->reason != XRSR_SESSION_END_REASON_EOS && stats->reason != XRSR_SESSION_END_REASON_EOT) || (this->server_ret_code != 0 && this->server_ret_code != 200)) {
        success = false;
    }

    // Check for SAT errors
    if(this->server_ret_code == 116 || this->server_ret_code == 117 || this->server_ret_code == 118 ||
       this->server_ret_code == 119 || this->server_ret_code == 120 || this->server_ret_code == 121 ||
       this->server_ret_code == 123) {
        XLOGD_INFO("SAT Error Code from server <%ld>", this->server_ret_code);
        ctrlm_main_invalidate_service_access_token();
    }

    // voiceMessage reset
    this->voice_message_available = false;

    ctrlm_voice_session_end_cb_t session_end;
    uuid_copy(session_end.header.uuid, dqm->uuid);
    uuid_clear(this->uuid);
    session_end.header.timestamp = dqm->timestamp;
    session_end.success          = success;
    session_end.stats            = dqm->stats;
    this->voice_obj->voice_session_end_callback(&session_end);
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_session_recv_msg_ws_nextgen(const char *transcription) {
    this->voice_obj->voice_session_transcription_callback(this->uuid, transcription);
}

void ctrlm_voice_endpoint_ws_nextgen_t::voice_session_server_return_code_ws_nextgen(const char *reason, long ret_code) {
    this->server_ret_code = ret_code;
    this->voice_obj->voice_server_return_code_callback(this->uuid, reason, ret_code);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_session_begin(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, xrsv_ws_nextgen_stream_params_t *stream_params, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_session_begin_cb_ws_nextgen_t msg = {0};

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
    endpoint->voice_session_begin_callback_ws_nextgen(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_session_end(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_session_end_cb_ws_nextgen_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);

    endpoint->voice_session_end_callback_ws_nextgen(&msg, sizeof(msg));
    endpoint->voice_obj->voice_status_set(uuid);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_begin(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_stream_begin_cb_ws_nextgen_t msg;
    uuid_copy(msg.uuid, uuid);
    msg.src           = src;
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_begin_callback_ws_nextgen(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_kwd(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_stream_kwd_callback(&data);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_stream_end(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_stream_end_cb_ws_nextgen_t msg;
    uuid_copy(msg.uuid, uuid);
    SET_IF_VALID(msg.stats, stats);
    msg.timestamp     = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_stream_end_callback_ws_nextgen(&msg, sizeof(msg));
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_connected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_connected_callback(&data);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_disconnected(const uuid_t uuid, bool retry, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_disconnected_cb_t data;
    uuid_copy(data.header.uuid, uuid);
    data.retry            = retry;
    data.header.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_disconnected_callback(&data);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_sent_init(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    ctrlm_voice_cb_header_t data;
    uuid_copy(data.uuid, uuid);
    data.timestamp = ctrlm_voice_endpoint_t::valid_timestamp_get(timestamp);
    endpoint->voice_obj->voice_server_sent_init_callback(&data);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_listening(void *user_data) {
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_asr(const char *str_transcription, bool final, void *user_data) {
    if(final) {
        ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
        endpoint->voice_session_recv_msg_ws_nextgen(str_transcription);
    }
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_conn_close(const char *reason, long ret_code, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    endpoint->voice_session_server_return_code_ws_nextgen(reason, ret_code);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_response_vrex(long ret_code, void *user_data) {
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_wuw_verification(const uuid_t uuid, bool passed, long confidence, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    rdkx_timestamp_t timestamp;
    rdkx_timestamp_get_realtime(&timestamp);
    endpoint->voice_obj->voice_action_keyword_verification_callback(uuid, passed, timestamp);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_source_error(xrsr_src_t src, void *user_data) {
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_mute(bool mute, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    endpoint->voice_obj->voice_action_tv_mute_callback(mute);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_power(bool power, bool toggle, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    endpoint->voice_obj->voice_action_tv_power_callback(power, toggle);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_tv_volume(bool up, uint32_t repeat_count, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    endpoint->voice_obj->voice_action_tv_volume_callback(up, repeat_count);
}

void ctrlm_voice_endpoint_ws_nextgen_t::ctrlm_voice_handler_ws_nextgen_server_message(const char *msg, unsigned long length, void *user_data) {
    ctrlm_voice_endpoint_ws_nextgen_t *endpoint = (ctrlm_voice_endpoint_ws_nextgen_t *)user_data;
    endpoint->voice_obj->server_message(msg, length);
}

// End Function Implementations

// Static Helper Functions
const char *controller_name_to_audio_model(const char *controller) {
    return(NEXTGEN_AUDIO_MODEL_PTT);
}
const char *controller_name_to_audio_profile(const char *controller) {
    if(!strncmp(controller, "XR11-", 5)) {
        return("XR11");
    } else if(!strncmp(controller, "XR15-1", 6)) {
        return("XR15");
    } else if(!strncmp(controller, "XR15-2", 6)) {
        return("XR15V2");
    } else if(!strncmp(controller, "XR16-", 5)) {
        return("XR16");
    } else if(!strncmp(controller, "XRA-", 4)) {
        return("XRA");
    } else if(strlen(controller) == 0) {
        return("UNKNOWN");
    }
    return(NULL);
}
// End Static Helper Functions
