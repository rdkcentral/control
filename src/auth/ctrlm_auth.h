/*
 * If not stated otherwise in this file or this component's LICENSE file
 * the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#ifndef __CTRLM_AUTH_H__
#define __CTRLM_AUTH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <openssl/ssl.h>

class ctrlm_auth_t {
public:
   ctrlm_auth_t();
   virtual ~ctrlm_auth_t();

   virtual bool is_ready()                                    = 0;
   virtual bool get_receiver_id(std::string &receiver_id)     = 0;
   virtual bool get_device_id(std::string &device_id)         = 0;
   virtual bool get_account_id(std::string &account_id)       = 0;
   virtual bool get_partner_id(std::string &partner_id)       = 0;
   virtual bool get_experience(std::string &experience)       = 0;
   virtual bool get_sat(std::string &sat, time_t &expiration) = 0;
   virtual bool supports_sat_expiration() const               = 0;

};

ctrlm_auth_t *ctrlm_auth_service_create(std::string url);

#endif
