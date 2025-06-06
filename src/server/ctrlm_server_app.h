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
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <jansson.h>

class ctrlms_app_interface_t
{
   public:
   virtual ~ctrlms_app_interface_t() {};

   virtual void ws_connected(void);
   virtual void ws_disconnected(void);
   virtual bool ws_receive_audio(const unsigned char *payload, int payload_size);
   virtual bool ws_receive_json(const json_t *json_obj);
           void ws_send_json(const json_t *json_obj);
           void ws_handle_set(void *handle);

   private:
   void *ws_handle;
};

#ifdef __cplusplus
extern "C" {
#endif

ctrlms_app_interface_t *ctrlms_app_interface_create(void);

#ifdef __cplusplus
}
#endif
