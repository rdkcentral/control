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
#ifndef __CTRLM_VOICE_IPC_IARM_ALL_H__
#define __CTRLM_VOICE_IPC_IARM_ALL_H__
#include "ctrlm_voice_ipc.h"
#include <vector>

class ctrlm_voice_ipc_iarm_all_t : public ctrlm_voice_ipc_t {
public:
    ctrlm_voice_ipc_iarm_all_t(ctrlm_voice_t *obj_voice);
    virtual ~ctrlm_voice_ipc_iarm_all_t();

    bool register_ipc() const;
    bool session_begin(const ctrlm_voice_ipc_event_session_begin_t &session_begin);
    bool stream_begin(const ctrlm_voice_ipc_event_stream_begin_t &stream_begin);
    bool stream_end(const ctrlm_voice_ipc_event_stream_end_t &stream_end);
    bool session_end(const ctrlm_voice_ipc_event_session_end_t &session_end);
    bool server_message(const char *message, unsigned long size);
    bool keyword_verification(const ctrlm_voice_ipc_event_keyword_verification_t &keyword_verification);
    bool session_statistics(const ctrlm_voice_ipc_event_session_statistics_t &session_stats);
    void deregister_ipc() const;

private:
    std::vector<ctrlm_voice_ipc_t *> ipc;
};


#endif
