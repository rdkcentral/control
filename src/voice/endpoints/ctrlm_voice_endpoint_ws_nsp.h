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
#ifndef __CTRLM_VOICE_ENDPOINT_WS_NSP_H__
#define __CTRLM_VOICE_ENDPOINT_WS_NSP_H__

#include "ctrlm_voice_endpoint.h"

class ctrlm_voice_endpoint_ws_nsp_t : public ctrlm_voice_endpoint_t {
public:
    ctrlm_voice_endpoint_ws_nsp_t(ctrlm_voice_t *voice_obj);
    virtual ~ctrlm_voice_endpoint_ws_nsp_t();
    bool open();
    bool get_handlers(xrsr_handlers_t *handlers);

protected:
    void voice_session_begin_callback_ws_nsp(void *data, int size);
    void voice_stream_begin_callback_ws_nsp(void *data, int size);
    void voice_stream_end_callback_ws_nsp(void *data, int size);
    void voice_session_end_callback_ws_nsp(void *data, int size);
    void voice_session_server_return_code_ws_nsp(long ret_code);

protected:
    static void ctrlm_voice_handler_ws_nsp_session_begin(void *data, const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_keyword_detector_result_t *detector_result, xrsr_session_config_out_t *config_out, xrsr_session_config_in_t *config_in, rdkx_timestamp_t *timestamp, const char *transcription_in);
    static void ctrlm_voice_handler_ws_nsp_session_end(void *data, const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp);
    static void ctrlm_voice_handler_ws_nsp_stream_begin(void *data, const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp);
    static void ctrlm_voice_handler_ws_nsp_stream_end(void *data, const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp);
    static bool ctrlm_voice_handler_ws_nsp_connected(void *data, const uuid_t uuid, xrsr_handler_send_t send, void *param, rdkx_timestamp_t *timestamp, xrsr_session_config_update_t *session_config_update);
    static void ctrlm_voice_handler_ws_nsp_disconnected(void *data, const uuid_t uuid, xrsr_session_end_reason_t reason, bool retry, bool *detect_resume, rdkx_timestamp_t *timestamp);
    static void ctrlm_voice_handler_ws_nsp_source_error(void *data, xrsr_src_t src);
    static void ctrlm_voice_handler_ws_nsp_conn_close(const char *reason, long ret_code, void *user_data);

private:
    uuid_t uuid;
    long   server_ret_code;
};

#endif
