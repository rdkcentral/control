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
#ifndef _CTRLM_FTA_LIB_H_
#define _CTRLM_FTA_LIB_H_

#include <stdint.h>
#include <ctrlm_fta_caa.h>

#define CTRLMF_SYSTEMD_METHOD_START   "StartUnit"
#define CTRLMF_SYSTEMD_METHOD_STOP    "StopUnit"
#define CTRLMF_SYSTEMD_METHOD_RESTART "RestartUnit"

#define MIC_RAW_AUDIO        (true)

// Default values
#define CTRLMF_SNR_MIN ( 30.0)
#define CTRLMF_SNR_MAX (120.0)
#define CTRLMF_SNR_VAR ( 10.0)
#define CTRLMF_DURATION (1000)

#ifdef __cplusplus
extern "C" {
#endif

bool ctrlmf_init(xlog_level_t level, bool requires_audio_playback);
void ctrlmf_term(void);

bool ctrlmf_factory_reset(void);

bool ctrlmf_systemd_service_exec(const char *unit_name, const char* method);
bool ctrlmf_systemd_is_service_active(const char *unit_name);

bool ctrlmf_audio_control_mute(bool mute);
bool ctrlmf_audio_control_attenuate(bool enable, bool relative, double vol);

bool ctrlmf_audio_playback_start(const char *filename);

bool ctrlmf_mic_test_factory(uint32_t duration, const char *output_filename, uint32_t level, const char *audio_filename, double *snr_min, double *snr_max, double *snr_var, ctrlmf_test_result_t *test_result);

#ifdef __cplusplus
}
#endif

#endif
