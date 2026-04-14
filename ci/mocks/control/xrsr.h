/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
#ifndef _XRSR_H_
#define _XRSR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include "xr_timestamp.h"

/* Minimal opaque placeholders to avoid requiring OpenSSL dev headers in mocks. */
typedef struct xrsr_x509_stub X509;
typedef struct xrsr_evp_pkey_stub EVP_PKEY;
typedef struct xrsr_x509_stack_stub X509_STACK;
#define STACK_OF(type) type##_STACK
#ifdef __cplusplus
extern "C" {
#endif

#define XRSR_SAT_TOKEN_LEN_MAX    (2048)
#define XRSR_QUERY_STRING_QTY_MAX (16)
#define XRSR_SESSION_BY_TEXT_MAX  (256)
#define XRSR_DST_PARAM_LEN_MAX    (256)
#define XRSR_USER_AGENT_LEN_MAX   (512)
#define XRSR_DST_QTY_MAX          (4)

/* Source types */
typedef enum {
    XRSR_SRC_RCU_PTT        = 0,
    XRSR_SRC_RCU_FF         = 1,
    XRSR_SRC_MICROPHONE     = 2,
    XRSR_SRC_MICROPHONE_TAP = 3,
    XRSR_SRC_INVALID        = 4
} xrsr_src_t;

/* Audio format values used in ctrlm. */
enum {
    XRSR_AUDIO_FORMAT_NONE           = 0,
    XRSR_AUDIO_FORMAT_PCM            = (1 << 0),
    XRSR_AUDIO_FORMAT_PCM_32_BIT     = (1 << 1),
    XRSR_AUDIO_FORMAT_PCM_32_BIT_MULTI = (1 << 2),
    XRSR_AUDIO_FORMAT_PCM_RAW        = (1 << 3),
    XRSR_AUDIO_FORMAT_ADPCM          = (1 << 4),
    XRSR_AUDIO_FORMAT_ADPCM_FRAME    = (1 << 5),
    XRSR_AUDIO_FORMAT_OPUS           = (1 << 6),
    XRSR_AUDIO_FORMAT_OPUS_XVP       = (1 << 7),
    XRSR_AUDIO_FORMAT_MAX            = (1 << 8)
};

typedef struct {
    uint32_t size_packet;
    uint32_t size_header;
    uint32_t offset_step_size_index;
    uint32_t offset_predicted_sample_lsb;
    uint32_t offset_predicted_sample_msb;
    uint32_t offset_sequence_value;
    uint32_t shift_sequence_value;
    uint32_t sequence_value_min;
    uint32_t sequence_value_max;
} xrsr_audio_format_adpcm_frame_t;

typedef struct {
    int type;
    union {
        xrsr_audio_format_adpcm_frame_t adpcm_frame;
    } value;
} xrsr_audio_format_t;

typedef enum {
    XRSR_SESSION_END_REASON_EOS                  = 0,
    XRSR_SESSION_END_REASON_TERMINATE            = 1,
    XRSR_SESSION_END_REASON_EOT                  = 2,
    XRSR_SESSION_END_REASON_DISCONNECT_REMOTE    = 3,
    XRSR_SESSION_END_REASON_ERROR_AUDIO_BEGIN    = 4,
    XRSR_SESSION_END_REASON_ERROR_AUDIO_DURATION = 5,
    XRSR_SESSION_END_REASON_INVALID              = 6
} xrsr_session_end_reason_t;

typedef enum {
    XRSR_STREAM_FROM_BEGINNING    = 0,
    XRSR_STREAM_FROM_KEYWORD      = 1,
    XRSR_STREAM_FROM_KEYWORD_BEGIN = 2,
    XRSR_STREAM_FROM_LIVE         = 3,
    XRSR_STREAM_FROM_INVALID      = 4
} xrsr_stream_from_t;

typedef enum {
    XRSR_STREAM_UNTIL_END_OF_STREAM = 0,
    XRSR_STREAM_UNTIL_END_OF_SPEECH = 1,
    XRSR_STREAM_UNTIL_END_OF_KEYWORD = 2,
    XRSR_STREAM_UNTIL_INVALID       = 3
} xrsr_stream_until_t;

typedef enum {
    XRSR_POWER_MODE_FULL  = 0,
    XRSR_POWER_MODE_LOW   = 1,
    XRSR_POWER_MODE_SLEEP = 2,
    XRSR_POWER_MODE_QTY   = 3
} xrsr_power_mode_t;

typedef enum {
    XRSR_KWD_RESULT_DETECTED     = 0,
    XRSR_KWD_RESULT_NOT_DETECTED = 1,
    XRSR_KWD_RESULT_INVALID      = 2
} xrsr_keyword_detector_result_t;

typedef enum {
    XRSR_CERT_TYPE_P12  = 0,
    XRSR_CERT_TYPE_X509 = 1,
    XRSR_CERT_TYPE_PEM  = 2
} xrsr_cert_type_t;

typedef struct {
    xrsr_cert_type_t type;
    union {
        struct {
            const char *filename;
            const char *passphrase;
        } p12;
        struct {
            X509 *x509;
            EVP_PKEY *pkey;
            STACK_OF(X509) *chain;
        } x509;
        struct {
            const char *filename_cert;
            const char *filename_pkey;
            const char *filename_chain;
            const char *passphrase;
        } pem;
    } cert;
} xrsr_cert_t;

typedef struct {
    uint32_t connect_check_interval;
    uint32_t timeout_connect;
    uint32_t timeout_inactivity;
    uint32_t timeout_session;
    bool     ipv4_fallback;
    uint32_t backoff_delay;
} xrsr_dst_params_t;

typedef struct xrsr_session_config_in_t xrsr_session_config_in_t;
typedef struct xrsr_session_config_out_t xrsr_session_config_out_t;
typedef struct xrsr_session_config_update_t xrsr_session_config_update_t;

typedef void (*xrsr_session_config_cb_t)(const uuid_t uuid, xrsr_session_config_in_t *config_in);
typedef xrsr_session_config_cb_t xrsr_callback_session_config_t;
typedef bool (*xrsr_handler_send_t)(const void *data, size_t size, void *user_data);
typedef void (*xrsr_session_audio_cb_t)(int bytes_sent, void *user_data);

typedef struct {
    bool     valid;
    uint32_t packets_processed;
    uint32_t packets_lost;
    uint32_t samples_processed;
    uint32_t samples_lost;
    uint32_t decoder_failures;
    uint32_t samples_buffered_max;
    double   snr_peak;
    double   snr_mean;
} xrsr_audio_stats_t;

typedef struct {
    bool               result;
    int                stream_end_reason;
    xrsr_audio_stats_t audio_stats;
} xrsr_stream_stats_t;

typedef struct {
    xrsr_session_end_reason_t session_end_reason;
    int                       ret_code_protocol;
    int                       ret_code_library;
    int                       ret_code_internal;
    const char               *server_ip;
    int                       time_dns;
    int                       time_connect;
    int                       stream_end_reason;
    xrsr_stream_stats_t       stream_stats;
} xrsr_session_stats_t;

typedef struct xrsr_handlers_t {
    void (*session_begin)(void *data, const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_keyword_detector_result_t *detector_result, xrsr_session_config_out_t *config_out, xrsr_session_config_in_t *config_in, rdkx_timestamp_t *timestamp, const char *transcription_in);
    void (*session_config)(void *data, const uuid_t uuid, xrsr_session_config_update_t *session_config_update, rdkx_timestamp_t *timestamp);
    void (*session_end)(void *data, const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp);
    void (*stream_begin)(void *data, const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp);
    void (*stream_kwd)(void *data, const uuid_t uuid, rdkx_timestamp_t *timestamp);
    void (*stream_audio)(void *data, const uuid_t uuid, const uint8_t *buffer, size_t size, rdkx_timestamp_t *timestamp);
    void (*stream_end)(void *data, const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp);
    bool (*connected)(void *data, const uuid_t uuid, xrsr_handler_send_t send, void *param, rdkx_timestamp_t *timestamp, xrsr_session_config_update_t *session_config_update);
    void (*disconnected)(void *data, const uuid_t uuid, xrsr_session_end_reason_t reason, bool retry, bool *detect_resume, rdkx_timestamp_t *timestamp);
    void (*recv_msg)(void *data, const char *msg, size_t msg_len);
    void (*source_error)(void *data, xrsr_src_t src);
    void *data;
} xrsr_handlers_t;

typedef struct {
    const char      *url;
    xrsr_handlers_t  handlers;
    int              formats;
    uint32_t         stream_time_min;
    xrsr_stream_from_t stream_from;
    int32_t          stream_offset;
    xrsr_stream_until_t stream_until;
    const xrsr_dst_params_t *params[XRSR_POWER_MODE_QTY];
} xrsr_dst_t;

typedef struct {
    xrsr_src_t src;
    uint32_t   dst_qty;
    xrsr_dst_t dsts[XRSR_DST_QTY_MAX];
} xrsr_route_t;

struct xrsr_session_config_out_t {
    bool                 detect_keyword;
    bool                 has_stream;
    bool                 push_to_talk;
    xrsr_stream_from_t   stream_from;
    xrsr_audio_format_t  format;
    bool                 user_initiated;
    xrsr_session_config_cb_t cb_session_config;
};

struct xrsr_session_config_in_t {
    xrsr_src_t src;
    struct {
        xrsr_cert_t client_cert;
        bool        host_verify;
        bool        ocsp_verify_stapling;
        bool        ocsp_verify_ca;
        const char *sat_token;
        const char *user_agent;
        const char *query_strs[XRSR_QUERY_STRING_QTY_MAX + 1];
    } http;
    struct {
        xrsr_cert_t client_cert;
        const char *sat_token;
        bool        host_verify;
        bool        ocsp_verify_stapling;
        bool        ocsp_verify_ca;
        bool        cert_expired_allow;
        bool        cert_revoked_allow;
        bool        ocsp_expired_allow;
        uint32_t    keyword_begin;
        uint32_t    keyword_duration;
        void       *app_config;
    } ws;
};

struct xrsr_session_config_update_t {
    int dummy;
};

typedef struct {
    bool     delete_files;
    bool     enable;
    bool     use_curtail;
    uint32_t file_qty_max;
    uint32_t file_size_max;
    const char *dir_path;
} xrsr_capture_config_t;

typedef enum {
    XRSR_AUDIO_CONTAINER_NONE = 0,
    XRSR_AUDIO_CONTAINER_WAV  = 1
} xrsr_audio_container_t;

typedef enum {
    XRSR_SESSION_REQUEST_TYPE_INVALID    = 0,
    XRSR_SESSION_REQUEST_TYPE_TEXT       = 1,
    XRSR_SESSION_REQUEST_TYPE_AUDIO_FILE = 2,
    XRSR_SESSION_REQUEST_TYPE_AUDIO_MIC  = 3,
    XRSR_SESSION_REQUEST_TYPE_AUDIO_FD   = 4
} xrsr_session_request_type_t;

typedef struct {
    xrsr_session_request_type_t type;
    union {
        struct {
            const char *text;
        } text;
        struct {
            const char *path;
        } audio_file;
        struct {
            bool stream_params_required;
        } audio_mic;
        struct {
            int                  audio_fd;
            xrsr_audio_format_t  audio_format;
            xrsr_session_audio_cb_t callback;
            void                *user_data;
        } audio_fd;
    } value;
} xrsr_session_request_t;

typedef struct {
    const char *vsdk_config;
    const char *host_name;
    bool        networked_standby;
    bool        local_mic;
    bool        local_mic_tap;
    int         dummy;
} xrsr_config_t;

bool        xrsr_open(const char *host_name, xrsr_route_t *routes, const xrsr_handlers_t *handlers, const xrsr_capture_config_t *capture_config, xrsr_power_mode_t power_mode, bool privacy, bool mask_pii, void *json_obj_vsdk);
void        xrsr_close(void);
bool        xrsr_route(xrsr_route_t *routes);
void        xrsr_session_terminate(xrsr_src_t src);
bool        xrsr_config_get(xrsr_config_t *config);
bool        xrsr_power_mode_set(xrsr_power_mode_t power_mode);
bool        xrsr_privacy_mode_set(bool enable);
bool        xrsr_privacy_mode_get(bool *enable);
void        xrsr_mask_pii_set(bool enable);
bool        xrsr_session_request(xrsr_src_t src, xrsr_audio_format_t format, xrsr_session_request_t request, const uuid_t *uuid, bool low_latency, bool low_cpu_util);
bool        xrsr_session_audio_fd_set(xrsr_src_t src, int audio_fd, xrsr_audio_format_t format, xrsr_session_audio_cb_t callback, void *user_data);
void        xrsr_session_audio_stream_start(xrsr_src_t src);
void        xrsr_session_keyword_info_set(xrsr_src_t src, uint32_t pre_keyword_sample_qty, uint32_t keyword_sample_qty);
bool        xrsr_session_capture_start(xrsr_audio_container_t container, const char *file_path, bool raw_mic_enable);
bool        xrsr_session_capture_stop(void);
bool        xrsr_capture_config_set(const xrsr_capture_config_t *capture_config);
const char *xrsr_audio_container_str(xrsr_audio_container_t container);
const char *xrsr_src_str(xrsr_src_t src);
const char *xrsr_session_end_reason_str(xrsr_session_end_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif /* _XRSR_H_ */
