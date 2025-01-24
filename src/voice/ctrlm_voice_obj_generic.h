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
#ifndef __CTRLM_VOICE_GENERIC_H__
#define __CTRLM_VOICE_GENERIC_H__
#include "ctrlm_voice_obj.h"
#include "xrsr.h"
#include "xrsv_http.h"
#include "ctrlm_voice_endpoint.h"


class ctrlm_voice_generic_t : public ctrlm_voice_t {
    public:

    // Application Interface
    ctrlm_voice_generic_t();
    virtual ~ctrlm_voice_generic_t();
    
    protected:
    void                  voice_sdk_open(json_t *json_obj_vsdk);
    void                  voice_sdk_close();
    void                  voice_sdk_update_routes();
    void                  query_strings_updated();
    void                  mask_pii_updated(bool enable);
    bool                  url_hostname_get(std::string *url, std::string &hostname);
    void                  url_hostname_verify(std::string *url);

    private:
    #ifdef SUPPORT_VOICE_DEST_HTTP
    ctrlm_voice_endpoint_t * obj_http;
    #endif
    ctrlm_voice_endpoint_t * obj_ws_nextgen;
    ctrlm_voice_endpoint_t * obj_ws_nsp;
    #ifdef SUPPORT_VOICE_DEST_ALSA
    ctrlm_voice_endpoint_t * obj_sdt;
    #endif
};
#endif
