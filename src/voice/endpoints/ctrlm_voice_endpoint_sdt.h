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
#ifndef __CTRLM_VOICE_ENDPOINT_SDT_H__
#define __CTRLM_VOICE_ENDPOINT_SDT_H__

#include "ctrlm_voice_endpoint.h"
#include "vmic_sdt.h"

class ctrlm_voice_endpoint_sdt_t : public ctrlm_voice_endpoint_t {
public:
    ctrlm_voice_endpoint_sdt_t(ctrlm_voice_t *voice_obj);
    virtual ~ctrlm_voice_endpoint_sdt_t();
    bool open();
    bool get_handlers(xrsr_handlers_t *handlers);

    bool voice_init_set(const char *blob) const;
    bool voice_message(const char *msg) const;
    void voice_stb_data_pii_mask_set(bool enable);

protected:
    void voice_session_begin_callback_sdt(void *data, int size);
    void voice_stream_begin_callback_sdt(void *data, int size);
    void voice_stream_end_callback_sdt(void *data, int size);
    void voice_session_end_callback_sdt(void *data, int size);
    void voice_session_recv_msg_sdt(const char *transcription);
    void voice_session_server_return_code_sdt(long ret_code);
    void voice_message_send(void *data, int size);

protected:
    static void ctrlm_voice_handler_sdt_session_begin(const uuid_t uuid, xrsr_src_t src, uint32_t dst_index, xrsr_session_config_out_t *configuration, vmic_sdt_stream_params_t *stream_params, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_session_end(const uuid_t uuid, xrsr_session_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_stream_begin(const uuid_t uuid, xrsr_src_t src, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_stream_kwd(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_stream_end(const uuid_t uuid, xrsr_stream_stats_t *stats, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_connected(const uuid_t uuid, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_disconnected(const uuid_t uuid, bool retry, rdkx_timestamp_t *timestamp, void *user_data);
    static void ctrlm_voice_handler_sdt_server_message(const char *msg, unsigned long length, void *user_data);
protected:
    void       *vmic_obj_sdt;
private:
    uuid_t      uuid;
    long        server_ret_code;
    // Voice Message Synchronization.. All accesses of these variables MUST be on main thread
    bool                     voice_message_available;
    std::vector<std::string> voice_message_queue;
};

#endif

