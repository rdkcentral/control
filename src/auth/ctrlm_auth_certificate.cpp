/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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
#include <string.h>
#include <sys/stat.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <errno.h>
#include <dirent.h>
#include "rdkcertselector.h"
#include "ctrlm_auth_certificate.h"
#include "ctrlm_utils.h"
#include "ctrlm_log.h"

#define CERT_FILENAME_PREFIX "file://"

ctrlm_auth_certificate_t *ctrlm_auth_certificate_get() {
   return(new(ctrlm_auth_certificate_t));
}

ctrlm_auth_certificate_t::ctrlm_auth_certificate_t() {

   this->device_cert.type = CTRLM_VOICE_CERT_TYPE_NONE;
   this->ocsp_verify_stapling = false;
   this->ocsp_verify_ca       = false;

   char *cert_path     = NULL;
   char *cert_password = NULL;
   rdkcertselector_h cert_selector = rdkcertselector_new( NULL, NULL, "MTLS" );

   if(cert_selector == NULL){
      XLOGD_TELEMETRY("cert selector init failed");
   } else {
      rdkcertselectorStatus_t cert_status = rdkcertselector_getCert(cert_selector, &cert_path, &cert_password);

      if(cert_status != certselectorOk) {
         XLOGD_TELEMETRY("cert selector retrieval failed");
      } else {
         if(cert_path == NULL || cert_password == NULL) {
            XLOGD_TELEMETRY("cert selector get failed");
         } else {

            char *local_path = cert_path;
            if(strncmp(local_path, CERT_FILENAME_PREFIX, strlen(CERT_FILENAME_PREFIX)) == 0) {
               local_path += strlen(CERT_FILENAME_PREFIX);
            }
            if(!this->device_cert_p12_set(local_path, cert_password)) {
               XLOGD_TELEMETRY("unable to set device certificate <%s>", local_path);
            } else {
               struct stat file_info;
               // OCSP is a global setting that is enabled via RFC in systemd service ocsp-support
               if(stat("/tmp/.EnableOCSPStapling", &file_info) == 0) {
                  XLOGD_TELEMETRY("OCSP verification enabled (stapling)");
                  this->ocsp_verify_stapling = true;
               }
               if(stat("/tmp/.EnableOCSPCA", &file_info) == 0) {
                  XLOGD_TELEMETRY("OCSP verification enabled (CA)");
                  this->ocsp_verify_ca = true;
               }
               if(!this->ocsp_verify_stapling && !this->ocsp_verify_ca) {
                  XLOGD_TELEMETRY("OCSP verification disabled");
               }
            }
         }
      }
   }

   if(cert_selector != NULL) {
      rdkcertselector_free(&cert_selector);
   }
}

ctrlm_auth_certificate_t::~ctrlm_auth_certificate_t() {
   if(this->device_cert.type == CTRLM_VOICE_CERT_TYPE_P12) {
      if(this->device_cert.cert.p12.certificate != NULL) {
         free((void *)this->device_cert.cert.p12.certificate);
      }
      if(this->device_cert.cert.p12.passphrase != NULL) {
         free((void *)this->device_cert.cert.p12.passphrase);
      }
   }
}

bool ctrlm_auth_certificate_t::device_cert_get(ctrlm_voice_cert_t &device_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca) {

   device_cert          = this->device_cert;
   ocsp_verify_stapling = this->ocsp_verify_stapling;
   ocsp_verify_ca       = this->ocsp_verify_ca;
   return(true);
}

bool ctrlm_auth_certificate_t::device_cert_p12_set(const char *certificate, const char *passphrase) {
   bool cert_valid = false;

   // Extract the certificate, private key and additional certificates
   PKCS12 *p12_cert = NULL;
   EVP_PKEY *pkey   = NULL;
   X509 *x509_cert  = NULL;
   STACK_OF(X509) *additional_certs = NULL;

   do {
      FILE *fp = fopen(certificate, "rb");
      if(fp == NULL) {
         XLOGD_ERROR("unable to open P12 certificate <%s>", certificate);
         break;
      }

      d2i_PKCS12_fp(fp, &p12_cert);
      fclose(fp);
      fp = NULL;

      if(p12_cert == NULL) {
         XLOGD_ERROR("unable to read P12 certificate <%s>", certificate);
         break;
      }

      if(1 != PKCS12_parse(p12_cert, passphrase, &pkey, &x509_cert, &additional_certs)) {
         XLOGD_ERROR("unable to parse P12 certificate <%s>", certificate);
         break;
      }

      this->device_cert.type = CTRLM_VOICE_CERT_TYPE_P12;
      this->device_cert.cert.p12.certificate = strdup(certificate);
      this->device_cert.cert.p12.passphrase  = strdup(passphrase);

      // Ensure the strings were duplicated
      if(this->device_cert.cert.p12.certificate == NULL || this->device_cert.cert.p12.passphrase == NULL) {
         this->device_cert.type = CTRLM_VOICE_CERT_TYPE_NONE;
         if(this->device_cert.cert.p12.certificate != NULL) {
            free((void *)this->device_cert.cert.p12.certificate);
         }
         if(this->device_cert.cert.p12.passphrase != NULL) {
            free((void *)this->device_cert.cert.p12.passphrase);
         }
      } else {
         cert_valid = true;
      }

   } while(0);

   if(p12_cert != NULL) {
      PKCS12_free(p12_cert);
   }
   if(pkey != NULL) {
      EVP_PKEY_free(pkey);
   }
   if(x509_cert != NULL) {
      X509_free(x509_cert);
   }
   if(additional_certs != NULL) {
      sk_X509_pop_free(additional_certs, X509_free);
   }

   return(cert_valid);
}
