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
#ifndef _CTRLM_SHARED_MEMORY_H_
#define _CTRLM_SHARED_MEMORY_H_

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

bool ctrlm_sm_init(void);
void ctrlm_sm_term(bool unlink);

void ctrlm_sm_recovery_crash_count_read(uint32_t *value);
void ctrlm_sm_recovery_crash_count_write(uint32_t value);

void ctrlm_sm_recovery_invalid_hal_nvm_read(uint32_t *value);
void ctrlm_sm_recovery_invalid_hal_nvm_write(uint32_t value);

void ctrlm_sm_recovery_invalid_ctrlm_db_read(uint32_t *value);
void ctrlm_sm_recovery_invalid_ctrlm_db_write(uint32_t value);

void ctrlm_sm_voice_sat_enable_read(bool &value);
void ctrlm_sm_voice_sat_enable_write(bool value);

void ctrlm_sm_voice_mtls_enable_read(bool &value);
void ctrlm_sm_voice_mtls_enable_write(bool value);

void ctrlm_sm_voice_secure_url_required_read(bool &value);
void ctrlm_sm_voice_secure_url_required_write(bool value);

void ctrlm_sm_voice_url_ptt_read(std::string &url);
void ctrlm_sm_voice_url_ptt_write(std::string url);

void ctrlm_sm_voice_url_ff_read(std::string &url);
void ctrlm_sm_voice_url_ff_write(std::string url);

#ifdef CTRLM_LOCAL_MIC_TAP
void ctrlm_sm_voice_url_mic_tap_read(std::string &url);
void ctrlm_sm_voice_url_mic_tap_write(std::string url);
#endif

void ctrlm_sm_voice_query_string_ptt_count_read(uint8_t &count);
void ctrlm_sm_voice_query_string_ptt_count_write(uint8_t count);

void ctrlm_sm_voice_query_string_ptt_read(unsigned int index, std::string &key, std::string &value);
void ctrlm_sm_voice_query_string_ptt_write(unsigned int index, const std::string &key, const std::string &value);

bool ctrlm_sm_voice_init_blob_read(std::string &init, bool *valid);
void ctrlm_sm_voice_init_blob_write(const std::string &init);

#ifdef __cplusplus
}
#endif

#endif
