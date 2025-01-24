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
#ifndef __CTRLM_VOICE_ENDPOINT_HTTP_H__
#define __CTRLM_VOICE_ENDPOINT_HTTP_H__

#include "ctrlm_voice_endpoint.h"
#include "xrsv_http.h"

class ctrlm_voice_endpoint_http_t : public ctrlm_voice_endpoint_t {
public:
    ctrlm_voice_endpoint_http_t(ctrlm_voice_t *voice_obj);
    virtual ~ctrlm_voice_endpoint_http_t();
    bool open();
    bool get_handlers(xrsr_handlers_t *handlers);

public:
    void voice_stb_data_receiver_id_set(std::string &receiver_id);
    void voice_stb_data_device_id_set(std::string &device_id);
    void voice_stb_data_partner_id_set(std::string &partner_id);
    void voice_stb_data_experience_set(std::string &experience);
    void voice_stb_data_guide_language_set(const char *language);
    void voice_stb_data_pii_mask_set(bool enable);

protected:
    void voice_session_begin_callback_http(void *data, int size);
    void voice_stream_begin_callback_http(void *data, int size);
    void voice_stream_end_callback_http(void *data, int size);
    void voice_session_end_callback_http(void *data, int size);
    void voice_session_recv_msg_http(const char *transcription, long ret_code);

protected:
    static void ctrlm_voice_handler_http_session_begin(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_session_end(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_stream_begin(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_stream_end(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_connected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_disconnected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_http_recv_msg(xrsv_http_recv_msg_t *msg, void *user_data);

protected:
    void       *xrsv_obj_http;
    char        user_agent[XRSR_USER_AGENT_LEN_MAX];

private:
    uuid_t      uuid;
    long        server_ret_code;
};

#endif
