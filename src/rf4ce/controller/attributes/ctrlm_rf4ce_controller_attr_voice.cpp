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
#include "ctrlm_rf4ce_controller_attr_voice.h"
#include "rf4ce/ctrlm_rf4ce_controller.h"
#include "ctrlm_db_types.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include <algorithm>
#include <sstream>
#include "ctrlm_database.h"
#include "ctrlm_config_types.h"
#include "ctrlm_voice_obj.h"

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_controller_audio_profiles_t
#define AUDIO_PROFILES_ID    (0x17)
#define AUDIO_PROFILES_INDEX (0x00)
#define AUDIO_PROFILES_LEN   (0x02)
ctrlm_rf4ce_controller_audio_profiles_t::ctrlm_rf4ce_controller_audio_profiles_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_audio_profiles_t(ctrlm_audio_profiles_t::profile::NONE),
ctrlm_db_attr_t(net, id),
ctrlm_rf4ce_rib_attr_t(AUDIO_PROFILES_ID, AUDIO_PROFILES_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)
{
    this->set_name_prefix("Controller ");
}

ctrlm_rf4ce_controller_audio_profiles_t::~ctrlm_rf4ce_controller_audio_profiles_t() {

}

bool ctrlm_rf4ce_controller_audio_profiles_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= AUDIO_PROFILES_LEN) {
        data[0] =  this->supported_profiles       & 0xFF;
        data[1] = (this->supported_profiles >> 8) & 0xFF;
        ret = true;
    }
    return(ret);
}

bool ctrlm_rf4ce_controller_audio_profiles_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("audio_profiles", this->get_table());
    char buf[AUDIO_PROFILES_LEN] = {0,0};

    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len == AUDIO_PROFILES_LEN) {
                this->supported_profiles = (buf[1] << 8) | buf[0];
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_controller_audio_profiles_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("audio_profiles", this->get_table());
    char buf[AUDIO_PROFILES_LEN] = {0,0};

    this->to_buffer(buf, sizeof(buf));
    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_audio_profiles_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = AUDIO_PROFILES_LEN;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_audio_profiles_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == AUDIO_PROFILES_LEN) {
            int temp = this->supported_profiles;
            this->supported_profiles = data[0] + (data[1] << 8);
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s written to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(temp != this->supported_profiles && false == importing) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

void ctrlm_rf4ce_controller_audio_profiles_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[AUDIO_PROFILES_LEN];
    if(ctrlm_rf4ce_rib_attr_t::status::SUCCESS == this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}
// end ctrlm_rf4ce_controller_audio_profiles_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_voice_statistics_t
#define VOICE_STATISTICS_ID    (0x19)
#define VOICE_STATISTICS_INDEX (0x00)
#define VOICE_STATISTICS_LEN   (0x08)
ctrlm_rf4ce_voice_statistics_t::ctrlm_rf4ce_voice_statistics_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_attr_t("Controller Voice Statistics"),
ctrlm_db_attr_t(net, id),
ctrlm_rf4ce_rib_attr_t(VOICE_STATISTICS_ID, VOICE_STATISTICS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)
{
    this->voice_sessions = 0;
    this->tx_time        = 0;
}

ctrlm_rf4ce_voice_statistics_t::~ctrlm_rf4ce_voice_statistics_t() {

}

bool ctrlm_rf4ce_voice_statistics_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= VOICE_STATISTICS_LEN) {
        data[0] = (uint8_t)(this->voice_sessions);
        data[1] = (uint8_t)(this->voice_sessions >> 8);
        data[2] = (uint8_t)(this->voice_sessions >> 16);
        data[3] = (uint8_t)(this->voice_sessions >> 24);
        data[4] = (uint8_t)(this->tx_time);
        data[5] = (uint8_t)(this->tx_time >> 8);
        data[6] = (uint8_t)(this->tx_time >> 16);
        data[7] = (uint8_t)(this->tx_time >> 24);
        ret = true;
    }
    return(ret);
}

std::string ctrlm_rf4ce_voice_statistics_t::to_string() const {
    std::stringstream ss;
    ss << "Voice Sessions Activated <" << this->voice_sessions << "> ";
    ss << "Audio Data TX Time <" << this->tx_time << ">";
    return(ss.str());
}

bool ctrlm_rf4ce_voice_statistics_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("voice_statistics", this->get_table());
    char buf[VOICE_STATISTICS_LEN] = {0};

    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len == VOICE_STATISTICS_LEN) {
                this->voice_sessions = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
                this->tx_time        = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_voice_statistics_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("audio_profiles", this->get_table());
    char buf[VOICE_STATISTICS_LEN] = {0};

    this->to_buffer(buf, sizeof(buf));
    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_statistics_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = VOICE_STATISTICS_LEN;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_statistics_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == VOICE_STATISTICS_LEN) {
            uint32_t temp_sessions = this->voice_sessions;
            uint32_t temp_tx_time  = this->tx_time;

            this->voice_sessions = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
            this->tx_time        = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s written to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if((temp_sessions != this->voice_sessions ||
                temp_tx_time  != this->tx_time) &&
                false == importing) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

void ctrlm_rf4ce_voice_statistics_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[VOICE_STATISTICS_LEN];
    if(ctrlm_rf4ce_rib_attr_t::status::SUCCESS == this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}
// end ctrlm_rf4ce_voice_statistics_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_voice_session_statistics_t
#define VOICE_SESSION_STATISTICS_ID    (0x1C)
#define VOICE_SESSION_STATISTICS_INDEX (0x00)
#define VOICE_SESSION_STATISTICS_LEN   (16)
ctrlm_rf4ce_voice_session_statistics_t::ctrlm_rf4ce_voice_session_statistics_t(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) :
ctrlm_rf4ce_rib_attr_t(VOICE_SESSION_STATISTICS_ID, VOICE_SESSION_STATISTICS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)
{
    this->network_id    = network_id;
    this->controller_id = controller_id;
}

ctrlm_rf4ce_voice_session_statistics_t::~ctrlm_rf4ce_voice_session_statistics_t() {

}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_session_statistics_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == VOICE_SESSION_STATISTICS_LEN) {
            uint16_t total_packets, total_dropped, drop_retry, drop_buffer, mac_retries, network_retries, cca_sense;
            float drop_percent, retry_percent, buffer_percent;
            total_packets   = (data[1]  << 8) | data[0];
            drop_retry      = (data[3]  << 8) | data[2];
            drop_buffer     = (data[5]  << 8) | data[4];
            total_dropped   = drop_retry + drop_buffer;
            drop_percent    = (100.0 * total_dropped / total_packets);
            retry_percent   = (100.0 * drop_retry    / total_packets);
            buffer_percent  = (100.0 * drop_buffer   / total_packets);
            // Interference indicators
            mac_retries     = (data[7]  << 8) | data[6];
            network_retries = (data[9]  << 8) | data[8];
            cca_sense       = (data[11] << 8) | data[10];
   
            // Write this data to the database??

            XLOGD_INFO("Voice Session Stats written to RIB: Total Packets %u Dropped %u (%5.2f%%) due to Retry %u (%5.2f%%) Buffer %u (%5.2f%%)", total_packets, total_dropped, drop_percent, drop_retry, retry_percent, drop_buffer, buffer_percent);

            if(mac_retries != 0xFFFF || network_retries != 0xFFFF || cca_sense != 0xFFFF) {
                XLOGD_INFO("Total MAC Retries %u Network Retries %u CCA Sense Failures %u", mac_retries, network_retries, cca_sense);
            }

            // Send data to voice object
            ctrlm_voice_t *obj = ctrlm_get_voice_obj();
            if(NULL != obj) {
               ctrlm_voice_stats_session_t stats_session;

               stats_session.available        = 1;
               stats_session.packets_total    = total_packets;
               stats_session.dropped_retry    = drop_retry;
               stats_session.dropped_buffer   = drop_buffer;
               stats_session.retry_mac        = mac_retries;
               stats_session.retry_network    = network_retries;
               stats_session.cca_sense        = cca_sense;

               // The following are not set here and will be ignored
               //stats_session.rf_channel       = 0;
               //stats_session.buffer_watermark = 0;
               //stats_session.packets_lost     = 0;
               //stats_session.link_quality     = 0;

               obj->voice_session_stats(stats_session);
            }

            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;

        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_voice_session_statistics_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_voice_command_status_t
#define VOICE_COMMAND_STATUS_ID    (0x10)
#define VOICE_COMMAND_STATUS_INDEX (0x00)
#define VOICE_COMMAND_STATUS_LEN_OLD      (1)
#define VOICE_COMMAND_STATUS_LEN_NEW      (5)
ctrlm_rf4ce_voice_command_status_t::ctrlm_rf4ce_voice_command_status_t(ctrlm_obj_controller_rf4ce_t *controller) :
ctrlm_attr_t("Voice Command Status"),
ctrlm_rf4ce_rib_attr_t(VOICE_COMMAND_STATUS_ID, VOICE_COMMAND_STATUS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET) 
{
    this->controller = controller;
    this->reset();
}

ctrlm_rf4ce_voice_command_status_t::~ctrlm_rf4ce_voice_command_status_t() {

}

void ctrlm_rf4ce_voice_command_status_t::reset() {
    this->vcs = ctrlm_rf4ce_voice_command_status_t::status::NO_COMMAND_SENT;
    this->flags  = 0x00;
}

std::string ctrlm_rf4ce_voice_command_status_t::status_str(ctrlm_rf4ce_voice_command_status_t::status s) {
    std::stringstream ss; ss.str("INVALID");
    switch(s) {
        case ctrlm_rf4ce_voice_command_status_t::status::PENDING:            {ss.str("PENDING");            break;}
        case ctrlm_rf4ce_voice_command_status_t::status::TIMEOUT:            {ss.str("TIMEOUT");            break;}
        case ctrlm_rf4ce_voice_command_status_t::status::OFFLINE:            {ss.str("OFFLINE");            break;}
        case ctrlm_rf4ce_voice_command_status_t::status::SUCCESS:            {ss.str("SUCCESS");            break;}
        case ctrlm_rf4ce_voice_command_status_t::status::FAILURE:            {ss.str("FAILURE");            break;}
        case ctrlm_rf4ce_voice_command_status_t::status::NO_COMMAND_SENT:    {ss.str("NO_COMMAND_SENT");    break;}
        case ctrlm_rf4ce_voice_command_status_t::status::TV_AVR_COMMAND:     {ss.str("TV_AVR_COMMAND");     break;}
        case ctrlm_rf4ce_voice_command_status_t::status::MICROPHONE_COMMAND: {ss.str("MICROPHONE_COMMAND"); break;}
        case ctrlm_rf4ce_voice_command_status_t::status::AUDIO_COMMAND:      {ss.str("AUDIO_COMMAND");      break;}
        default: {ss << " <" << s << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_voice_command_status_t::tv_avr_command_flag_str(ctrlm_rf4ce_voice_command_status_t::tv_avr_command_flag f) {
    std::stringstream ss; ss.str("INVALID");
    switch(f) {
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command_flag::TOGGLE_FALLBACK: {ss.str("TOGGLE_FALLBACK"); break;}
        default: {ss << " <" << f << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_voice_command_status_t::tv_avr_command_str(ctrlm_rf4ce_voice_command_status_t::tv_avr_command c) {
    std::stringstream ss; ss.str("INVALID");
    switch(c) {
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::POWER_OFF:    {ss.str("POWER_OFF");    break;}
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::POWER_ON:     {ss.str("POWER_ON");     break;}
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_UP:    {ss.str("VOLUME_UP");    break;}
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_DOWN:  {ss.str("VOLUME_DOWN");  break;}
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_MUTE:  {ss.str("VOLUME_MUTE");  break;}
        case ctrlm_rf4ce_voice_command_status_t::tv_avr_command::POWER_TOGGLE: {ss.str("POWER_TOGGLE"); break;}
        default: {ss << " <" << c << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_voice_command_status_t::to_string() const {
    std::stringstream ss;
    ss << "Status <" << this->status_str(this->vcs) << "> ";
    if(this->vcs == ctrlm_rf4ce_voice_command_status_t::status::TV_AVR_COMMAND) {
        ss << "Flags <";
        if(this->flags & ctrlm_rf4ce_voice_command_status_t::tv_avr_command_flag::TOGGLE_FALLBACK) {
            ss << this->tv_avr_command_flag_str(ctrlm_rf4ce_voice_command_status_t::tv_avr_command_flag::TOGGLE_FALLBACK);
        }
        ss << "> ";
        ss << "Command <" << this->tv_avr_command_str(this->tac) << "> ";
        if(this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_UP   ||
           this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_DOWN ||
           this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_MUTE) {
            ss << "IR Repeats <" << this->ir_repeats << "> ";
        }
    }
    return(ss.str());
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_command_status_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= VOICE_COMMAND_STATUS_LEN_OLD) {
            data[0] = this->vcs & 0xFF;
            if(*len >= VOICE_COMMAND_STATUS_LEN_NEW) {
                memset(&data[1], 0, *len-1);
                data[1] = this->flags & 0xFF;
                if(this->vcs == ctrlm_rf4ce_voice_command_status_t::status::TV_AVR_COMMAND) {
                    data[2] = this->tac & 0xFF;
                    if(this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_UP   ||
                       this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_DOWN ||
                       this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_MUTE) {
                        data[3] = this->ir_repeats & 0xFF;
                    }
                }
                *len = VOICE_COMMAND_STATUS_LEN_NEW;
            } else {
                *len = VOICE_COMMAND_STATUS_LEN_OLD;
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(this->vcs != ctrlm_rf4ce_voice_command_status_t::status::PENDING && accessor == ctrlm_rf4ce_rib_attr_t::CONTROLLER) {
                ctrlm_voice_t *obj = ctrlm_get_voice_obj();
                if(obj != NULL) {
                    obj->voice_controller_command_status_read(this->controller->network_id_get(), this->controller->controller_id_get());
                }
                this->reset();
            }
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_command_status_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == VOICE_COMMAND_STATUS_LEN_OLD || len == VOICE_COMMAND_STATUS_LEN_NEW) {
            this->vcs = (ctrlm_rf4ce_voice_command_status_t::status)data[0];
            if(len == VOICE_COMMAND_STATUS_LEN_NEW) {
                this->flags = data[1];
                if(this->vcs == ctrlm_rf4ce_voice_command_status_t::status::TV_AVR_COMMAND) {
                    this->tac = (ctrlm_rf4ce_voice_command_status_t::tv_avr_command)data[2];
                    if(this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_UP   ||
                       this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_DOWN ||
                       this->tac == ctrlm_rf4ce_voice_command_status_t::tv_avr_command::VOLUME_MUTE) {
                        this->ir_repeats = data[3] & 0xFF;
                    }
                }
            }
            if(!(this->controller && this->controller->controller_type_get() == RF4CE_CONTROLLER_TYPE_XR19)) { // IF NOT XR19
                if(this->vcs >= ctrlm_rf4ce_voice_command_status_t::status::TV_AVR_COMMAND) {
                    this->vcs = ctrlm_rf4ce_voice_command_status_t::status::SUCCESS;
                }
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s written to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_voice_command_status_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_voice_command_length_t
#define VOICE_COMMAND_LENGTH_ID    (0x11)
#define VOICE_COMMAND_LENGTH_INDEX (0x00)
#define VOICE_COMMAND_LENGTH_LEN   (1)
ctrlm_rf4ce_voice_command_length_t::ctrlm_rf4ce_voice_command_length_t() :
ctrlm_attr_t("Voice Session Length"),
ctrlm_rf4ce_rib_attr_t(VOICE_COMMAND_LENGTH_ID, VOICE_COMMAND_LENGTH_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET)
{
    this->vcl = ctrlm_rf4ce_voice_command_length_t::length::VALUE;
}

ctrlm_rf4ce_voice_command_length_t::~ctrlm_rf4ce_voice_command_length_t() {

}

void ctrlm_rf4ce_voice_command_length_t::set_updated_listener(ctrlm_rf4ce_voice_command_length_listener_t listener) {
    this->updated_listener = listener;
}

std::string ctrlm_rf4ce_voice_command_length_t::length_str(length l) {
    std::stringstream ss; ss << "INVALID";
    switch(l) {
        case ctrlm_rf4ce_voice_command_length_t::length::CONTROLLER_DEFAULT: {ss.str("CONTROLLER_DEFAULT"); break;}
        case ctrlm_rf4ce_voice_command_length_t::length::PROFILE_NEGOTIATION: {ss.str("PROFILE_NEGOTIATION"); break;}
        case ctrlm_rf4ce_voice_command_length_t::length::VALUE: {ss.str(""); ss << "VALUE <" << (int)ctrlm_rf4ce_voice_command_length_t::length::VALUE << " samples>"; break;}
        default: {ss << " <" << (int)l << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_voice_command_length_t::to_string() const {
    return(this->length_str(this->vcl));
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_command_length_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= VOICE_COMMAND_LENGTH_LEN) {
            data[0] = this->vcl & 0xFF;
            *len = VOICE_COMMAND_LENGTH_LEN;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_voice_command_length_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == VOICE_COMMAND_LENGTH_LEN) {
            ctrlm_rf4ce_voice_command_length_t::length old_length = this->vcl;
            this->vcl = (ctrlm_rf4ce_voice_command_length_t::length)data[0];
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s written to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(this->vcl != old_length) {
                if(this->updated_listener) {
                    this->updated_listener(*this);
                }
            }
        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_voice_command_length_t
