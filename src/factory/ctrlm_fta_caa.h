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
#ifndef _CTRLM_FTA_CAA_H_
#define _CTRLM_FTA_CAA_H_

#include <stdint.h>

#define MIC_CHANNEL_QTY      (3)
#define AUDIO_FRAME_DURATION (20) // in milliseconds
#define SAMPLE_RATE          (16000)
#define SAMPLES_PER_FRAME    (SAMPLE_RATE * AUDIO_FRAME_DURATION / 1000)
#define SAMPLE_SIZE          sizeof(int32_t)

#define AUDIO_FRAME_SIZE_PER_BEAM  (SAMPLE_SIZE * SAMPLES_PER_FRAME)
#define AUDIO_FRAME_SIZE_ALL_BEAMS (AUDIO_FRAME_SIZE_PER_BEAM * MIC_CHANNEL_QTY)

typedef enum {
   CTRLMF_TEST_FACTORY = 0,
   CTRLMF_TEST_QTY     = 1
} ctrlmf_test_type_t;

typedef int32_t (*ctrlmf_audio_frame_t)[MIC_CHANNEL_QTY][SAMPLES_PER_FRAME];

typedef struct {
   bool   pass;
   double snr[MIC_CHANNEL_QTY];
} ctrlmf_test_result_t;

#ifdef __cplusplus
extern "C" {
#endif

bool ctrlmf_mic_test_audio_analyze(ctrlmf_test_type_t test_type, const char *output_filename, uint32_t level, ctrlmf_audio_frame_t audio_frames_noise, ctrlmf_audio_frame_t audio_frames_signal, uint32_t frame_qty, uint32_t mic_qty, ctrlmf_test_result_t *test_result);

#ifdef __cplusplus
}
#endif
#endif