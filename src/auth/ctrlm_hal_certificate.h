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

#ifndef __CTRLM_HAL_CERTIFICATE_H__
#define __CTRLM_HAL_CERTIFICATE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <openssl/ssl.h>

#define PASSPHRASE_LEN_MAX   (32)

typedef enum {
   CTRLM_VOICE_CERT_TYPE_NONE    = 0,
   CTRLM_VOICE_CERT_TYPE_P12     = 1,
   CTRLM_VOICE_CERT_TYPE_PEM     = 2,
   CTRLM_VOICE_CERT_TYPE_X509    = 3,
   CTRLM_VOICE_CERT_TYPE_INVALID = 4
} ctrlm_voice_cert_type_t;

typedef struct {
   const char *certificate;
   const char *passphrase;
} ctrlm_voice_cert_p12_t;

typedef struct {
   const char *filename_cert;
   const char *filename_pkey;
   const char *filename_chain;
   const char *passphrase;
} ctrlm_voice_cert_pem_t;

typedef struct {
   X509 *          cert_x509;
   EVP_PKEY *      cert_pkey;
   STACK_OF(X509) *cert_chain;
} ctrlm_voice_cert_x509_t;

typedef struct {
   ctrlm_voice_cert_type_t type;
   union {
   ctrlm_voice_cert_p12_t  p12;
   ctrlm_voice_cert_pem_t  pem;
   ctrlm_voice_cert_x509_t x509;
   } cert;
} ctrlm_voice_cert_t;

class ctrlm_hal_certificate_t {
public:
   ctrlm_hal_certificate_t();
   virtual ~ctrlm_hal_certificate_t();

   virtual bool device_cert_get(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca);

private:
   virtual bool device_cert_p12_set(const char *certificate, const char *passphrase);

   ctrlm_voice_cert_t device_cert;
   bool               ocsp_verify_stapling;
   bool               ocsp_verify_ca;
};

ctrlm_hal_certificate_t *ctrlm_hal_certificate_get();

#endif
