/*
 * Stub for xrsv_http.h - Voice HTTP endpoint types
 *
 * Copyright 2024 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
#ifndef _XRSV_HTTP_H_
#define _XRSV_HTTP_H_

#include "xrsr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *xrsv_http_object_t;

typedef struct {
    const char *device_id;
    const char *partner_id;
    const char *experience;
    const char *app_id;
    const char *language;
    bool        test_flag;
    bool        mask_pii;
    void       *user_data;
} xrsv_http_params_t;

typedef struct {
    const char *transcription;
    long        return_code;
} xrsv_http_recv_msg_t;

typedef struct {
    void (*session_begin)(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, rdkx_timestamp_t *timestamp, void *user_data);
    void (*session_end)(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    void (*stream_begin)(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data);
    void (*stream_end)(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    void (*connected)(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    void (*disconnected)(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    void (*recv_msg)(xrsv_http_recv_msg_t *msg, void *user_data);
} xrsv_http_handlers_t;

xrsv_http_object_t xrsv_http_create(const xrsv_http_params_t *params);
void               xrsv_http_destroy(xrsv_http_object_t obj);
bool               xrsv_http_handlers(xrsv_http_object_t obj, const xrsv_http_handlers_t *handlers, xrsr_handlers_t *handlers_out);
void               xrsv_http_update_device_id(xrsv_http_object_t obj, const char *device_id);
void               xrsv_http_update_partner_id(xrsv_http_object_t obj, const char *partner_id);
void               xrsv_http_update_experience(xrsv_http_object_t obj, const char *experience);
void               xrsv_http_update_language(xrsv_http_object_t obj, const char *language);
void               xrsv_http_update_mask_pii(xrsv_http_object_t obj, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* _XRSV_HTTP_H_ */
