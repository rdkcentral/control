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
/*
/ This file is for voice types that are shared between voice components (object, IPC, etc)
*/
#ifndef __CTRLM_VOICE_TYPES_H__
#define __CTRLM_VOICE_TYPES_H__

#include <stdint.h>

#define VOICE_BUFFER_STATS                                     // Print buffer statistics
//#define TIMING_START_TO_FIRST_FRAGMENT                         // Print lag time from voice session begin to receipt of first voice fragment
//#define TIMING_LAST_FRAGMENT_TO_STOP                           // Print lag time from receipt of the last voice fragment to the voice session end

typedef enum {
    CTRLM_VOICE_DEVICE_PTT,
    CTRLM_VOICE_DEVICE_FF,
    #ifdef CTRLM_LOCAL_MIC
    CTRLM_VOICE_DEVICE_MICROPHONE,
    #ifdef CTRLM_LOCAL_MIC_TAP
    CTRLM_VOICE_DEVICE_MICROPHONE_TAP,
    #endif
    #endif
    CTRLM_VOICE_DEVICE_INVALID,
} ctrlm_voice_device_t;

typedef enum {
    CTRLM_VOICE_FORMAT_ADPCM_FRAME,
    CTRLM_VOICE_FORMAT_PCM,
    CTRLM_VOICE_FORMAT_PCM_32_BIT,
    CTRLM_VOICE_FORMAT_PCM_32_BIT_MULTI,
    CTRLM_VOICE_FORMAT_PCM_RAW,
    CTRLM_VOICE_FORMAT_OPUS_XVP,
    CTRLM_VOICE_FORMAT_OPUS,
    CTRLM_VOICE_FORMAT_INVALID
} ctrlm_voice_format_type_t;

typedef struct {
   uint8_t size_packet;
   uint8_t size_header;
   uint8_t offset_step_size_index;
   uint8_t offset_predicted_sample_lsb;
   uint8_t offset_predicted_sample_msb;
   uint8_t offset_sequence_value;
   uint8_t sequence_value_min;
   uint8_t sequence_value_max;
} ctrlm_adpcm_frame_t;

typedef struct {
   ctrlm_voice_format_type_t type;
   union {
      ctrlm_adpcm_frame_t adpcm_frame;
   } value;
} ctrlm_voice_format_t;

#endif
