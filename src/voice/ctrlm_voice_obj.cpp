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
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include "include/ctrlm_ipc.h"
#include "include/ctrlm_ipc_voice.h"
#include "ctrlm_voice_obj.h"
#include "ctrlm.h"
#include "ctrlm_config_default.h"
#include "ctrlm_log.h"
#include "ctrlm_tr181.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"
#include "ctrlm_shared_memory.h"
#include "ctrlm_network.h"
#include "jansson.h"
#include "xrsr.h"
#include "xrsv_http.h"
#include "json_config.h"
#include "ctrlm_voice_ipc_iarm_all.h"
#include "ctrlm_voice_endpoint.h"

#define MIN_VAL(x, y) ((x) < (y) ? (x) : (y)) // MIN is already defined, but in GLIB.. Eventually we need to get rid of this dependency

#define PIPE_READ  (0)
#define PIPE_WRITE (1)

#define CTRLM_VOICE_MIMETYPE_ADPCM  "audio/x-adpcm"
#define CTRLM_VOICE_SUBTYPE_ADPCM   "ADPCM"
#define CTRLM_VOICE_LANGUAGE        "eng"

#define CTRLM_CONTROLLER_CMD_STATUS_READ_TIMEOUT (10000)
#define CTRLM_VOICE_KEYWORD_BEEP_TIMEOUT         ( 1500)

#define ADPCM_COMMAND_ID_MIN             (0x20)    ///< Minimum bound of command id as defined by XVP Spec.
#define ADPCM_COMMAND_ID_MAX             (0x3F)    ///< Maximum bound of command id as defined by XVP Spec.

static void ctrlm_voice_session_response_confirm(bool result, signed long long rsp_time, unsigned int rsp_window, const std::string &err_str, ctrlm_timestamp_t *timestamp, void *user_data);
static void ctrlm_voice_data_post_processing_cb(int bytes_sent, void *user_data);

static ctrlm_voice_session_group_t voice_device_to_session_group(ctrlm_voice_device_t device_type);

#ifdef BEEP_ON_KWD_ENABLED
static void ctrlm_voice_system_audio_player_event_handler(system_audio_player_event_t event, void *user_data);
#endif

static xrsr_power_mode_t voice_xrsr_power_map(ctrlm_power_state_t ctrlm_power_state);

#ifdef VOICE_BUFFER_STATS
static unsigned long voice_packet_interval_get(ctrlm_voice_format_t format, uint32_t opus_samples_per_packet);
#endif

// Application Interface Implementation
ctrlm_voice_t::ctrlm_voice_t() {
    XLOGD_INFO("Constructor");

    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        ctrlm_voice_session_t *session = &this->voice_session[group];
        session->network_id         = CTRLM_MAIN_NETWORK_ID_INVALID;
        session->network_type       = CTRLM_NETWORK_TYPE_INVALID;
        session->controller_id      = CTRLM_MAIN_CONTROLLER_ID_INVALID;
        session->voice_device       = CTRLM_VOICE_DEVICE_INVALID;
        session->format.type        = CTRLM_VOICE_FORMAT_INVALID;
        session->server_ret_code    = 0;

        errno_t safec_rc = memset_s(&session->stream_params, sizeof(session->stream_params), 0, sizeof(session->stream_params));
        ERR_CHK(safec_rc);

        session->session_active_server     = false;
        session->session_active_controller = false;
        session->state_src                 = CTRLM_VOICE_STATE_SRC_INVALID;
        session->state_dst                 = CTRLM_VOICE_STATE_DST_INVALID;
        session->end_reason                = CTRLM_VOICE_SESSION_END_REASON_DONE;

        session->audio_pipe[PIPE_READ]     = -1;
        session->audio_pipe[PIPE_WRITE]    = -1;
        session->audio_sent_bytes          =  0;
        session->audio_sent_samples        =  0;
        session->packets_processed         = 0;
        session->packets_lost              = 0;

        session->is_session_by_text        = false;
        session->transcription_in          = "";
        session->transcription             = "";

        session->is_session_by_file        = false;
        session->is_session_by_fifo        = false;

        session->keyword_verified          = false;

        session->endpoint_current          = NULL;
        session->confidence                = .0;

        safec_rc = memset_s(&session->session_timing, sizeof(session->session_timing), 0, sizeof(session->session_timing));
        ERR_CHK(safec_rc);

        safec_rc = memset_s(&session->status, sizeof(session->status), 0, sizeof(session->status));
        ERR_CHK(safec_rc);

        // VSRsp Error Tracking
        session->current_vsr_err_rsp_time        = 0;
        session->current_vsr_err_rsp_window      = 0;
        session->current_vsr_err_string          = "";
        sem_init(&session->current_vsr_err_semaphore, 0, 1);

        session->timeout_ctrl_cmd_status_read    =  0;
        session->timeout_packet_tag              =  0;

        session->last_cmd_id                     = 0;
        session->next_cmd_id                     = 0;

        voice_session_info_reset(session);
    }
    this->timeout_ctrl_session_stats_rxd  =  0;
    this->timeout_keyword_beep            =  0;
    this->session_id                      =  0;
    this->software_version                = "N/A";
    this->mask_pii                        = ctrlm_is_production_build() ? JSON_ARRAY_VAL_BOOL_CTRLM_GLOBAL_MASK_PII_0 : JSON_ARRAY_VAL_BOOL_CTRLM_GLOBAL_MASK_PII_1;
    this->ocsp_verify_stapling            = false;
    this->ocsp_verify_ca                  = false;
    this->capture_active                  = false;
    this->device_cert.type                = CTRLM_VOICE_CERT_TYPE_NONE;
    this->prefs.server_url_src_ptt        = JSON_STR_VALUE_VOICE_URL_SRC_PTT;
    this->prefs.server_url_src_ff         = JSON_STR_VALUE_VOICE_URL_SRC_FF;
    #ifdef CTRLM_LOCAL_MIC_TAP
    this->prefs.server_url_src_mic_tap    = JSON_STR_VALUE_VOICE_URL_SRC_MIC_TAP;
    #endif
    #ifdef JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_0
    this->url_hostname_pattern_add(JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_0);
    #endif
    #ifdef JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_1
    this->url_hostname_pattern_add(JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_1);
    #endif
    #ifdef JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_2
    this->url_hostname_pattern_add(JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_2);
    #endif
    #ifdef JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_3
    this->url_hostname_pattern_add(JSON_ARRAY_VAL_STR_VOICE_SERVER_HOSTS_3);
    #endif
    this->prefs.aspect_ratio                 = JSON_STR_VALUE_VOICE_ASPECT_RATIO;
    this->prefs.guide_language               = JSON_STR_VALUE_VOICE_LANGUAGE;
    this->prefs.app_id_http                  = JSON_STR_VALUE_VOICE_APP_ID_HTTP;
    this->prefs.app_id_ws                    = JSON_STR_VALUE_VOICE_APP_ID_WS;
    this->prefs.timeout_vrex_connect         = JSON_INT_VALUE_VOICE_VREX_REQUEST_TIMEOUT;
    this->prefs.timeout_vrex_session         = JSON_INT_VALUE_VOICE_VREX_RESPONSE_TIMEOUT;
    this->prefs.timeout_stats                = JSON_INT_VALUE_VOICE_TIMEOUT_STATS;
    this->prefs.timeout_packet_initial       = JSON_INT_VALUE_VOICE_TIMEOUT_PACKET_INITIAL;
    this->prefs.timeout_packet_subsequent    = JSON_INT_VALUE_VOICE_TIMEOUT_PACKET_SUBSEQUENT;
    this->prefs.bitrate_minimum              = JSON_INT_VALUE_VOICE_BITRATE_MINIMUM;
    this->prefs.time_threshold               = JSON_INT_VALUE_VOICE_TIME_THRESHOLD;
    this->prefs.utterance_save               = ctrlm_is_production_build() ? JSON_ARRAY_VAL_BOOL_VOICE_SAVE_LAST_UTTERANCE_0 : JSON_ARRAY_VAL_BOOL_VOICE_SAVE_LAST_UTTERANCE_1;
    this->prefs.utterance_use_curtail        = JSON_BOOL_VALUE_VOICE_UTTERANCE_USE_CURTAIL;
    this->prefs.utterance_file_qty_max       = JSON_INT_VALUE_VOICE_UTTERANCE_FILE_QTY_MAX;
    this->prefs.utterance_file_size_max      = JSON_INT_VALUE_VOICE_UTTERANCE_FILE_SIZE_MAX;
    this->prefs.utterance_path               = JSON_STR_VALUE_VOICE_UTTERANCE_PATH;
    this->prefs.utterance_duration_min       = JSON_INT_VALUE_VOICE_MINIMUM_DURATION;
    this->prefs.ffv_leading_samples          = JSON_INT_VALUE_VOICE_FFV_LEADING_SAMPLES;
    this->prefs.force_voice_settings         = JSON_BOOL_VALUE_VOICE_FORCE_VOICE_SETTINGS;
    this->prefs.keyword_sensitivity          = JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY;
    this->prefs.vrex_test_flag               = JSON_BOOL_VALUE_VOICE_VREX_TEST_FLAG;
    this->prefs.vrex_wuw_bypass_success_flag = JSON_BOOL_VALUE_VOICE_VREX_WUW_BYPASS_SUCCESS_FLAG;
    this->prefs.vrex_wuw_bypass_failure_flag = JSON_BOOL_VALUE_VOICE_VREX_WUW_BYPASS_FAILURE_FLAG;
    this->prefs.force_toggle_fallback        = JSON_BOOL_VALUE_VOICE_FORCE_TOGGLE_FALLBACK;
    this->prefs.par_voice_enabled            = false;
    this->prefs.par_voice_eos_method         = JSON_INT_VALUE_VOICE_PAR_VOICE_EOS_METHOD;
    this->prefs.par_voice_eos_timeout        = JSON_INT_VALUE_VOICE_PAR_VOICE_EOS_TIMEOUT;
    this->voice_params_opus_encoder_default();
    this->xrsr_opened                        = false;
    this->voice_ipc                          = NULL;
    this->packet_loss_threshold              = JSON_INT_VALUE_VOICE_PACKET_LOSS_THRESHOLD;
    this->vsdk_config                        = NULL;
    this->nsm_voice_session                  = false;

    #ifndef TELEMETRY_SUPPORT
    XLOGD_WARN("telemetry is not enabled");
    #else
    ctrlm_telemetry_t *telemetry = ctrlm_get_telemetry_obj();
    if(telemetry) {
        telemetry->add_listener(ctrlm_telemetry_report_t::VOICE, std::bind(&ctrlm_voice_t::telemetry_report_handler, this));
    }
    #endif

    #ifdef DEEP_SLEEP_ENABLED
    this->prefs.dst_params_standby.connect_check_interval = JSON_INT_VALUE_VOICE_DST_PARAMS_STANDBY_CONNECT_CHECK_INTERVAL;
    this->prefs.dst_params_standby.timeout_connect        = JSON_INT_VALUE_VOICE_DST_PARAMS_STANDBY_TIMEOUT_CONNECT;
    this->prefs.dst_params_standby.timeout_inactivity     = JSON_INT_VALUE_VOICE_DST_PARAMS_STANDBY_TIMEOUT_INACTIVITY;
    this->prefs.dst_params_standby.timeout_session        = JSON_INT_VALUE_VOICE_DST_PARAMS_STANDBY_TIMEOUT_SESSION;
    this->prefs.dst_params_standby.ipv4_fallback          = JSON_BOOL_VALUE_VOICE_DST_PARAMS_STANDBY_IPV4_FALLBACK;
    this->prefs.dst_params_standby.backoff_delay          = JSON_INT_VALUE_VOICE_DST_PARAMS_STANDBY_BACKOFF_DELAY;
    #endif

    this->prefs.dst_params_low_latency.connect_check_interval = JSON_INT_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_CONNECT_CHECK_INTERVAL;
    this->prefs.dst_params_low_latency.timeout_connect        = JSON_INT_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_CONNECT;
    this->prefs.dst_params_low_latency.timeout_inactivity     = JSON_INT_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_INACTIVITY;
    this->prefs.dst_params_low_latency.timeout_session        = JSON_INT_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_SESSION;
    this->prefs.dst_params_low_latency.ipv4_fallback          = JSON_BOOL_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_IPV4_FALLBACK;
    this->prefs.dst_params_low_latency.backoff_delay          = JSON_INT_VALUE_VOICE_DST_PARAMS_LOW_LATENCY_BACKOFF_DELAY;

    // Device Status initialization
    sem_init(&this->device_status_semaphore, 0, 1);
    this->device_status[CTRLM_VOICE_DEVICE_PTT]            = CTRLM_VOICE_DEVICE_STATUS_NONE;
    this->device_requires_stb_data[CTRLM_VOICE_DEVICE_PTT] = true;
    this->device_status[CTRLM_VOICE_DEVICE_FF]             = CTRLM_VOICE_DEVICE_STATUS_NONE;
    this->device_requires_stb_data[CTRLM_VOICE_DEVICE_FF]  = true;
#ifdef CTRLM_LOCAL_MIC
    this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE]            = CTRLM_VOICE_DEVICE_STATUS_NONE;
    this->device_requires_stb_data[CTRLM_VOICE_DEVICE_MICROPHONE] = true;
#endif
#ifdef CTRLM_LOCAL_MIC_TAP
    this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE_TAP]            = CTRLM_VOICE_DEVICE_STATUS_NONE;
    this->device_requires_stb_data[CTRLM_VOICE_DEVICE_MICROPHONE_TAP] = true;
#endif
    this->device_status[CTRLM_VOICE_DEVICE_INVALID]             = CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED;
    this->device_requires_stb_data[CTRLM_VOICE_DEVICE_INVALID]  = true;

    this->sat_token_required        = JSON_BOOL_VALUE_VOICE_ENABLE_SAT;
    this->mtls_required             = JSON_BOOL_VALUE_VOICE_ENABLE_MTLS;
    this->secure_url_required       = JSON_BOOL_VALUE_VOICE_REQUIRE_SECURE_URL;

    XLOGD_TELEMETRY("require i_SAT <%s> i_MTLS <%s> i_secure_url <%s>", this->sat_token_required ? "YES" : "NO", this->mtls_required ? "YES" : "NO", this->secure_url_required ? "YES" : "NO");

    errno_t safec_rc = memset_s(this->sat_token, sizeof(this->sat_token), 0, sizeof(this->sat_token));
    ERR_CHK(safec_rc);
    // These semaphores are used to make sure we have all the data before calling the session begin callback
    sem_init(&this->vsr_semaphore, 0, 0);

    #ifdef BEEP_ON_KWD_ENABLED
    this->obj_sap = Thunder::SystemAudioPlayer::ctrlm_thunder_plugin_system_audio_player_t::getInstance();
    this->obj_sap->add_event_handler(ctrlm_voice_system_audio_player_event_handler, this);
    this->sap_opened = this->obj_sap->open(SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_WAV, SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_FILE, SYSTEM_AUDIO_PLAYER_PLAY_MODE_SYSTEM);
    if(!this->sap_opened) {
       XLOGD_WARN("unable to open system audio player");
    }
    #endif

    // Set audio mode to default
    ctrlm_voice_audio_settings_t settings = CTRLM_VOICE_AUDIO_SETTINGS_INITIALIZER;
    this->set_audio_mode(&settings);

    // Setup RFC callbacks
    ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
    if(rfc) {
        rfc->add_changed_listener(ctrlm_rfc_t::attrs::VOICE, std::bind(&ctrlm_voice_t::voice_rfc_retrieved_handler, this, std::placeholders::_1));
        rfc->add_changed_listener(ctrlm_rfc_t::attrs::VSDK, std::bind(&ctrlm_voice_t::vsdk_rfc_retrieved_handler, this, std::placeholders::_1));
    }
}

ctrlm_voice_t::~ctrlm_voice_t() {
    XLOGD_INFO("Destructor");

    voice_sdk_close();
    if(this->voice_ipc) {
        this->voice_ipc->deregister_ipc();
        delete this->voice_ipc;
        this->voice_ipc = NULL;
    }
    if(this->vsdk_config) {
        json_decref(this->vsdk_config);
        this->vsdk_config = NULL;
    }
    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        ctrlm_voice_session_t *session = &this->voice_session[group];
        if(session->audio_pipe[PIPE_READ] >= 0) {
            close(session->audio_pipe[PIPE_READ]);
            session->audio_pipe[PIPE_READ] = -1;
        }
        if(session->audio_pipe[PIPE_WRITE] >= 0) {
            close(session->audio_pipe[PIPE_WRITE]);
            session->audio_pipe[PIPE_WRITE] = -1;
        }
    }

    #ifdef BEEP_ON_KWD_ENABLED
    if(this->sap_opened) {
        if(!this->obj_sap->close()) {
            XLOGD_WARN("unable to close system audio player");
        }
        this->sap_opened = false;
    }
    #endif

    /* Close Voice SDK */

    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        ctrlm_voice_session_t *session = &this->voice_session[group];
        sem_destroy(&session->current_vsr_err_semaphore);
    }
}

bool ctrlm_voice_t::vsdk_is_privacy_enabled(void) {
   bool privacy = true;

   if(!xrsr_privacy_mode_get(&privacy)) {
      XLOGD_ERROR("error getting privacy mode, defaulting to ON");
      privacy = true;
   }

   return privacy;
}

double ctrlm_voice_t::vsdk_keyword_sensitivity_limit_check(double sensitivity) {
    float sensitivity_min;
    float sensitivity_max;

    if(!xrsr_keyword_sensitivity_limits_get(&sensitivity_min, &sensitivity_max)) {
        XLOGD_WARN("Unable to get keyword detector sensitivity limits. Using default sensitivity <%f>.", JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY);
        return(JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY);
    } else {
        if(((float)(sensitivity) < sensitivity_min) || ((float)(sensitivity) > sensitivity_max)) {
            XLOGD_WARN("Keyword detector sensitivity <%f> outside of range <%f to %f>. Using default sensitivity <%f>.", (float)(sensitivity), sensitivity_min, sensitivity_max, JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY);
            return(JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY);
        }
    }
    return(sensitivity);
}

void ctrlm_voice_t::voice_sdk_open(json_t *json_obj_vsdk) {
   if(this->xrsr_opened) {
      XLOGD_ERROR("already open");
      return;
   }
   xrsr_route_t          routes[1];
   xrsr_keyword_config_t kw_config;
   xrsr_capture_config_t capture_config = {
         .delete_files  = !this->prefs.utterance_save,
         .enable        = this->prefs.utterance_save,
         .use_curtail   = this->prefs.utterance_use_curtail,
         .file_qty_max  = (uint32_t)this->prefs.utterance_file_qty_max,
         .file_size_max = (uint32_t)this->prefs.utterance_file_size_max,
         .dir_path      = this->prefs.utterance_path.c_str()
   };

   int ind = -1;
   errno_t safec_rc = strcmp_s(JSON_STR_VALUE_VOICE_UTTERANCE_PATH, strlen(JSON_STR_VALUE_VOICE_UTTERANCE_PATH), capture_config.dir_path, &ind);
   ERR_CHK(safec_rc);
   if((safec_rc == EOK) && (!ind)) {
      capture_config.dir_path = "/opt/logs"; // Default value specifies a file, but now it needs to be a directory
   }

   /* Open Voice SDK */
   routes[0].src                  = XRSR_SRC_INVALID;
   routes[0].dst_qty              = 0;
   kw_config.sensitivity          = this->prefs.keyword_sensitivity;

   char host_name[HOST_NAME_MAX];
   host_name[0] = '\0';

   int rc = gethostname(host_name, sizeof(host_name));
   if(rc != 0) {
       int errsv = errno;
       XLOGD_ERROR("Failed to get host name <%s>", strerror(errsv));
   }

   //HAL is not available to query mute state because xrsr is not open, so use stored status.
   bool privacy = this->voice_is_privacy_enabled();
   ctrlm_power_state_t ctrlm_power_state = ctrlm_main_get_power_state();
   xrsr_power_mode_t   xrsr_power_mode   = voice_xrsr_power_map(ctrlm_power_state);

   if(!xrsr_open(host_name, routes, &kw_config, &capture_config, xrsr_power_mode, privacy, this->mask_pii, json_obj_vsdk)) {
      XLOGD_ERROR("Failed to open speech router");
      g_assert(0);
   }

   this->xrsr_opened = true;

   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      session->state_src = CTRLM_VOICE_STATE_SRC_READY;
      session->state_dst = CTRLM_VOICE_STATE_DST_READY;
   }
}

void ctrlm_voice_t::voice_sdk_close() {
    if(this->xrsr_opened) {
       xrsr_close();
       this->xrsr_opened = false;
    }
}

bool ctrlm_voice_t::voice_configure_config_file_json(json_t *obj_voice, json_t *json_obj_vsdk, bool local_conf) {
    json_config                       conf;
    ctrlm_voice_iarm_call_settings_t *voice_settings     = NULL;
    uint32_t                          voice_settings_len = 0;
    std::string                       init;

    XLOGD_INFO("Configuring voice");
    ctrlm_voice_audio_settings_t audio_settings = {this->audio_mode, this->audio_timing, this->audio_confidence_threshold, this->audio_ducking_type, this->audio_ducking_level, this->audio_ducking_beep_enabled};

    if (conf.config_object_set(obj_voice)){
        bool enabled = true;

        conf.config_value_get(JSON_BOOL_NAME_VOICE_FORCE_VOICE_SETTINGS,        this->prefs.force_voice_settings);

        if(!this->prefs.force_voice_settings) {
            XLOGD_INFO("Ignoring application configurable voice settings from JSON due to force_voice_settings being set to FALSE");
       } else {
            //Force voice settings that the application might set
            XLOGD_INFO("Getting voice setings from JSON due to force_voice_settings being set to TRUE");
            conf.config_value_get(JSON_BOOL_NAME_VOICE_ENABLE,                      enabled);
            conf.config_value_get(JSON_STR_NAME_VOICE_URL_SRC_PTT,                  this->prefs.server_url_src_ptt);
            conf.config_value_get(JSON_STR_NAME_VOICE_URL_SRC_FF,                   this->prefs.server_url_src_ff);
            #ifdef CTRLM_LOCAL_MIC_TAP
            conf.config_value_get(JSON_STR_NAME_VOICE_URL_SRC_MIC_TAP,              this->prefs.server_url_src_mic_tap);
            #endif
            conf.config_value_get(JSON_STR_NAME_VOICE_LANGUAGE,                     this->prefs.guide_language);
            conf.config_value_get(JSON_INT_NAME_VOICE_MINIMUM_DURATION,             this->prefs.utterance_duration_min);
            if(conf.config_value_get(JSON_BOOL_NAME_VOICE_ENABLE_SAT,                  this->sat_token_required)) {
                ctrlm_sm_voice_sat_enable_write(this->sat_token_required);
                XLOGD_TELEMETRY("require c_SAT <%s>", this->sat_token_required ? "YES" : "NO");
            }
            conf.config_value_get(JSON_STR_NAME_VOICE_ASPECT_RATIO,                 this->prefs.aspect_ratio);
        }

        std::vector<std::string> obj_server_hosts;
        if(conf.config_value_get(JSON_ARRAY_NAME_VOICE_SERVER_HOSTS, obj_server_hosts)) {
           this->url_hostname_patterns(obj_server_hosts);
        }

        conf.config_value_get(JSON_INT_NAME_VOICE_VREX_REQUEST_TIMEOUT,         this->prefs.timeout_vrex_connect,0);
        conf.config_value_get(JSON_INT_NAME_VOICE_VREX_RESPONSE_TIMEOUT,        this->prefs.timeout_vrex_session,0);
        conf.config_value_get(JSON_INT_NAME_VOICE_TIMEOUT_STATS,                this->prefs.timeout_stats);
        conf.config_value_get(JSON_INT_NAME_VOICE_TIMEOUT_PACKET_INITIAL,       this->prefs.timeout_packet_initial);
        conf.config_value_get(JSON_INT_NAME_VOICE_TIMEOUT_PACKET_SUBSEQUENT,    this->prefs.timeout_packet_subsequent);
        conf.config_value_get(JSON_INT_NAME_VOICE_BITRATE_MINIMUM,              this->prefs.bitrate_minimum);
        conf.config_value_get(JSON_INT_NAME_VOICE_TIME_THRESHOLD,               this->prefs.time_threshold);
        if(conf.config_value_get(JSON_BOOL_NAME_VOICE_ENABLE_MTLS,              this->mtls_required)) {
            ctrlm_sm_voice_mtls_enable_write(this->mtls_required);
            XLOGD_TELEMETRY("require c_MTLS <%s>", this->mtls_required ? "YES" : "NO");
        }
        if(conf.config_value_get(JSON_BOOL_NAME_VOICE_REQUIRE_SECURE_URL,       this->secure_url_required)) {
            ctrlm_sm_voice_secure_url_required_write(this->secure_url_required);
            XLOGD_TELEMETRY("require c_secure_url <%s>", this->secure_url_required ? "YES" : "NO");
        }
        conf.config_value_get(JSON_ARRAY_NAME_VOICE_SAVE_LAST_UTTERANCE,        this->prefs.utterance_save, ctrlm_is_production_build() ? CTRLM_JSON_ARRAY_INDEX_PRD : CTRLM_JSON_ARRAY_INDEX_DEV);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_UTTERANCE_USE_CURTAIL,       this->prefs.utterance_use_curtail);
        conf.config_value_get(JSON_INT_NAME_VOICE_UTTERANCE_FILE_QTY_MAX,       this->prefs.utterance_file_qty_max, 1, 100000);
        conf.config_value_get(JSON_INT_NAME_VOICE_UTTERANCE_FILE_SIZE_MAX,      this->prefs.utterance_file_size_max, 4 * 1024);
        conf.config_value_get(JSON_STR_NAME_VOICE_UTTERANCE_PATH,               this->prefs.utterance_path);
        conf.config_value_get(JSON_INT_NAME_VOICE_FFV_LEADING_SAMPLES,          this->prefs.ffv_leading_samples, 0);
        conf.config_value_get(JSON_STR_NAME_VOICE_APP_ID_HTTP,                  this->prefs.app_id_http);
        conf.config_value_get(JSON_STR_NAME_VOICE_APP_ID_WS,                    this->prefs.app_id_ws);
        conf.config_value_get(JSON_FLOAT_NAME_VOICE_KEYWORD_DETECT_SENSITIVITY, this->prefs.keyword_sensitivity, 0.0, DBL_MAX);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_VREX_TEST_FLAG,              this->prefs.vrex_test_flag);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_VREX_WUW_BYPASS_SUCCESS_FLAG,this->prefs.vrex_wuw_bypass_success_flag);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_VREX_WUW_BYPASS_FAILURE_FLAG,this->prefs.vrex_wuw_bypass_failure_flag);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_FORCE_TOGGLE_FALLBACK,       this->prefs.force_toggle_fallback);

        std::string opus_encoder_params_str;
        conf.config_value_get(JSON_STR_NAME_VOICE_OPUS_ENCODER_PARAMS,          opus_encoder_params_str);
        this->voice_params_opus_encoder_validate(opus_encoder_params_str);

        conf.config_value_get(JSON_INT_NAME_VOICE_PAR_VOICE_EOS_METHOD,         this->prefs.par_voice_eos_method);
        conf.config_value_get(JSON_INT_NAME_VOICE_PAR_VOICE_EOS_TIMEOUT,        this->prefs.par_voice_eos_timeout);
        conf.config_value_get(JSON_INT_NAME_VOICE_PACKET_LOSS_THRESHOLD,        this->packet_loss_threshold, 0);
        conf.config_value_get(JSON_INT_NAME_VOICE_AUDIO_MODE,                   audio_settings.mode);
        conf.config_value_get(JSON_INT_NAME_VOICE_AUDIO_TIMING,                 audio_settings.timing);
        conf.config_value_get(JSON_FLOAT_NAME_VOICE_AUDIO_CONFIDENCE_THRESHOLD, audio_settings.confidence_threshold, 0.0, 1.0);
        conf.config_value_get(JSON_INT_NAME_VOICE_AUDIO_DUCKING_TYPE,           audio_settings.ducking_type, CTRLM_VOICE_AUDIO_DUCKING_TYPE_ABSOLUTE, CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE);
        conf.config_value_get(JSON_FLOAT_NAME_VOICE_AUDIO_DUCKING_LEVEL,        audio_settings.ducking_level, 0.0, 1.0);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_AUDIO_DUCKING_BEEP,          audio_settings.ducking_beep);

        #ifdef DEEP_SLEEP_ENABLED
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_CONNECT_CHECK_INTERVAL, this->prefs.dst_params_standby.connect_check_interval);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_CONNECT,        this->prefs.dst_params_standby.timeout_connect);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_INACTIVITY,     this->prefs.dst_params_standby.timeout_inactivity);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_SESSION,        this->prefs.dst_params_standby.timeout_session);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_DST_PARAMS_STANDBY_IPV4_FALLBACK,         this->prefs.dst_params_standby.ipv4_fallback);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_BACKOFF_DELAY,          this->prefs.dst_params_standby.backoff_delay);
        #endif

        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_CONNECT_CHECK_INTERVAL, this->prefs.dst_params_low_latency.connect_check_interval);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_CONNECT,        this->prefs.dst_params_low_latency.timeout_connect);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_INACTIVITY,     this->prefs.dst_params_low_latency.timeout_inactivity);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_SESSION,        this->prefs.dst_params_low_latency.timeout_session);
        conf.config_value_get(JSON_BOOL_NAME_VOICE_DST_PARAMS_LOW_LATENCY_IPV4_FALLBACK,         this->prefs.dst_params_low_latency.ipv4_fallback);
        conf.config_value_get(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_BACKOFF_DELAY,          this->prefs.dst_params_low_latency.backoff_delay);

        // Check if enabled
        if(!enabled) {
            for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
                this->voice_device_disable((ctrlm_voice_device_t)i, false, NULL);
            }
        }
    }

    // Get voice settings
    if(ctrlm_db_voice_valid()) {
        if(this->prefs.force_voice_settings) {
            XLOGD_INFO("Ignoring vrex settings from DB due to force_voice_settings being set to TRUE");
        } else {
            uint8_t query_string_qty = 0;
            XLOGD_INFO("Reading voice settings from Voice DB");
            for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
                uint8_t voice_device_status = CTRLM_VOICE_DEVICE_STATUS_NONE;
                ctrlm_db_voice_read_device_status(i, (int *)&voice_device_status);
                #ifdef CTRLM_LOCAL_MIC
                if((i == CTRLM_VOICE_DEVICE_MICROPHONE) && (voice_device_status & CTRLM_VOICE_DEVICE_STATUS_PRIVACY)) {
                    this->voice_privacy_enable(false);
                }
                #endif
                if((voice_device_status & CTRLM_VOICE_DEVICE_STATUS_LEGACY) && (voice_device_status != CTRLM_VOICE_DEVICE_STATUS_DISABLED)) { // Convert from legacy to current (some value other than DISABLED set)
                    XLOGD_INFO("Converting legacy device status value 0x%02X", voice_device_status);
                    voice_device_status = CTRLM_VOICE_DEVICE_STATUS_NONE;
                    ctrlm_db_voice_write_device_status((ctrlm_voice_device_t)i, (voice_device_status & CTRLM_VOICE_DEVICE_STATUS_MASK_DB));
                }
                if(voice_device_status & CTRLM_VOICE_DEVICE_STATUS_DISABLED) {
                   this->voice_device_disable((ctrlm_voice_device_t)i, false, NULL);
                }
            }
            ctrlm_sm_voice_url_ptt_read(this->prefs.server_url_src_ptt);
            ctrlm_sm_voice_url_ff_read(this->prefs.server_url_src_ff);
            #ifdef CTRLM_LOCAL_MIC_TAP
            ctrlm_sm_voice_url_mic_tap_read(this->prefs.server_url_src_mic_tap);
            #endif
            ctrlm_sm_voice_sat_enable_read(this->sat_token_required);
            ctrlm_sm_voice_mtls_enable_read(this->mtls_required);
            ctrlm_sm_voice_secure_url_required_read(this->secure_url_required);
            XLOGD_TELEMETRY("require m_SAT <%s> m_MTLS <%s> m_secure_url <%s>", this->sat_token_required ? "YES" : "NO", this->mtls_required ? "YES" : "NO", this->secure_url_required ? "YES" : "NO");

            ctrlm_db_voice_read_guide_language(this->prefs.guide_language);
            ctrlm_db_voice_read_aspect_ratio(this->prefs.aspect_ratio);
            ctrlm_db_voice_read_utterance_duration_min(this->prefs.utterance_duration_min);
            ctrlm_sm_voice_query_string_ptt_count_read(query_string_qty);
            XLOGD_INFO("Voice SM contains %u query strings", query_string_qty);
            // Create query strings
            for(unsigned int i = 0; i < query_string_qty; i++) {
                std::string key, value;
                ctrlm_sm_voice_query_string_ptt_read(i, key, value);
                this->query_strs_ptt.push_back(std::make_pair(key, value));
                XLOGD_INFO("Query String %u : key <%s> value <%s>", i, this->mask_pii ? "***" : key.c_str(), this->mask_pii ? "***" : value.c_str());
            }
        }
        bool valid;
        ctrlm_sm_voice_init_blob_read(init, &valid);
        ctrlm_db_voice_read_audio_ducking_beep_enable(this->audio_ducking_beep_enabled);
        audio_settings.ducking_beep = this->audio_ducking_beep_enabled;

        ctrlm_db_voice_read_par_voice_status(this->prefs.par_voice_enabled);
    } else {
        XLOGD_WARN("Reading voice settings from old style DB");
        ctrlm_db_voice_settings_read((guchar **)&voice_settings, &voice_settings_len);
        if(voice_settings_len == 0) {
            XLOGD_INFO("no voice settings in DB");
        } else if(voice_settings_len != sizeof(ctrlm_voice_iarm_call_settings_t)) {
            XLOGD_WARN("voice iarm settings is not the correct length, throwing away!");
        } else if(voice_settings != NULL) {
            this->voice_configure(voice_settings, true); // We want to write this to the database now, as this now writes to the new style DB
            free(voice_settings);
        }
    }

    this->set_audio_mode(&audio_settings);
    this->process_xconf(&json_obj_vsdk, local_conf);

    // Disable muting/ducking to recover in case ctrlm restarts while muted/ducked.
    this->audio_state_set(false);
    this->vsdk_config = json_incref(json_obj_vsdk);
    this->voice_sdk_open(this->vsdk_config);

    // Update routes
    this->voice_sdk_update_routes();

    #ifdef CTRLM_LOCAL_MIC
    // Read privacy mode state from the DB in case power cycle lost HW GPIO state
    if(this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_DISABLED) {
        XLOGD_INFO("voice is disabled, skip privacy");
    } else {
        bool privacy_enabled = this->voice_is_privacy_enabled();
        if(privacy_enabled != this->vsdk_is_privacy_enabled()) {
            privacy_enabled ? this->voice_privacy_enable(false) : this->voice_privacy_disable(false);
        }
        // Check keyword detector sensitivity value against limits; apply default if out of range.
        double sensitivity_set = this->vsdk_keyword_sensitivity_limit_check(this->prefs.keyword_sensitivity);
        if(sensitivity_set != this->prefs.keyword_sensitivity) {
            xrsr_keyword_config_t kw_config;
            kw_config.sensitivity = (float)sensitivity_set;
            if(!xrsr_keyword_config_set(&kw_config)) {
                XLOGD_ERROR("error updating keyword config");
            } else {
                this->prefs.keyword_sensitivity = sensitivity_set;
            }
        }
    }
    #endif

    // Set init message if read from DB
    if(!init.empty()) {
        this->voice_init_set(init.c_str(), false);
    }

    // Update query strings
    this->query_strings_updated();

    return(true);
}

bool ctrlm_voice_t::voice_configure(ctrlm_voice_iarm_call_settings_t *settings, bool db_write) {
    bool update_routes = false;
    if(settings == NULL) {
        XLOGD_ERROR("settings are null");
        return(false);
    }

    if(this->prefs.force_voice_settings) {
        XLOGD_INFO("Ignoring vrex settings from XRE due to force_voice_settings being set to TRUE");
        return(true);
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_VOICE_ENABLED) {
        XLOGD_INFO("Voice Control is <%s> : %u", settings->voice_control_enabled ? "ENABLED" : "DISABLED", settings->voice_control_enabled);
        for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
            if(settings->voice_control_enabled) {
                this->voice_device_enable((ctrlm_voice_device_t)i, db_write, &update_routes);
            } else {
                this->voice_device_disable((ctrlm_voice_device_t)i, db_write, &update_routes);
            }
        }
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_VREX_SERVER_URL) {
        settings->vrex_server_url[CTRLM_VOICE_SERVER_URL_MAX_LENGTH - 1] = '\0';
        XLOGD_INFO("vrex URL <%s>", this->mask_pii ? "***" : settings->vrex_server_url);
        this->prefs.server_url_src_ptt = settings->vrex_server_url;
        update_routes = true;
        if(db_write) {
            ctrlm_sm_voice_url_ptt_write(this->prefs.server_url_src_ptt);
        }
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_VREX_SAT_ENABLED) {
        XLOGD_INFO("Vrex SAT is <%s> : (%d)", settings->vrex_sat_enabled ? "ENABLED" : "DISABLED", settings->vrex_sat_enabled);
        this->sat_token_required = (settings->vrex_sat_enabled) ? true : false;
        if(db_write) {
            ctrlm_sm_voice_sat_enable_write(this->sat_token_required);
        }
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_GUIDE_LANGUAGE) {
        settings->guide_language[CTRLM_VOICE_GUIDE_LANGUAGE_MAX_LENGTH - 1] = '\0';
        XLOGD_INFO("Guide Language <%s>", settings->guide_language);
        voice_stb_data_guide_language_set(settings->guide_language);
        if(db_write) {
            ctrlm_db_voice_write_guide_language(this->prefs.guide_language);
        }
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_ASPECT_RATIO) {
        settings->aspect_ratio[CTRLM_VOICE_ASPECT_RATIO_MAX_LENGTH - 1] = '\0';
        XLOGD_INFO("Aspect Ratio <%s>", settings->aspect_ratio);
        this->prefs.aspect_ratio = settings->aspect_ratio;
        if(db_write) {
            ctrlm_db_voice_write_aspect_ratio(this->prefs.aspect_ratio);
        }
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_UTTERANCE_DURATION) {
        XLOGD_INFO("Utterance Duration Minimum <%lu ms>", settings->utterance_duration_minimum);
        if(settings->utterance_duration_minimum > CTRLM_VOICE_MIN_UTTERANCE_DURATION_MAXIMUM) {
            this->prefs.utterance_duration_min = CTRLM_VOICE_MIN_UTTERANCE_DURATION_MAXIMUM;
        } else {
            this->prefs.utterance_duration_min = settings->utterance_duration_minimum;
        }
        if(db_write) {
            ctrlm_db_voice_write_utterance_duration_min(this->prefs.utterance_duration_min);
        }
        update_routes = true;
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_QUERY_STRINGS) {
        if(settings->query_strings.pair_count > CTRLM_VOICE_QUERY_STRING_MAX_PAIRS) {
            XLOGD_WARN("Query String Pair Count too high <%d>.  Setting to <%d>.", settings->query_strings.pair_count, CTRLM_VOICE_QUERY_STRING_MAX_PAIRS);
            settings->query_strings.pair_count = CTRLM_VOICE_QUERY_STRING_MAX_PAIRS;
        }
        this->query_strs_ptt.clear();
        int query = 0;
        for(; query < settings->query_strings.pair_count; query++) {
            std::string key, value;

            //Make sure the strings are NULL terminated
            settings->query_strings.query_string[query].name[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1] = '\0';
            settings->query_strings.query_string[query].value[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1] = '\0';
            key   = settings->query_strings.query_string[query].name;
            value = settings->query_strings.query_string[query].value;
            XLOGD_INFO("Query String[%d] name <%s> value <%s>", query, this->mask_pii ? "***" : key.c_str(), this->mask_pii ? "***" : value.c_str());
            this->query_strs_ptt.push_back(std::make_pair(key, value));
            if(db_write) {
                ctrlm_sm_voice_query_string_ptt_write(query, key, value);
            }
        }
        if(db_write) {
            ctrlm_sm_voice_query_string_ptt_count_write(settings->query_strings.pair_count);
        }
        this->query_strings_updated();
    }

    if(settings->available & CTRLM_VOICE_SETTINGS_FARFIELD_VREX_SERVER_URL) {
        settings->server_url_vrex_src_ff[CTRLM_VOICE_SERVER_URL_MAX_LENGTH - 1] = '\0';
        XLOGD_INFO("Farfield vrex URL <%s>", this->mask_pii ? "***" : settings->server_url_vrex_src_ff);
        this->prefs.server_url_src_ff = settings->server_url_vrex_src_ff;
        update_routes = true;
        if(db_write) {
            ctrlm_sm_voice_url_ff_write(this->prefs.server_url_src_ff);
        }
    }
    #ifdef CTRLM_LOCAL_MIC_TAP
    if(settings->available & CTRLM_VOICE_SETTINGS_MIC_TAP_SERVER_URL) {
        settings->server_url_src_mic_tap[CTRLM_VOICE_SERVER_URL_MAX_LENGTH - 1] = '\0';
        XLOGD_INFO("Mic tap URL <%s>", this->mask_pii ? "***" : settings->server_url_src_mic_tap);
        this->prefs.server_url_src_mic_tap = settings->server_url_src_mic_tap;
        update_routes = true;
        if(db_write) {
            ctrlm_sm_voice_url_mic_tap_write(this->prefs.server_url_src_mic_tap);
        }
    }
    #endif

    if(update_routes && this->xrsr_opened) {
        this->voice_sdk_update_routes();
    }

    return(true);
}

bool ctrlm_voice_t::voice_configure(json_t *settings, bool db_write) {
    json_config conf;
    bool update_routes = false;
    if(NULL == settings) {
        XLOGD_ERROR("JSON object NULL");
        return(false);
    }

    if(this->xrsr_opened == false) { // We are not ready to configure voice settings yet.
        XLOGD_WARN("Voice object not ready for settings, return failure");
        return(false);
    }

    if(this->prefs.force_voice_settings) {
        XLOGD_INFO("Ignoring vrex settings from configure call due to force_voice_settings being set to TRUE");
        return(true);
    }


    if(conf.config_object_set(settings)) {
        bool enable;
        bool prv_enabled;
        std::string url;
        json_config device_config;

        if(conf.config_value_get("enable", enable)) {
            XLOGD_INFO("Voice Control is <%s>", enable ? "ENABLED" : "DISABLED");
            for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
                if(enable) {
                    this->voice_device_enable((ctrlm_voice_device_t)i, db_write, &update_routes);
                } else {
                    this->voice_device_disable((ctrlm_voice_device_t)i, db_write, &update_routes);
                }
            }
        }
        if(conf.config_value_get("urlAll", url)) {
            #ifdef CTRLM_LOCAL_MIC_TAP
            this->prefs.server_url_src_mic_tap = url;
            #endif
            this->prefs.server_url_src_ptt     = url;
            this->prefs.server_url_src_ff      = std::move(url);
            update_routes = true;
        }
        if(conf.config_value_get("urlPtt", url)) {
            this->prefs.server_url_src_ptt = std::move(url);
            update_routes = true;
        }
        if(conf.config_value_get("urlHf", url)) {
            this->prefs.server_url_src_ff  = std::move(url);
            update_routes = true;
        }
        #ifdef CTRLM_LOCAL_MIC_TAP
        if(conf.config_value_get("urlMicTap", url)) {
            this->prefs.server_url_src_mic_tap  = std::move(url);
            update_routes = true;
        }
        #endif
        if(conf.config_value_get("prv", prv_enabled)) {
            this->prefs.par_voice_enabled = prv_enabled;
            XLOGD_INFO("Press and Release Voice is <%s>", this->prefs.par_voice_enabled ? "ENABLED" : "DISABLED");
            if(db_write) {
               ctrlm_db_voice_write_par_voice_status(this->prefs.par_voice_enabled);
            }
        }
        if(conf.config_object_get("ptt", device_config)) {
            if(device_config.config_value_get("enable", enable)) {
                XLOGD_INFO("Voice Control PTT is <%s>", enable ? "ENABLED" : "DISABLED");
                if(enable) {
                    this->voice_device_enable(CTRLM_VOICE_DEVICE_PTT, db_write, &update_routes);
                } else {
                    this->voice_device_disable(CTRLM_VOICE_DEVICE_PTT, db_write, &update_routes);
                }
            }
        }
        if(conf.config_object_get("ff", device_config)) {
            if(device_config.config_value_get("enable", enable)) {
                XLOGD_INFO("Voice Control FF is <%s>", enable ? "ENABLED" : "DISABLED");
                if(enable) {
                    this->voice_device_enable(CTRLM_VOICE_DEVICE_FF, db_write, &update_routes);
                } else {
                    this->voice_device_disable(CTRLM_VOICE_DEVICE_FF, db_write, &update_routes);
                }
            }
        }
        #ifdef CTRLM_LOCAL_MIC
        if(conf.config_object_get("mic", device_config)) {
            if(device_config.config_value_get("enable", enable)) {
                XLOGD_TELEMETRY("Voice Control MIC is <%s>", enable ? "ENABLED" : "DISABLED");
                #ifdef CTRLM_LOCAL_MIC_DISABLE_VIA_PRIVACY
                bool privacy_enabled = this->voice_is_privacy_enabled();
                if(enable) {
                    if(privacy_enabled) {
                        this->voice_privacy_disable(true);
                    }
                } else if(!privacy_enabled) {
                    this->voice_privacy_enable(true);
                }
                #else
                if(enable) {
                    this->voice_device_enable(CTRLM_VOICE_DEVICE_MICROPHONE, db_write, &update_routes);
                } else {
                    this->voice_device_disable(CTRLM_VOICE_DEVICE_MICROPHONE, db_write, &update_routes);
                }
                #endif
            }
        }
        #endif
        if(conf.config_value_get("wwFeedback", enable)) { // This option will enable / disable the Wake Word feedback (typically an audible beep).
           XLOGD_INFO("Voice Control kwd feedback is <%s>", enable ? "ENABLED" : "DISABLED");
           this->audio_ducking_beep_enabled = enable;

           if(db_write) {
              ctrlm_db_voice_write_audio_ducking_beep_enable(enable);
           }
        }
        if(update_routes && this->xrsr_opened) {
            this->voice_sdk_update_routes();
            if(db_write) {
                ctrlm_sm_voice_url_ptt_write(this->prefs.server_url_src_ptt);
                ctrlm_sm_voice_url_ff_write(this->prefs.server_url_src_ff);
                #ifdef CTRLM_LOCAL_MIC_TAP
                ctrlm_sm_voice_url_mic_tap_write(this->prefs.server_url_src_mic_tap);
                #endif
            }
        }
    }
    return(true);
}

bool ctrlm_voice_t::voice_message(std::string &uuid, const char *message) {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
    if(uuid.empty()) { // uuid was not provided, assume
       XLOGD_WARN("using VOICE_SESSION_GROUP_DEFAULT until uuid requirement is enforced. message <%s>", this->mask_pii ? "***" : message);
    } else {
       session = voice_session_from_uuid(uuid);
    }

    if(session == NULL) {
        XLOGD_ERROR("session not found for uuid <%s>", uuid.c_str());
        return(false);
    }

    if(session->endpoint_current) {
        return(session->endpoint_current->voice_message(message));
    }
    else {
#ifdef SUPPORT_ASYNC_SRVR_MSG
        bool ret = true;
        for(const auto &itr : this->endpoints) {
            if(!itr->voice_message(message)){
            ret = false;
        }
      }
      return(ret);
#endif
    }

    return(false);
}

bool ctrlm_voice_t::voice_status(ctrlm_voice_status_t *status) {
    bool ret = false;
    /* TODO
     * Defaulted Press and Release voice to true/supported
     * Will make this dynamic once PAR EoS is implemented and
     * by some criteria timeout is not adequate
     */
    ctrlm_voice_status_capabilities_t capabilities = {
       .prv = true,
    #ifdef BEEP_ON_KWD_ENABLED
       .wwFeedback = true,
    #else
       .wwFeedback = false,
    #endif
    };

    if(!this->xrsr_opened) {
        XLOGD_ERROR("xrsr not opened yet");
    } else if(NULL == status) {
        XLOGD_ERROR("invalid params");
    } else {
        status->urlPtt    = this->prefs.server_url_src_ptt;
        status->urlHf     = this->prefs.server_url_src_ff;
        #ifdef CTRLM_LOCAL_MIC_TAP
        status->urlMicTap = this->prefs.server_url_src_mic_tap;
        #endif
        sem_wait(&this->device_status_semaphore);
        for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
            status->status[i] = this->device_status[i];
        }
        status->wwFeedback   = this->audio_ducking_beep_enabled;
        status->prv_enabled  = this->prefs.par_voice_enabled;
        status->capabilities = capabilities;
        sem_post(&this->device_status_semaphore);
        ret = true;
    }
    return(ret);
}

bool ctrlm_voice_t::server_message(const char *message, unsigned long length) {
    bool ret = false;
    if(this->voice_ipc) {
        ret = this->voice_ipc->server_message(message, length);
    }
    return(ret);
}

bool ctrlm_voice_t::voice_init_set(const char *init, bool db_write) {
    bool ret = false; // Return true if at least one endpoint was set

    if(this->xrsr_opened == false) { // We are not ready for set init call
        XLOGD_WARN("Voice object not ready for set init, return failure");
    } else {
        for(const auto &itr : this->endpoints) {
            if(itr->voice_init_set(init)) {
                ret = true;
            }
        }
        if(db_write) {
            ctrlm_sm_voice_init_blob_write(std::string(init));
        }
    }
    return(ret);
}

void ctrlm_voice_t::process_xconf(json_t **json_obj_vsdk, bool local_conf) {
   XLOGD_INFO("Voice XCONF Settings");
   int result;

   char vsdk_config_str[CTRLM_RFC_MAX_PARAM_LEN] = {0}; //MAX_PARAM_LEN from rfcapi.h is 2048

#ifdef CTRLM_NETWORK_RF4CE
   char encoder_params_str[CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS * 2 + 1] = {0};

   result  = ctrlm_tr181_string_get(CTRLM_RF4CE_TR181_RF4CE_OPUS_ENCODER_PARAMS, encoder_params_str, sizeof(encoder_params_str));
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
      std::string opus_encoder_params_str = encoder_params_str;
      this->voice_params_opus_encoder_validate(opus_encoder_params_str);

      XLOGD_INFO("opus encoder params <%s>", this->prefs.opus_encoder_params_str.c_str());
   }
#endif

   ctrlm_voice_audio_settings_t audio_settings = {this->audio_mode, this->audio_timing, this->audio_confidence_threshold, this->audio_ducking_type, this->audio_ducking_level, this->audio_ducking_beep_enabled};
   bool changed = false;

   result = ctrlm_tr181_int_get(CTRLM_TR181_VOICE_PARAMS_AUDIO_MODE, (int*)&audio_settings.mode);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
       changed = true;
   }
   result = ctrlm_tr181_int_get(CTRLM_TR181_VOICE_PARAMS_AUDIO_TIMING, (int *)&audio_settings.timing);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
       changed = true;
   }
   result = ctrlm_tr181_real_get(CTRLM_TR181_VOICE_PARAMS_AUDIO_CONFIDENCE_THRESHOLD, &audio_settings.confidence_threshold);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
       changed = true;
   }
   result = ctrlm_tr181_int_get(CTRLM_TR181_VOICE_PARAMS_AUDIO_DUCKING_TYPE, (int *)&audio_settings.ducking_type);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
       changed = true;
   }
   result = ctrlm_tr181_real_get(CTRLM_TR181_VOICE_PARAMS_AUDIO_DUCKING_LEVEL, &audio_settings.ducking_level);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
       changed = true;
   }

   // CTRLM_TR181_VOICE_PARAMS_AUDIO_DUCKING_BEEP doesn't exist because this is a user configurable setting via configureVoice thunder api

   double keyword_sensitivity = 0.0;
   result = ctrlm_tr181_real_get(CTRLM_TR181_VOICE_PARAMS_KEYWORD_SENSITIVITY, &keyword_sensitivity);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
      this->prefs.keyword_sensitivity = (keyword_sensitivity < 0.0) ? JSON_FLOAT_VALUE_VOICE_KEYWORD_DETECT_SENSITIVITY : keyword_sensitivity;
   }

   result = ctrlm_tr181_string_get(CTRLM_TR181_VOICE_PARAMS_VSDK_CONFIGURATION, &vsdk_config_str[0], CTRLM_RFC_MAX_PARAM_LEN);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
      json_error_t jerror;
      json_t *jvsdk;
      char *decoded_buf = NULL;
      size_t decoded_buf_len = 0;

      decoded_buf = (char *)g_base64_decode((const gchar*)vsdk_config_str, &decoded_buf_len);
      if(decoded_buf) {
         if(decoded_buf_len > 0 && decoded_buf_len < CTRLM_RFC_MAX_PARAM_LEN) {
            XLOGD_INFO("VSDK configuration taken from XCONF");
            XLOGD_INFO("%s", decoded_buf);

            jvsdk = json_loads(&decoded_buf[0], 0, &jerror);
            do {
               if(NULL == jvsdk) {
                  XLOGD_ERROR("XCONF has VSDK params but json_loads() failed, line %d: %s ", jerror.line, jerror.text );
                  break;
               }
               if(!json_is_object(jvsdk))
               {
                  XLOGD_ERROR("found VSDK in text but invalid object");
                  break;
               }

               //If execution reaches here we have XCONF settings to use. If developer has used local conf settings, keep them.
               if(local_conf) {
                  if(!json_object_update(jvsdk, *json_obj_vsdk)) {
                     XLOGD_ERROR("failed to update json_obj_vsdk");
                     break;
                  }
               }

               *json_obj_vsdk = json_deep_copy(jvsdk);
               if(NULL == *json_obj_vsdk)
               {
                  XLOGD_ERROR("found VSDK object but failed to copy. We have lost any /opt file VSDK parameters");
                  /* Nothing to do about this unlikely error. If I copy to a temp pointer to protect the input,
                   * then I have to copy from temp to real, and check that copy for failure. Where would it end?
                   */
                  break;
               }
            }while(0);
         } else {
            XLOGD_WARN("incorrect length");
         }
         free(decoded_buf);
      } else {
         XLOGD_WARN("failed to decode base64");
      }
   }

   int value = 0;
   result = ctrlm_tr181_int_get(CTRLM_RF4CE_TR181_PRESS_AND_RELEASE_EOS_TIMEOUT, &value, 0, UINT16_MAX);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
      this->prefs.par_voice_eos_timeout = value;
   }

   result = ctrlm_tr181_int_get(CTRLM_RF4CE_TR181_PRESS_AND_RELEASE_EOS_METHOD, &value, 0, UINT8_MAX);
   if(result == CTRLM_TR181_RESULT_SUCCESS) {
      this->prefs.par_voice_eos_method = value;
   }

   if(changed) {
       this->set_audio_mode(&audio_settings);
   }
}

void ctrlm_voice_t::query_strings_updated() {
    // N/A
}

void ctrlm_voice_t::voice_params_qos_get(voice_params_qos_t *params) {
   if(params == NULL) {
      XLOGD_ERROR("NULL param");
      return;
   }
   params->timeout_packet_initial    = this->prefs.timeout_packet_initial;
   params->timeout_packet_subsequent = this->prefs.timeout_packet_subsequent;
   params->bitrate_minimum           = this->prefs.bitrate_minimum;
   params->time_threshold            = this->prefs.time_threshold;
}

void ctrlm_voice_t::voice_params_opus_encoder_get(voice_params_opus_encoder_t *params) {
   if(params == NULL) {
      XLOGD_ERROR("NULL param");
      return;
   }
   errno_t safec_rc = memcpy_s(params->data, sizeof(params->data), this->prefs.opus_encoder_params, CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS);
   ERR_CHK(safec_rc);
}

void ctrlm_voice_t::voice_params_par_get(voice_params_par_t *params) {
   if(params == NULL) {
      XLOGD_ERROR("NULL param");
      return;
   }
   params->par_voice_enabled     = this->prefs.par_voice_enabled;
   params->par_voice_eos_method  = this->prefs.par_voice_eos_method;
   params->par_voice_eos_timeout = this->prefs.par_voice_eos_timeout;
}

void ctrlm_voice_t::voice_params_opus_encoder_default(void) {
   this->prefs.opus_encoder_params_str = JSON_STR_VALUE_VOICE_OPUS_ENCODER_PARAMS;
   this->voice_params_hex_str_to_bytes(this->prefs.opus_encoder_params_str, this->prefs.opus_encoder_params, sizeof(this->prefs.opus_encoder_params));
   this->voice_params_opus_samples_per_packet_set();
}

void ctrlm_voice_t::voice_params_opus_samples_per_packet_set(void) {
   guchar fr_dur = (this->prefs.opus_encoder_params[3] >> 4) & 0xF;
   switch(fr_dur) {
      case 1: { this->opus_samples_per_packet =  8 *   5; break; } // 2.5 ms
      case 2: { this->opus_samples_per_packet = 16 *   5; break; } //   5 ms
      case 3: { this->opus_samples_per_packet = 16 *  10; break; } //  10 ms
      case 5: { this->opus_samples_per_packet = 16 *  40; break; } //  40 ms
      case 6: { this->opus_samples_per_packet = 16 *  60; break; } //  60 ms
      case 7: { this->opus_samples_per_packet = 16 *  80; break; } //  80 ms
      case 8: { this->opus_samples_per_packet = 16 * 100; break; } // 100 ms
      case 9: { this->opus_samples_per_packet = 16 * 120; break; } // 120 ms
      default:{ this->opus_samples_per_packet = 16 * 20;  break; } //  20 ms
   }
}

bool ctrlm_voice_t::voice_params_hex_str_to_bytes(std::string hex_string, guchar *data, guint32 length) {
   if(hex_string.length() != (length << 1)) {
      XLOGD_ERROR("INVALID hex string length");
      return(false);
   }

   // Convert hex string to bytes
   const char *str = hex_string.c_str();
   for(guchar index = 0; index < length; index++) {
      unsigned int value;
      int rc = sscanf(&str[index << 1], "%02X", &value);
      if(rc != 1) {
         XLOGD_ERROR("INVALID hex digit <%s>", str);
         return(false);
      }
      data[index] = (guchar)value;
   }
   return(true);
}

bool ctrlm_voice_t::voice_params_opus_encoder_validate(std::string &opus_encoder_params_str) {
   bool invalid = false;
   uint8_t opus_encoder_params[CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS];
   do {
      if(!voice_params_hex_str_to_bytes(opus_encoder_params_str,opus_encoder_params, sizeof(opus_encoder_params))) {
         XLOGD_ERROR("INVALID hex string <%s>", opus_encoder_params_str.c_str());
         invalid = true;
         break;
      }
      XLOGD_INFO("str <%s> 0x%02X %02X %02X %02X %02X", opus_encoder_params_str.c_str(), opus_encoder_params[0], opus_encoder_params[1], opus_encoder_params[2], opus_encoder_params[3], opus_encoder_params[4]);

      // Check for invalid encoder parameters
      guchar flags = opus_encoder_params[0];
      if((flags & 0x3) == 0x3) {
         XLOGD_ERROR("INVALID application - flags 0x%02X", flags);
         invalid = true;
         break;
      }
      if((flags & 0xE0) > 0x80) {
         XLOGD_ERROR("INVALID max bandwidth - flags 0x%02X", flags);
         invalid = true;
         break;
      }
      guchar plp = opus_encoder_params[1] & 0x7F;
      if(plp > 100) {
         XLOGD_ERROR("INVALID plp %u", plp);
         invalid = true;
         break;
      }
      guchar bitrate = opus_encoder_params[2];
      if(bitrate > 128 && bitrate < 255) {
         XLOGD_ERROR("INVALID bitrate %u", bitrate);
         invalid = true;
         break;
      }
      guchar comp = opus_encoder_params[3] & 0xF;
      if(comp > 10) {
         XLOGD_ERROR("INVALID comp %u", comp);
         invalid = true;
         break;
      }
      guchar fr_dur = (opus_encoder_params[3] >> 4) & 0xF;
      if(fr_dur > 9) {
         XLOGD_ERROR("INVALID frame duration %u", fr_dur);
         invalid = true;
         break;
      }
      guchar lsbd = opus_encoder_params[4] & 0x1F;
      if(lsbd < 8 || lsbd > 24) {
         XLOGD_ERROR("INVALID lsbd %u", lsbd);
         invalid = true;
         break;
      }
   } while(0);

   if(invalid) { // Restore default settings
      XLOGD_WARN("keep current settings - str <%s> 0x%02X %02X %02X %02X %02X", this->prefs.opus_encoder_params_str.c_str(), this->prefs.opus_encoder_params[0], this->prefs.opus_encoder_params[1], this->prefs.opus_encoder_params[2], this->prefs.opus_encoder_params[3], this->prefs.opus_encoder_params[4]);
   } else {
      this->prefs.opus_encoder_params_str = opus_encoder_params_str;
      errno_t safec_rc = memcpy_s(this->prefs.opus_encoder_params, sizeof(this->prefs.opus_encoder_params), opus_encoder_params, sizeof(opus_encoder_params));
      ERR_CHK(safec_rc);
      this->voice_params_opus_samples_per_packet_set();
   }
   return(!invalid);
}

void ctrlm_voice_t::voice_status_set(const uuid_t uuid) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

    this->voice_status_set(session);
}

void ctrlm_voice_t::voice_status_set(ctrlm_voice_session_t *session) {
   if(session == NULL) {
      XLOGD_ERROR("invalid session");
      return;
   }

   if(session->network_id != CTRLM_MAIN_NETWORK_ID_INVALID && session->controller_id != CTRLM_MAIN_CONTROLLER_ID_INVALID) {
      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::voice_command_status_set, &session->status, sizeof(session->status), NULL, session->network_id);

      if(session->status.status != VOICE_COMMAND_STATUS_PENDING) {
         rdkx_timestamp_get_realtime(&session->session_timing.ctrl_cmd_status_wr);

         if(session->controller_command_status && session->timeout_ctrl_cmd_status_read <= 0) { // Set a timeout to clean up in case controller does not read the status
            session->timeout_ctrl_cmd_status_read = g_timeout_add(CTRLM_CONTROLLER_CMD_STATUS_READ_TIMEOUT, ctrlm_voice_controller_command_status_read_timeout, NULL);
         } else {
             if(!session->session_active_server) {
                session->state_src = CTRLM_VOICE_STATE_SRC_READY;
                voice_session_stats_print(session);
                voice_session_stats_clear(session);
             }
         }
      }
   }
}

bool ctrlm_voice_t::voice_device_streaming(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        ctrlm_voice_session_t *session = &this->voice_session[group];
        if(session->network_id == network_id && session->controller_id == controller_id) {
            if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING && (session->state_dst >= CTRLM_VOICE_STATE_DST_REQUESTED && session->state_dst <= CTRLM_VOICE_STATE_DST_STREAMING)) {
                return(true);
            }
        }
    }
    return(false);
}

void ctrlm_voice_t::voice_controller_command_status_read(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {

   // TODO Access to the state variables in this function are not thread safe.  Need to make the entire voice object thread safe.

   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT]; // voice status is for PTT only

   if(session->network_id == network_id && session->controller_id == controller_id) {
      session->session_active_controller = false;
       rdkx_timestamp_get_realtime(&session->session_timing.ctrl_cmd_status_rd);

       // Remove controller status read timeout
       if(session->timeout_ctrl_cmd_status_read > 0) {
           g_source_remove(session->timeout_ctrl_cmd_status_read);
           session->timeout_ctrl_cmd_status_read = 0;
       }

       if(!session->session_active_server) {
          session->state_src   = CTRLM_VOICE_STATE_SRC_READY;
          voice_session_stats_print(session);
          voice_session_stats_clear(session);

         // Send session stats only if they haven't already been sent
         if(session->stats_session_id != 0 && this->voice_ipc) {
            ctrlm_voice_ipc_event_session_statistics_t stats;

            stats.common  = session->ipc_common_data;
            stats.session = session->stats_session;
            stats.reboot  = session->stats_reboot;

            this->voice_ipc->session_statistics(stats);
         }
       }
   }
}

void ctrlm_voice_t::voice_session_controller_command_status_read_timeout(void) {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT]; // voice status is for PTT only

   if(session->network_id == CTRLM_MAIN_NETWORK_ID_INVALID || session->controller_id == CTRLM_MAIN_CONTROLLER_ID_INVALID) {
      XLOGD_ERROR("no active voice session");
   } else if(session->timeout_ctrl_cmd_status_read > 0) {
      session->session_active_controller    = false;
      session->timeout_ctrl_cmd_status_read = 0;
      XLOGD_INFO("controller failed to read command status");
      xrsr_session_terminate(voice_device_to_xrsr(session->voice_device)); // Synchronous - this will take a bit of time.  Might need to revisit this down the road.

      if(!session->session_active_server) {
         session->state_src   = CTRLM_VOICE_STATE_SRC_READY;
         voice_session_stats_print(session);
         voice_session_stats_clear(session);

         // Send session stats only if they haven't already been sent
         if(session->stats_session_id != 0 && this->voice_ipc) {
            ctrlm_voice_ipc_event_session_statistics_t stats;

            stats.common  = session->ipc_common_data;
            stats.session = session->stats_session;
            stats.reboot  = session->stats_reboot;

            this->voice_ipc->session_statistics(stats);
         }
      }
   }
}

ctrlm_voice_session_response_status_t ctrlm_voice_t::voice_session_req(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, 
                                                                       ctrlm_voice_device_t device_type, ctrlm_voice_format_t format,
                                                                       voice_session_req_stream_params *stream_params,
                                                                       const char *controller_name, const char *sw_version, const char *hw_version, double voltage, bool command_status,
                                                                       ctrlm_timestamp_t *timestamp, ctrlm_voice_session_rsp_confirm_t *cb_confirm, void **cb_confirm_param, bool use_external_data_pipe, const char *l_transcription_in, const char *audio_file_in, const uuid_t *uuid, bool low_latency, bool low_cpu_util, int audio_fd) {

    ctrlm_voice_session_t *session = &this->voice_session[voice_device_to_session_group(device_type)];

    if(CTRLM_VOICE_STATE_SRC_INVALID == session->state_src) {
        XLOGD_ERROR("Voice is not ready");
        this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_SERVER_NOT_READY);
        return(VOICE_SESSION_RESPONSE_SERVER_NOT_READY);
    } 
#ifdef AUTH_ENABLED
    else if(this->voice_session_requires_stb_data(device_type) && !this->voice_session_has_stb_data()) {
        XLOGD_ERROR("Authentication Data missing");
        this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_NO_RECEIVER_ID);
        return(VOICE_SESSION_RESPONSE_SERVER_NOT_READY);
    }
#endif
    else if(!this->voice_session_can_request(device_type)) { // This is an unsafe check but we don't want to block on a semaphore here
        XLOGD_ERROR("Voice Device <%s> is <%s>", ctrlm_voice_device_str(device_type), ctrlm_voice_device_status_str(this->device_status[device_type]).c_str());
        this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_VOICE_DISABLED);  // TODO Add other abort reasons
        return(VOICE_SESSION_RESPONSE_FAILURE);
    }

    if(CTRLM_VOICE_STATE_SRC_WAITING == session->state_src) {
        // Remove controller status read timeout
        if(session->timeout_ctrl_cmd_status_read > 0) {
            g_source_remove(session->timeout_ctrl_cmd_status_read);
            session->timeout_ctrl_cmd_status_read = 0;
        }
        uuid_clear(session->uuid);

        // Cancel current speech router session
        XLOGD_INFO("Waiting on the results from previous session, aborting this and continuing..");
        xrsr_session_terminate(voice_device_to_xrsr(session->voice_device)); // Synchronous - this will take a bit of time.  Might need to revisit this down the road.
    }
    if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING || session->state_dst != CTRLM_VOICE_STATE_DST_READY) { // Voice session is in progress
        if(session->controller_id == controller_id) { // session in progress with same controller
            if(session->requested_more_audio) { // More audio in the same session
                XLOGD_WARN("Session in progress with same controller - src <%s> dst <%s>, sending more audio.", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
                session->requested_more_audio = false;
            } else { // Cancel current speech router session
                XLOGD_WARN("Session in progress with same controller - src <%s> dst <%s>, aborting this and continuing..", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
                xrsr_session_terminate(voice_device_to_xrsr(session->voice_device)); // Synchronous - this will take a bit of time.  Might need to revisit this down the road.
            }
        } else { // session in progress with different controller
            XLOGD_ERROR("Session in progress with different controller - src <%s> dst <%s>, rejecting.", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
            this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_BUSY);
            return(VOICE_SESSION_RESPONSE_BUSY);
        }
    }

    int fds[2] = { -1, -1 };
    bool is_mic = ctrlm_voice_device_is_mic(device_type);
    bool is_session_by_text = (l_transcription_in != NULL);
    bool is_session_by_file = (audio_file_in      != NULL);
    bool is_session_by_fifo = (use_external_data_pipe && audio_fd != -1);
    fds[PIPE_READ] = audio_fd;

    xrsr_session_request_t request_params;
    request_params.type = XRSR_SESSION_REQUEST_TYPE_INVALID;
    
    if(is_session_by_text) {
        XLOGD_INFO("Requesting the speech router start a text-only session with transcription = <%s>", l_transcription_in);
        if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING || session->state_dst != CTRLM_VOICE_STATE_DST_READY) {
            XLOGD_ERROR("unable to accept a text-only session due to current session - state src <%s> dst <%s>.", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
            return VOICE_SESSION_RESPONSE_BUSY;
        }
        request_params.type = XRSR_SESSION_REQUEST_TYPE_TEXT;
        request_params.value.text.text = l_transcription_in;

        xrsr_audio_format_t xrsr_format = { .type = XRSR_AUDIO_FORMAT_NONE};
        if (false == xrsr_session_request(voice_device_to_xrsr(device_type), xrsr_format, request_params, uuid, false, false)) {
            XLOGD_ERROR("Failed to acquire the text-only session from the speech router.");
            return VOICE_SESSION_RESPONSE_BUSY;
        }
    } else if(is_session_by_file) {
        XLOGD_INFO("Requesting the speech router start a session with audio file  = <%s>", audio_file_in);
        if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING || session->state_dst != CTRLM_VOICE_STATE_DST_READY) {
            XLOGD_ERROR("unable to accept an audio file session due to current session - state src <%s> dst <%s>.", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
            return VOICE_SESSION_RESPONSE_BUSY;
        }
        request_params.type = XRSR_SESSION_REQUEST_TYPE_AUDIO_FILE;
        request_params.value.audio_file.path = audio_file_in;

        xrsr_audio_format_t xrsr_format;
        if(format.type == CTRLM_VOICE_FORMAT_OPUS) {
            xrsr_format.type = XRSR_AUDIO_FORMAT_OPUS;
        } else {
            xrsr_format.type = XRSR_AUDIO_FORMAT_PCM;
        }

        if (false == xrsr_session_request(voice_device_to_xrsr(device_type), xrsr_format, request_params, uuid, false, false)) {
            XLOGD_ERROR("Failed to acquire the audio file session from the speech router.");
            return VOICE_SESSION_RESPONSE_BUSY;
        }
    } else if(is_mic) {
       XLOGD_INFO("Requesting the speech router start a session with device <%s> format <%s> low latency <%s> low cpu <%s>", ctrlm_voice_device_str(device_type), ctrlm_voice_format_str(format), low_latency ? "YES" : "NO", low_cpu_util ? "YES" : "NO");
       xrsr_audio_format_t xrsr_format = { .type = XRSR_AUDIO_FORMAT_PCM };
       if(format.type == CTRLM_VOICE_FORMAT_PCM_RAW) {
          xrsr_format.type = XRSR_AUDIO_FORMAT_PCM_RAW;
       } else if(format.type == CTRLM_VOICE_FORMAT_PCM_32_BIT) {
          xrsr_format.type = XRSR_AUDIO_FORMAT_PCM_32_BIT;
       } else if(format.type == CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI) {
          xrsr_format.type = XRSR_AUDIO_FORMAT_PCM_32_BIT_MULTI;
       }
       request_params.type = XRSR_SESSION_REQUEST_TYPE_AUDIO_MIC;
       request_params.value.audio_mic.stream_params_required = this->nsm_voice_session;

       if(false == xrsr_session_request(voice_device_to_xrsr(device_type), xrsr_format, request_params, uuid, low_latency, low_cpu_util)) {
           XLOGD_ERROR("Failed to acquire the microphone session from the speech router.");
           return VOICE_SESSION_RESPONSE_BUSY;
       }
    } else {
        XLOGD_INFO("Requesting the speech router start a session with audio fd <%d>", fds[PIPE_READ]);
        if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING || session->state_dst != CTRLM_VOICE_STATE_DST_READY) {
            XLOGD_ERROR("unable to accept an audio fd session due to current session - state src <%s> dst <%s>.", ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
            return VOICE_SESSION_RESPONSE_BUSY;
        }

        bool create_pipe = (device_type == CTRLM_VOICE_DEVICE_PTT && !use_external_data_pipe);
        if(create_pipe) {
            errno = 0;
            if(pipe(fds) < 0) {
                int errsv = errno;
                XLOGD_ERROR("Failed to create pipe <%s>", strerror(errsv));
                this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_FAILURE);
                return(VOICE_SESSION_RESPONSE_FAILURE);
            } // set to non-blocking
        }

        request_params.type = XRSR_SESSION_REQUEST_TYPE_AUDIO_FD;
        request_params.value.audio_fd.audio_fd     = fds[PIPE_READ];
        request_params.value.audio_fd.audio_format = voice_format_to_xrsr(format);
        request_params.value.audio_fd.callback     = (create_pipe) ? NULL : ctrlm_voice_data_post_processing_cb; // RF4CE does not use pipe read callback
        request_params.value.audio_fd.user_data    = (create_pipe) ? NULL : (void *)this;

        if(false == xrsr_session_request(voice_device_to_xrsr(device_type), voice_format_to_xrsr(format), request_params, uuid, false, false)) {
            XLOGD_TELEMETRY("Failed to acquire voice session");
            this->voice_session_notify_abort(network_id, controller_id, 0, CTRLM_VOICE_SESSION_ABORT_REASON_BUSY);
            if(create_pipe) {
                close(fds[PIPE_WRITE]);
                close(fds[PIPE_READ]);
            }
            return(VOICE_SESSION_RESPONSE_BUSY);
        }
    }

    session->is_session_by_text        = is_session_by_text;
    session->transcription_in          = is_session_by_text ? l_transcription_in : "";
    session->is_session_by_file        = is_session_by_file;
    session->is_session_by_fifo        = is_session_by_fifo;
    session->state_src                 = CTRLM_VOICE_STATE_SRC_STREAMING;
    session->state_dst                 = CTRLM_VOICE_STATE_DST_REQUESTED;
    session->voice_device              = device_type;
    session->format                    = format;
    session->audio_pipe[PIPE_READ]     = fds[PIPE_READ];
    session->audio_pipe[PIPE_WRITE]    = fds[PIPE_WRITE];
    session->controller_id             = controller_id;
    session->network_id                = network_id;
    session->network_type              = ctrlm_network_type_get(network_id);
    session->session_active_server     = true;
    session->session_active_controller = true;
    session->controller_name           = controller_name;
    session->controller_version_sw     = sw_version;
    session->controller_version_hw     = hw_version;
    session->controller_voltage        = voltage;
    session->controller_command_status = command_status;
    session->audio_sent_bytes          = 0;
    session->audio_sent_samples        = 0;
    session->packets_processed         = 0;
    session->packets_lost              = 0;
    session->requested_more_audio      = false;
    if(stream_params == NULL) {
        session->has_stream_params  = false;
    } else {
        session->has_stream_params  = true;
        session->stream_params      = *stream_params;

        xrsr_session_keyword_info_set(XRSR_SRC_RCU_FF, stream_params->pre_keyword_sample_qty, stream_params->keyword_sample_qty);
    }

    if(CTRLM_VOICE_DEVICE_PTT == session->voice_device) {
        errno_t safec_rc = memset_s(&session->status, sizeof(session->status), 0, sizeof(session->status));
        ERR_CHK(safec_rc);
        session->status.controller_id  = session->controller_id;
        this->voice_status_set(session);
        session->last_cmd_id           = 0;
        session->next_cmd_id           = 0;
        session->lqi_total             = 0;
        session->stats_session_id      = voice_session_id_get() + 1;
        memset(&session->stats_reboot, 0, sizeof(session->stats_reboot));
        memset(&session->stats_session, 0, sizeof(session->stats_session));
        session->stats_session.dropped_retry = ULONG_MAX; // Used to indicate whether controller provides stats or not

        #ifdef VOICE_BUFFER_STATS
        voice_buffer_warning_triggered = 0;
        voice_buffer_high_watermark    = 0;
        voice_packet_interval          = voice_packet_interval_get(session->format, this->opus_samples_per_packet);
        #ifdef TIMING_START_TO_FIRST_FRAGMENT
        ctrlm_timestamp_get(&voice_session_begin_timestamp);
        #endif
        #endif

        // Start packet timeout, but not if this is a voice session by text or file
        if(!session->is_session_by_text && !session->is_session_by_file) {
            if(session->network_type == CTRLM_NETWORK_TYPE_IP || session->is_session_by_fifo) {
                session->timeout_packet_tag = g_timeout_add(3000, ctrlm_voice_packet_timeout, NULL);
            } else {
                session->timeout_packet_tag = g_timeout_add(this->prefs.timeout_packet_initial, ctrlm_voice_packet_timeout, NULL);
            }
        }
    }
    if(timestamp != NULL) {
       session->session_timing.ctrl_request = *timestamp;

       // set response time to 50 ms in case transmission confirm callback doesn't work
       session->session_timing.ctrl_response = *timestamp;
       rdkx_timestamp_add_ms(&session->session_timing.ctrl_response, 50);
    }
    if(cb_confirm != NULL && cb_confirm_param != NULL) {
       *cb_confirm       = ctrlm_voice_session_response_confirm;
       *cb_confirm_param = NULL;
    }

    // Post
    sem_post(&this->vsr_semaphore);

    XLOGD_DEBUG("Voice session acquired <%d, %d, %s> pipe wr <%d> rd <%d>", network_id, controller_id, ctrlm_voice_format_str(format), session->audio_pipe[PIPE_WRITE], session->audio_pipe[PIPE_READ]);
    return (this->prefs.par_voice_enabled) ? VOICE_SESSION_RESPONSE_AVAILABLE_PAR_VOICE : VOICE_SESSION_RESPONSE_AVAILABLE;
}

bool ctrlm_voice_t::voice_session_term(std::string &session_id) {
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];

      if(CTRLM_VOICE_STATE_SRC_INVALID == session->state_src) {
          XLOGD_ERROR("Voice is not ready");
          return(false);
      }

      if(session->ipc_common_data.session_id_server == session_id) {
          if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING || session->state_dst != CTRLM_VOICE_STATE_DST_READY) {
             // Cancel current speech router session
             XLOGD_INFO("session id <%s> src <%s> dst <%s>", session_id.c_str(), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
             xrsr_session_terminate(voice_device_to_xrsr(session->voice_device));
             return(true);
          } else {
             XLOGD_WARN("session id <%s> src <%s> dst <%s> not active", session_id.c_str(), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
             return(false);
          }
      }
   }
   XLOGD_WARN("session id <%s> not found", session_id.c_str());
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      XLOGD_WARN("session id <%s> src <%s> dst <%s>", session->ipc_common_data.session_id_server.c_str(), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
   }
   return(false);
}

bool ctrlm_voice_t::voice_session_audio_stream_start(std::string &session_id) {
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];

      if(CTRLM_VOICE_STATE_SRC_INVALID == session->state_src) {
          XLOGD_ERROR("Voice is not ready");
          return(false);
      }

      if(session->ipc_common_data.session_id_server == session_id) {
         if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING) {
            XLOGD_WARN("session id <%s> src <%s> dst <%s> already streaming audio", session_id.c_str(), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
            return(false);
         }
         // Start another audio stream in the current speech router session
         XLOGD_INFO("session id <%s> device <%s> src <%s> dst <%s>", session_id.c_str(), ctrlm_voice_device_str(session->voice_device), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
         xrsr_session_audio_stream_start(voice_device_to_xrsr(session->voice_device));
         session->requested_more_audio = true;
         return(true);
      }
   }
   XLOGD_WARN("session id <%s> not found", session_id.c_str());
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      XLOGD_WARN("session id <%s> src <%s> dst <%s>", session->ipc_common_data.session_id_server.c_str(), ctrlm_voice_state_src_str(session->state_src), ctrlm_voice_state_dst_str(session->state_dst));
   }
   return(false);
}

void ctrlm_voice_t::voice_session_rsp_confirm(bool result, signed long long rsp_time, unsigned int rsp_window, const std::string &err_str, ctrlm_timestamp_t *timestamp) {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT]; // this is for PTT only
   if(session->state_src != CTRLM_VOICE_STATE_SRC_STREAMING) {
       if(ctrlm_main_get_power_state() == CTRLM_POWER_STATE_DEEP_SLEEP) {
           XLOGD_WARN("missed voice session response window after waking from <%s>", ctrlm_power_state_str(ctrlm_main_get_power_state()));
       } else {
           XLOGD_ERROR("No voice session in progress");
       }
       return;
   }
   if(!result) {
       XLOGD_TELEMETRY("failed to send voice session response");
       session->current_vsr_err_rsp_time   = rsp_time;
       session->current_vsr_err_rsp_window = rsp_window;
       session->current_vsr_err_string     = err_str;
       return;
   }

   // Session response transmission is confirmed
   if(timestamp == NULL) {
      rdkx_timestamp_get_realtime(&session->session_timing.ctrl_response);
   } else {
      session->session_timing.ctrl_response = *timestamp;
   }
}

static void ctrlm_voice_data_post_processing_cb(int bytes_sent, void *user_data) {
    if(NULL != user_data) {
        ((ctrlm_voice_t *)user_data)->voice_session_data_post_processing(bytes_sent, "sent", NULL);
    } else {
        XLOGD_ERROR("voice object NULL");
    }
}

void ctrlm_voice_t::voice_session_data_post_processing(int bytes_sent, const char *action, ctrlm_timestamp_t *timestamp) {
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];  // This is for PTT only

    if(session->timeout_packet_tag > 0) {
        g_source_remove(session->timeout_packet_tag);
        // QOS timeout handled at end of this function as it depends on time stamps and samples transmitted
        if(!this->controller_supports_qos(session->voice_device) && bytes_sent != 0) {
            if(session->network_type == CTRLM_NETWORK_TYPE_IP || session->is_session_by_fifo) {
                session->timeout_packet_tag = g_timeout_add(3000, ctrlm_voice_packet_timeout, NULL);
            } else {
                 session->timeout_packet_tag = g_timeout_add(this->prefs.timeout_packet_subsequent, ctrlm_voice_packet_timeout, NULL);
            }
        }
    }
    if(bytes_sent == 0) { // Signals end of stream.  Just stop interpacket timer above...
       XLOGD_INFO("Audio end of stream");
       return;
    }
    if(bytes_sent < 0) {
       XLOGD_INFO("Audio read error <%d>", bytes_sent);
       return;
    }

    #ifdef VOICE_BUFFER_STATS
    ctrlm_timestamp_t before;
    ctrlm_timestamp_get(&before);
    #ifdef TIMING_LAST_FRAGMENT_TO_STOP
    voice_session_last_fragment_timestamp = before;
    #endif

    if(session->audio_sent_bytes == 0) {
        first_fragment = before; // timestamp for start of utterance voice data
        #ifdef TIMING_START_TO_FIRST_FRAGMENT
        XLOGD_INFO("Session Start to First Fragment: %8lld ms lag", ctrlm_timestamp_subtract_ms(voice_session_begin_timestamp, first_fragment));
        #endif
    }

    unsigned long long session_time = this->voice_packet_interval + ctrlm_timestamp_subtract_us(first_fragment, before); // in microseconds

    if(session->network_type == CTRLM_NETWORK_TYPE_BLUETOOTH_LE) { // BLE pipes data directly so this is the first point to increment packet counter.  Lost packet count not available (TODO)
       session->packets_processed++;
    }

    // The total packets (received + lost)
    uint32_t packets_total = session->packets_processed + session->packets_lost;
    long long session_delta = (session_time - ((packets_total - 1) * this->voice_packet_interval)); // in microseconds
    unsigned long watermark = (session_delta / this->voice_packet_interval) + 1;
    if(session_delta > 0 && watermark > voice_buffer_high_watermark) {
        voice_buffer_high_watermark = watermark;
    }
    if(session_delta > (long long)(VOICE_BUFFER_WARNING_THRESHOLD * this->voice_packet_interval)) {
        voice_buffer_warning_triggered = 1;
    }
    #endif

    if(timestamp != NULL) {
        if(session->audio_sent_bytes == 0) {
            session->session_timing.ctrl_audio_rxd_first = *timestamp;
            session->session_timing.ctrl_audio_rxd_final = *timestamp;
        } else {
            session->session_timing.ctrl_audio_rxd_final = *timestamp;
        }
    }

    session->audio_sent_bytes += bytes_sent;

    // Handle input format
    if(session->format.type == CTRLM_VOICE_FORMAT_OPUS_XVP) {
       session->audio_sent_samples += this->opus_samples_per_packet; // From opus encoding parameters
    } else if(session->format.type == CTRLM_VOICE_FORMAT_ADPCM_FRAME) {
       session->audio_sent_samples += (bytes_sent - session->format.value.adpcm_frame.size_header) * 2; // N byte header, one nibble per sample
    } else { // PCM
       session->audio_sent_samples += bytes_sent >> 1; // 16 bit samples
    }

    if(session->has_stream_params && !session->stream_params.push_to_talk && !session->session_timing.has_keyword) {
       if(session->audio_sent_samples >= (session->stream_params.pre_keyword_sample_qty + session->stream_params.keyword_sample_qty) && timestamp != NULL) {
          session->session_timing.ctrl_audio_rxd_keyword = *timestamp;
          session->session_timing.has_keyword = true;
       }
    }
    if(controller_supports_qos(session->voice_device)) {
       guint32 timeout = (session->audio_sent_samples * 2 * 8) / this->prefs.bitrate_minimum;
       signed long long elapsed = rdkx_timestamp_subtract_ms(session->session_timing.ctrl_audio_rxd_first, session->session_timing.ctrl_audio_rxd_final);

       timeout -= elapsed;

       if(elapsed + timeout < this->prefs.time_threshold) {
          timeout = this->prefs.time_threshold - elapsed;
       } else if(timeout > 2000) { // Cap at 2 seconds - effectively interpacket timeout
          timeout = 2000;
       }
       float rx_rate = (elapsed == 0) ? 0 : (session->audio_sent_samples * 2 * 8) / elapsed;

       session->timeout_packet_tag = g_timeout_add(timeout, ctrlm_voice_packet_timeout, NULL);
       XLOGD_INFO("Audio %s bytes <%lu> samples <%lu> rate <%6.2f kbps> timeout <%lu ms>", action, session->audio_sent_bytes, session->audio_sent_samples, rx_rate, timeout);
    } else {
       #ifdef VOICE_BUFFER_STATS
       if(voice_buffer_warning_triggered) {
          XLOGD_WARN("Audio %s bytes <%lu> samples <%lu> pkt cnt <%3u> elapsed <%8llu ms> lag <%8lld ms> (%4.2f packets)", action, session->audio_sent_bytes, session->audio_sent_samples, packets_total, session_time / 1000, session_delta / 1000, (((float)session_delta) / this->voice_packet_interval));
       } else {
          XLOGD_INFO("Audio %s bytes <%lu> samples <%lu>", action, session->audio_sent_bytes, session->audio_sent_samples);
       }
       #else
       XLOGD_INFO("Audio %s bytes <%lu> samples <%lu>", action, session->audio_sent_bytes, session->audio_sent_samples);
       #endif
    }
}

bool ctrlm_voice_t::voice_session_data(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int fd, const uuid_t *uuid) {
    bool ret = false;

    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT]; // This is for PTT only

    if(network_id != session->network_id || controller_id != session->controller_id) {
        XLOGD_ERROR("Data from wrong controller, ignoring");
        return(false); 
    }
    if(!session->is_session_by_text && !session->is_session_by_file) {
        if(false == xrsr_session_audio_fd_set(voice_device_to_xrsr(session->voice_device), fd, voice_format_to_xrsr(session->format), ctrlm_voice_data_post_processing_cb, (void *)this)) {
            XLOGD_ERROR("Failed setting post data read callback to ctrlm");
            return(false); 
        } else {
            if(session->audio_pipe[PIPE_READ] >= 0) {
                XLOGD_INFO("Closing previous pipe - READ fd <%d>", session->audio_pipe[PIPE_READ]);
                close(session->audio_pipe[PIPE_READ]);
                session->audio_pipe[PIPE_READ] = -1;
            }
            if(session->audio_pipe[PIPE_WRITE] >= 0) {
                XLOGD_INFO("Closing previous pipe - WRITE fd <%d>", session->audio_pipe[PIPE_WRITE]);
                close(session->audio_pipe[PIPE_WRITE]);
                session->audio_pipe[PIPE_WRITE] = -1;
            }

            XLOGD_INFO("Setting read pipe to fd <%d> and beginning session...", fd);
            session->audio_pipe[PIPE_READ] = fd;
            ret = true;
        }
    }
    return ret;
}

bool ctrlm_voice_t::voice_session_data(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, const char *buffer, long unsigned int length, ctrlm_timestamp_t *timestamp, uint8_t *lqi) {
    char              local_buf[length + 1];
    long unsigned int bytes_written = 0;
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT]; // This is for PTT only
    if(session->state_src != CTRLM_VOICE_STATE_SRC_STREAMING) {
        XLOGD_ERROR("No voice session in progress");
        return(false);
    } else if(network_id != session->network_id || controller_id != session->controller_id) {
        XLOGD_ERROR("Data from wrong controller, ignoring");
        return(false); 
    } else if(session->audio_pipe[PIPE_WRITE] < 0) {
        XLOGD_ERROR("Pipe doesn't exist");
        return(false);
    } else if(length <= 0) {
        XLOGD_ERROR("Length is <= 0");
        return(false);
    }

    uint8_t cmd_id = buffer[0];

    if(session->network_type == CTRLM_NETWORK_TYPE_RF4CE) {
        if(session->last_cmd_id == cmd_id) {
            XLOGD_INFO("RF4CE Duplicate Voice Packet");
            if(session->timeout_packet_tag > 0 && !controller_supports_qos(session->voice_device)) {
               g_source_remove(session->timeout_packet_tag);
               session->timeout_packet_tag = g_timeout_add(this->prefs.timeout_packet_subsequent, ctrlm_voice_packet_timeout, NULL);
            }
            return(true);
        }

        // Maintain a running packet received and lost count (which is overwritten by post session stats)
        session->packets_processed++;
        if(session->next_cmd_id != 0 && session->next_cmd_id != cmd_id) {
           session->packets_lost += (cmd_id > session->next_cmd_id) ? (cmd_id - session->next_cmd_id) : (ADPCM_COMMAND_ID_MAX + 1 - session->next_cmd_id) + (cmd_id - ADPCM_COMMAND_ID_MIN);
        }

        session->last_cmd_id = cmd_id;
        session->next_cmd_id = session->last_cmd_id + 1;
        if(session->next_cmd_id > ADPCM_COMMAND_ID_MAX) {
            session->next_cmd_id = ADPCM_COMMAND_ID_MIN;
        }
    }

    const char *action = "dumped";
    if(session->state_dst != CTRLM_VOICE_STATE_DST_READY) { // destination is accepting more data
       action = "sent";
       if(session->format.type == CTRLM_VOICE_FORMAT_OPUS || session->format.type == CTRLM_VOICE_FORMAT_OPUS_XVP) {
           // Copy to local buffer to perform a single write to the pipe.  TODO: have the caller reserve 1 bytes at the beginning
           // of the buffer to eliminate this copy
           local_buf[0] = length;
           errno_t safec_rc = memcpy_s(&local_buf[1], sizeof(local_buf)-1, buffer, length);
           ERR_CHK(safec_rc);
           buffer = local_buf;
           length++;
       }

       bytes_written = write(session->audio_pipe[PIPE_WRITE], buffer, length);
       if(bytes_written != length) {
           XLOGD_ERROR("Failed to write data to pipe: %s", strerror(errno));
           return(false);
       }
    }

    if(lqi != NULL) {
        session->lqi_total += *lqi;
    }

    voice_session_data_post_processing(length, action, timestamp);

    return(true);
}

void ctrlm_voice_t::voice_session_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_voice_session_end_reason_t reason, ctrlm_timestamp_t *timestamp, ctrlm_voice_session_end_stats_t *stats) {
   ctrlm_voice_session_t *session = voice_session_from_controller(network_id, controller_id);

   if(session == NULL) {
      XLOGD_ERROR("session not found");
      return;
   }
   this->voice_session_end(session, reason, timestamp, stats);
}

void ctrlm_voice_t::voice_session_end(ctrlm_voice_session_t *session, ctrlm_voice_session_end_reason_t reason, ctrlm_timestamp_t *timestamp, ctrlm_voice_session_end_stats_t *stats) {
    XLOGD_TELEMETRY("voice session end < %s >", ctrlm_voice_session_end_reason_str(reason));
    if(session->state_src != CTRLM_VOICE_STATE_SRC_STREAMING) {
        XLOGD_ERROR("No voice session in progress");
        return;
    }

    session->end_reason = reason;

    if(timestamp != NULL) {
       session->session_timing.ctrl_stop = *timestamp;
    } else {
       rdkx_timestamp_get_realtime(&session->session_timing.ctrl_stop);
    }

    // Stop packet timeout
    if(session->timeout_packet_tag > 0) {
        g_source_remove(session->timeout_packet_tag);
        session->timeout_packet_tag = 0;
    }

    if(session->audio_pipe[PIPE_WRITE] >= 0) {
        XLOGD_INFO("Close write pipe - fd <%d>", session->audio_pipe[PIPE_WRITE]);
        close(session->audio_pipe[PIPE_WRITE]);
        session->audio_pipe[PIPE_WRITE] = -1;
    }

    // Send main queue message
    ctrlm_main_queue_msg_voice_session_end_t end = {0};

    end.controller_id       = session->controller_id;
    end.reason              = reason;
    end.utterance_too_short = (session->audio_sent_bytes == 0 ? 1 : 0);
    // Don't need to fill out other info
    if(stats != NULL) {
       session->stats_session.rf_channel       = stats->rf_channel;
        #ifdef VOICE_BUFFER_STATS
        #ifdef TIMING_LAST_FRAGMENT_TO_STOP
        ctrlm_timestamp_t after;
        ctrlm_timestamp_get(&after);
        unsigned long long lag_time = ctrlm_timestamp_subtract_ns(voice_session_last_fragment_timestamp, after);
        XLOGD_INFO("Last Fragment to Session Stop: %8llu ms lag", lag_time / 1000000);
        #endif
        session->stats_session.buffer_watermark  = voice_buffer_high_watermark;
        #else
        session->stats_session.buffer_watermark  = ULONG_MAX;
        #endif
    }
    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::ind_process_voice_session_end, &end, sizeof(end), NULL, session->network_id);

    // clear session_active_controller for controllers that don't support voice command status
    if(!session->controller_command_status) {
       session->session_active_controller = false;
    }

    // Set a timeout for receiving voice session stats from the controller
    this->timeout_ctrl_session_stats_rxd = g_timeout_add(this->prefs.timeout_stats, ctrlm_voice_controller_session_stats_rxd_timeout, NULL);

    XLOGD_DEBUG("session_active_server = <%d>, session_active_controller = <%d>", session->session_active_server, session->session_active_controller);

    // Update source state
    if(session->session_active_controller) {
       session->state_src = CTRLM_VOICE_STATE_SRC_WAITING;
    } else {
       session->state_src = CTRLM_VOICE_STATE_SRC_READY;
       if(!session->session_active_server) {
          voice_session_stats_print(session);
          voice_session_stats_clear(session);
       }
    }

    if(this->nsm_voice_session) {
        this->nsm_voice_session = false;
        if(!ctrlm_power_state_change(CTRLM_POWER_STATE_ON)) {
            XLOGD_ERROR("failed to set power state!");
        }
    }
}

void ctrlm_voice_t::voice_session_controller_stats_rxd_timeout() {
   this->timeout_ctrl_session_stats_rxd = 0;
   ctrlm_get_voice_obj()->voice_session_notify_stats();
}

void ctrlm_voice_t::voice_session_stats(ctrlm_voice_stats_session_t session) {
   ctrlm_voice_session_t *voice_session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];  //
   voice_session->stats_session.available      = session.available;
   voice_session->stats_session.packets_total  = session.packets_total;
   voice_session->stats_session.dropped_retry  = session.dropped_retry;
   voice_session->stats_session.dropped_buffer = session.dropped_buffer;
   voice_session->stats_session.retry_mac      = session.retry_mac;
   voice_session->stats_session.retry_network  = session.retry_network;
   voice_session->stats_session.cca_sense      = session.cca_sense;

   voice_session_notify_stats();
}

void ctrlm_voice_t::voice_session_stats(ctrlm_voice_stats_reboot_t reboot) {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
   session->stats_reboot = reboot;

   // Send the notification
   voice_session_notify_stats();
}

void ctrlm_voice_t::voice_session_notify_stats() {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
   if(session->stats_session_id == 0) {
      XLOGD_INFO("already sent, ignoring.");
      return;
   }
   if(session->stats_session_id != session->ipc_common_data.session_id_ctrlm) {
      XLOGD_INFO("stale session, ignoring.");
      return;
   }

   // Stop session stats timeout
   if(this->timeout_ctrl_session_stats_rxd > 0) {
      g_source_remove(this->timeout_ctrl_session_stats_rxd);
      this->timeout_ctrl_session_stats_rxd = 0;
   }

   // Send session stats only if the server voice session has ended
   if(!session->session_active_server && this->voice_ipc) {
      ctrlm_voice_ipc_event_session_statistics_t stats;

      stats.common  = session->ipc_common_data;
      stats.session = session->stats_session;
      stats.reboot  = session->stats_reboot;

      this->voice_ipc->session_statistics(stats);
      session->stats_session_id = 0;
   }
}

void ctrlm_voice_t::voice_session_timeout() {
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
   session->timeout_packet_tag = 0;
   ctrlm_voice_session_end_reason_t reason = CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_INTERPACKET;
   if(session->audio_sent_bytes == 0) {
      reason = CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_FIRST_PACKET;
   } else if(controller_supports_qos(session->voice_device)) {
      // Bitrate = transmitted PCM data size / elapsed time

      ctrlm_timestamp_t timestamp;
      rdkx_timestamp_get_realtime(&timestamp);
      signed long long elapsed = rdkx_timestamp_subtract_ms(session->session_timing.ctrl_audio_rxd_first, timestamp);
      float rx_rate = (elapsed == 0) ? 0 : (session->audio_sent_samples * 2 * 8) / elapsed;

      XLOGD_INFO("elapsed time <%llu> ms rx samples <%u> rate <%6.1f> kbps", elapsed, session->audio_sent_samples, rx_rate);
      reason = CTRLM_VOICE_SESSION_END_REASON_MINIMUM_QOS;
   }
   XLOGD_INFO("%s", ctrlm_voice_session_end_reason_str(reason));
   this->voice_session_end(session, reason);
}

void ctrlm_voice_t::voice_session_stats_clear(ctrlm_voice_session_t *session) {
   ctrlm_voice_session_timing_t *timing = &session->session_timing;

   timing->available              = false;
   timing->connect_attempt        = false;
   timing->connect_success        = false;
   timing->has_keyword            = false;
   timing->ctrl_request           = { 0, 0 };
   timing->ctrl_response          = { 0, 0 };
   timing->ctrl_audio_rxd_first   = { 0, 0 };
   timing->ctrl_audio_rxd_keyword = { 0, 0 };
   timing->ctrl_audio_rxd_final   = { 0, 0 };
   timing->ctrl_stop              = { 0, 0 };
   timing->ctrl_cmd_status_wr     = { 0, 0 };
   timing->ctrl_cmd_status_rd     = { 0, 0 };
   timing->srvr_request           = { 0, 0 };
   timing->srvr_connect           = { 0, 0 };
   timing->srvr_init_txd          = { 0, 0 };
   timing->srvr_audio_txd_first   = { 0, 0 };
   timing->srvr_audio_txd_keyword = { 0, 0 };
   timing->srvr_audio_txd_final   = { 0, 0 };
   timing->srvr_rsp_keyword       = { 0, 0 };
   timing->srvr_disconnect        = { 0, 0 };
}

void ctrlm_voice_t::voice_session_stats_print(ctrlm_voice_session_t *session) {
   ctrlm_voice_session_timing_t *timing = &session->session_timing;
   errno_t safec_rc = -1;
   if(!timing->available) {
      XLOGD_INFO("not available");
      return;
   }

   signed long long ctrl_response, ctrl_audio_rxd_first, ctrl_audio_rxd_final, ctrl_audio_rxd_stop, ctrl_cmd_status_wr;
   ctrl_response        = rdkx_timestamp_subtract_us(timing->ctrl_request,         timing->ctrl_response);
   ctrl_audio_rxd_first = rdkx_timestamp_subtract_us(timing->ctrl_response,        timing->ctrl_audio_rxd_first);

   ctrl_cmd_status_wr   = rdkx_timestamp_subtract_us(timing->ctrl_request,         timing->ctrl_cmd_status_wr);
   

   char str_keyword[40];
   str_keyword[0] = '\0';
   if(timing->has_keyword) { // Far field with keyword
      signed long long ctrl_audio_rxd_kwd;

      ctrl_audio_rxd_kwd   = rdkx_timestamp_subtract_us(timing->ctrl_audio_rxd_first,   timing->ctrl_audio_rxd_keyword);
      ctrl_audio_rxd_final = rdkx_timestamp_subtract_us(timing->ctrl_audio_rxd_keyword, timing->ctrl_audio_rxd_final);
      safec_rc = sprintf_s(str_keyword, sizeof(str_keyword), "keyword <%lld> ", ctrl_audio_rxd_kwd);
      if(safec_rc < EOK) {
        ERR_CHK(safec_rc);
      }
   } else {
      ctrl_audio_rxd_final = rdkx_timestamp_subtract_us(timing->ctrl_audio_rxd_first, timing->ctrl_audio_rxd_final);
   }
   ctrl_audio_rxd_stop = rdkx_timestamp_subtract_us(timing->ctrl_audio_rxd_final, timing->ctrl_stop);

   XLOGD_INFO("ctrl rsp <%lld> first <%lld> %sfinal <%lld> stop <%lld> us", ctrl_response, ctrl_audio_rxd_first, str_keyword, ctrl_audio_rxd_final, ctrl_audio_rxd_stop);
   XLOGD_INFO("ctrl cmd status wr <%lld> us", ctrl_cmd_status_wr);

   if(timing->connect_attempt) { // Attempted connection
      signed long long srvr_request, srvr_init_txd, srvr_disconnect;
      srvr_request = rdkx_timestamp_subtract_us(timing->ctrl_request, timing->srvr_request);

      if(!timing->connect_success) { // Did not connect
         srvr_disconnect = rdkx_timestamp_subtract_us(timing->srvr_request, timing->srvr_disconnect);
         XLOGD_INFO("srvr request <%lld> disconnect <%lld> us", srvr_request, srvr_disconnect);
      } else { // Connected
         signed long long srvr_connect, srvr_audio_txd_first, srvr_audio_txd_final;
         srvr_connect         = rdkx_timestamp_subtract_us(timing->srvr_request,  timing->srvr_connect);
         srvr_init_txd        = rdkx_timestamp_subtract_us(timing->srvr_connect,  timing->srvr_init_txd);
         srvr_audio_txd_first = rdkx_timestamp_subtract_us(timing->srvr_init_txd, timing->srvr_audio_txd_first);

         if(timing->has_keyword) { // Far field with keyword
            signed long long srvr_audio_txd_kwd, srvr_audio_kwd_verify;

            srvr_audio_txd_kwd    = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_first,   timing->srvr_audio_txd_keyword);
            srvr_audio_txd_final  = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_keyword, timing->srvr_audio_txd_final);
            srvr_audio_kwd_verify = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_keyword, timing->srvr_rsp_keyword);
            safec_rc = sprintf_s(str_keyword, sizeof(str_keyword), "keyword <%lld> verify <%lld> ", srvr_audio_txd_kwd, srvr_audio_kwd_verify);
            if(safec_rc < EOK) {
              ERR_CHK(safec_rc);
            }
         } else {
            srvr_audio_txd_final = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_first, timing->srvr_audio_txd_final);
         }
         srvr_disconnect = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_final, timing->srvr_disconnect);

         XLOGD_INFO("srvr request <%lld> connect <%lld> init <%lld> first <%lld> %sfinal <%lld> disconnect <%lld> us", srvr_request, srvr_connect, srvr_init_txd, srvr_audio_txd_first, str_keyword, srvr_audio_txd_final, srvr_disconnect);

         if(timing->has_keyword) {
            signed long long kwd_total, kwd_transmit, kwd_response;

            kwd_total    = rdkx_timestamp_subtract_us(timing->ctrl_request, timing->srvr_rsp_keyword);
            kwd_transmit = rdkx_timestamp_subtract_us(timing->ctrl_request, timing->srvr_audio_txd_keyword);
            kwd_response = rdkx_timestamp_subtract_us(timing->srvr_audio_txd_keyword, timing->srvr_rsp_keyword);
            XLOGD_INFO("keyword latency total <%lld> transmit <%lld> response <%lld> us", kwd_total, kwd_transmit, kwd_response);
         }
      }
   }
}

void ctrlm_voice_t::voice_session_info(xrsr_src_t src, ctrlm_voice_session_info_t *data) {
   ctrlm_voice_session_t *session = &this->voice_session[voice_device_to_session_group(xrsr_to_voice_device(src))];
   this->voice_session_info(session, data);
}

void ctrlm_voice_t::voice_session_info(ctrlm_voice_session_t *session, ctrlm_voice_session_info_t *data) {
    if(data) {
        data->controller_name       = session->controller_name;
        data->controller_version_sw = session->controller_version_sw;
        data->controller_version_hw = session->controller_version_hw;
        data->controller_voltage    = session->controller_voltage;
        data->stb_name              = this->stb_name;
        data->ffv_leading_samples   = this->prefs.ffv_leading_samples;
        data->has_stream_params     = session->has_stream_params;
        if(data->has_stream_params) {
            data->stream_params = session->stream_params;
        }
        data->rf_protocol           = ctrlm_network_type_str(session->network_type);
    }
}

void ctrlm_voice_t::voice_session_info_reset(ctrlm_voice_session_t *session) {
    session->controller_name        = "";
    session->controller_version_sw  = "";
    session->controller_version_hw  = "";
    session->controller_voltage     = 0.0;
    session->has_stream_params      = false;
}

ctrlm_voice_state_src_t ctrlm_voice_t::voice_state_src_get(ctrlm_voice_session_group_t group) const {
    if(group >= VOICE_SESSION_GROUP_QTY) {
        XLOGD_ERROR("session index out of range <%u>", group);
        return(this->voice_session[0].state_src);
    }
    return(this->voice_session[group].state_src);
}

ctrlm_voice_state_dst_t ctrlm_voice_t::voice_state_dst_get(ctrlm_voice_session_group_t group) const {
    if(group >= VOICE_SESSION_GROUP_QTY) {
        XLOGD_ERROR("session index out of range <%u>", group);
        return(this->voice_session[0].state_dst);
    }
    return(this->voice_session[group].state_dst);
}

void ctrlm_voice_t::voice_stb_data_stb_sw_version_set(std::string &sw_version) {
    XLOGD_DEBUG("STB version set to %s", sw_version.c_str());
    this->software_version = sw_version;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_stb_sw_version_set(sw_version);
    }
}

std::string ctrlm_voice_t::voice_stb_data_stb_sw_version_get() const {
    return this->software_version;
}

void ctrlm_voice_t::voice_stb_data_stb_name_set(std::string &stb_name) {
    XLOGD_DEBUG("STB name set to %s", stb_name.c_str());
    this->stb_name = stb_name;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_stb_name_set(stb_name);
    }
}

std::string ctrlm_voice_t::voice_stb_data_stb_name_get() const {
    return(this->stb_name);
}

void ctrlm_voice_t::voice_stb_data_account_number_set(std::string &account_number) {
    XLOGD_DEBUG("Account number set to %s", account_number.c_str());
    this->account_number = account_number;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_account_number_set(account_number);
    }
}

std::string ctrlm_voice_t::voice_stb_data_account_number_get() const {
    return(this->account_number);
}

void ctrlm_voice_t::voice_stb_data_receiver_id_set(std::string &receiver_id) {
    XLOGD_DEBUG("Receiver ID set to %s", receiver_id.c_str());
    this->receiver_id = receiver_id;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_receiver_id_set(receiver_id);
    }
}

std::string ctrlm_voice_t::voice_stb_data_receiver_id_get() const {
    return(this->receiver_id);
}

void ctrlm_voice_t::voice_stb_data_device_id_set(std::string &device_id) {
    XLOGD_DEBUG("Device ID set to %s", device_id.c_str());
    this->device_id = device_id;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_device_id_set(device_id);
    }
}

std::string ctrlm_voice_t::voice_stb_data_device_id_get() const {
    return(this->device_id);
}

void ctrlm_voice_t::voice_stb_data_device_type_set(ctrlm_device_type_t device_type) {
    XLOGD_DEBUG("Device Type set to %s", ctrlm_device_type_str(device_type));
    this->device_type = device_type;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_device_type_set(device_type);
    }
}

ctrlm_device_type_t ctrlm_voice_t::voice_stb_data_device_type_get() const {
    return(this->device_type);
}

void ctrlm_voice_t::voice_stb_data_partner_id_set(std::string &partner_id) {
    XLOGD_DEBUG("Partner ID set to %s", partner_id.c_str());
    this->partner_id = partner_id;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_partner_id_set(partner_id);
    }
}

std::string ctrlm_voice_t::voice_stb_data_partner_id_get() const {
    return(this->partner_id);
}
 
void ctrlm_voice_t::voice_stb_data_experience_set(std::string &experience) {
    XLOGD_DEBUG("Experience Tag set to %s", experience.c_str());
    this->experience = experience;
    for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_experience_set(experience);
    }
}

std::string ctrlm_voice_t::voice_stb_data_experience_get() const {
    return(this->experience);
}

std::string ctrlm_voice_t::voice_stb_data_app_id_http_get() const {
    return(this->prefs.app_id_http);
}

std::string ctrlm_voice_t::voice_stb_data_app_id_ws_get() const {
    return(this->prefs.app_id_ws);
}

void ctrlm_voice_t::voice_stb_data_guide_language_set(const char *language) {
   XLOGD_DEBUG("Guide language set to %s", language);
   this->prefs.guide_language = language;
   for(const auto &itr : this->endpoints) {
        itr->voice_stb_data_guide_language_set(language);
   }
}

std::string ctrlm_voice_t::voice_stb_data_guide_language_get() const {
   return(this->prefs.guide_language);
}

void ctrlm_voice_t::voice_stb_data_sat_set(std::string &sat_token) {
    XLOGD_DEBUG("SAT Token set to %s", sat_token.c_str());

    errno_t safec_rc = strcpy_s(this->sat_token, sizeof(this->sat_token), sat_token.c_str());
    ERR_CHK(safec_rc);
}

const char *ctrlm_voice_t::voice_stb_data_sat_get() const {
    return(this->sat_token);
}

bool ctrlm_voice_t::voice_stb_data_test_get() const {
    return(this->prefs.vrex_test_flag);
}

bool ctrlm_voice_t::voice_stb_data_bypass_wuw_verify_success_get() const {
    return(this->prefs.vrex_wuw_bypass_success_flag);
}

bool ctrlm_voice_t::voice_stb_data_bypass_wuw_verify_failure_get() const {
    return(this->prefs.vrex_wuw_bypass_failure_flag);
}

void ctrlm_voice_t::voice_stb_data_pii_mask_set(bool mask_pii) {
   if(this->mask_pii != mask_pii) {
      this->mask_pii = mask_pii;
      if(this->xrsr_opened) {
         xrsr_mask_pii_set(mask_pii);
      }
      this->mask_pii_updated(mask_pii);
   }
}

bool ctrlm_voice_t::voice_stb_data_pii_mask_get() const {
   return(this->mask_pii);
}

bool ctrlm_voice_t::voice_stb_data_device_certificate_set(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca) {
   this->ocsp_verify_stapling = ocsp_verify_stapling;
   this->ocsp_verify_ca       = ocsp_verify_ca;
   if(device_cert.type == CTRLM_VOICE_CERT_TYPE_P12) {
      return(this->voice_stb_data_device_certificate_set(device_cert.cert.p12.certificate, device_cert.cert.p12.passphrase));
   }
   if(device_cert.type == CTRLM_VOICE_CERT_TYPE_PEM) {
      return(this->voice_stb_data_device_certificate_set(device_cert.cert.pem.filename_cert, device_cert.cert.pem.filename_pkey, device_cert.cert.pem.filename_chain, device_cert.cert.pem.passphrase));
   }
   if(device_cert.type == CTRLM_VOICE_CERT_TYPE_X509) {
      return(this->voice_stb_data_device_certificate_set(device_cert.cert.x509.cert_x509, device_cert.cert.x509.cert_pkey, device_cert.cert.x509.cert_chain));
   }
   this->ocsp_verify_stapling = false;
   this->ocsp_verify_ca       = false;
   if(device_cert.type == CTRLM_VOICE_CERT_TYPE_NONE) {
      XLOGD_WARN("NO certificate specified");
      return(true);
   }

   XLOGD_ERROR("invalid certificate type specified <%s>", ctrlm_voice_cert_type_str(device_cert.type));
   return(false);
}

bool ctrlm_voice_t::voice_stb_data_device_certificate_set(const char *p12_cert, const char *p12_pass) {
   if(p12_cert == NULL) {
      XLOGD_ERROR("invalid params");
      return(false);
   }

   this->device_cert.type = CTRLM_VOICE_CERT_TYPE_P12;
   this->device_cert.cert.p12.certificate = p12_cert;
   this->device_cert.cert.p12.passphrase  = p12_pass;

   XLOGD_INFO("P12 certificate <%s>", p12_cert);
   return(true);
}

bool ctrlm_voice_t::voice_stb_data_device_certificate_set(const char *pem_cert, const char *pem_pkey, const char *pem_chain, const char *pem_passphrase) {
   if(pem_cert == NULL || pem_pkey == NULL) {
      XLOGD_ERROR("invalid params");
      return(false);
   }

   this->device_cert.type = CTRLM_VOICE_CERT_TYPE_PEM;
   this->device_cert.cert.pem.filename_cert  = pem_cert;
   this->device_cert.cert.pem.filename_pkey  = pem_pkey;
   this->device_cert.cert.pem.filename_chain = pem_chain;
   this->device_cert.cert.pem.passphrase     = pem_passphrase;

   XLOGD_INFO("PEM certificate <%s> pkey <%s> chain <%s>", pem_cert, pem_pkey, (pem_chain != NULL) ? pem_chain : "");
   return(true);
}

bool ctrlm_voice_t::voice_stb_data_device_certificate_set(X509 *cert_x509, EVP_PKEY *cert_pkey, STACK_OF(X509) *cert_chain) {
   if(cert_x509 == NULL || cert_pkey != NULL) {
      XLOGD_ERROR("invalid params");
      return(false);
   }

   this->device_cert.type = CTRLM_VOICE_CERT_TYPE_X509;
   this->device_cert.cert.x509.cert_x509  = cert_x509;
   this->device_cert.cert.x509.cert_pkey  = cert_pkey;
   this->device_cert.cert.x509.cert_chain = cert_chain;

   XLOGD_INFO("X509 certificate");
   return(true);
}

void ctrlm_voice_t::voice_stb_data_device_certificate_get(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca) {
   if(!this->mtls_required) {
      device_cert.type = CTRLM_VOICE_CERT_TYPE_NONE;
      return;
   }
   if(this->device_cert.type == CTRLM_VOICE_CERT_TYPE_NONE) {
      XLOGD_ERROR("device cert not available for MTLS");
   }
   device_cert          = this->device_cert;
   ocsp_verify_stapling = this->ocsp_verify_stapling;
   ocsp_verify_ca       = this->ocsp_verify_ca;
}

bool ctrlm_voice_t::voice_session_requires_stb_data(ctrlm_voice_device_t device_type) {
   return(this->device_requires_stb_data[device_type]);
}

bool ctrlm_voice_t::voice_session_has_stb_data() {
#if defined(AUTH_RECEIVER_ID) || defined(AUTH_DEVICE_ID)
    if(this->receiver_id == "" && this->device_id == "") {
        XLOGD_TELEMETRY("No receiver/device id");
        return(false);
    }
#endif
#ifdef AUTH_PARTNER_ID
    if(this->partner_id == "") {
        XLOGD_INFO("No partner id");
        return(false);
    }
#endif
#ifdef AUTH_EXPERIENCE
    if(this->experience == "") {
        XLOGD_INFO("No experience tag");
        return(false);
    }
#endif
#ifdef AUTH_SAT_TOKEN
    if(this->sat_token_required && this->sat_token[0] == '\0') {
        XLOGD_INFO("No SAT token");
        return(false);
    }
#endif
    return(true);
}

unsigned long ctrlm_voice_t::voice_session_id_next() {
    if(this->session_id == 0xFFFFFFFF) {
        this->session_id = 1;
    } else {
        this->session_id++;
    }
    return(this->session_id);
}

void ctrlm_voice_t::voice_session_notify_abort(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long session_id, ctrlm_voice_session_abort_reason_t reason) {
    XLOGD_INFO("voice session abort < %s >", ctrlm_voice_session_abort_reason_str(reason));
    if(this->voice_ipc) {
        ctrlm_voice_ipc_event_session_end_t end;
        // Do not use ipc_common_data attribute, as this function is called before voice session is acquired so it is not accurate.
        end.common.network_id        = network_id;
        end.common.network_type      = ctrlm_network_type_get(network_id);
        end.common.controller_id     = controller_id;
        end.common.session_id_ctrlm  = session_id;
        end.common.session_id_server = "N/A";
        end.result                   = SESSION_END_ABORT;
        end.reason                   = (int)reason;
        this->voice_ipc->session_end(end);
    }

    if(this->nsm_voice_session) {
        this->nsm_voice_session = false;
        if(!ctrlm_power_state_change(CTRLM_POWER_STATE_ON)) {
            XLOGD_ERROR("failed to set power state!");
        }
    }
}
// Application Interface Implementation End

// Callback Interface Implementation
void ctrlm_voice_t::voice_session_begin_callback(ctrlm_voice_session_begin_cb_t *session_begin) {
    if(NULL == session_begin) {
        XLOGD_ERROR("NULL data");
        return;
    }

    // Fetch session based on xrsr source
    ctrlm_voice_session_group_t group = voice_device_to_session_group(xrsr_to_voice_device(session_begin->src));
    ctrlm_voice_session_t *session = &this->voice_session[group];

    if(!uuid_is_null(session_begin->header.uuid)) {
        char uuid_str[37] = {'\0'};
        uuid_unparse_lower(session_begin->header.uuid, uuid_str);
        session->uuid_str  = uuid_str;
        uuid_copy(session->uuid, session_begin->header.uuid);
    }

    uuid_copy(session->uuid, session_begin->header.uuid);
    session->confidence                     = 0;
    session->dual_sensitivity_immediate     = false;
    session->transcription                  = "";
    session->server_message                 = "";
    session->server_ret_code                = 0;
    session->voice_device                   = xrsr_to_voice_device(session_begin->src);
    if(ctrlm_voice_device_is_mic(session->voice_device)) {
       session->network_id       = CTRLM_MAIN_NETWORK_ID_DSP;
       session->controller_id    = CTRLM_MAIN_CONTROLLER_ID_DSP;
       session->network_type     = CTRLM_NETWORK_TYPE_DSP;
       session->keyword_verified = false;
       XLOGD_INFO("src <%s> setting network and controller to DSP", ctrlm_voice_device_str(session->voice_device));
    }
    session->ipc_common_data.network_id        = session->network_id;
    session->ipc_common_data.network_type      = ctrlm_network_type_get(session->network_id);
    session->ipc_common_data.controller_id     = session->controller_id;
    session->ipc_common_data.session_id_ctrlm  = this->voice_session_id_next();
    session->ipc_common_data.session_id_server = session->uuid_str;
    session->ipc_common_data.voice_assistant   = is_voice_assistant(session->voice_device);
    session->ipc_common_data.device_type       = session->voice_device;
    session->endpoint_current               = session_begin->endpoint;
    session->end_reason                     = CTRLM_VOICE_SESSION_END_REASON_DONE;

    errno_t safec_rc = memset_s(&session->status, sizeof(session->status), 0 , sizeof(session->status));
    ERR_CHK(safec_rc);
    session->status.controller_id  = session->controller_id;
    this->voice_status_set(session);

    session->session_timing.srvr_request = session_begin->header.timestamp;
    session->session_timing.connect_attempt = true;

    // Parse the stream params
    if(session->has_stream_params) {
        session->dual_sensitivity_immediate = (!session->stream_params.high_search_pt_support || (session->stream_params.high_search_pt_support && session->stream_params.standard_search_pt_triggered));
        for(int i = 0; i < CTRLM_FAR_FIELD_BEAMS_MAX; i++) {
            if(session->stream_params.beams[i].selected) {
                if(session->stream_params.beams[i].confidence_normalized) {
                    session->confidence = session->stream_params.beams[i].confidence;
                }
            }
        }
    }

    // Check if we should change audio states
    bool kw_verification_required = false;
    if(is_voice_assistant(session->voice_device)) {
        if(this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_VSR || 
          (this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE && session->confidence >= this->audio_confidence_threshold) ||
          (this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_DUAL_SENSITIVITY && session->dual_sensitivity_immediate)) {
            this->voice_keyword_verified_action();
        } else { // Await keyword verification
            kw_verification_required = true;
        }
    }

    // Update device status
    this->voice_session_set_active(session->voice_device);

    // IARM Begin Event
    if(this->voice_ipc) {
        ctrlm_voice_ipc_event_session_begin_t begin;
        begin.common                        = session->ipc_common_data;
        begin.mime_type                     = CTRLM_VOICE_MIMETYPE_ADPCM;
        begin.sub_type                      = CTRLM_VOICE_SUBTYPE_ADPCM;
        begin.language                      = this->prefs.guide_language;
        begin.keyword_verification          = session_begin->keyword_verification;
        begin.keyword_verification_required = kw_verification_required;
        this->voice_ipc->session_begin(begin);
    } else {
        XLOGD_INFO("src <%s>", ctrlm_voice_device_str(session->voice_device));
    }
    if(session->ipc_common_data.session_id_ctrlm == 1) { 
        XLOGD_TELEMETRY("first voice session");
    }
    
    if(session->state_dst != CTRLM_VOICE_STATE_DST_REQUESTED && !(ctrlm_voice_device_is_mic(session->voice_device) && session->state_dst == CTRLM_VOICE_STATE_DST_READY)) {
        XLOGD_WARN("src <%s> unexpected dst state <%s>", ctrlm_voice_device_str(session->voice_device), ctrlm_voice_state_dst_str(session->state_dst));
    }
    session->state_dst = CTRLM_VOICE_STATE_DST_OPENED;


    if (session->is_session_by_text) {
        XLOGD_WARN("src <%s> Ending voice session immediately because this is a text-only session", ctrlm_voice_device_str(session->voice_device));
        this->voice_session_end(session, CTRLM_VOICE_SESSION_END_REASON_DONE);
    }
}

void ctrlm_voice_t::voice_session_end_callback(ctrlm_voice_session_end_cb_t *session_end) {
    if(NULL == session_end) {
        XLOGD_ERROR("NULL data");
        return;
    }
    errno_t safec_rc = -1;

    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(session_end->header.uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

    //If this is a voice assistant, unmute the audio
    if(is_voice_assistant(session->voice_device)) {
        this->audio_state_set(false);
    }

    xrsr_session_stats_t *stats = &session_end->stats;

    if(stats == NULL) {
        XLOGD_ERROR("src <%s> stats are NULL", ctrlm_voice_device_str(session->voice_device));
        return;
    }

    ctrlm_voice_command_status_t command_status = session->status.status;
    if(command_status == VOICE_COMMAND_STATUS_PENDING) {
        switch(session->voice_device) {
            case CTRLM_VOICE_DEVICE_FF: {
                session->status.status = (stats->reason == XRSR_SESSION_END_REASON_EOS) ? VOICE_COMMAND_STATUS_SUCCESS : VOICE_COMMAND_STATUS_FAILURE;
                command_status = session->status.status;
                this->voice_status_set(session);
                break;
            }
            #ifdef CTRLM_LOCAL_MIC
            case CTRLM_VOICE_DEVICE_MICROPHONE: {
                command_status = (session_end->success ? VOICE_COMMAND_STATUS_SUCCESS : VOICE_COMMAND_STATUS_FAILURE);
                // No need to set, as it's not a controller
                break;
            }
            #endif
            default: {
                break;
            }
        }
    }

    XLOGD_INFO("src <%s> audio sent bytes <%u> samples <%u> reason <%s> voice command status <%s>", ctrlm_voice_device_str(session->voice_device), session->audio_sent_bytes, session->audio_sent_samples, xrsr_session_end_reason_str(stats->reason), ctrlm_voice_command_status_str(command_status));

    // Update device status
    this->voice_session_set_inactive(session->voice_device);

    if (session->is_session_by_text) {
        // if this is a text only session, we don't get an ASR message from vrex with the transcription so copy it here.
        session->transcription = session->transcription_in;
    }

    // Send Results IARM Event
    if(this->voice_ipc) {
        if(stats->reason == XRSR_SESSION_END_REASON_ERROR_AUDIO_DURATION) {
            ctrlm_voice_ipc_event_session_end_t end;
            end.common = session->ipc_common_data;
            end.result = SESSION_END_SHORT_UTTERANCE;
            end.reason = (int)session->end_reason;
            this->voice_ipc->session_end(end);
        } else {
            ctrlm_voice_ipc_event_session_end_server_stats_t server_stats;
            ctrlm_voice_ipc_event_session_end_t end;
            end.common                       = session->ipc_common_data;
            end.result                       = (session_end->success ? SESSION_END_SUCCESS : SESSION_END_FAILURE);
            end.reason                       = stats->reason;
            end.return_code_protocol         = stats->ret_code_protocol;
            end.return_code_protocol_library = stats->ret_code_library;
            end.return_code_server           = session->server_ret_code;
            end.return_code_server_str       = session->server_message;
            end.return_code_internal         = stats->ret_code_internal;
            end.transcription                = session->transcription;
            
            if(stats->server_ip[0] != 0) {
                server_stats.server_ip = stats->server_ip;
            }
            server_stats.dns_time = stats->time_dns;
            server_stats.connect_time = stats->time_connect;
            end.server_stats = &server_stats;

            this->voice_ipc->session_end(end);
        }
    }

    // Update controller metrics
    ctrlm_main_queue_msg_controller_voice_metrics_t metrics = {0};
    metrics.controller_id       = session->controller_id;
    metrics.short_utterance     = (stats->reason == XRSR_SESSION_END_REASON_ERROR_AUDIO_DURATION ? 1 : 0);
    metrics.packets_total       = session->packets_processed + session->packets_lost;
    metrics.packets_lost        = session->packets_lost;
    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::process_voice_controller_metrics, &metrics, sizeof(metrics), NULL, session->network_id);

    // Send results internally to controlMgr
    if(session->network_id != CTRLM_MAIN_NETWORK_ID_INVALID && session->controller_id != CTRLM_MAIN_CONTROLLER_ID_INVALID) {
        ctrlm_main_queue_msg_voice_session_result_t msg = {0};
        msg.controller_id     = session->controller_id;
        safec_rc = strncpy_s(msg.transcription, sizeof(msg.transcription), session->transcription.c_str(), sizeof(msg.transcription)-1);
        ERR_CHK(safec_rc);
        msg.transcription[CTRLM_VOICE_SESSION_TEXT_MAX_LENGTH - 1] = '\0';
        msg.success           =  (session_end->success == true ? 1 : 0);
        ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::ind_process_voice_session_result, &msg, sizeof(msg), NULL, session->network_id);
    }

    if(session->voice_device == CTRLM_VOICE_DEVICE_FF && session->status.status == VOICE_COMMAND_STATUS_PENDING) {
        session->status.status = (session_end->success ? VOICE_COMMAND_STATUS_SUCCESS : VOICE_COMMAND_STATUS_FAILURE);
    }

    #ifndef TELEMETRY_SUPPORT
    XLOGD_WARN("telemetry is not enabled");
    #else
    // Report voice session telemetry
    ctrlm_telemetry_t *telemetry = ctrlm_get_telemetry_obj();
    if(telemetry) {
        ctrlm_telemetry_event_t<int> vs_marker(MARKER_VOICE_SESSION_TOTAL, 1);
        ctrlm_telemetry_event_t<int> vs_status_marker(session_end->success ? MARKER_VOICE_SESSION_SUCCESS : MARKER_VOICE_SESSION_FAILURE, 1);
        ctrlm_telemetry_event_t<int> vs_end_reason_marker(MARKER_VOICE_END_REASON_PREFIX + std::string(ctrlm_voice_session_end_reason_str(session->end_reason)), 1);
        ctrlm_telemetry_event_t<int> vs_xrsr_end_reason_marker(MARKER_VOICE_XRSR_END_REASON_PREFIX + std::string(xrsr_session_end_reason_str(stats->reason)), 1);

        // Handle all VSRsp error telemetry
        if(session->current_vsr_err_string != "") {
            sem_wait(&session->current_vsr_err_semaphore);
            ctrlm_voice_telemetry_vsr_error_t *vsr_error = this->vsr_errors[session->current_vsr_err_string];
            if(vsr_error) {
                vsr_error->update(session->packets_processed > 0, session->current_vsr_err_rsp_window, session->current_vsr_err_rsp_time);
                telemetry->event(ctrlm_telemetry_report_t::VOICE, *vsr_error);
            }
            vsr_error = this->vsr_errors[MARKER_VOICE_VSR_FAIL_TOTAL];
            if(vsr_error) {
                vsr_error->update(session->packets_processed > 0, session->current_vsr_err_rsp_window, session->current_vsr_err_rsp_time);
                telemetry->event(ctrlm_telemetry_report_t::VOICE, *vsr_error);
            }
            sem_post(&session->current_vsr_err_semaphore);
        }
        // End VSRsp telemetry

        telemetry->event(ctrlm_telemetry_report_t::VOICE, vs_marker);
        telemetry->event(ctrlm_telemetry_report_t::VOICE, vs_status_marker);
        telemetry->event(ctrlm_telemetry_report_t::VOICE, vs_end_reason_marker);
        telemetry->event(ctrlm_telemetry_report_t::VOICE, vs_xrsr_end_reason_marker);
    }
    #endif
    session->current_vsr_err_rsp_time   = 0;
    session->current_vsr_err_rsp_window = 0;
    session->current_vsr_err_string     = "";


    if(session->state_dst != CTRLM_VOICE_STATE_DST_OPENED) {
        XLOGD_WARN("src <%s> unexpected dst state <%s>", ctrlm_voice_device_str(session->voice_device), ctrlm_voice_state_dst_str(session->state_dst));
    }

    session->session_active_server = false;
    if(session->state_src == CTRLM_VOICE_STATE_SRC_STREAMING) {
        voice_session_end(session, CTRLM_VOICE_SESSION_END_REASON_OTHER_ERROR);
    } else if(!session->session_active_controller) {
        session->state_src = CTRLM_VOICE_STATE_SRC_READY;

        // Send session stats only if the session stats have been received from the controller (or it has timed out)
        if(session->stats_session_id != 0 && this->voice_ipc) {
            ctrlm_voice_ipc_event_session_statistics_t stats;
            stats.common  = session->ipc_common_data;
            stats.session = session->stats_session;
            stats.reboot  = session->stats_reboot;

            this->voice_ipc->session_statistics(stats);
        }
    }

    session->state_dst        = CTRLM_VOICE_STATE_DST_READY;
    session->endpoint_current = NULL;
    voice_session_info_reset(session);

    // Check for incorrect semaphore values
    int val;
    sem_getvalue(&this->vsr_semaphore, &val);
    if(val > 0) {
        XLOGD_TELEMETRY("src <%s> VSR semaphore has invalid value... resetting..", ctrlm_voice_device_str(session->voice_device));
        for(int i = 0; i < val; i++) {
            sem_wait(&this->vsr_semaphore);
        }
    }
}

void ctrlm_voice_t::voice_server_message_callback(const char *msg, unsigned long length) {
    if(this->voice_ipc) {
        this->voice_ipc->server_message(msg, length);
    }
}

void ctrlm_voice_t::voice_server_connected_callback(ctrlm_voice_cb_header_t *connected) {

    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(connected->uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

   session->session_timing.srvr_connect           = connected->timestamp;
   session->session_timing.srvr_init_txd          = session->session_timing.srvr_connect;
   session->session_timing.srvr_audio_txd_first   = session->session_timing.srvr_connect;
   session->session_timing.srvr_audio_txd_keyword = session->session_timing.srvr_connect;
   session->session_timing.srvr_audio_txd_final   = session->session_timing.srvr_connect;
   session->session_timing.srvr_rsp_keyword       = session->session_timing.srvr_connect;

   session->session_timing.connect_success = true;
}

void ctrlm_voice_t::voice_server_disconnected_callback(ctrlm_voice_disconnected_cb_t *disconnected) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(disconnected->header.uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }
    session->session_timing.srvr_disconnect = disconnected->header.timestamp;
    session->session_timing.available = true;
}

void ctrlm_voice_t::voice_server_sent_init_callback(ctrlm_voice_cb_header_t *init) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(init->uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

    session->session_timing.srvr_init_txd = init->timestamp;
}

void ctrlm_voice_t::voice_stream_begin_callback(ctrlm_voice_stream_begin_cb_t *stream_begin) {
    if(NULL == stream_begin) {
        XLOGD_ERROR("NULL data");
        return;
    }

    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(stream_begin->header.uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

   session->session_timing.srvr_audio_txd_first   = stream_begin->header.timestamp;

   session->session_timing.srvr_audio_txd_keyword = session->session_timing.srvr_audio_txd_first;
   session->session_timing.srvr_audio_txd_final   = session->session_timing.srvr_audio_txd_first;

   if(session->state_dst != CTRLM_VOICE_STATE_DST_OPENED) {
      XLOGD_WARN("src <%s> unexpected dst state <%s>", ctrlm_voice_device_str(session->voice_device), ctrlm_voice_state_dst_str(session->state_dst));
   }
   session->state_dst = CTRLM_VOICE_STATE_DST_STREAMING;

   if(this->voice_ipc) {
       // call the ipc's stream begin event handler
       ctrlm_voice_ipc_event_stream_begin_t begin;
       begin.common = session->ipc_common_data;
       this->voice_ipc->stream_begin(begin);
   }
}

void ctrlm_voice_t::voice_stream_kwd_callback(ctrlm_voice_cb_header_t *kwd) {
   if(NULL == kwd) {
       XLOGD_ERROR("NULL data");
       return;
   }
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(kwd->uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

   session->session_timing.srvr_audio_txd_keyword = kwd->timestamp;

   session->session_timing.srvr_audio_txd_final   = session->session_timing.srvr_audio_txd_keyword;
}

void ctrlm_voice_t::voice_stream_end_callback(ctrlm_voice_stream_end_cb_t *stream_end) {
    if(NULL == stream_end) {
        XLOGD_ERROR("NULL data");
        return;
    }

    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(stream_end->header.uuid);

    if(session == NULL) {
       XLOGD_ERROR("session not found");
       return;
    }

    // Stop packet timeout
    if(session->timeout_packet_tag > 0) {
        g_source_remove(session->timeout_packet_tag);
        session->timeout_packet_tag = 0;
    }

    xrsr_stream_stats_t *stats = &stream_end->stats;
    if(!stats->audio_stats.valid) {
        XLOGD_INFO("src <%s> end of stream <%s> audio stats not present", ctrlm_voice_device_str(session->voice_device), (stats->result) ? "SUCCESS" : "FAILURE");
        session->packets_processed           = 0;
        session->packets_lost                = 0;
        session->stats_session.packets_total = 0;
        session->stats_session.packets_lost  = 0;
        session->stats_session.link_quality  = 0;

    } else {
        session->packets_processed           = stats->audio_stats.packets_processed;
        session->packets_lost                = stats->audio_stats.packets_lost;
        session->stats_session.available     = 1;
        session->stats_session.packets_total = session->packets_lost + session->packets_processed;
        session->stats_session.packets_lost  = session->packets_lost;
        session->stats_session.link_quality  = (session->packets_processed > 0) ? (session->lqi_total / session->packets_processed) : 0;

        uint32_t samples_processed    = stats->audio_stats.samples_processed;
        uint32_t samples_lost         = stats->audio_stats.samples_lost;
        uint32_t decoder_failures     = stats->audio_stats.decoder_failures;
        uint32_t samples_buffered_max = stats->audio_stats.samples_buffered_max;

        XLOGD_INFO("src <%s> end of stream <%s>", ctrlm_voice_device_str(session->voice_device), (stats->result) ? "SUCCESS" : "FAILURE");

        if(session->packets_processed > 0) {
            uint32_t stream_duration = session->packets_processed * 20; // assume 20 ms per packet
            if(session->format.type == CTRLM_VOICE_FORMAT_ADPCM_FRAME) {
                uint32_t frame_duration_us = (session->format.value.adpcm_frame.size_packet - session->format.value.adpcm_frame.size_header) * 125; // 125 us per byte for ADPCM at 16 kHz
                stream_duration = (session->packets_processed * frame_duration_us) / 1000;
            }
            XLOGD_TELEMETRY("src <%s> Packets Lost/Total <%u/%u> %.02f%% duration <%u> ms", ctrlm_voice_device_str(session->voice_device), session->packets_lost, session->packets_lost + session->packets_processed, 100.0 * ((double)session->packets_lost / (double)(session->packets_lost + session->packets_processed)), stream_duration);
        }
        if(samples_processed > 0) {
            XLOGD_INFO("src <%s> Samples Lost/Total <%u/%u> %.02f%% buffered max <%u>", ctrlm_voice_device_str(session->voice_device), samples_lost, samples_lost + samples_processed, 100.0 * ((double)samples_lost / (double)(samples_lost + samples_processed)), samples_buffered_max);
        }
        if(decoder_failures > 0) {
            XLOGD_WARN("src <%s> decoder failures <%u>", ctrlm_voice_device_str(session->voice_device), decoder_failures);
        }
    }

    if(CTRLM_VOICE_DEVICE_PTT == session->voice_device && session->status.status == VOICE_COMMAND_STATUS_PENDING) { // Set voice command status
        session->status.status = (stats->result) ? VOICE_COMMAND_STATUS_SUCCESS : VOICE_COMMAND_STATUS_FAILURE;
        this->voice_status_set(session);
    }

    //If this is a voice assistant, unmute the audio
    if(is_voice_assistant(session->voice_device)) {
        this->audio_state_set(false);
    }

    if(this->voice_ipc) {
        // This is a STREAM end..
        ctrlm_voice_ipc_event_stream_end_t end;
        end.common = session->ipc_common_data;
        end.reason = (int)session->end_reason;
        this->voice_ipc->stream_end(end);
    }

    if(session->state_dst != CTRLM_VOICE_STATE_DST_STREAMING) {
       XLOGD_WARN("src <%s> unexpected dst state <%s>", ctrlm_voice_device_str(session->voice_device), ctrlm_voice_state_dst_str(session->state_dst));
    }
    session->state_dst = CTRLM_VOICE_STATE_DST_OPENED;

    session->session_timing.srvr_audio_txd_final = stream_end->header.timestamp;
}

void ctrlm_voice_t::voice_action_tv_mute_callback(bool mute) {
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
    session->status.status          = VOICE_COMMAND_STATUS_TV_AVR_CMD;
    session->status.data.tv_avr.cmd = CTRLM_VOICE_TV_AVR_CMD_VOLUME_MUTE;
}

void ctrlm_voice_t::voice_action_tv_power_callback(bool power, bool toggle) {
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
    session->status.status                      = VOICE_COMMAND_STATUS_TV_AVR_CMD;
    session->status.data.tv_avr.cmd             = power ? CTRLM_VOICE_TV_AVR_CMD_POWER_ON : CTRLM_VOICE_TV_AVR_CMD_POWER_OFF;
    session->status.data.tv_avr.toggle_fallback = (this->prefs.force_toggle_fallback ? true : toggle);
}

void ctrlm_voice_t::voice_action_tv_volume_callback(bool up, uint32_t repeat_count) {
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
    session->status.status                      = VOICE_COMMAND_STATUS_TV_AVR_CMD;
    session->status.data.tv_avr.cmd             = up ? CTRLM_VOICE_TV_AVR_CMD_VOLUME_UP : CTRLM_VOICE_TV_AVR_CMD_VOLUME_DOWN;
    session->status.data.tv_avr.ir_repeats      = repeat_count;
}
void ctrlm_voice_t::voice_action_keyword_verification_callback(const uuid_t uuid, bool success, rdkx_timestamp_t timestamp) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(uuid);

    if(session == NULL) {
        XLOGD_ERROR("session not found");
        return;
    }

    session->session_timing.srvr_rsp_keyword = timestamp;
    if(this->voice_ipc) {
        ctrlm_voice_ipc_event_keyword_verification_t kw_verification;
        kw_verification.common = session->ipc_common_data;
        kw_verification.verified = success;
        this->voice_ipc->keyword_verification(kw_verification);
    }

    //If the keyword was detected and this is a voice assistant, mute the audio
    if(success && is_voice_assistant(session->voice_device)) {
        if(this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_CLOUD || 
          (this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE && session->confidence < this->audio_confidence_threshold) ||
          (this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_DUAL_SENSITIVITY && !session->dual_sensitivity_immediate)) {
            this->voice_keyword_verified_action();
        }
    }

    session->keyword_verified = success;
}

void ctrlm_voice_t::voice_keyword_verified_action(void) {
   #ifdef BEEP_ON_KWD_ENABLED
   if(this->audio_ducking_beep_enabled) { // play beep audio before ducking audio
      if(this->audio_ducking_beep_in_progress) {
         XLOGD_WARN("audio ducking beep already in progress!");
         this->obj_sap->close();
         this->audio_ducking_beep_in_progress = false;
         // remove timeout
         if(this->timeout_keyword_beep > 0) {
             g_source_remove(this->timeout_keyword_beep);
             this->timeout_keyword_beep = 0;
         }
      }
      int8_t retry = 1;
      do {
         if(!this->sap_opened) {
            this->sap_opened = this->obj_sap->open(SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_WAV, SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_FILE, SYSTEM_AUDIO_PLAYER_PLAY_MODE_SYSTEM);
            if(!this->sap_opened) {
               XLOGD_WARN("unable to open system audio player");
               retry--;
               continue;
            }
         }

         if(!this->obj_sap->play("file://" BEEP_ON_KWD_FILE)) {
            XLOGD_WARN("unable to play beep file <%s>", BEEP_ON_KWD_FILE);
            if(!this->obj_sap->close()) {
               XLOGD_WARN("unable to close system audio player");
            }
            this->sap_opened = false;
            retry--;
            continue;
         }
         this->audio_ducking_beep_in_progress = true;

         rdkx_timestamp_get(&this->sap_play_timestamp);

         // start a timer in case playback end event is not received
         this->timeout_keyword_beep = g_timeout_add(CTRLM_VOICE_KEYWORD_BEEP_TIMEOUT, ctrlm_voice_keyword_beep_end_timeout, NULL);
         return;
      } while(retry >= 0);
   }
   #endif
   this->audio_state_set(true);
}

#ifdef BEEP_ON_KWD_ENABLED
void ctrlm_voice_t::voice_keyword_beep_completed_normal(void *data, int size) {
   this->voice_keyword_beep_completed_callback(false, false);
}

void ctrlm_voice_t::voice_keyword_beep_completed_error(void *data, int size) {
   this->voice_keyword_beep_completed_callback(false, true);
}

void ctrlm_voice_t::voice_keyword_beep_completed_callback(bool timeout, bool playback_error) {
   if(!this->audio_ducking_beep_in_progress) {
      return;
   }
   ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];

   if(!timeout) { // remove timeout
      if(this->timeout_keyword_beep > 0) {
         g_source_remove(this->timeout_keyword_beep);
         this->timeout_keyword_beep = 0;
      }
      if(playback_error) {
         XLOGD_ERROR("playback failure");
      }
   } else {
      XLOGD_ERROR("timeout");
   }

   this->audio_ducking_beep_in_progress = false;

   XLOGD_WARN("duration <%llu> ms dst <%s>", rdkx_timestamp_since_ms(this->sap_play_timestamp), ctrlm_voice_state_dst_str(session->state_dst));

   if(session->state_dst >= CTRLM_VOICE_STATE_DST_REQUESTED && session->state_dst <= CTRLM_VOICE_STATE_DST_STREAMING) {
      this->audio_state_set(true);
   }
}
#endif

void ctrlm_voice_t::voice_session_transcription_callback(const uuid_t uuid, const char *transcription) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(uuid);

    if(session == NULL) {
        XLOGD_ERROR("session not found");
        return;
    }

    if(transcription) {
        session->transcription = transcription;
    } else {
        session->transcription = "";
    }
    XLOGD_INFO("src <%s> Voice Session Transcription: \"%s\"", ctrlm_voice_device_str(session->voice_device), this->mask_pii ? "***" : session->transcription.c_str());
}

void ctrlm_voice_t::voice_server_return_code_callback(const uuid_t uuid, const char *reason, long ret_code) {
    // Get session based on uuid
    ctrlm_voice_session_t *session = voice_session_from_uuid(uuid);

    if(session == NULL) {
        XLOGD_ERROR("session not found");
        return;
    }
    session->server_ret_code = ret_code;

    XLOGD_TELEMETRY("src <%s>, <%s> code <%d>, reason <%s>, power state <%s>",  \
        ctrlm_voice_device_str(session->voice_device), \
        (ret_code == 0) ? "success" : "error", \
        ret_code, \
        ((reason == NULL) ? "NULL" : reason), \
        ((ctrlm_main_get_power_state() == CTRLM_POWER_STATE_ON) ? "FPM" : "NSM"));

}
// Callback Interface Implementation End

// Helper Functions
const char *ctrlm_voice_format_str(ctrlm_voice_format_t format) {
    switch(format.type) {
        case CTRLM_VOICE_FORMAT_ADPCM_FRAME:      return("ADPCM_FRAME");
        case CTRLM_VOICE_FORMAT_PCM:              return("PCM");
        case CTRLM_VOICE_FORMAT_PCM_32_BIT:       return("PCM_32_BIT");
        case CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI: return("PCM_32_BIT_MULTI");
        case CTRLM_VOICE_FORMAT_PCM_RAW:          return("PCM_RAW");
        case CTRLM_VOICE_FORMAT_OPUS_XVP:         return("OPUS_XVP");
        case CTRLM_VOICE_FORMAT_OPUS:             return("OPUS");
        case CTRLM_VOICE_FORMAT_INVALID:          return("INVALID");
    }
    return("UNKNOWN");
}

const char *ctrlm_voice_state_src_str(ctrlm_voice_state_src_t state) {
    switch(state) {
        case CTRLM_VOICE_STATE_SRC_READY:       return("READY");
        case CTRLM_VOICE_STATE_SRC_STREAMING:   return("STREAMING");
        case CTRLM_VOICE_STATE_SRC_WAITING:     return("WAITING");
        case CTRLM_VOICE_STATE_SRC_INVALID:     return("INVALID");
    }
    return("UNKNOWN");
}

const char *ctrlm_voice_state_dst_str(ctrlm_voice_state_dst_t state) {
    switch(state) {
        case CTRLM_VOICE_STATE_DST_READY:     return("READY");
        case CTRLM_VOICE_STATE_DST_REQUESTED: return("REQUESTED");
        case CTRLM_VOICE_STATE_DST_OPENED:    return("OPENED");
        case CTRLM_VOICE_STATE_DST_STREAMING: return("STREAMING");
        case CTRLM_VOICE_STATE_DST_INVALID:   return("INVALID");
    }
    return("UNKNOWN");
}

const char *ctrlm_voice_device_str(ctrlm_voice_device_t device) {
   switch(device) {
       case CTRLM_VOICE_DEVICE_PTT:            return("PTT");
       case CTRLM_VOICE_DEVICE_FF:             return("FF");
       #ifdef CTRLM_LOCAL_MIC
       case CTRLM_VOICE_DEVICE_MICROPHONE:     return("MICROPHONE");
       #endif
       #ifdef CTRLM_LOCAL_MIC_TAP
       case CTRLM_VOICE_DEVICE_MICROPHONE_TAP: return("MICROPHONE_TAP");
       #endif
       case CTRLM_VOICE_DEVICE_INVALID:        return("INVALID");
   }
   return("UNKNOWN");
}

std::string ctrlm_voice_device_status_str(uint8_t status) {
    std::stringstream ss;

    if(status == CTRLM_VOICE_DEVICE_STATUS_NONE) {
      ss << "NONE";
    } else {
       bool is_first = true;
       if(status & CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE) {                             is_first = false;   ss << "ACTIVE";        }
       if(status & CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED)  { if(!is_first) { ss << ", "; is_first = false; } ss << "NOT SUPPORTED"; }
       if(status & CTRLM_VOICE_DEVICE_STATUS_DEVICE_UPDATE)  { if(!is_first) { ss << ", "; is_first = false; } ss << "DEVICE_UPDATE"; }
       if(status & CTRLM_VOICE_DEVICE_STATUS_DISABLED)       { if(!is_first) { ss << ", "; is_first = false; } ss << "DISABLED";      }
       if(status & CTRLM_VOICE_DEVICE_STATUS_PRIVACY)        { if(!is_first) { ss << ", ";                   } ss << "PRIVACY";       }
    }

    return(ss.str());
}

const char *ctrlm_voice_audio_mode_str(ctrlm_voice_audio_mode_t mode) {
    switch(mode) {
        case CTRLM_VOICE_AUDIO_MODE_OFF:     return("OFF");
        case CTRLM_VOICE_AUDIO_MODE_MUTING:  return("MUTING");
        case CTRLM_VOICE_AUDIO_MODE_DUCKING: return("DUCKING");
    }
    return("UNKNOWN");
}

const char *ctrlm_voice_audio_timing_str(ctrlm_voice_audio_timing_t timing) {
    switch(timing) {
        case CTRLM_VOICE_AUDIO_TIMING_VSR:              return("VSR");
        case CTRLM_VOICE_AUDIO_TIMING_CLOUD:            return("CLOUD");
        case CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE:       return("CONFIDENCE");
        case CTRLM_VOICE_AUDIO_TIMING_DUAL_SENSITIVITY: return("DUAL SENSITIVITY");
    }
    return("UNKNOWN");
}

const char *ctrlm_voice_audio_ducking_type_str(ctrlm_voice_audio_ducking_type_t ducking_type) {
    switch(ducking_type) {
        case CTRLM_VOICE_AUDIO_DUCKING_TYPE_ABSOLUTE: return("ABSOLUTE");
        case CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE: return("RELATIVE");
    }
    return("UNKNOWN");
}

ctrlm_voice_device_t xrsr_to_voice_device(xrsr_src_t device) {
    ctrlm_voice_device_t ret = CTRLM_VOICE_DEVICE_INVALID;
    switch(device) {
        case XRSR_SRC_RCU_PTT: {
            ret = CTRLM_VOICE_DEVICE_PTT;
            break;
        }
        case XRSR_SRC_RCU_FF: {
            ret = CTRLM_VOICE_DEVICE_FF;
            break;
        }
        #ifdef CTRLM_LOCAL_MIC
        case XRSR_SRC_MICROPHONE: {
            ret = CTRLM_VOICE_DEVICE_MICROPHONE;
            break;
        }
        #endif
        #ifdef CTRLM_LOCAL_MIC_TAP
        case XRSR_SRC_MICROPHONE_TAP: {
            ret = CTRLM_VOICE_DEVICE_MICROPHONE_TAP;
            break;
        }
        #endif
        default: {
            XLOGD_ERROR("unrecognized device type %d", device);
            break;
        }
    }
    return(ret);
}

xrsr_src_t voice_device_to_xrsr(ctrlm_voice_device_t device) {
    xrsr_src_t ret = XRSR_SRC_INVALID;
    switch(device) {
        case CTRLM_VOICE_DEVICE_PTT: {
            ret = XRSR_SRC_RCU_PTT;
            break;
        }
        case CTRLM_VOICE_DEVICE_FF: {
            ret = XRSR_SRC_RCU_FF;
            break;
        }
        #ifdef CTRLM_LOCAL_MIC
        case CTRLM_VOICE_DEVICE_MICROPHONE: {
            ret = XRSR_SRC_MICROPHONE;
            break;
        }
        #endif
        #ifdef CTRLM_LOCAL_MIC_TAP
        case CTRLM_VOICE_DEVICE_MICROPHONE_TAP: {
            ret = XRSR_SRC_MICROPHONE_TAP;
            break;
        }
        #endif
        default: {
            XLOGD_ERROR("unrecognized device type %d", device);
            break;
        }
    }
    return(ret);
}

ctrlm_voice_session_group_t voice_device_to_session_group(ctrlm_voice_device_t device_type) {
   #ifdef CTRLM_LOCAL_MIC_TAP
   if(device_type == CTRLM_VOICE_DEVICE_MICROPHONE_TAP) {
      return(VOICE_SESSION_GROUP_MIC_TAP);
   }
   #endif
   return(VOICE_SESSION_GROUP_DEFAULT);
}

xrsr_audio_format_t voice_format_to_xrsr(ctrlm_voice_format_t format) {
    xrsr_audio_format_t ret = { .type = XRSR_AUDIO_FORMAT_NONE };

    switch(format.type) {
        case CTRLM_VOICE_FORMAT_ADPCM_FRAME: {
            
            ret.type = XRSR_AUDIO_FORMAT_ADPCM_FRAME;
            ret.value.adpcm_frame.size_packet                 = format.value.adpcm_frame.size_packet;
            ret.value.adpcm_frame.size_header                 = format.value.adpcm_frame.size_header;
            ret.value.adpcm_frame.offset_step_size_index      = format.value.adpcm_frame.offset_step_size_index;
            ret.value.adpcm_frame.offset_predicted_sample_lsb = format.value.adpcm_frame.offset_predicted_sample_lsb;
            ret.value.adpcm_frame.offset_predicted_sample_msb = format.value.adpcm_frame.offset_predicted_sample_msb;
            ret.value.adpcm_frame.offset_sequence_value       = format.value.adpcm_frame.offset_sequence_value;
            ret.value.adpcm_frame.sequence_value_min          = format.value.adpcm_frame.sequence_value_min;
            ret.value.adpcm_frame.sequence_value_max          = format.value.adpcm_frame.sequence_value_max;
            break;
        }
        case CTRLM_VOICE_FORMAT_OPUS_XVP: {
            ret.type = XRSR_AUDIO_FORMAT_OPUS; // TODO
            break;
        }
        case CTRLM_VOICE_FORMAT_OPUS: {
            ret.type = XRSR_AUDIO_FORMAT_OPUS;
            break;
        }
        case CTRLM_VOICE_FORMAT_PCM: {
            ret.type = XRSR_AUDIO_FORMAT_PCM;
            break;
        }
        case CTRLM_VOICE_FORMAT_PCM_32_BIT: {
            ret.type = XRSR_AUDIO_FORMAT_PCM_32_BIT;
            break;
        }
        case CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI: {
            ret.type = XRSR_AUDIO_FORMAT_PCM_32_BIT_MULTI;
            break;
        }
        case CTRLM_VOICE_FORMAT_PCM_RAW: {
            ret.type = XRSR_AUDIO_FORMAT_PCM_RAW;
            break;
        }
        default: {
            break;
        }
    }
    return(ret);
}

bool ctrlm_voice_t::is_voice_assistant(ctrlm_voice_device_t device) {
    bool voice_assistant = false;
    switch(device) {
        #ifdef CTRLM_LOCAL_MIC
        case CTRLM_VOICE_DEVICE_MICROPHONE:
        #endif
        case CTRLM_VOICE_DEVICE_FF:  {
            voice_assistant = true;
            break;
        }
        #ifdef CTRLM_LOCAL_MIC_TAP
        case CTRLM_VOICE_DEVICE_MICROPHONE_TAP:
        #endif
        case CTRLM_VOICE_DEVICE_PTT: 
        case CTRLM_VOICE_DEVICE_INVALID:
        default: {
            voice_assistant = false;
            break;
        }
    }
    return(voice_assistant);
}

bool ctrlm_voice_t::controller_supports_qos(ctrlm_voice_device_t device) {
   if(device == CTRLM_VOICE_DEVICE_FF) {
      return(true);
   }
   return(false);
}

void ctrlm_voice_t::set_audio_mode(ctrlm_voice_audio_settings_t *settings) {
    switch(settings->mode) {
        case CTRLM_VOICE_AUDIO_MODE_OFF:
        case CTRLM_VOICE_AUDIO_MODE_MUTING:
        case CTRLM_VOICE_AUDIO_MODE_DUCKING: {
            this->audio_mode = settings->mode;
            break;
        }
        default: {
            this->audio_mode = (ctrlm_voice_audio_mode_t)JSON_INT_VALUE_VOICE_AUDIO_MODE;
            break;
        }
    }
    switch(settings->timing) {
        case CTRLM_VOICE_AUDIO_TIMING_VSR:
        case CTRLM_VOICE_AUDIO_TIMING_CLOUD:
        case CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE:
        case CTRLM_VOICE_AUDIO_TIMING_DUAL_SENSITIVITY: {
            this->audio_timing = settings->timing;
            break;
        }
        default: {
            this->audio_timing = (ctrlm_voice_audio_timing_t)JSON_INT_VALUE_VOICE_AUDIO_TIMING;
            break;
        }
    }
    switch(settings->ducking_type) {
        case CTRLM_VOICE_AUDIO_DUCKING_TYPE_ABSOLUTE:
        case CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE: {
            this->audio_ducking_type = settings->ducking_type;
            break;
        }
        default: {
            this->audio_ducking_type = (ctrlm_voice_audio_ducking_type_t)JSON_INT_VALUE_VOICE_AUDIO_DUCKING_TYPE;
            break;
        }
    }

    if(settings->ducking_level >= 0 && settings->ducking_level <= 1) {
        this->audio_ducking_level = settings->ducking_level;
    } else {
        this->audio_ducking_level = JSON_FLOAT_VALUE_VOICE_AUDIO_DUCKING_LEVEL;
    }

    if(settings->confidence_threshold >= 0 && settings->confidence_threshold <= 1) {
        this->audio_confidence_threshold = settings->confidence_threshold;
    } else {
        this->audio_confidence_threshold = JSON_FLOAT_VALUE_VOICE_AUDIO_CONFIDENCE_THRESHOLD;
    }

    this->audio_ducking_beep_enabled = settings->ducking_beep;

    // Print configuration
    std::stringstream ss;

    ss << "Audio Mode < " << ctrlm_voice_audio_mode_str(this->audio_mode) << " >";
    if(this->audio_mode == CTRLM_VOICE_AUDIO_MODE_DUCKING) {
        ss << ", Ducking Type < " << ctrlm_voice_audio_ducking_type_str(this->audio_ducking_type) << " >";
        ss << ", Ducking Level < " << this->audio_ducking_level * 100 << "% >";
        if(this->audio_ducking_beep_enabled) {
           ss << ", Ducking Beep < ENABLED >";
        } else {
           ss << ", Ducking Beep < DISABLED >";
        }
    }
    if(this->audio_mode != CTRLM_VOICE_AUDIO_MODE_OFF) {
        ss << ", Audio Timing < " << ctrlm_voice_audio_timing_str(this->audio_timing) << " >";
        if(this->audio_timing == CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE) {
            ss << ", Confidence Threshold < " << this->audio_confidence_threshold << " >";
        }
    }

    XLOGD_INFO("%s", ss.str().c_str());
    // End print configuration
}

// Helper Functions end

// RF4CE HAL callbacks

void ctrlm_voice_session_response_confirm(bool result, signed long long rsp_time, unsigned int rsp_window, const std::string &err_str, ctrlm_timestamp_t *timestamp, void *user_data) {
   ctrlm_get_voice_obj()->voice_session_rsp_confirm(result, rsp_time, rsp_window, err_str, timestamp);
}

// Timeouts
int ctrlm_voice_t::ctrlm_voice_packet_timeout(void *data) {
    ctrlm_get_voice_obj()->voice_session_timeout();
    return(false);
}

int ctrlm_voice_t::ctrlm_voice_controller_session_stats_rxd_timeout(void *data) {
    ctrlm_get_voice_obj()->voice_session_controller_stats_rxd_timeout();
    return(false);
}

int ctrlm_voice_t::ctrlm_voice_controller_command_status_read_timeout(void *data) {
    ctrlm_get_voice_obj()->voice_session_controller_command_status_read_timeout();
    return(false);
}

#ifdef BEEP_ON_KWD_ENABLED
int ctrlm_voice_t::ctrlm_voice_keyword_beep_end_timeout(void *data) {
    ctrlm_get_voice_obj()->voice_keyword_beep_completed_callback(true, false);
    return(false);
}
#endif

// Timeouts end

const char *ctrlm_voice_command_status_str(ctrlm_voice_command_status_t status) {
   switch(status) {
      case VOICE_COMMAND_STATUS_PENDING:    return("PENDING");
      case VOICE_COMMAND_STATUS_TIMEOUT:    return("TIMEOUT");
      case VOICE_COMMAND_STATUS_OFFLINE:    return("OFFLINE");
      case VOICE_COMMAND_STATUS_SUCCESS:    return("SUCCESS");
      case VOICE_COMMAND_STATUS_FAILURE:    return("FAILURE");
      case VOICE_COMMAND_STATUS_NO_CMDS:    return("NO_CMDS");
      case VOICE_COMMAND_STATUS_TV_AVR_CMD: return("TV_AVR_CMD");
      case VOICE_COMMAND_STATUS_MIC_CMD:    return("MIC_CMD");
      case VOICE_COMMAND_STATUS_AUDIO_CMD:  return("AUDIO_CMD");
   }
   return(ctrlm_invalid_return(status));
}

const char *ctrlm_voice_command_status_tv_avr_str(ctrlm_voice_tv_avr_cmd_t cmd) {
    switch(cmd) {
        case CTRLM_VOICE_TV_AVR_CMD_POWER_OFF: return("POWER OFF");
        case CTRLM_VOICE_TV_AVR_CMD_POWER_ON:  return("POWER ON");
        case CTRLM_VOICE_TV_AVR_CMD_VOLUME_UP: return("VOLUME UP");
        case CTRLM_VOICE_TV_AVR_CMD_VOLUME_DOWN: return("VOLUME DOWN");
        case CTRLM_VOICE_TV_AVR_CMD_VOLUME_MUTE: return ("VOLUME MUTE");
    }
    return(ctrlm_invalid_return(cmd));
}

const char *ctrlm_voice_session_response_status_str(ctrlm_voice_session_response_status_t status) {
   switch(status) {
      case VOICE_SESSION_RESPONSE_AVAILABLE:                           return("AVAILABLE");
      case VOICE_SESSION_RESPONSE_BUSY:                                return("BUSY");
      case VOICE_SESSION_RESPONSE_SERVER_NOT_READY:                    return("NOT_READY");
      case VOICE_SESSION_RESPONSE_UNSUPPORTED_AUDIO_FORMAT:            return("UNSUPPORTED_AUDIO_FORMAT");
      case VOICE_SESSION_RESPONSE_FAILURE:                             return("FAILURE");
      case VOICE_SESSION_RESPONSE_AVAILABLE_SKIP_CHAN_CHECK:           return("AVAILABLE_SKIP_CHAN_CHECK");
      case VOICE_SESSION_RESPONSE_AVAILABLE_PAR_VOICE:                 return("AVAILABLE_PAR_VOICE");
      case VOICE_SESSION_RESPONSE_AVAILABLE_SKIP_CHAN_CHECK_PAR_VOICE: return("AVAILABLE_SKIP_CHAN_CHECK_PAR_VOICE");
   }
   return(ctrlm_invalid_return(status));
}

const char *ctrlm_voice_session_group_str(ctrlm_voice_session_group_t group) {
   switch(group) {
      case VOICE_SESSION_GROUP_DEFAULT: return("DEFAULT");
      #ifdef CTRLM_LOCAL_MIC_TAP
      case VOICE_SESSION_GROUP_MIC_TAP: return("MIC_TAP");
      #endif
      case VOICE_SESSION_GROUP_QTY: break; // fall thru to return an invalid group
   }
   return(ctrlm_invalid_return(group));
}

const char *ctrlm_voice_cert_type_str(ctrlm_voice_cert_type_t cert_type) {
   switch(cert_type) {
      case CTRLM_VOICE_CERT_TYPE_NONE:    return("NONE");
      case CTRLM_VOICE_CERT_TYPE_P12:     return("P12");
      case CTRLM_VOICE_CERT_TYPE_PEM:     return("PEM");
      case CTRLM_VOICE_CERT_TYPE_X509:    return("X509");
      case CTRLM_VOICE_CERT_TYPE_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(cert_type));
}

void ctrlm_voice_t::ctrlm_voice_xrsr_session_capture_start(ctrlm_main_queue_msg_audio_capture_start_t *capture_start) {
   xrsr_audio_container_t xrsr_container;

   if(this->voice_is_privacy_enabled()) {
      XLOGD_WARN("Privacy mode is enabled, audio capture is disabled");
      return;
   }

   if(CTRLM_AUDIO_CONTAINER_WAV == capture_start->container) {
      xrsr_container = XRSR_AUDIO_CONTAINER_WAV;
   } else if(CTRLM_AUDIO_CONTAINER_NONE == capture_start->container) {
      xrsr_container = XRSR_AUDIO_CONTAINER_NONE;
   } else {
      XLOGD_ERROR("invalid audio container");
      return;
   }

   XLOGD_INFO("container <%s> file path <%s> raw_mic_enable <%d>", xrsr_audio_container_str(xrsr_container), capture_start->file_path, capture_start->raw_mic_enable);

   if(!xrsr_session_capture_start(xrsr_container, capture_start->file_path, capture_start->raw_mic_enable)) {
      XLOGD_ERROR("failed to start audio capture");
      return;
   }

   this->capture_active = true;
}

void ctrlm_voice_t::ctrlm_voice_xrsr_session_capture_stop(void) {

   if(!this->capture_active) {
      XLOGD_WARN("audio capture is not active");
      return;
   }

   XLOGD_INFO("");

   if(!xrsr_session_capture_stop()) {
      XLOGD_ERROR("failed to stop audio capture");
      return;
   }

   this->capture_active = false;

}

void ctrlm_voice_t::audio_state_set(bool session) {
    switch(this->audio_mode) {
        case CTRLM_VOICE_AUDIO_MODE_MUTING: {
            ctrlm_dsmgr_mute_audio(session);
            break;
        }
        case CTRLM_VOICE_AUDIO_MODE_DUCKING: {
            ctrlm_dsmgr_duck_audio(session, (this->audio_ducking_type == CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE), this->audio_ducking_level);
            break;
        }
        default: {
            XLOGD_WARN("invalid audio mode");
            break;
        }
    }
}

bool ctrlm_voice_t::voice_session_id_is_current(uuid_t uuid) {
    bool found = false;

    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        ctrlm_voice_session_t *session = &this->voice_session[group];

        if(0 == uuid_compare(uuid, session->uuid)) {
           found = true;
           break;
        }
    }
    if(!found) {
       char uuid_str_rxd[37] = {'\0'};
       uuid_unparse_lower(uuid, uuid_str_rxd);

       for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
           ctrlm_voice_session_t *session = &this->voice_session[group];

           char uuid_str_exp[37] = {'\0'};
           uuid_unparse_lower(session->uuid, uuid_str_exp);

           XLOGD_WARN("uuid mismatch group <%s> exp <%s> rxd <%s> (ignoring)", ctrlm_voice_session_group_str((ctrlm_voice_session_group_t)group), uuid_str_exp, uuid_str_rxd);
       }
    }

    return(found);
}

ctrlm_voice_session_t *ctrlm_voice_t::voice_session_from_uuid(const uuid_t uuid) {
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      if(0 == uuid_compare(uuid, session->uuid)) {
         return(session);
      }
   }
   return(NULL);
}

ctrlm_voice_session_t *ctrlm_voice_t::voice_session_from_uuid(std::string &uuid) {
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      if(session->uuid_str == uuid) {
         return(session);
      }
   }
   return(NULL);
}

ctrlm_voice_session_t *ctrlm_voice_t::voice_session_from_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
      ctrlm_voice_session_t *session = &this->voice_session[group];
      if(session->network_id == network_id && session->controller_id == controller_id) {
         return(session);
      }
   }
   return(NULL);
}

unsigned long ctrlm_voice_t::voice_session_id_get() {
    return(this->session_id);
}

void ctrlm_voice_t::voice_set_ipc(ctrlm_voice_ipc_t *ipc) {
    if(this->voice_ipc) {
        this->voice_ipc->deregister_ipc();
        delete this->voice_ipc;
        this->voice_ipc = NULL;
    }
    this->voice_ipc = ipc;
}

void ctrlm_voice_t::voice_device_update_in_progress_set(bool in_progress) {
    // This function is used to disable voice when foreground download is active
    sem_wait(&this->device_status_semaphore);
    if(in_progress) {
        this->device_status[CTRLM_VOICE_DEVICE_PTT] |= CTRLM_VOICE_DEVICE_STATUS_DEVICE_UPDATE;
    } else {
        this->device_status[CTRLM_VOICE_DEVICE_PTT] &= ~CTRLM_VOICE_DEVICE_STATUS_DEVICE_UPDATE;
    }
    sem_post(&this->device_status_semaphore);
    XLOGD_INFO("Voice PTT is <%s>", in_progress ? "DISABLED" : "ENABLED");
}

void  ctrlm_voice_t::voice_power_state_change(ctrlm_power_state_t power_state) {

   xrsr_power_mode_t xrsr_power_mode = voice_xrsr_power_map(power_state);

   #ifdef CTRLM_LOCAL_MIC
   if(power_state == CTRLM_POWER_STATE_ON) {
      bool privacy_enabled = this->vsdk_is_privacy_enabled();
      if(privacy_enabled != this->voice_is_privacy_enabled()) {
         privacy_enabled ? this->voice_privacy_enable(false) : this->voice_privacy_disable(false);
      }
   }
   #endif

   if(!xrsr_power_mode_set(xrsr_power_mode)) {
      XLOGD_ERROR("failed to set xrsr to power state %s", ctrlm_power_state_str(power_state));
   }
}

bool ctrlm_voice_t::voice_session_can_request(ctrlm_voice_device_t device) {
   return((this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_MASK_SESSION_REQ) == 0);
}

void ctrlm_voice_t::voice_session_set_active(ctrlm_voice_device_t device) {
    sem_wait(&this->device_status_semaphore);
    if(this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE) {
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("device <%s> already active", ctrlm_voice_device_str(device));
        return;
    }
    this->device_status[device] |= CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE;
    sem_post(&this->device_status_semaphore);
}

void ctrlm_voice_t::voice_session_set_inactive(ctrlm_voice_device_t device) {
    sem_wait(&this->device_status_semaphore);
    if(!(this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE)) {
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("device <%s> already inactive", ctrlm_voice_device_str(device));
        return;
    }
    this->device_status[device] &= ~CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE;
    sem_post(&this->device_status_semaphore);
}

bool ctrlm_voice_t::voice_is_privacy_enabled(void) {
   #ifdef CTRLM_LOCAL_MIC
   sem_wait(&this->device_status_semaphore);
   bool value = (this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_PRIVACY) ? true : false;
   sem_post(&this->device_status_semaphore);
   return(value);
   #else
   return(false);
   #endif
}

void ctrlm_voice_t::voice_privacy_enable(bool update_vsdk) {
   #ifdef CTRLM_LOCAL_MIC
    sem_wait(&this->device_status_semaphore);
    if(this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_PRIVACY) {
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("already enabled");
        return;
    }

    this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] |= CTRLM_VOICE_DEVICE_STATUS_PRIVACY;
    ctrlm_db_voice_write_device_status(CTRLM_VOICE_DEVICE_MICROPHONE, (this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_MASK_DB));

    #ifdef CTRLM_LOCAL_MIC_DISABLE_VIA_PRIVACY
    if(update_vsdk && this->xrsr_opened && !xrsr_privacy_mode_set(true)) {
        XLOGD_ERROR("xrsr_privacy_mode_set failed");
    }
    #endif

    sem_post(&this->device_status_semaphore);
    #endif
}

void ctrlm_voice_t::voice_privacy_disable(bool update_vsdk) {
    #ifdef CTRLM_LOCAL_MIC
    sem_wait(&this->device_status_semaphore);
    if(!(this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_PRIVACY)) {
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("already disabled");
        return;
    }

    this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] &= ~CTRLM_VOICE_DEVICE_STATUS_PRIVACY;
    ctrlm_db_voice_write_device_status(CTRLM_VOICE_DEVICE_MICROPHONE, (this->device_status[CTRLM_VOICE_DEVICE_MICROPHONE] & CTRLM_VOICE_DEVICE_STATUS_MASK_DB));

    #ifdef CTRLM_LOCAL_MIC_DISABLE_VIA_PRIVACY
    if(update_vsdk && this->xrsr_opened && !xrsr_privacy_mode_set(false)) {
        XLOGD_ERROR("xrsr_privacy_mode_set failed");
    }
    #endif
    sem_post(&this->device_status_semaphore);
    #endif
}

void ctrlm_voice_t::voice_device_enable(ctrlm_voice_device_t device, bool db_write, bool *update_routes) {
    sem_wait(&this->device_status_semaphore);
    if((this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_DISABLED) == 0x00) { // if device IS NOT disabled
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("already enabled");
        return;
    }
    this->device_status[device] &= ~CTRLM_VOICE_DEVICE_STATUS_DISABLED;
    if(db_write) {
        ctrlm_db_voice_write_device_status(device, (this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_MASK_DB));
    }
    if(update_routes != NULL) {
        *update_routes = true;
    }

    sem_post(&this->device_status_semaphore);
}

void ctrlm_voice_t::voice_device_disable(ctrlm_voice_device_t device, bool db_write, bool *update_routes) {
    sem_wait(&this->device_status_semaphore);
    if(this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_DISABLED) { // if device IS disabled
        sem_post(&this->device_status_semaphore);
        XLOGD_WARN("already disabled");
        return;
    }
    this->device_status[device] |= CTRLM_VOICE_DEVICE_STATUS_DISABLED;
    if(db_write) {
        ctrlm_db_voice_write_device_status(device, (this->device_status[device] & CTRLM_VOICE_DEVICE_STATUS_MASK_DB));
    }
    if(update_routes != NULL) {
        *update_routes = true;
    }

    sem_post(&this->device_status_semaphore);
}

#ifdef BEEP_ON_KWD_ENABLED
void ctrlm_voice_system_audio_player_event_handler(system_audio_player_event_t event, void *user_data) {
   if(user_data == NULL) {
      return;
   }
   switch(event) {
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_STARTED: {
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_FINISHED: {
         // Send event to control manager thread to make this call.
         ctrlm_main_queue_handler_push(CTRLM_HANDLER_VOICE, (ctrlm_msg_handler_voice_t)&ctrlm_voice_t::voice_keyword_beep_completed_normal, NULL, 0, (void *)user_data);
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_PAUSED: {
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_RESUMED: {
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR");
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR");
         // Send event to control manager thread to make this call.
         ctrlm_main_queue_handler_push(CTRLM_HANDLER_VOICE, (ctrlm_msg_handler_voice_t)&ctrlm_voice_t::voice_keyword_beep_completed_error, NULL, 0, (void *)user_data);
         break;
      }
      case SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA: {
         XLOGD_ERROR("SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA");
         break;
      }
      default: {
         XLOGD_ERROR("INVALID EVENT");
         break;
      }
   }
}
#endif

#ifdef DEEP_SLEEP_ENABLED
void ctrlm_voice_t::voice_nsm_session_request(void) {
    ctrlm_network_id_t network_id = CTRLM_MAIN_NETWORK_ID_DSP;
    ctrlm_controller_id_t controller_id = CTRLM_MAIN_CONTROLLER_ID_DSP;
    ctrlm_voice_device_t device = CTRLM_VOICE_DEVICE_MICROPHONE;
    ctrlm_voice_format_t format = { .type = CTRLM_VOICE_FORMAT_PCM_32_BIT };

    #ifdef CTRLM_LOCAL_MIC_DISABLE_VIA_PRIVACY
    //If the user un-muted the microphones in standby, we must un-mute our components
    if(this->voice_is_privacy_enabled()) {
        this->voice_privacy_disable(true);
    }
    #endif

    this->nsm_voice_session = true;
    voice_session_req(network_id, controller_id, device, format, NULL, "DSP", "1",  "1",  0.0,  false,  NULL,  NULL,  NULL,  false, NULL);

}
#endif

int ctrlm_voice_t::packet_loss_threshold_get() const {
    return(this->packet_loss_threshold);
}

#ifdef VOICE_BUFFER_STATS
unsigned long voice_packet_interval_get(ctrlm_voice_format_t format, uint32_t opus_samples_per_packet) {
   if(format.type == CTRLM_VOICE_FORMAT_ADPCM_FRAME) {
      return((format.value.adpcm_frame.size_packet - format.value.adpcm_frame.size_header) * 2000 / 16); // amount of time between voice packets in microseconds (payloade size * 2/16000)*1000000
   } else if (format.type == CTRLM_VOICE_FORMAT_PCM) {
      return(20000);
   } else if (format.type == CTRLM_VOICE_FORMAT_OPUS_XVP || format.type == CTRLM_VOICE_FORMAT_OPUS) {
      return(opus_samples_per_packet * 1000 / 16); // amount of time between voice packets in microseconds (samples per packet/16000)*1000000
   }
   return(0);
}
#endif

xrsr_power_mode_t voice_xrsr_power_map(ctrlm_power_state_t ctrlm_power_state) {

   xrsr_power_mode_t xrsr_power_mode;

   switch(ctrlm_power_state) {
      case CTRLM_POWER_STATE_DEEP_SLEEP:
         #ifdef DEEP_SLEEP_ENABLED
         xrsr_power_mode = ctrlm_main_iarm_networked_standby() ? XRSR_POWER_MODE_LOW : XRSR_POWER_MODE_SLEEP;
         #else
         xrsr_power_mode = XRSR_POWER_MODE_SLEEP;
         #endif
         break;
      case CTRLM_POWER_STATE_ON:
         xrsr_power_mode = XRSR_POWER_MODE_FULL;
         break;
      default:
         XLOGD_WARN("defaulting to FULL because %s is unexpected", ctrlm_power_state_str(ctrlm_power_state));
         xrsr_power_mode = XRSR_POWER_MODE_FULL;
         break;
   }

   return xrsr_power_mode;
}

void ctrlm_voice_t::voice_rfc_retrieved_handler(const ctrlm_rfc_attr_t& attr) {
    bool enabled = true;
    bool reroute = false;

    attr.get_rfc_value(JSON_INT_NAME_VOICE_VREX_REQUEST_TIMEOUT,         this->prefs.timeout_vrex_connect,0);
    attr.get_rfc_value(JSON_INT_NAME_VOICE_VREX_RESPONSE_TIMEOUT,        this->prefs.timeout_vrex_session,0);
    attr.get_rfc_value(JSON_INT_NAME_VOICE_TIMEOUT_PACKET_INITIAL,       this->prefs.timeout_packet_initial);
    attr.get_rfc_value(JSON_INT_NAME_VOICE_TIMEOUT_PACKET_SUBSEQUENT,    this->prefs.timeout_packet_subsequent);
    attr.get_rfc_value(JSON_INT_NAME_VOICE_BITRATE_MINIMUM,              this->prefs.bitrate_minimum);
    attr.get_rfc_value(JSON_INT_NAME_VOICE_TIME_THRESHOLD,               this->prefs.time_threshold);
    if(attr.get_rfc_value(JSON_BOOL_NAME_VOICE_ENABLE_SAT,               this->sat_token_required)) {
        ctrlm_sm_voice_sat_enable_write(this->sat_token_required);
        XLOGD_TELEMETRY("require r_SAT <%s>",  this->sat_token_required  ? "YES" : "NO");
    }
    if(attr.get_rfc_value(JSON_BOOL_NAME_VOICE_ENABLE_MTLS,              this->mtls_required)) {
        ctrlm_sm_voice_mtls_enable_write(this->mtls_required);
        XLOGD_TELEMETRY("require r_MTLS <%s>", this->mtls_required ? "YES" : "NO");
    }
    if(attr.get_rfc_value(JSON_BOOL_NAME_VOICE_REQUIRE_SECURE_URL,       this->secure_url_required)) {
        ctrlm_sm_voice_secure_url_required_write(this->secure_url_required);
        XLOGD_TELEMETRY("require r_secure_url <%s>", this->secure_url_required ? "YES" : "NO");
    }
    attr.get_rfc_value(JSON_INT_NAME_VOICE_FFV_LEADING_SAMPLES,          this->prefs.ffv_leading_samples, 0);
    attr.get_rfc_value(JSON_STR_NAME_VOICE_APP_ID_HTTP,                  this->prefs.app_id_http);
    attr.get_rfc_value(JSON_STR_NAME_VOICE_APP_ID_WS,                    this->prefs.app_id_ws);
    attr.get_rfc_value(JSON_STR_NAME_VOICE_LANGUAGE,                     this->prefs.guide_language);

    double keyword_sensitivity = 0.0;
    if(attr.get_rfc_value(JSON_FLOAT_NAME_VOICE_KEYWORD_DETECT_SENSITIVITY, keyword_sensitivity)) {
        // Check keyword detector sensitivity value against limits; apply default if out of range.
        double sensitivity_set = this->vsdk_keyword_sensitivity_limit_check(keyword_sensitivity);
        if(sensitivity_set != keyword_sensitivity) {
            XLOGD_ERROR("keyword sensitivity <%5.2f> out of limits", keyword_sensitivity);
        } else {
            this->prefs.keyword_sensitivity = keyword_sensitivity;
            XLOGD_INFO("keyword sensitivity <%5.2f>", this->prefs.keyword_sensitivity);
            xrsr_keyword_config_t kw_config;
            kw_config.sensitivity = (float)this->prefs.keyword_sensitivity;
            if(!xrsr_keyword_config_set(&kw_config)) {
                XLOGD_ERROR("error updating keyword config");
            }
        }
    }
    attr.get_rfc_value(JSON_BOOL_NAME_VOICE_VREX_TEST_FLAG,              this->prefs.vrex_test_flag);
    attr.get_rfc_value(JSON_BOOL_NAME_VOICE_VREX_WUW_BYPASS_SUCCESS_FLAG,this->prefs.vrex_wuw_bypass_success_flag);
    attr.get_rfc_value(JSON_BOOL_NAME_VOICE_VREX_WUW_BYPASS_FAILURE_FLAG,this->prefs.vrex_wuw_bypass_failure_flag);
    attr.get_rfc_value(JSON_BOOL_NAME_VOICE_FORCE_TOGGLE_FALLBACK,       this->prefs.force_toggle_fallback);

    std::string opus_encoder_params_str;
    if(attr.get_rfc_value(JSON_STR_NAME_VOICE_OPUS_ENCODER_PARAMS, opus_encoder_params_str)) {
        if(!this->voice_params_opus_encoder_validate(opus_encoder_params_str)) {
            XLOGD_ERROR("opus encoder params <%s> invalid", opus_encoder_params_str.c_str());
        } else {
            XLOGD_INFO("opus encoder params <%s>", this->prefs.opus_encoder_params_str.c_str());
        }
    }

    if(attr.get_rfc_value(JSON_INT_NAME_VOICE_PAR_VOICE_EOS_METHOD, this->prefs.par_voice_eos_method, 0, UINT8_MAX)) {
        XLOGD_INFO("par voice eos method <%u>", this->prefs.par_voice_eos_method);
    }
    if(attr.get_rfc_value(JSON_INT_NAME_VOICE_PAR_VOICE_EOS_TIMEOUT, this->prefs.par_voice_eos_timeout, 0, UINT16_MAX)) {
        XLOGD_INFO("par voice eos timeout <%u>", this->prefs.par_voice_eos_timeout);
    }

    attr.get_rfc_value(JSON_INT_NAME_VOICE_PACKET_LOSS_THRESHOLD,        this->packet_loss_threshold, 0);
    if(attr.get_rfc_value(JSON_INT_NAME_VOICE_AUDIO_MODE, this->audio_mode) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_AUDIO_TIMING, this->audio_timing) |
       attr.get_rfc_value(JSON_FLOAT_NAME_VOICE_AUDIO_CONFIDENCE_THRESHOLD, this->audio_confidence_threshold, 0.0, 1.0) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_AUDIO_DUCKING_TYPE, this->audio_ducking_type, CTRLM_VOICE_AUDIO_DUCKING_TYPE_ABSOLUTE, CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE) |
       attr.get_rfc_value(JSON_FLOAT_NAME_VOICE_AUDIO_DUCKING_LEVEL, this->audio_ducking_level, 0.0, 1.0) |
       attr.get_rfc_value(JSON_BOOL_NAME_VOICE_AUDIO_DUCKING_BEEP, this->audio_ducking_beep_enabled)) {
        ctrlm_voice_audio_settings_t audio_settings = {this->audio_mode, this->audio_timing, this->audio_confidence_threshold, this->audio_ducking_type, this->audio_ducking_level, this->audio_ducking_beep_enabled};
        this->set_audio_mode(&audio_settings);
    }

    // All attributes that need capture configuration to be set
    if(attr.get_rfc_value(JSON_ARRAY_NAME_VOICE_SAVE_LAST_UTTERANCE,   this->prefs.utterance_save, ctrlm_is_production_build() ? CTRLM_JSON_ARRAY_INDEX_PRD : CTRLM_JSON_ARRAY_INDEX_DEV) |
       attr.get_rfc_value(JSON_BOOL_NAME_VOICE_UTTERANCE_USE_CURTAIL,  this->prefs.utterance_use_curtail) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_UTTERANCE_FILE_QTY_MAX,  this->prefs.utterance_file_qty_max, 1, 100000) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_UTTERANCE_FILE_SIZE_MAX, this->prefs.utterance_file_size_max, 4 * 1024) |
       attr.get_rfc_value(JSON_STR_NAME_VOICE_UTTERANCE_PATH,          this->prefs.utterance_path)) {
        xrsr_capture_config_t capture_config = {
           .delete_files  = !this->prefs.utterance_save,
           .enable        = this->prefs.utterance_save,
           .use_curtail   = this->prefs.utterance_use_curtail,
           .file_qty_max  = (uint32_t)this->prefs.utterance_file_qty_max,
           .file_size_max = (uint32_t)this->prefs.utterance_file_size_max,
           .dir_path      = this->prefs.utterance_path.c_str()
        };

        int ind = -1;
        errno_t safec_rc = strcmp_s(JSON_STR_VALUE_VOICE_UTTERANCE_PATH, strlen(JSON_STR_VALUE_VOICE_UTTERANCE_PATH), capture_config.dir_path, &ind);
        ERR_CHK(safec_rc);
        if((safec_rc == EOK) && (!ind)) {
           capture_config.dir_path = "/opt/logs"; // Default value specifies a file, but now it needs to be a directory
        }

        if(!xrsr_capture_config_set(&capture_config)) {
            XLOGD_ERROR("unable to set capture config");
        }
    }

    // All attributes that need a re-route to apply
    if(attr.get_rfc_value(JSON_INT_NAME_VOICE_MINIMUM_DURATION,                              this->prefs.utterance_duration_min) |
    #ifdef DEEP_SLEEP_ENABLED
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_CONNECT_CHECK_INTERVAL,     this->prefs.dst_params_standby.connect_check_interval) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_CONNECT,            this->prefs.dst_params_standby.timeout_connect) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_INACTIVITY,         this->prefs.dst_params_standby.timeout_inactivity) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_TIMEOUT_SESSION,            this->prefs.dst_params_standby.timeout_session) |
       attr.get_rfc_value(JSON_BOOL_NAME_VOICE_DST_PARAMS_STANDBY_IPV4_FALLBACK,             this->prefs.dst_params_standby.ipv4_fallback) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_STANDBY_BACKOFF_DELAY,              this->prefs.dst_params_standby.backoff_delay) |
    #endif
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_CONNECT_CHECK_INTERVAL, this->prefs.dst_params_low_latency.connect_check_interval) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_CONNECT,        this->prefs.dst_params_low_latency.timeout_connect) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_INACTIVITY,     this->prefs.dst_params_low_latency.timeout_inactivity) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_TIMEOUT_SESSION,        this->prefs.dst_params_low_latency.timeout_session) |
       attr.get_rfc_value(JSON_BOOL_NAME_VOICE_DST_PARAMS_LOW_LATENCY_IPV4_FALLBACK,         this->prefs.dst_params_low_latency.ipv4_fallback) |
       attr.get_rfc_value(JSON_INT_NAME_VOICE_DST_PARAMS_LOW_LATENCY_BACKOFF_DELAY,          this->prefs.dst_params_low_latency.backoff_delay)) {
        reroute = true;
    }

    std::vector<std::string> obj_server_hosts;
    if(attr.get_rfc_value(JSON_ARRAY_NAME_VOICE_SERVER_HOSTS, obj_server_hosts)) {
        this->url_hostname_patterns(obj_server_hosts);
    }

    attr.get_rfc_value(JSON_BOOL_NAME_VOICE_FORCE_VOICE_SETTINGS,        this->prefs.force_voice_settings);
    if(attr.get_rfc_value(JSON_BOOL_NAME_VOICE_FORCE_VOICE_SETTINGS, this->prefs.force_voice_settings) && this->prefs.force_voice_settings) {
        attr.get_rfc_value(JSON_BOOL_NAME_VOICE_ENABLE,                      enabled);
        attr.get_rfc_value(JSON_STR_NAME_VOICE_URL_SRC_PTT,                  this->prefs.server_url_src_ptt);
        attr.get_rfc_value(JSON_STR_NAME_VOICE_URL_SRC_FF,                   this->prefs.server_url_src_ff);
        #ifdef CTRLM_LOCAL_MIC_TAP
        attr.get_rfc_value(JSON_STR_NAME_VOICE_URL_SRC_MIC_TAP,              this->prefs.server_url_src_mic_tap);
        #endif
        // Check if enabled
        if(!enabled) {
            for(int i = CTRLM_VOICE_DEVICE_PTT; i < CTRLM_VOICE_DEVICE_INVALID; i++) {
                this->voice_device_disable((ctrlm_voice_device_t)i, true, NULL);
            }
        }
        reroute = true;
    }
    if(reroute) {
        this->voice_sdk_update_routes();
    }
}

void ctrlm_voice_t::vsdk_rfc_retrieved_handler(const ctrlm_rfc_attr_t& attr) {
    json_t *obj_vsdk = NULL;
    if(attr.get_rfc_json_value(&obj_vsdk) && obj_vsdk) {
        XLOGD_INFO("VSDK values from XCONF, reopening xrsr..");
        if(this->vsdk_config) {
            json_object_update_missing(obj_vsdk, this->vsdk_config);
        }
        // This is temporary until the VSDK supports receiving a config on the fly
        this->voice_sdk_close();
        this->voice_sdk_open(obj_vsdk);
        this->voice_sdk_update_routes();

        // Set init message if read from shared memory
        std::string init;
        bool valid = false;
        bool ret = ctrlm_sm_voice_init_blob_read(init, &valid);

        if(!ret) {
            XLOGD_ERROR("init blob read error");
        } else if(!valid) { // Init blob is not present on cold boot until set by Voice Control Thunder API
            XLOGD_WARN("init blob not set");
        } else if(init.empty()) {
            XLOGD_ERROR("empty init blob");
        } else {
            this->voice_init_set(init.c_str(), false);
        }
        json_decref(obj_vsdk);
    }
}

void ctrlm_voice_t::telemetry_report_handler() {
    XLOGD_INFO("clearing vsr_errs");
    #ifndef TELEMETRY_SUPPORT
    XLOGD_WARN("telemetry is not enabled");
    #else
    ctrlm_voice_session_t *session = &this->voice_session[VOICE_SESSION_GROUP_DEFAULT];
    sem_wait(&session->current_vsr_err_semaphore);
    this->vsr_errors.clear();
    sem_post(&session->current_vsr_err_semaphore);
    #endif
}

void ctrlm_voice_t::url_hostname_pattern_add(const char *pattern) {
    // Convert the input pattern to a regular expression
    // input must match the entire string so start of line and end of line are added
    std::string regex;
    regex.append("^");
    regex.append(pattern);
    regex.append("$");

    // Escape . characters
    size_t pos = 0;
    do {
        pos = regex.find(".", pos);
        if(pos != std::string::npos) {
            regex.replace(pos, 1, "\\.");
            pos += 2;
        } else {
            break;
        }
    } while(1);

    // Replace * with .*
    pos = 0;
    do {
        pos = regex.find("*", pos);
        if(pos != std::string::npos) {
            regex.replace(pos, 1, ".*");
            pos += 2;
        } else {
            break;
        }
    } while(1);

    this->prefs.server_hosts.push_back(regex);
}

void ctrlm_voice_t::url_hostname_patterns(const std::vector<std::string> &obj_server_hosts) {
    this->prefs.server_hosts.clear();

    for(auto &itr : obj_server_hosts) {
        XLOGD_INFO("Adding server host pattern <%s>", itr.c_str());
        this->url_hostname_pattern_add(itr.c_str());
    }
}
