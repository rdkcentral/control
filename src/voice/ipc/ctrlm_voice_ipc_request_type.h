/* If not stated otherwise in this file or this component's license file the
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

#ifndef __CTRLM_VOICE_IPC_REQUEST_TYPE_H__
#define __CTRLM_VOICE_IPC_REQUEST_TYPE_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctrlm_voice_types.h>

typedef struct {
   bool                 requires_transcription;
   bool                 requires_audio_file;
   bool                 supports_named_pipe;
   ctrlm_voice_device_t device;
   ctrlm_voice_format_t format;
   bool                 low_latency;
   bool                 low_cpu_util;
} ctrlm_voice_ipc_request_config_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*voice_session_request_type_func_t)(ctrlm_voice_ipc_request_config_t *config);

typedef struct voice_session_request_type_handler_s { const char *name; voice_session_request_type_func_t func; } voice_session_request_type_handler_t;

bool ctrlm_voice_ipc_request_supported_ptt_transcription(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_ptt_audio_file(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_transcription(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_audio_file(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_stream_default(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_stream_single(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_stream_multi(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_tap_stream_single(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_tap_stream_multi(ctrlm_voice_ipc_request_config_t *config);
bool ctrlm_voice_ipc_request_supported_mic_factory_test(ctrlm_voice_ipc_request_config_t *config);

struct voice_session_request_type_handler_s *voice_session_request_type_handler_get(const char *str, size_t len);

#ifdef __cplusplus
}
#endif

#endif
