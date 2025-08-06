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
#include "ctrlm_voice_endpoint.h"
#include "ctrlm_log.h"

ctrlm_voice_endpoint_t::ctrlm_voice_endpoint_t(ctrlm_voice_t *voice_obj) {
    this->voice_obj     = voice_obj;
    clear_query_strings();
}
ctrlm_voice_endpoint_t::~ctrlm_voice_endpoint_t() {}

bool ctrlm_voice_endpoint_t::add_query_string(const char *key, const char *value) {
    if(this->query_str_qty + 1 >= CTRLM_VOICE_QUERY_STRING_MAX_PAIRS) {
        XLOGD_ERROR("Too many query strings");
        return(false);
    }
    errno_t safec_rc = sprintf_s(this->query_str[this->query_str_qty], sizeof(this->query_str[this->query_str_qty]), "%s=%s", key, value);
    if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
    }
    this->query_strs[this->query_str_qty] = this->query_str[this->query_str_qty];
    this->query_str_qty++;
    return(true);
}

void ctrlm_voice_endpoint_t::clear_query_strings() {
    this->query_str_qty = 0;
    errno_t safec_rc = memset_s(&this->query_strs, sizeof (this->query_strs), 0, sizeof (this->query_strs));
    ERR_CHK(safec_rc);
    safec_rc = memset_s(&this->query_str, sizeof (this->query_str), 0, sizeof (this->query_str));
    ERR_CHK(safec_rc);
}

bool ctrlm_voice_endpoint_t::voice_init_set(const char *blob) const {
    XLOGD_INFO("Endpoint does not support this.");
    return(false);
}

bool ctrlm_voice_endpoint_t::voice_message(const char *msg) const {
    XLOGD_INFO("Endpoint does not support this.");
    return(false);
}

void ctrlm_voice_endpoint_t::voice_stb_data_stb_sw_version_set(std::string &sw_version) {}
void ctrlm_voice_endpoint_t::voice_stb_data_stb_name_set(std::string &stb_name) {}
void ctrlm_voice_endpoint_t::voice_stb_data_account_number_set(std::string &account_number) {}
void ctrlm_voice_endpoint_t::voice_stb_data_receiver_id_set(std::string &receiver_id) {}
void ctrlm_voice_endpoint_t::voice_stb_data_device_id_set(std::string &device_id) {}
void ctrlm_voice_endpoint_t::voice_stb_data_device_type_set(ctrlm_device_type_t device_type) {}
void ctrlm_voice_endpoint_t::voice_stb_data_partner_id_set(std::string &partner_id) {}
void ctrlm_voice_endpoint_t::voice_stb_data_experience_set(std::string &experience) {}
void ctrlm_voice_endpoint_t::voice_stb_data_guide_language_set(const char *language) {}
void ctrlm_voice_endpoint_t::voice_stb_data_mask_pii_set(bool enable) {}

bool ctrlm_voice_endpoint_t::voice_stb_data_client_certificate_get(xrsr_cert_t *client_cert, bool &ocsp_verify_stapling, bool &ocsp_verify_ca) {
    bool use_mtls = false;
    ctrlm_voice_cert_t device_cert;
    this->voice_obj->voice_stb_data_device_certificate_get(device_cert, ocsp_verify_stapling, ocsp_verify_ca);

    if(device_cert.type == CTRLM_VOICE_CERT_TYPE_P12) {
        const ctrlm_voice_cert_p12_t *cert_p12 = &device_cert.cert.p12;
        client_cert->type                = XRSR_CERT_TYPE_P12;
        client_cert->cert.p12.filename   = cert_p12->certificate;
        client_cert->cert.p12.passphrase = cert_p12->passphrase;
        use_mtls = true;
    } else if(device_cert.type == CTRLM_VOICE_CERT_TYPE_X509) {
        const ctrlm_voice_cert_x509_t *cert_x509 = &device_cert.cert.x509;
        client_cert->type            = XRSR_CERT_TYPE_X509;
        client_cert->cert.x509.x509  = cert_x509->cert_x509;
        client_cert->cert.x509.pkey  = cert_x509->cert_pkey;
        client_cert->cert.x509.chain = cert_x509->cert_chain;
        use_mtls = true;
    } else if(device_cert.type == CTRLM_VOICE_CERT_TYPE_PEM) {
        const ctrlm_voice_cert_pem_t *cert_pem = &device_cert.cert.pem;
        client_cert->type                    = XRSR_CERT_TYPE_PEM;
        client_cert->cert.pem.filename_cert  = cert_pem->filename_cert;
        client_cert->cert.pem.filename_pkey  = cert_pem->filename_pkey;
        client_cert->cert.pem.filename_chain = cert_pem->filename_chain;
        client_cert->cert.pem.passphrase     = cert_pem->passphrase;
        use_mtls = true;
    }
    return(use_mtls);
}

sem_t* ctrlm_voice_endpoint_t::voice_session_vsr_semaphore_get() {
    sem_t *sem = NULL;
    if(this->voice_obj) {
        sem = &this->voice_obj->vsr_semaphore;
    }
    return(sem);
}

rdkx_timestamp_t ctrlm_voice_endpoint_t::valid_timestamp_get(rdkx_timestamp_t *t) {
    rdkx_timestamp_t ret;
    if(t) {
        ret = *t;
    } else {
        rdkx_timestamp_get_realtime(&ret);
    }
    return(ret);
}
