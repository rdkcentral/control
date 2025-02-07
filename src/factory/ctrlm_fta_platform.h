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
#ifndef _CTRLM_FTA_PLATFORM_H_
#define _CTRLM_FTA_PLATFORM_H_

#include <stdint.h>

typedef enum {
   CTRLM_FTA_PLATFORM_VOICE_CERT_TYPE_NONE    = 0,
   CTRLM_FTA_PLATFORM_VOICE_CERT_TYPE_P12     = 1,
   CTRLM_FTA_PLATFORM_VOICE_CERT_TYPE_INVALID = 2
} ctrlm_fta_platform_voice_cert_type_t;

typedef struct {
   ctrlm_fta_platform_voice_cert_type_t   type;
   char                                  *filename;
   char                                  *password;
}ctrlm_fta_platform_cert_info_t;


#ifdef __cplusplus
extern "C" {
#endif

ctrlm_fta_platform_cert_info_t *ctrlm_fta_platform_cert_info_get(bool allow_expired);
void ctrlm_fta_platform_cert_info_free(ctrlm_fta_platform_cert_info_t *cert_info);

#ifdef __cplusplus
}
#endif

#endif
