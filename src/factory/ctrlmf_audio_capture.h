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
#ifndef _CTRLMF_AUDIO_CAPTURE_H_
#define _CTRLMF_AUDIO_CAPTURE_H_

#include <stdint.h>
#include <ctrlm_fta_lib.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ctrlmf_audio_capture_init(uint32_t audio_frame_size, bool use_mic_tap);
bool ctrlmf_audio_capture_term(void);

bool ctrlmf_audio_capture_start(const char *request_type, ctrlmf_audio_frame_t audio_frames, uint32_t audio_frame_qty, uint32_t duration);
bool ctrlmf_audio_capture_stop(void);

#ifdef __cplusplus
}
#endif

#endif
