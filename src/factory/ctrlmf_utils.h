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
#ifndef _CTRLMF_UTILS_H_
#define _CTRLMF_UTILS_H_

#include <stdbool.h>
#include <ctrlm_ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ctrlmf_is_initialized(void);
bool ctrlmf_is_production(void);

bool ctrlmf_audio_control_init(void);
bool ctrlmf_audio_control_term(void);

const char *ctrlm_iarm_call_result_str(ctrlm_iarm_call_result_t value);
const char *iarm_result_str(IARM_Result_t value);

#ifdef __cplusplus
}
#endif

#endif
