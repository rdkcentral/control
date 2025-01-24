/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#ifndef __CTRLM_VOICE_ENDPOINT_H__
#define __CTRLM_VOICE_ENDPOINT_H__
#include "xrsr.h"
#include "ctrlm.h"
#include "ctrlm_voice_obj.h"
#include "xr_timestamp.h"

// Macro for setting stats structures if they exist, and memseting them if they don't.
#define SET_IF_VALID(x, y)        if(y) {                       \
                                      x = *y;                   \
                                  } else {                      \
                                      memset(&x, 0, sizeof(x)); \
                                  }

class ctrlm_voice_endpoint_t {
public:
    ctrlm_voice_endpoint_t(ctrlm_voice_t *voice_obj);
    virtual ~ctrlm_voice_endpoint_t();
    virtual bool open() = 0;
    virtual bool get_handlers(xrsr_handlers_t *handlers) = 0;

    bool add_query_string(const char *key, const char *value);
    void clear_query_strings();

    virtual bool voice_init_set(const char *blob) const;
    virtual bool voice_message(const char *msg) const;

    // Data Setters
    virtual void voice_stb_data_stb_sw_version_set(std::string &sw_version);
    virtual void voice_stb_data_stb_name_set(std::string &stb_name);
    virtual void voice_stb_data_account_number_set(std::string &account_number);
    virtual void voice_stb_data_receiver_id_set(std::string &receiver_id);
    virtual void voice_stb_data_device_id_set(std::string &device_id);
    virtual void voice_stb_data_device_type_set(ctrlm_device_type_t device_type);
    virtual void voice_stb_data_partner_id_set(std::string &partner_id);
    virtual void voice_stb_data_experience_set(std::string &experience);
    virtual void voice_stb_data_guide_language_set(const char *language);
    virtual void voice_stb_data_mask_pii_set(bool enable);
    // End Data Setters

protected:
    // Helper Functions
    sem_t* voice_session_vsr_semaphore_get();
    static rdkx_timestamp_t valid_timestamp_get(rdkx_timestamp_t *t = NULL);
    bool   voice_stb_data_client_certificate_get(xrsr_cert_t *client_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca);
    // End Helper Functions

protected:
    ctrlm_voice_t *voice_obj;
    const char    *query_strs[CTRLM_VOICE_QUERY_STRING_MAX_PAIRS + 1];
    uint8_t        query_str_qty;
    char           query_str[CTRLM_VOICE_QUERY_STRING_MAX_PAIRS][CTRLM_VOICE_QUERY_STRING_MAX_LENGTH * 2];
};


#endif
