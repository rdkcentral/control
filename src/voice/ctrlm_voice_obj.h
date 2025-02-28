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
#ifndef __CTRLM_VOICE_H__
#define __CTRLM_VOICE_H__
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <uuid/uuid.h>
#include <openssl/ssl.h>
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_voice.h"
#include "ctrlm.h"
#include "ctrlm_auth.h"
#include "jansson.h"
#include "json_config.h"
#include "xr_timestamp.h"
#include "ctrlm_voice_types.h"
#include "ctrlm_voice_ipc.h"
#include "ctrlm_rfc.h"
#include "xrsr.h"
#include "ctrlm_voice_telemetry_events.h"

#ifdef BEEP_ON_KWD_ENABLED
#include "ctrlm_thunder_plugin_system_audio_player.h"
#endif

#define VOICE_SESSION_REQ_DATA_LEN_MAX (33)

typedef enum {
   VOICE_SESSION_GROUP_DEFAULT = 0, // Session index for regular voice sessions (PTT, FFV)
   #ifdef CTRLM_LOCAL_MIC_TAP
   VOICE_SESSION_GROUP_MIC_TAP = 1, // Session index for microphone tap voice sessions
   VOICE_SESSION_GROUP_QTY     = 2
   #else
   VOICE_SESSION_GROUP_QTY     = 1
   #endif
} ctrlm_voice_session_group_t;

#ifdef VOICE_BUFFER_STATS
#define VOICE_BUFFER_WARNING_THRESHOLD (4) // Number of packets in HW buffer before printing.  Set to zero to print all packets.
#endif

#define CTRLM_VOICE_AUDIO_SETTINGS_INITIALIZER    {(ctrlm_voice_audio_mode_t)JSON_INT_VALUE_VOICE_AUDIO_MODE, (ctrlm_voice_audio_timing_t)JSON_INT_VALUE_VOICE_AUDIO_TIMING, JSON_FLOAT_VALUE_VOICE_AUDIO_CONFIDENCE_THRESHOLD, (ctrlm_voice_audio_ducking_type_t)JSON_INT_VALUE_VOICE_AUDIO_DUCKING_TYPE, JSON_FLOAT_VALUE_VOICE_AUDIO_DUCKING_LEVEL, JSON_BOOL_VALUE_VOICE_AUDIO_DUCKING_BEEP}
typedef enum {
   CTRLM_VOICE_AUDIO_MODE_OFF             = 0,
   CTRLM_VOICE_AUDIO_MODE_MUTING          = 1,
   CTRLM_VOICE_AUDIO_MODE_DUCKING         = 2
} ctrlm_voice_audio_mode_t; // explicitly defined each value as they are passed through RFC

typedef enum {
   CTRLM_VOICE_AUDIO_TIMING_VSR              = 0,
   CTRLM_VOICE_AUDIO_TIMING_CLOUD            = 1,
   CTRLM_VOICE_AUDIO_TIMING_CONFIDENCE       = 2,
   CTRLM_VOICE_AUDIO_TIMING_DUAL_SENSITIVITY = 3,
} ctrlm_voice_audio_timing_t; // explicitly defined each value as they are passed through RFC

typedef double ctrlm_voice_audio_confidence_threshold_t;
typedef double ctrlm_voice_audio_ducking_level_t;

typedef enum {
   CTRLM_VOICE_AUDIO_DUCKING_TYPE_ABSOLUTE = 0,
   CTRLM_VOICE_AUDIO_DUCKING_TYPE_RELATIVE = 1,
} ctrlm_voice_audio_ducking_type_t;

typedef struct {
   ctrlm_voice_audio_mode_t                 mode;
   ctrlm_voice_audio_timing_t               timing;
   ctrlm_voice_audio_confidence_threshold_t confidence_threshold;
   ctrlm_voice_audio_ducking_type_t         ducking_type;
   ctrlm_voice_audio_ducking_level_t        ducking_level;
   bool                                     ducking_beep;
} ctrlm_voice_audio_settings_t;

typedef enum {
   VOICE_COMMAND_STATUS_PENDING    = 0,
   VOICE_COMMAND_STATUS_TIMEOUT    = 1,
   VOICE_COMMAND_STATUS_OFFLINE    = 2,
   VOICE_COMMAND_STATUS_SUCCESS    = 3,
   VOICE_COMMAND_STATUS_FAILURE    = 4,
   VOICE_COMMAND_STATUS_NO_CMDS    = 5,
   VOICE_COMMAND_STATUS_TV_AVR_CMD = 6,
   VOICE_COMMAND_STATUS_MIC_CMD    = 7,
   VOICE_COMMAND_STATUS_AUDIO_CMD  = 8
} ctrlm_voice_command_status_t;

typedef enum {
   VOICE_SESSION_TYPE_STANDARD  = 0x00,
   VOICE_SESSION_TYPE_SIGNALING = 0x01,
   VOICE_SESSION_TYPE_FAR_FIELD = 0x02,
} voice_session_type_t;

typedef enum {
   VOICE_SESSION_RESPONSE_AVAILABLE                           = 0x00,
   VOICE_SESSION_RESPONSE_BUSY                                = 0x01,
   VOICE_SESSION_RESPONSE_SERVER_NOT_READY                    = 0x02,
   VOICE_SESSION_RESPONSE_UNSUPPORTED_AUDIO_FORMAT            = 0x03,
   VOICE_SESSION_RESPONSE_FAILURE                             = 0x04,
   VOICE_SESSION_RESPONSE_AVAILABLE_SKIP_CHAN_CHECK           = 0x05,
   VOICE_SESSION_RESPONSE_AVAILABLE_PAR_VOICE                 = 0x10,
   VOICE_SESSION_RESPONSE_AVAILABLE_SKIP_CHAN_CHECK_PAR_VOICE = 0x11,
} ctrlm_voice_session_response_status_t;

typedef enum {
   VOICE_SESSION_RESPONSE_STREAM_BUF_BEGIN     = 0x00,
   VOICE_SESSION_RESPONSE_STREAM_KEYWORD_BEGIN = 0x01,
   VOICE_SESSION_RESPONSE_STREAM_KEYWORD_END   = 0x02,
} voice_session_response_stream_t;

typedef enum {
    CTRLM_VOICE_STATE_SRC_READY     = 0x00,
    CTRLM_VOICE_STATE_SRC_STREAMING = 0x01,
    CTRLM_VOICE_STATE_SRC_WAITING   = 0x02,
    CTRLM_VOICE_STATE_SRC_INVALID   = 0x03
} ctrlm_voice_state_src_t;

typedef enum {
    CTRLM_VOICE_STATE_DST_READY      = 0x00,
    CTRLM_VOICE_STATE_DST_REQUESTED  = 0x01,
    CTRLM_VOICE_STATE_DST_OPENED     = 0x02,
    CTRLM_VOICE_STATE_DST_STREAMING  = 0x03,
    CTRLM_VOICE_STATE_DST_INVALID    = 0x04
} ctrlm_voice_state_dst_t;

// The voice device status is a bitfield with bits for enable/disable, privacy on/off, session active/inactive, OTA active/inactive
#define CTRLM_VOICE_DEVICE_STATUS_NONE           (0x00)
#define CTRLM_VOICE_DEVICE_STATUS_LEGACY         (0x07) // Legacy values (only disable flag is maintained)
#define CTRLM_VOICE_DEVICE_STATUS_DISABLED       (0x02)
#define CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED  (0x08)
#define CTRLM_VOICE_DEVICE_STATUS_SESSION_ACTIVE (0x10)
#define CTRLM_VOICE_DEVICE_STATUS_DEVICE_UPDATE  (0x20)
#define CTRLM_VOICE_DEVICE_STATUS_PRIVACY        (0x40)
#define CTRLM_VOICE_DEVICE_STATUS_RESERVED       (0x80)

// Mask for values that are stored in the DB (persistent across reboots and application restart)
#define CTRLM_VOICE_DEVICE_STATUS_MASK_DB          (CTRLM_VOICE_DEVICE_STATUS_DISABLED | CTRLM_VOICE_DEVICE_STATUS_PRIVACY)
// Mask for values that shall prevent a voice session from being granted
#define CTRLM_VOICE_DEVICE_STATUS_MASK_SESSION_REQ (CTRLM_VOICE_DEVICE_STATUS_DISABLED | CTRLM_VOICE_DEVICE_STATUS_DEVICE_UPDATE | CTRLM_VOICE_DEVICE_STATUS_PRIVACY | CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED)

typedef enum {
   CTRLM_VOICE_REMOTE_VOICE_END_MIC_KEY_RELEASE     =  1,
   CTRLM_VOICE_REMOTE_VOICE_END_EOS_DETECTION       =  2,
   CTRLM_VOICE_REMOTE_VOICE_END_SECONDARY_KEY_PRESS =  3,
   CTRLM_VOICE_REMOTE_VOICE_END_TIMEOUT_MAXIMUM     =  4,
   CTRLM_VOICE_REMOTE_VOICE_END_TIMEOUT_TARGET      =  5,
   CTRLM_VOICE_REMOTE_VOICE_END_OTHER_ERROR         =  6
} ctrlm_voice_remote_voice_end_reasons_t;

typedef enum {
   CTRLM_VOICE_TV_AVR_CMD_POWER_OFF   = 0,
   CTRLM_VOICE_TV_AVR_CMD_POWER_ON    = 1,
   CTRLM_VOICE_TV_AVR_CMD_VOLUME_UP   = 2,
   CTRLM_VOICE_TV_AVR_CMD_VOLUME_DOWN = 3,
   CTRLM_VOICE_TV_AVR_CMD_VOLUME_MUTE = 4,
} ctrlm_voice_tv_avr_cmd_t;


#define CTRLM_FAR_FIELD_BEAMS_MAX (4)

typedef struct {
   guint32 pre_keyword_sample_qty;
   guint32 keyword_sample_qty;
   guint16 doa;
   bool    standard_search_pt_triggered;
   double  standard_search_pt;
   bool    high_search_pt_support;
   bool    high_search_pt_triggered;
   double  high_search_pt;
   double  dynamic_gain;
   struct {
      bool     selected;
      bool     triggered;
      uint16_t angle; // 0-360
      double   confidence;
      bool     confidence_normalized;
      double   snr;
   } beams[CTRLM_FAR_FIELD_BEAMS_MAX];
   bool    push_to_talk;
} voice_session_req_stream_params;

typedef struct {
   ctrlm_controller_id_t                 controller_id;
   ctrlm_timestamp_t                     timestamp;
   voice_session_type_t                  type;
   ctrlm_voice_format_t                  audio_format;
   unsigned char                         data_len;
   unsigned char                         data[VOICE_SESSION_REQ_DATA_LEN_MAX];
   ctrlm_voice_session_response_status_t status;
   ctrlm_voice_session_abort_reason_t    reason;
} ctrlm_main_queue_msg_voice_session_request_t;


typedef struct {
   ctrlm_controller_id_t            controller_id;
   ctrlm_timestamp_t                timestamp;
   ctrlm_voice_session_end_reason_t session_end_reason;
   unsigned char                    key_code;
} ctrlm_main_queue_msg_voice_session_stop_t;

typedef struct {
   ctrlm_controller_id_t            controller_id;
   ctrlm_voice_session_end_reason_t reason;
   unsigned char                    utterance_too_short;
} ctrlm_main_queue_msg_voice_session_end_t;

typedef struct {
   ctrlm_controller_id_t            controller_id;
   char                             transcription[CTRLM_VOICE_SESSION_TEXT_MAX_LENGTH];
   unsigned char                    success;
} ctrlm_main_queue_msg_voice_session_result_t;

typedef struct {
   ctrlm_controller_id_t         controller_id;
   ctrlm_voice_command_status_t  status;
   union {
      struct {
         ctrlm_voice_tv_avr_cmd_t cmd;
         bool                     toggle_fallback;
         unsigned char            ir_repeats;
      } tv_avr;
   } data;
} ctrlm_main_queue_msg_voice_command_status_t;

// Event Callback Structures
typedef struct {
   uuid_t                       uuid;
   rdkx_timestamp_t             timestamp;
} ctrlm_voice_cb_header_t;

typedef struct {
    ctrlm_voice_cb_header_t      header;
    xrsr_src_t                   src;
    xrsr_session_config_out_t    configuration;
    ctrlm_voice_endpoint_t       *endpoint;
    bool                         keyword_verification;
} ctrlm_voice_session_begin_cb_t;

typedef struct {
    ctrlm_voice_cb_header_t  header;
    xrsr_src_t               src;
} ctrlm_voice_stream_begin_cb_t;

typedef struct {
    ctrlm_voice_cb_header_t  header;
    xrsr_stream_stats_t      stats;
} ctrlm_voice_stream_end_cb_t;

typedef struct {
    ctrlm_voice_cb_header_t  header;
    bool                     retry;
} ctrlm_voice_disconnected_cb_t;

typedef struct {
    ctrlm_voice_cb_header_t     header;
    bool                        success;
    xrsr_session_stats_t        stats;
} ctrlm_voice_session_end_cb_t;

// End Event Callback Structures

typedef struct {
   std::string                 server_url_src_ptt;
   std::string                 server_url_src_ff;
   #ifdef CTRLM_LOCAL_MIC_TAP
   std::string                 server_url_src_mic_tap;
   #endif
   std::vector<std::string>    server_hosts;
   std::string                 aspect_ratio;
   std::string                 guide_language;
   std::string                 app_id_http;
   std::string                 app_id_ws;
   unsigned long               timeout_vrex_connect;
   unsigned long               timeout_vrex_session;
   unsigned long               timeout_stats;
   unsigned long               timeout_packet_initial;
   unsigned long               timeout_packet_subsequent;
   guchar                      bitrate_minimum;
   guint16                     time_threshold;
   bool                        utterance_save;
   bool                        utterance_use_curtail;
   unsigned long               utterance_file_qty_max;
   unsigned long               utterance_file_size_max;
   std::string                 utterance_path;
   unsigned long               utterance_duration_min;
   unsigned long               ffv_leading_samples;
   bool                        force_voice_settings;
   double                      keyword_sensitivity;
   bool                        vrex_test_flag;
   bool                        vrex_wuw_bypass_success_flag;
   bool                        vrex_wuw_bypass_failure_flag;
   std::string                 opus_encoder_params_str;
   uint8_t                     opus_encoder_params[CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS];
   bool                        force_toggle_fallback;
   #ifdef DEEP_SLEEP_ENABLED
   xrsr_dst_params_t           dst_params_standby;
   #endif
   xrsr_dst_params_t           dst_params_low_latency;
   bool                        par_voice_enabled;
   uint8_t                     par_voice_eos_method;
   uint16_t                    par_voice_eos_timeout;
} voice_session_prefss_t;

typedef struct {
   guint16 timeout_packet_initial;
   guint16 timeout_packet_subsequent;
   guchar  bitrate_minimum;
   guint16 time_threshold;
} voice_params_qos_t;

typedef struct {
   guchar data[CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS];
} voice_params_opus_encoder_t;

typedef struct {
   bool     par_voice_enabled;
   uint8_t  par_voice_eos_method;
   uint16_t par_voice_eos_timeout;
} voice_params_par_t;

typedef struct {
   bool             available;
   bool             connect_attempt;
   bool             connect_success;
   bool             has_keyword;
   rdkx_timestamp_t ctrl_request;
   rdkx_timestamp_t ctrl_response;
   rdkx_timestamp_t ctrl_audio_rxd_first;
   rdkx_timestamp_t ctrl_audio_rxd_keyword;
   rdkx_timestamp_t ctrl_audio_rxd_final;
   rdkx_timestamp_t ctrl_stop;
   rdkx_timestamp_t ctrl_cmd_status_wr;
   rdkx_timestamp_t ctrl_cmd_status_rd;
   rdkx_timestamp_t srvr_request;
   rdkx_timestamp_t srvr_connect;
   rdkx_timestamp_t srvr_init_txd;
   rdkx_timestamp_t srvr_audio_txd_first;
   rdkx_timestamp_t srvr_audio_txd_keyword;
   rdkx_timestamp_t srvr_audio_txd_final;
   rdkx_timestamp_t srvr_rsp_keyword;
   rdkx_timestamp_t srvr_disconnect;
} ctrlm_voice_session_timing_t;

typedef struct {
   std::string                     controller_name;
   std::string                     controller_version_sw;
   std::string                     controller_version_hw;
   double                          controller_voltage;
   std::string                     stb_name;
   uint32_t                        ffv_leading_samples; //This was a long. We can't have that many samples and "if(stream_params->keyword_sample_begin > info.ffv_leading_samples)" evaluates to true if keyword_sample_begin is < 0 and ffv_leading_samples is long
   bool                            has_stream_params;
   voice_session_req_stream_params stream_params;
   std::string                     rf_protocol;
} ctrlm_voice_session_info_t;

typedef struct {
   unsigned long rf_channel;       // The rf channel that the voice session used (typically 15, 20 or 25)
} ctrlm_voice_session_end_stats_t;

typedef struct {
   bool prv;
   bool wwFeedback;
} ctrlm_voice_status_capabilities_t;

typedef struct {
   std::string                       urlPtt;
   std::string                       urlHf;
   #ifdef CTRLM_LOCAL_MIC_TAP
   std::string                       urlMicTap;
   #endif
   uint8_t                           status[CTRLM_VOICE_DEVICE_INVALID];
   bool                              prv_enabled;
   bool                              wwFeedback;
   ctrlm_voice_status_capabilities_t capabilities;
} ctrlm_voice_status_t;

typedef void (*ctrlm_voice_session_rsp_confirm_t)(bool result, signed long long rsp_time, unsigned int rsp_window, const std::string &err_str, ctrlm_timestamp_t *timestamp, void *user_data);

typedef struct {
   ctrlm_network_id_t               network_id;
   ctrlm_network_type_t             network_type;
   ctrlm_controller_id_t            controller_id;
   std::string                      controller_name;
   std::string                      controller_version_sw;
   std::string                      controller_version_hw;
   double                           controller_voltage;
   bool                             controller_command_status;
   ctrlm_voice_device_t             voice_device;
   ctrlm_voice_format_t             format;
   long                             server_ret_code;
   std::string                      server_message;
   bool                             has_stream_params;
   voice_session_req_stream_params  stream_params;

   bool                             session_active_server;
   bool                             session_active_controller;
   ctrlm_voice_state_src_t          state_src;
   ctrlm_voice_state_dst_t          state_dst;
   uuid_t                           uuid;
   std::string                      uuid_str;
   int                              audio_pipe[2];
   unsigned long                    audio_sent_bytes;
   unsigned long                    audio_sent_samples;
   bool                             requested_more_audio;

   // RF4CE specific fields (move to a separate struct?)
   uint32_t                         packets_processed;
   uint32_t                         packets_lost;
   uint32_t                         lqi_total;

   ctrlm_voice_session_end_reason_t end_reason;

   bool                             is_session_by_text;
   std::string                      transcription_in;
   std::string                      transcription;

   bool                             is_session_by_file;
   bool                             is_session_by_fifo;

   bool                             keyword_verified;

   ctrlm_voice_endpoint_t *         endpoint_current;

   ctrlm_voice_ipc_event_common_t   ipc_common_data;

   ctrlm_main_queue_msg_voice_command_status_t status;

   unsigned long                    stats_session_id;
   ctrlm_voice_stats_reboot_t       stats_reboot;
   ctrlm_voice_stats_session_t      stats_session;

   ctrlm_voice_session_timing_t     session_timing;
   double                           confidence;
   bool                             dual_sensitivity_immediate;

   sem_t                            current_vsr_err_semaphore;
   std::string                      current_vsr_err_string;
   signed long long                 current_vsr_err_rsp_time;
   unsigned int                     current_vsr_err_rsp_window;

   unsigned int                     timeout_ctrl_cmd_status_read;
   unsigned int                     timeout_packet_tag;

   uint8_t                          last_cmd_id; // Needed for ADPCM over RF4CE, since duplicate packets are possible
   uint8_t                          next_cmd_id;

} ctrlm_voice_session_t;

class ctrlm_voice_t {
    public:

    // Application Interface
    ctrlm_voice_t();
    virtual ~ctrlm_voice_t();

    ctrlm_voice_session_response_status_t voice_session_req(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_voice_device_t device_type, ctrlm_voice_format_t format, voice_session_req_stream_params *stream_params, const char *controller_name, const char *sw_version, const char *hw_version, double voltage, bool command_status=false, ctrlm_timestamp_t *timestamp=NULL, ctrlm_voice_session_rsp_confirm_t *cb_confirm=NULL, void **cb_confirm_param=NULL, bool use_external_data_pipe=false, const char *transcription_in=NULL, const char *audio_file_in=NULL, const uuid_t *uuid = NULL, bool low_latency=false, bool low_cpu_util=false, int audio_fd = -1);
    void                                  voice_session_rsp_confirm(bool result, signed long long rsp_time, unsigned int rsp_window, const std::string &err_str, ctrlm_timestamp_t *timestamp);
    bool                                  voice_session_data(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, const char *buffer, long unsigned int length, ctrlm_timestamp_t *timestamp=NULL, uint8_t *lqi=NULL);
    bool                                  voice_session_data(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int fd, const uuid_t *uuid=NULL);
    void                                  voice_session_data_post_processing(int bytes_sent, const char *action, ctrlm_timestamp_t *timestamp);
    void                                  voice_session_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_voice_session_end_reason_t reason, ctrlm_timestamp_t *timestamp=NULL, ctrlm_voice_session_end_stats_t *stats=NULL);
    void                                  voice_session_end(ctrlm_voice_session_t *session, ctrlm_voice_session_end_reason_t reason, ctrlm_timestamp_t *timestamp=NULL, ctrlm_voice_session_end_stats_t *stats=NULL);
    void                                  voice_session_controller_stats_rxd_timeout();
    void                                  voice_session_stats(ctrlm_voice_stats_session_t session);
    void                                  voice_session_stats(ctrlm_voice_stats_reboot_t reboot);
    bool                                  voice_session_term(std::string &session_id);
    bool                                  voice_session_audio_stream_start(std::string &session_id);
    void                                  voice_session_info(xrsr_src_t src, ctrlm_voice_session_info_t *data);
    void                                  voice_session_info(ctrlm_voice_session_t *session, ctrlm_voice_session_info_t *data);
    bool                                  voice_session_id_is_current(uuid_t uuid);
    unsigned long                         voice_session_id_get();
    ctrlm_voice_session_t *               voice_session_from_uuid(const uuid_t uuid);
    ctrlm_voice_session_t *               voice_session_from_uuid(std::string &uuid);
    ctrlm_voice_session_t *               voice_session_from_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

    virtual void                          voice_stb_data_stb_sw_version_set(std::string &sw_version);
    std::string                           voice_stb_data_stb_sw_version_get() const;
    virtual void                          voice_stb_data_stb_name_set(std::string &stb_name);
    std::string                           voice_stb_data_stb_name_get() const;
    virtual void                          voice_stb_data_account_number_set(std::string &account_number);
    std::string                           voice_stb_data_account_number_get() const;
    virtual void                          voice_stb_data_receiver_id_set(std::string &receiver_id);
    std::string                           voice_stb_data_receiver_id_get() const;
    virtual void                          voice_stb_data_device_id_set(std::string &device_id);
    std::string                           voice_stb_data_device_id_get() const;
    virtual void                          voice_stb_data_device_type_set(ctrlm_device_type_t device_type);
    ctrlm_device_type_t                   voice_stb_data_device_type_get() const;
    virtual void                          voice_stb_data_partner_id_set(std::string &partner_id);
    std::string                           voice_stb_data_partner_id_get() const;
    virtual void                          voice_stb_data_experience_set(std::string &experience);
    std::string                           voice_stb_data_experience_get() const;
    std::string                           voice_stb_data_app_id_http_get() const;
    std::string                           voice_stb_data_app_id_ws_get() const;
    virtual void                          voice_stb_data_guide_language_set(const char *language);
    std::string                           voice_stb_data_guide_language_get() const;
    virtual void                          voice_stb_data_sat_set(std::string &sat_token);
    const char *                          voice_stb_data_sat_get() const;
    bool                                  voice_stb_data_test_get() const;
    bool                                  voice_stb_data_bypass_wuw_verify_success_get() const;
    bool                                  voice_stb_data_bypass_wuw_verify_failure_get() const;
    virtual void                          voice_stb_data_pii_mask_set(bool mask_pii);
    bool                                  voice_stb_data_pii_mask_get() const;
    virtual bool                          voice_stb_data_device_certificate_set(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca);
    virtual bool                          voice_stb_data_device_certificate_set(const char *p12_cert, const char *p12_pass);
    virtual bool                          voice_stb_data_device_certificate_set(const char *pem_cert, const char *pem_pkey, const char *pem_chain, const char *pem_passphrase);
    virtual bool                          voice_stb_data_device_certificate_set(X509 *cert_x509, EVP_PKEY *cert_pkey, STACK_OF(X509) *cert_chain);
    virtual void                          voice_stb_data_device_certificate_get(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca);

    virtual bool                          voice_configure_config_file_json(json_t *obj_voice, json_t *json_obj_vsdk, bool local_conf);
    virtual bool                          voice_configure(ctrlm_voice_iarm_call_settings_t *settings, bool db_write);
    virtual bool                          voice_configure(json_t *settings, bool db_write);
    virtual bool                          voice_status(ctrlm_voice_status_t *status);
    virtual bool                          voice_init_set(const char *init, bool db_write = true);
    virtual bool                          voice_message(std::string &uuid, const char *msg);
    virtual bool                          server_message(const char *message, unsigned long length);
    void                                  voice_params_qos_get(voice_params_qos_t *params);
    void                                  voice_params_opus_encoder_get(voice_params_opus_encoder_t *params);
    void                                  voice_params_par_get(voice_params_par_t *params);
    virtual void                          process_xconf(json_t **json_obj_vsdk, bool local_conf);
    virtual void                          query_strings_updated();
    int                                   packet_loss_threshold_get() const;

    bool                                  voice_device_streaming(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
    void                                  voice_controller_command_status_read(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
    void                                  voice_status_set(ctrlm_voice_session_t *session);
    void                                  voice_status_set(const uuid_t uuid);
    void                                  voice_device_update_in_progress_set(bool in_progress);

    void                                  voice_rfc_retrieved_handler(const ctrlm_rfc_attr_t& attr);
    void                                  vsdk_rfc_retrieved_handler(const ctrlm_rfc_attr_t& attr);
    void                                  telemetry_report_handler();
    void                                  url_hostname_patterns(const std::vector<std::string> &obj_server_hosts);
    void                                  url_hostname_pattern_add(const char *pattern);
    void                                  ctrlm_voice_xrsr_session_capture_start(ctrlm_main_queue_msg_audio_capture_start_t *capture_start);
    void                                  ctrlm_voice_xrsr_session_capture_stop(void);
    bool                                  nsm_voice_session;

    // Helper semaphores for synchronization
    sem_t                    vsr_semaphore;

protected:
    void                                  voice_session_timeout();
    void                                  voice_session_controller_command_status_read_timeout();
    void                                  voice_session_stats_clear(ctrlm_voice_session_t *session);
    void                                  voice_session_stats_print(ctrlm_voice_session_t *session);
    void                                  voice_session_notify_stats();

    ctrlm_voice_state_src_t               voice_state_src_get(ctrlm_voice_session_group_t index) const;
    ctrlm_voice_state_dst_t               voice_state_dst_get(ctrlm_voice_session_group_t index) const;

    void                                  voice_session_info_reset(ctrlm_voice_session_t *session);

    void                                  voice_set_ipc(ctrlm_voice_ipc_t *ipc);
    // End Application Interface

    // Static Callbacks
    static int ctrlm_voice_packet_timeout(void *data);
    static int ctrlm_voice_controller_session_stats_rxd_timeout(void *data);
    static int ctrlm_voice_controller_command_status_read_timeout(void *data);
    #ifdef BEEP_ON_KWD_ENABLED
    static int ctrlm_voice_keyword_beep_end_timeout(void *data);
    #endif
    // End Static Callbacks

    // Event Interface
public:
    virtual void                  voice_session_begin_callback(ctrlm_voice_session_begin_cb_t *session_begin);
    virtual void                  voice_stream_begin_callback(ctrlm_voice_stream_begin_cb_t *stream_begin);
    virtual void                  voice_stream_end_callback(ctrlm_voice_stream_end_cb_t *stream_end);
    virtual void                  voice_session_end_callback(ctrlm_voice_session_end_cb_t *sesison_end);
    virtual void                  voice_server_message_callback(const char *msg, unsigned long length);
    virtual void                  voice_server_connected_callback(ctrlm_voice_cb_header_t *connected);
    virtual void                  voice_server_disconnected_callback(ctrlm_voice_disconnected_cb_t *disconnected);
    virtual void                  voice_server_sent_init_callback(ctrlm_voice_cb_header_t *init);
    virtual void                  voice_stream_kwd_callback(ctrlm_voice_cb_header_t *kwd);
    virtual void                  voice_action_tv_mute_callback(bool mute);
    virtual void                  voice_action_tv_power_callback(bool power, bool toggle);
    virtual void                  voice_action_tv_volume_callback(bool up, uint32_t repeat_count);
    virtual void                  voice_action_keyword_verification_callback(const uuid_t uuid, bool success, rdkx_timestamp_t timestamp);
    virtual void                  voice_server_return_code_callback(const uuid_t uuid, const char *reason, long ret_code);
    virtual void                  voice_session_transcription_callback(const uuid_t uuid, const char *transcription);
    virtual void                  voice_power_state_change(ctrlm_power_state_t power_state);
    #ifdef DEEP_SLEEP_ENABLED
    virtual void                  voice_nsm_session_request(void);
    #endif
    virtual void                  voice_keyword_verified_action(void);
    #ifdef BEEP_ON_KWD_ENABLED
    virtual void                  voice_keyword_beep_completed_normal(void *data, int size);
    virtual void                  voice_keyword_beep_completed_error(void *data, int size);
    virtual void                  voice_keyword_beep_completed_callback(bool timeout, bool playback_error);
    #endif
    // End Event Interface

    protected:
    virtual void          voice_sdk_open(json_t *json_obj_vsdk);
    virtual void          voice_sdk_close();
    virtual void          voice_sdk_update_routes() = 0;
    virtual void          mask_pii_updated(bool enable) = 0;
    virtual bool          voice_session_requires_stb_data(ctrlm_voice_device_t device_type);
    virtual bool          voice_session_has_stb_data();
    unsigned long         voice_session_id_next();
    void                  voice_session_notify_abort(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long session_id, ctrlm_voice_session_abort_reason_t reason);

    void                  voice_params_opus_encoder_default(void);
    bool                  voice_params_opus_encoder_validate(std::string &opus_encoder_params_str);
    void                  voice_params_opus_samples_per_packet_set(void);
    bool                  voice_params_hex_str_to_bytes(std::string hex_string, guchar *data, guint32 length);

    bool                  voice_session_can_request(ctrlm_voice_device_t device);
    void                  voice_session_set_active(ctrlm_voice_device_t device);
    void                  voice_session_set_inactive(ctrlm_voice_device_t device);

    bool                  voice_is_privacy_enabled(void);
    void                  voice_privacy_enable(bool update_vsdk);
    void                  voice_privacy_disable(bool update_vsdk);

    void                  voice_device_update_set_active(void);
    void                  voice_device_update_set_inactive(void);

    void                  voice_device_enable(ctrlm_voice_device_t device, bool db_update, bool *update_routes);
    void                  voice_device_disable(ctrlm_voice_device_t device, bool db_update, bool *update_routes);

    protected:
    // STB Data
    std::string              software_version;
    std::string              stb_name;
    std::string              account_number;
    std::string              device_id;
    ctrlm_device_type_t      device_type;
    std::string              receiver_id;
    std::string              partner_id;
    std::string              experience;
    char                     sat_token[XRSR_SAT_TOKEN_LEN_MAX];
    bool                     sat_token_required;
    bool                     mtls_required;
    bool                     secure_url_required;
    bool                     mask_pii;
    bool                     ocsp_verify_stapling;
    bool                     ocsp_verify_ca;
    bool                     capture_active;
    ctrlm_voice_cert_t       device_cert;
    voice_session_prefss_t   prefs;
    // End STB Data

    // Session Data
    ctrlm_voice_session_t    voice_session[VOICE_SESSION_GROUP_QTY];
    // End Session Data

    protected:
    sem_t                                             device_status_semaphore;
    uint8_t                                           device_status[CTRLM_VOICE_DEVICE_INVALID + 1];
    bool                                              device_requires_stb_data[CTRLM_VOICE_DEVICE_INVALID + 1];
    std::vector<ctrlm_voice_endpoint_t *>             endpoints;
    std::vector<std::pair<std::string, std::string> > query_strs_ptt;

    private:
    bool                     xrsr_opened;
    ctrlm_voice_ipc_t       *voice_ipc;

    #ifdef BEEP_ON_KWD_ENABLED
    Thunder::SystemAudioPlayer::ctrlm_thunder_plugin_system_audio_player_t *obj_sap;
    bool                                                                   sap_opened;
    ctrlm_timestamp_t                                                      sap_play_timestamp;
    #endif

    // Current Session Data
    unsigned long            opus_samples_per_packet;
    unsigned long            session_id;

    #ifdef VOICE_BUFFER_STATS
    ctrlm_timestamp_t       first_fragment;
    unsigned char           voice_buffer_warning_triggered;
    unsigned long           voice_buffer_high_watermark;
    unsigned long           voice_packet_interval; // amount of time between voice packets in microseconds

    #ifdef TIMING_START_TO_FIRST_FRAGMENT
    ctrlm_timestamp_t voice_session_begin_timestamp;
    #endif
    #ifdef TIMING_LAST_FRAGMENT_TO_STOP
    ctrlm_timestamp_t voice_session_last_fragment_timestamp;
    #endif
    #endif

    json_t                   *vsdk_config;

    ctrlm_voice_audio_mode_t                 audio_mode;
    ctrlm_voice_audio_timing_t               audio_timing;
    ctrlm_voice_audio_ducking_type_t         audio_ducking_type;
    ctrlm_voice_audio_ducking_level_t        audio_ducking_level;
    ctrlm_voice_audio_confidence_threshold_t audio_confidence_threshold;
    bool                                     audio_ducking_beep_enabled;
    bool                                     audio_ducking_beep_in_progress;

    // End Current Session Data
    // Timeout tags
    unsigned int             timeout_ctrl_session_stats_rxd;
    unsigned int             timeout_keyword_beep;
    // End Timeout tags

    int packet_loss_threshold;

    #ifdef TELEMETRY_SUPPORT
    ctrlm_voice_telemetry_vsr_error_map_t vsr_errors;
    #endif

    bool                 is_voice_assistant(ctrlm_voice_device_t device);
    bool                 controller_supports_qos(ctrlm_voice_device_t device);
    void                 set_audio_mode(ctrlm_voice_audio_settings_t *settings);
    void                 audio_state_set(bool session);
    bool                 vsdk_is_privacy_enabled(void);
    double               vsdk_keyword_sensitivity_limit_check(double sensitivity);
};

// Helper Functions
const char *ctrlm_voice_format_str(ctrlm_voice_format_t format);
const char *ctrlm_voice_state_src_str(ctrlm_voice_state_src_t state);
const char *ctrlm_voice_state_dst_str(ctrlm_voice_state_dst_t state);
const char *ctrlm_voice_device_str(ctrlm_voice_device_t device);
std::string ctrlm_voice_device_status_str(uint8_t status);
const char *ctrlm_voice_command_status_str(ctrlm_voice_command_status_t status);
const char *ctrlm_voice_command_status_tv_avr_str(ctrlm_voice_tv_avr_cmd_t cmd);
const char *ctrlm_voice_audio_mode_str(ctrlm_voice_audio_mode_t mode);
const char *ctrlm_voice_audio_timing_str(ctrlm_voice_audio_timing_t timing);
const char *ctrlm_voice_session_response_status_str(ctrlm_voice_session_response_status_t status);
const char *ctrlm_voice_session_group_str(ctrlm_voice_session_group_t group);
const char *ctrlm_voice_cert_type_str(ctrlm_voice_cert_type_t cert_type);
xrsr_audio_format_t voice_format_to_xrsr(ctrlm_voice_format_t format);
ctrlm_voice_device_t xrsr_to_voice_device(xrsr_src_t device);
xrsr_src_t voice_device_to_xrsr(ctrlm_voice_device_t device);


__inline bool ctrlm_voice_device_is_mic(ctrlm_voice_device_t device) {
    #ifdef CTRLM_LOCAL_MIC
    #ifdef CTRLM_LOCAL_MIC_TAP
    return(device == CTRLM_VOICE_DEVICE_MICROPHONE || device == CTRLM_VOICE_DEVICE_MICROPHONE_TAP);
    #else
    return(device == CTRLM_VOICE_DEVICE_MICROPHONE);
    #endif
    #else
    return(false);
    #endif
}

__inline bool ctrlm_voice_xrsr_src_is_mic(xrsr_src_t src) {
    #ifdef CTRLM_LOCAL_MIC
    #ifdef CTRLM_LOCAL_MIC_TAP
    return(src == XRSR_SRC_MICROPHONE || src == XRSR_SRC_MICROPHONE_TAP);
    #else
    return(src == XRSR_SRC_MICROPHONE);
    #endif
    #else
    return(false);
    #endif
}

#endif
