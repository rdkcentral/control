/*
 * Stub for xrsv_ws_nextgen.h - Voice WebSocket NextGen endpoint types
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _XRSV_WS_NEXTGEN_H_
#define _XRSV_WS_NEXTGEN_H_

#include "xrsr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *xrsv_ws_nextgen_object_t;

typedef enum {
    XRSV_WS_NEXTGEN_DEVICE_TYPE_STB = 0,
    XRSV_WS_NEXTGEN_DEVICE_TYPE_TV  = 1
} xrsv_ws_nextgen_device_type_t;

typedef struct {
    const char *device_id;
    const char *account_id;
    const char *partner_id;
    const char *experience;
    const char *audio_profile;
    const char *audio_model;
    const char *language;
    const char *device_mac;
    const char *rf_protocol;
    bool        test_flag;
    bool        bypass_wuw_verify_success;
    bool        bypass_wuw_verify_failure;
    bool        mask_pii;
    void       *user_data;
} xrsv_ws_nextgen_params_t;

typedef struct {
    uint32_t keyword_sample_begin;
    uint32_t keyword_sample_end;
    uint32_t keyword_doa;
    float    keyword_sensitivity;
    bool     keyword_sensitivity_triggered;
    float    keyword_sensitivity_high;
    bool     keyword_sensitivity_high_support;
    bool     keyword_sensitivity_high_triggered;
    float    dynamic_gain;
    float    linear_confidence;
    float    nonlinear_confidence;
    float    signal_noise_ratio;
    bool     push_to_talk;
    uint32_t par_eos_timeout;
} xrsv_ws_nextgen_stream_params_t;

typedef struct {
    void (*session_begin)(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, xrsv_ws_nextgen_stream_params_t *stream_params, rdkx_timestamp_t *timestamp, void *user_data);
    void (*session_end)(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    void (*stream_begin)(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data);
    void (*stream_kwd)(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    void (*stream_end)(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    void (*connected)(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    void (*disconnected)(const uuid_t uuid, bool retry, rdkx_timestamp_t *timestamp, void *user_data);
    void (*sent_init)(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    void (*listening)(void *user_data);
    void (*asr)(const char *str_transcription, bool final, void *user_data);
    void (*conn_close)(const char *reason, long ret_code, void *user_data);
    void (*response_vrex)(long ret_code, void *user_data);
    void (*wuw_verification)(const uuid_t uuid, bool passed, long confidence, void *user_data);
    void (*source_error)(xrsr_src_t src, void *user_data);
    void (*tv_mute)(bool mute, void *user_data);
    void (*tv_power)(bool power, bool toggle, void *user_data);
    void (*tv_volume)(bool up, uint32_t repeat_count, void *user_data);
    void (*msg)(const char *msg, unsigned long length, void *user_data);
} xrsv_ws_nextgen_handlers_t;

xrsv_ws_nextgen_object_t xrsv_ws_nextgen_create(const xrsv_ws_nextgen_params_t *params);
void                     xrsv_ws_nextgen_destroy(xrsv_ws_nextgen_object_t obj);
bool                     xrsv_ws_nextgen_handlers(xrsv_ws_nextgen_object_t obj, const xrsv_ws_nextgen_handlers_t *handlers, xrsr_handlers_t *handlers_out);
bool                     xrsv_ws_nextgen_update_init_app(xrsv_ws_nextgen_object_t obj, const char *init_app);
bool                     xrsv_ws_nextgen_send_msg(xrsv_ws_nextgen_object_t obj, const char *msg);
void                     xrsv_ws_nextgen_update_account_id(xrsv_ws_nextgen_object_t obj, const char *account_id);
void                     xrsv_ws_nextgen_update_device_id(xrsv_ws_nextgen_object_t obj, const char *device_id);
void                     xrsv_ws_nextgen_update_device_type(xrsv_ws_nextgen_object_t obj, xrsv_ws_nextgen_device_type_t type);
void                     xrsv_ws_nextgen_update_partner_id(xrsv_ws_nextgen_object_t obj, const char *partner_id);
void                     xrsv_ws_nextgen_update_experience(xrsv_ws_nextgen_object_t obj, const char *experience);
void                     xrsv_ws_nextgen_update_language(xrsv_ws_nextgen_object_t obj, const char *language);
void                     xrsv_ws_nextgen_update_mask_pii(xrsv_ws_nextgen_object_t obj, bool enable);
void                     xrsv_ws_nextgen_update_audio_profile(xrsv_ws_nextgen_object_t obj, const char *profile);
void                     xrsv_ws_nextgen_update_audio_model(xrsv_ws_nextgen_object_t obj, const char *model);
void                     xrsv_ws_nextgen_update_audio_rf_protocol(xrsv_ws_nextgen_object_t obj, const char *rf_protocol);

#ifdef __cplusplus
}
#endif

#endif /* _XRSV_WS_NEXTGEN_H_ */
