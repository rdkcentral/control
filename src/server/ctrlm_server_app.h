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
#ifndef _CTRLM_SERVER_APP_H_
#define _CTRLM_SERVER_APP_H_

#include <stdint.h>
#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio data received from websocket client
typedef bool (*ctrlms_ws_receive_audio_t)(const unsigned char *payload, int payload_size);

// Json object received from websocket client
typedef bool (*ctrlms_ws_receive_json_t)(const json_t *json_obj);

// Json object to send to websocket client
void ctrlms_ws_send_json(const json_t *json_obj);

#ifdef __cplusplus
}
#endif
#endif