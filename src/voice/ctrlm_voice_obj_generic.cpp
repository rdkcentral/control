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
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <regex>
#include <semaphore.h>
#include "ctrlm_voice_obj_generic.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_tr181.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"
#include "ctrlm_network.h"
#include "jansson.h"
#include "json_config.h"
#include "ctrlm_voice_ipc_iarm_all.h"
#include "ctrlm_voice_endpoint_ws_nextgen.h"
#include "ctrlm_voice_endpoint_ws_nsp.h"

#ifdef SUPPORT_VOICE_DEST_HTTP
#include "ctrlm_voice_endpoint_http.h"
#endif

#ifdef SUPPORT_VOICE_DEST_ALSA
#include "ctrlm_voice_endpoint_sdt.h"
#endif

#define PIPE_READ  (0)
#define PIPE_WRITE (1)

// Application Interface Implementation
ctrlm_voice_generic_t::ctrlm_voice_generic_t() : ctrlm_voice_t() {
    XLOGD_INFO("Constructor");
    ctrlm_voice_ipc_t *ipc = new ctrlm_voice_ipc_iarm_all_t(this);
    ipc->register_ipc();
    this->voice_set_ipc(ipc);
    #ifdef SUPPORT_VOICE_DEST_HTTP
    this->obj_http       = NULL;
    #endif
    this->obj_ws_nextgen = NULL;
    this->obj_ws_nsp     = NULL;
    #ifdef SUPPORT_VOICE_DEST_ALSA
    this->obj_sdt        = NULL;
    #endif
}

ctrlm_voice_generic_t::~ctrlm_voice_generic_t() {
    XLOGD_INFO("Destructor");
    this->voice_sdk_close();
}

void ctrlm_voice_generic_t::voice_sdk_open(json_t *json_obj_vsdk) {
    ctrlm_voice_t::voice_sdk_open(json_obj_vsdk);

    this->obj_ws_nextgen = new ctrlm_voice_endpoint_ws_nextgen_t(this);
    this->obj_ws_nsp     = new ctrlm_voice_endpoint_ws_nsp_t(this);
    #ifdef SUPPORT_VOICE_DEST_HTTP
    this->obj_http       = new ctrlm_voice_endpoint_http_t(this);
    #endif
    #ifdef SUPPORT_VOICE_DEST_ALSA
    this->obj_sdt        = new ctrlm_voice_endpoint_sdt_t(this);
    #endif
    if(this->obj_ws_nextgen->open() == false) {
        XLOGD_ERROR("Failed to open speech WS NextGen");
        xrsr_close();
        g_assert(0);
    } else if(this->obj_ws_nsp->open() == false) {
        XLOGD_ERROR("Failed to open speech WS No Server Protocol");
        xrsr_close();
        g_assert(0);
    }
    #ifdef SUPPORT_VOICE_DEST_HTTP
    else if(this->obj_http->open() == false) {
        XLOGD_ERROR("Failed to open speech HTTP");
        xrsr_close();
        g_assert(0);
    }
    #endif
    #ifdef SUPPORT_VOICE_DEST_ALSA
    else if(this->obj_sdt->open() == false) {
        XLOGD_ERROR("Failed to open speech sdt ");
        xrsr_close();
        g_assert(0);
    }
    #endif
    this->endpoints.push_back(this->obj_ws_nextgen);
    this->endpoints.push_back(this->obj_ws_nsp);
    #ifdef SUPPORT_VOICE_DEST_HTTP
    this->endpoints.push_back(this->obj_http);
    #endif
    #ifdef SUPPORT_VOICE_DEST_ALSA
    this->endpoints.push_back(this->obj_sdt);
    #endif
}

void ctrlm_voice_generic_t::voice_sdk_close() {
    for(uint32_t group = VOICE_SESSION_GROUP_DEFAULT; group < VOICE_SESSION_GROUP_QTY; group++) {
        if(this->voice_state_src_get((ctrlm_voice_session_group_t)group) != CTRLM_VOICE_STATE_SRC_READY) { // Need to terminate session before destroying endpoints
            XLOGD_WARN("Voice session in progress.. Terminating..");
            
            ctrlm_voice_session_t *session = &this->voice_session[group];

            if(session->audio_pipe[PIPE_WRITE] >= 0) { // close the audio input source
                XLOGD_WARN("Closing audio write pipe <%d>", session->audio_pipe[PIPE_WRITE]);
                close(session->audio_pipe[PIPE_WRITE]);
                session->audio_pipe[PIPE_WRITE] = -1;
            }

            xrsr_session_terminate(voice_device_to_xrsr(session->voice_device));
        }
    }

    ctrlm_voice_t::voice_sdk_close();

    if(this->obj_ws_nextgen != NULL) {
        delete this->obj_ws_nextgen;
        this->obj_ws_nextgen = NULL;
    }
    if(this->obj_ws_nsp != NULL) {
        delete this->obj_ws_nsp;
        this->obj_ws_nsp = NULL;
    }
    #ifdef SUPPORT_VOICE_DEST_HTTP
    if(this->obj_http != NULL) {
        delete this->obj_http;
        this->obj_http = NULL;
    }
    #endif
    #ifdef SUPPORT_VOICE_DEST_ALSA
    if(this->obj_sdt != NULL) {
        delete this->obj_sdt;
        this->obj_sdt = NULL;
    }
    #endif

    this->endpoints.clear();
}

void ctrlm_voice_generic_t::voice_sdk_update_routes() {
    xrsr_route_t routes[XRSR_SRC_INVALID + 1];
    std::vector<std::string> urls_translated;
    int          i = 0;

    errno_t safec_rc = memset_s(&routes, sizeof(routes), 0, sizeof(routes));
    ERR_CHK(safec_rc);

    // iterate over source to url mapping
    for(int j = 0; j < XRSR_SRC_INVALID; j++) {
        xrsr_src_t            src        = (xrsr_src_t)j;
        ctrlm_voice_device_t  src_device = xrsr_to_voice_device(src);
        std::string          *url        = NULL;

        sem_wait(&this->device_status_semaphore);
        if(this->device_status[src_device] != CTRLM_VOICE_DEVICE_STATUS_DISABLED && this->device_status[src_device] != CTRLM_VOICE_DEVICE_STATUS_NOT_SUPPORTED) {
            switch(src_device) {
                case CTRLM_VOICE_DEVICE_PTT: {
                    url = &this->prefs.server_url_src_ptt;
                    break;
                }
                #ifdef CTRLM_LOCAL_MIC
                case CTRLM_VOICE_DEVICE_MICROPHONE:
                #endif
                case CTRLM_VOICE_DEVICE_FF: {
                    url = &this->prefs.server_url_src_ff;
                    break;
                }
                #ifdef CTRLM_LOCAL_MIC_TAP
                case CTRLM_VOICE_DEVICE_MICROPHONE_TAP: {
                   url = &this->prefs.server_url_src_mic_tap;
                   break;
                }
                #endif
                default: {
                    break;
                }
            }
        }
        // Default to requiring stb data for all routes
        this->device_requires_stb_data[src_device] = true;
        sem_post(&this->device_status_semaphore);

        if(url == NULL || url->empty()) {
            continue;
        }

        // Check url against expected hostname list and emit a log if the hostname doesn't match
        this->url_hostname_verify(url);

        xrsr_stream_from_t  stream_from   = XRSR_STREAM_FROM_BEGINNING;
        int32_t             stream_offset = 0;
        xrsr_stream_until_t stream_until  = XRSR_STREAM_UNTIL_END_OF_STREAM;

        if(src == XRSR_SRC_MICROPHONE) {
           stream_from   = XRSR_STREAM_FROM_KEYWORD_BEGIN;
           stream_offset = - this->prefs.ffv_leading_samples;
           stream_until  = XRSR_STREAM_UNTIL_END_OF_SPEECH;
        }

        if(url->rfind("vrng", 0) == 0) {
            if(url->rfind("vrngs", 0) != 0) {
                if(this->secure_url_required) {
                    XLOGD_TELEMETRY("deny insecure url <%s>", url->c_str());
                    continue;
                } else {
                    XLOGD_TELEMETRY("accept insecure url <%s>", url->c_str());
                }
            }
            xrsr_handlers_t    handlers_xrsr = {0};
            int                translated_index = urls_translated.size();

            urls_translated.push_back("ws" + url->substr(4));

            if(!this->obj_ws_nextgen->get_handlers(&handlers_xrsr)) {
                XLOGD_ERROR("failed to get handlers ws");
            } else {
                routes[i].src                     = src;
                routes[i].dst_qty                 = 1;
                routes[i].dsts[0].url             = urls_translated[translated_index].c_str();
                routes[i].dsts[0].handlers        = handlers_xrsr;
                routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM;
                routes[i].dsts[0].stream_time_min = this->prefs.utterance_duration_min;
                routes[i].dsts[0].stream_from     = stream_from;
                routes[i].dsts[0].stream_offset   = stream_offset;
                routes[i].dsts[0].stream_until    = stream_until;
                #ifdef ENABLE_DEEP_SLEEP
                if(src == XRSR_SRC_MICROPHONE) {
                    routes[i].dsts[0].params[XRSR_POWER_MODE_LOW] = &this->prefs.dst_params_standby;
                }
                #endif
                i++;
                XLOGD_INFO("url translation from %s to %s", url->c_str(), urls_translated[translated_index].c_str());
            }
        } else if(url->rfind("aows", 0) == 0) { // Audio only with no server protocol layer over websocket
            if(url->rfind("aowss", 0) != 0) {
                if(this->secure_url_required) {
                    XLOGD_TELEMETRY("deny insecure url <%s>", url->c_str());
                    continue;
                } else {
                    XLOGD_TELEMETRY("accept insecure url <%s>", url->c_str());
                }
            }
            xrsr_handlers_t    handlers_xrsr = {0};
            int                translated_index = urls_translated.size();

            urls_translated.push_back(url->substr(2));

            if(!this->obj_ws_nsp->get_handlers(&handlers_xrsr)) {
                XLOGD_ERROR("failed to get handlers ws");
            } else {
                routes[i].src                     = src;
                routes[i].dst_qty                 = 1;
                routes[i].dsts[0].url             = urls_translated[translated_index].c_str();
                routes[i].dsts[0].handlers        = handlers_xrsr;
                routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM | XRSR_AUDIO_FORMAT_PCM_32_BIT | XRSR_AUDIO_FORMAT_PCM_32_BIT_MULTI | XRSR_AUDIO_FORMAT_PCM_RAW;
                routes[i].dsts[0].stream_time_min = 0;
                routes[i].dsts[0].stream_from     = XRSR_STREAM_FROM_LIVE;
                routes[i].dsts[0].stream_offset   = 0;
                routes[i].dsts[0].stream_until    = XRSR_STREAM_UNTIL_END_OF_STREAM;
                #ifdef ENABLE_DEEP_SLEEP
                if(src == XRSR_SRC_MICROPHONE) {
                    routes[i].dsts[0].params[XRSR_POWER_MODE_LOW] = &this->prefs.dst_params_standby;
                }
                #endif

                // Set low latency websocket parameters
                routes[i].dsts[0].params[XRSR_POWER_MODE_FULL] = &this->prefs.dst_params_low_latency;
                // do not require stb data for aows protocol
                this->device_requires_stb_data[src_device] = false;
                i++;
                XLOGD_INFO("url translation from %s to %s", url->c_str(), urls_translated[translated_index].c_str());
            }
        }
        #ifdef SUPPORT_VOICE_DEST_HTTP
          else if(url->rfind("http", 0) == 0) {
            if(url->rfind("https", 0) != 0) {
                if(this->secure_url_required) {
                    XLOGD_TELEMETRY("deny insecure url <%s>", url->c_str());
                    continue;
                } else {
                    XLOGD_TELEMETRY("accept insecure url <%s>", url->c_str());
                }
            }

            xrsr_handlers_t handlers_xrsr = {0};

            if(!this->obj_http->get_handlers(&handlers_xrsr)) {
                XLOGD_ERROR("failed to get handlers http");
            } else {
                if(std::string::npos == url->find("speech?") && '/' == url->back()) { // This entire if block needs to be removed after the full path is provided in the url
                    url->append("speech?");
                }

                routes[i].src                     = src;
                routes[i].dst_qty                 = 1;
                routes[i].dsts[0].url             = url->c_str();
                routes[i].dsts[0].handlers        = handlers_xrsr;
                #ifdef AUDIO_DECODE
                routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM;
                #else
                routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM | XRSR_AUDIO_FORMAT_ADPCM;
                #endif
                routes[i].dsts[0].stream_time_min = this->prefs.utterance_duration_min;
                routes[i].dsts[0].stream_from     = stream_from;
                routes[i].dsts[0].stream_offset   = stream_offset;
                routes[i].dsts[0].stream_until    = stream_until;
                #ifdef ENABLE_DEEP_SLEEP
                if(src == XRSR_SRC_MICROPHONE) {
                    routes[i].dsts[0].params[XRSR_POWER_MODE_LOW] = &this->prefs.dst_params_standby;
                }
                #endif
                i++;
            }
        }
        #endif
        #ifdef SUPPORT_VOICE_DEST_ALSA
          else if(url->rfind("sdt", 0) == 0) {
            xrsr_handlers_t    handlers_xrsr;
            memset(&handlers_xrsr, 0, sizeof(handlers_xrsr));

            if(!this->obj_sdt->get_handlers(&handlers_xrsr)) {
                XLOGD_ERROR("failed to get handlers ws");
            } else {

                routes[i].src                     = src;
                routes[i].dst_qty                 = 1;
                routes[i].dsts[0].url             = url->c_str();
                routes[i].dsts[0].handlers        = handlers_xrsr;
                routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM;
                routes[i].dsts[0].stream_time_min = this->prefs.utterance_duration_min;
                routes[i].dsts[0].stream_from     = stream_from;
                routes[i].dsts[0].stream_offset   = stream_offset;
                routes[i].dsts[0].stream_until    = stream_until;
                #ifdef ENABLE_DEEP_SLEEP
                if(src == XRSR_SRC_MICROPHONE) {
                    routes[i].dsts[0].params[XRSR_POWER_MODE_LOW] = &this->prefs.dst_params_standby;
                }
                #endif
                i++;
            }
        } else if(url->rfind("avs", 0) == 0) {
                xrsr_handlers_t    handlers_xrsr;
                memset(&handlers_xrsr, 0, sizeof(handlers_xrsr));
                int    translated_index = urls_translated.size();
                urls_translated.push_back("sdt" + url->substr(3));
                if(!this->obj_sdt->get_handlers(&handlers_xrsr)) {
                        XLOGD_ERROR("failed to get handlers ws");
                } else {

                        routes[i].src                     = src;
                        routes[i].dst_qty                 = 1;
                        routes[i].dsts[0].url             = urls_translated[translated_index].c_str();
                        routes[i].dsts[0].handlers        = handlers_xrsr;
                        routes[i].dsts[0].formats         = XRSR_AUDIO_FORMAT_PCM;
                        routes[i].dsts[0].stream_time_min = this->prefs.utterance_duration_min;
                        routes[i].dsts[0].stream_from     = stream_from;
                        routes[i].dsts[0].stream_offset   = stream_offset;
                        routes[i].dsts[0].stream_until    = stream_until;
                        #ifdef ENABLE_DEEP_SLEEP
                        if(src == XRSR_SRC_MICROPHONE) {
                                routes[i].dsts[0].params[XRSR_POWER_MODE_LOW] = &this->prefs.dst_params_standby;
                        }
                        #endif
                        i++;
               }
        }
        #endif
        else {
            XLOGD_ERROR("unsupported url <%s>", url->c_str());
        }
    }
    routes[i].src     = XRSR_SRC_INVALID;
    routes[i].dst_qty = 0;

    if(i == 0) {
        XLOGD_INFO("no routes available");
    } else {
        for(int j = 0; j < i; j++) {
            XLOGD_INFO("src <%s>", xrsr_src_str(routes[j].src));
            for(uint32_t k = 0; k < routes[j].dst_qty; k++) {
               XLOGD_INFO("dst <%u> url <%s>", k, routes[j].dsts[k].url);
            }
        }
    }

    if(!xrsr_route(routes)) {
        XLOGD_ERROR("failed to set routes");
    }
}

void ctrlm_voice_generic_t::query_strings_updated() {
    XLOGD_INFO("");
    #ifdef SUPPORT_VOICE_DEST_HTTP
    // update query strings for HTTP only
    if(this->obj_http) {
        this->obj_http->clear_query_strings();
        for(unsigned int i = 0; i < this->query_strs_ptt.size(); i++) {
            this->obj_http->add_query_string(this->query_strs_ptt[i].first.c_str(), this->query_strs_ptt[i].second.c_str());
        }
    }
    #endif
}

void ctrlm_voice_generic_t::mask_pii_updated(bool enable) {
    if(this->obj_ws_nextgen) {
        this->obj_ws_nextgen->voice_stb_data_mask_pii_set(enable);
    }
    #ifdef SUPPORT_VOICE_DEST_HTTP
    if(this->obj_http) {
        this->obj_http->voice_stb_data_mask_pii_set(enable);
    }
    #endif
    #ifdef SUPPORT_VOICE_DEST_ALSA
    if(this->obj_sdt != NULL) {
        this->obj_sdt->voice_stb_data_mask_pii_set(enable);
    }
    #endif
}

bool ctrlm_voice_generic_t::url_hostname_get(std::string *url, std::string &hostname) {
    std::size_t pos_begin = url->find("://");
    if(pos_begin == std::string::npos) {
        XLOGD_ERROR("unable to find hostname begin <%s>", url->c_str());
        return(false);
    }
    pos_begin += 3;
    std::size_t pos_end = url->find_first_of(":/", pos_begin);
    if(pos_end == std::string::npos) {
        XLOGD_ERROR("unable to find hostname end <%s>", url->c_str());
        return(false);
    }
    hostname = url->substr(pos_begin, pos_end - pos_begin);
    return(true);
}

void ctrlm_voice_generic_t::url_hostname_verify(std::string *url) {
    bool verified = false;

    std::string hostname;
    if(this->url_hostname_get(url, hostname)) {
        for(std::vector<std::string>::iterator it = this->prefs.server_hosts.begin(); it != this->prefs.server_hosts.end(); it++) {
            std::regex pattern(it->c_str());

            if(std::regex_match(hostname, pattern)) {
                XLOGD_DEBUG("regex <%s> matched <%s>", it->c_str(), hostname.c_str());
                verified = true;
                break;
            }
        }
    }

    if(!verified) {
        XLOGD_TELEMETRY("server hostname match failed <%s>", url->c_str());
        XLOGD_INFO("hostname <%s>", hostname.c_str());
        for(std::vector<std::string>::iterator it = this->prefs.server_hosts.begin(); it != this->prefs.server_hosts.end(); it++) {
            XLOGD_INFO("regex <%s>", it->c_str());
        }
    }
}
