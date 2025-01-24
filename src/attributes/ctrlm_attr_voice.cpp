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
#include "ctrlm_attr_voice.h"
#include "ctrlm_db_types.h"
#include "ctrlm_config_types.h"
#include "ctrlm_rfc.h"
#include "ctrlm_database.h"
#include "ctrlm_log.h"
#include "ctrlm_config_default.h"

#include <sstream>

#define VOICE_METRICS_LEN   (44)

ctrlm_voice_metrics_t::ctrlm_voice_metrics_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id, std::string db_key) :
ctrlm_attr_t("Voice Metrics"),
ctrlm_db_attr_t(net, id, std::move(db_key)),
ctrlm_config_attr_t("voice")
{
    this->voice_cmd_count_today                                = 0;
    this->voice_cmd_count_yesterday                            = 0;
    this->voice_cmd_short_today                                = 0;
    this->voice_cmd_short_yesterday                            = 0;
    this->today                                                = time(NULL) / (60 * 60 * 24);
    this->voice_packets_sent_today                             = 0;
    this->voice_packets_sent_yesterday                         = 0;
    this->voice_packets_lost_today                             = 0;
    this->voice_packets_lost_yesterday                         = 0;
    this->utterances_exceeding_packet_loss_threshold_today     = 0;
    this->utterances_exceeding_packet_loss_threshold_yesterday = 0;
    this->packet_loss_threshold                                = JSON_INT_VALUE_VOICE_PACKET_LOSS_THRESHOLD;
    ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
    if(rfc) {
        rfc->add_changed_listener(ctrlm_rfc_t::attrs::VOICE, std::bind(&ctrlm_voice_metrics_t::rfc_retrieved_handler, this, std::placeholders::_1));
    }
}

ctrlm_voice_metrics_t::~ctrlm_voice_metrics_t() {

}

void ctrlm_voice_metrics_t::process_time(bool write_db) {
    time_t time_in_seconds = time(NULL);
    time_t shutdown_time   = ctrlm_shutdown_time_get();
    if(time_in_seconds < shutdown_time) {
        XLOGD_WARN("Current Time <%ld> is less than the last shutdown time <%ld>.  Wait until time updates.", time_in_seconds, shutdown_time);
        return;
    }
    uint32_t now = time_in_seconds / (60 * 60 * 24);
    uint32_t day_change = now - this->today;

   //If this is a different day...
   if(day_change > 0) {
      //If this is the next day...
      if(day_change == 1) {
         this->voice_cmd_count_yesterday                            = this->voice_cmd_count_today;
         this->voice_cmd_short_yesterday                            = this->voice_cmd_short_today;
         this->voice_packets_sent_yesterday                         = this->voice_packets_sent_today;
         this->voice_packets_lost_yesterday                         = this->voice_packets_lost_today;
         this->utterances_exceeding_packet_loss_threshold_yesterday = this->utterances_exceeding_packet_loss_threshold_today;
      } else {
         this->voice_cmd_count_yesterday                            = 0;
         this->voice_cmd_short_yesterday                            = 0;
         this->voice_packets_sent_yesterday                         = 0;
         this->voice_packets_lost_yesterday                         = 0;
         this->utterances_exceeding_packet_loss_threshold_yesterday = 0;
      }

      this->voice_cmd_count_today                                   = 0;
      this->voice_cmd_short_today                                   = 0;
      this->voice_packets_sent_today                                = 0;
      this->voice_packets_lost_today                                = 0;
      this->utterances_exceeding_packet_loss_threshold_today        = 0;
      this->today                                                   = now;
      XLOGD_INFO("%s - day has changed by %u", this->get_name().c_str(), day_change);

      if(write_db) {
          ctrlm_db_attr_write(shared_from_this());
      }
   }
}

void ctrlm_voice_metrics_t::add_packets(uint32_t sent, uint32_t lost) {
    this->voice_packets_sent_today += sent;
    this->voice_packets_lost_today += lost;

    if((((float)lost/(float)sent)*100.0) > (float)(this->packet_loss_threshold)) {
        this->utterances_exceeding_packet_loss_threshold_today++;
    }
}

void ctrlm_voice_metrics_t::increment_voice_count(uint32_t sent, uint32_t lost) {
    this->voice_cmd_count_today++;
    this->add_packets(sent, lost);
    ctrlm_db_attr_write(shared_from_this());
}

void ctrlm_voice_metrics_t::increment_short_voice_count(uint32_t sent, uint32_t lost) {
    this->voice_cmd_short_today++;
    this->add_packets(sent, lost);
    ctrlm_db_attr_write(shared_from_this());
}

uint32_t ctrlm_voice_metrics_t::get_commands_today() const {
    return(this->voice_cmd_count_today);
}

uint32_t ctrlm_voice_metrics_t::get_commands_yesterday() const {
    return(this->voice_cmd_count_yesterday);
}

uint32_t ctrlm_voice_metrics_t::get_short_commands_today() const {
    return(this->voice_cmd_short_today);
}

uint32_t ctrlm_voice_metrics_t::get_short_commands_yesterday() const {
    return(this->voice_cmd_short_yesterday);
}

uint32_t ctrlm_voice_metrics_t::get_packets_sent_today() const {
    return(this->voice_packets_sent_today);
}

uint32_t ctrlm_voice_metrics_t::get_packets_sent_yesterday() const {
    return(this->voice_packets_sent_yesterday);
}

uint32_t ctrlm_voice_metrics_t::get_packets_lost_today() const {
    return(this->voice_packets_lost_today);
}

uint32_t ctrlm_voice_metrics_t::get_packets_lost_yesterday() const {
    return(this->voice_packets_lost_yesterday);
}

uint32_t ctrlm_voice_metrics_t::get_packet_loss_exceeding_threshold_today() const {
    return(this->utterances_exceeding_packet_loss_threshold_today);
}

uint32_t ctrlm_voice_metrics_t::get_packet_loss_exceeding_threshold_yesterday() const {
    return(this->utterances_exceeding_packet_loss_threshold_yesterday);
}

float ctrlm_voice_metrics_t::get_average_packet_loss_today() const {
    float ret = 0.0;
    if(this->voice_cmd_count_today > 0) {
        ret = (float)((float)this->voice_packets_lost_today/(float)this->voice_packets_sent_today) * 100.0;
    }
    return(ret);
}

float ctrlm_voice_metrics_t::get_average_packet_loss_yesterday() const {
    float ret = 0.0;
    if(this->voice_cmd_count_yesterday > 0) {
        ret = (float)((float)this->voice_packets_lost_yesterday/(float)this->voice_packets_sent_yesterday) * 100.0;
    }
    return(ret);
}

void ctrlm_voice_metrics_t::print(const char *prefix, bool single_line) const {
    const xlog_args_t xlog_args_info = {.options = XLOG_OPTS_DEFAULT, .color = XLOG_COLOR_NONE, .function = prefix, .line = XLOG_LINE_NONE, .level = XLOG_LEVEL_INFO, .id = XLOG_MODULE_ID, .size_max = XLOG_BUF_SIZE_DEFAULT};
    std::stringstream ss;
    if (single_line) {
        ss << "Voice Cmd Count Today <" << voice_cmd_count_today   << "> ";
        ss << "Voice Cmd Count Yesterday <" << voice_cmd_count_yesterday << "> ";
        ss << "Voice Cmd Short Today <" << voice_cmd_short_today  << "> ";
        ss << "Voice Cmd Short Yesterday <" << voice_cmd_short_yesterday << "> ";
        ss << "Voice Packets Sent Today <" << voice_packets_sent_today << "> ";
        ss << "Voice Packets Sent Yesterday <" << voice_packets_sent_yesterday << "> ";
        ss << "Packets Lost Today <" << voice_packets_lost_today << "> "; 
        ss << "Packets Lost Yesterday <" << voice_packets_lost_yesterday << "> ";
        ss << "Utterances Exceeding Pkt Loss Threshold Today <" << utterances_exceeding_packet_loss_threshold_today << "> ";
        ss << "Utterances Exceeding Pkt Loss Threshold Yesterday <" << utterances_exceeding_packet_loss_threshold_yesterday << "> ";
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "%s: %s\n",prefix, ss.str().c_str());
    } else {
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Count Today        : %lu", voice_cmd_count_today);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Count Yesterday    : %lu", voice_cmd_count_yesterday);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Short Today        : %lu", voice_cmd_short_today);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Short Yesterday    : %lu", voice_cmd_short_yesterday);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Packets Sent Today     : %lu", voice_packets_sent_today);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Packets Sent Yesterday : %lu", voice_packets_sent_yesterday);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Packets Lost Today     : %lu", voice_packets_lost_today);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Packets Lost Yesterday : %lu", voice_packets_lost_yesterday);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Utterances Exceeding Pkt Loss Threshold Today     : %lu", utterances_exceeding_packet_loss_threshold_today);
        xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Utterances Exceeding Pkt Loss Threshold Yesterday : %lu", utterances_exceeding_packet_loss_threshold_yesterday);
    }
}

std::string ctrlm_voice_metrics_t::to_string() const {
    std::stringstream ss;
    ss << "Voice Cmd Count Today <" << this->voice_cmd_count_today << "> ";
    ss << "Voice Cmd Count Yesterday <" << this->voice_cmd_count_yesterday << "> ";
    ss << "Voice Cmd Short Today <" << this->voice_cmd_short_today << "> ";
    ss << "Voice Cmd Short Yesterday <" << this->voice_cmd_short_yesterday << "> "; 
    ss << "Voice Packets Sent Today <" << this->voice_packets_sent_today << "> ";
    ss << "Voice Packets Sent Yesterday <" << this->voice_packets_sent_yesterday << "> ";
    ss << "Packets Lost Today <" << this->voice_packets_lost_today << "> "; 
    ss << "Packets Lost Yesterday <" << this->voice_packets_lost_yesterday << "> ";
    ss << "Utterances Exceeding Pkt Loss Threshold Today <" << this->utterances_exceeding_packet_loss_threshold_today << "> ";
    ss << "Utterances Exceeding Pkt Loss Threshold Yesterday <" << this->utterances_exceeding_packet_loss_threshold_yesterday << "> ";
    ss << " Today=<" << this->today << "> ";
    return(ss.str());
}

bool ctrlm_voice_metrics_t::read_config() {
    bool ret = false;
    ctrlm_config_int_t threshold("voice.packet_loss_threshold");
    if(threshold.get_config_value(this->packet_loss_threshold, 0, 100)) {
        XLOGD_INFO("Packet Loss Threshold from config file: %d%%", this->packet_loss_threshold);
        ret = true;
    } else {
        XLOGD_INFO("Packet Loss Threshold default: %d%%", this->packet_loss_threshold);
    }
    return(ret);
}

void ctrlm_voice_metrics_t::rfc_retrieved_handler(const ctrlm_rfc_attr_t &attr) {
    attr.get_rfc_value(JSON_INT_NAME_VOICE_PACKET_LOSS_THRESHOLD, this->packet_loss_threshold, 0);
}

bool ctrlm_voice_metrics_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());
    char buf[VOICE_METRICS_LEN];

    memset(buf, 0, sizeof(buf));
    if(blob.read_db(ctx)) {
        int len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len >= VOICE_METRICS_LEN) {
                this->voice_cmd_count_today                                 = ((buf[3]  << 24) | (buf[2]  << 16) | (buf[1]  << 8) | buf[0]);
                this->voice_cmd_count_yesterday                             = ((buf[7]  << 24) | (buf[6]  << 16) | (buf[5]  << 8) | buf[4]);
                this->voice_cmd_short_today                                 = ((buf[11] << 24) | (buf[10] << 16) | (buf[9]  << 8) | buf[8]);
                this->voice_cmd_short_yesterday                             = ((buf[15] << 24) | (buf[14] << 16) | (buf[13] << 8) | buf[12]);
                this->today                                                 = ((buf[19] << 24) | (buf[18] << 16) | (buf[17] << 8) | buf[16]);
                this->voice_packets_sent_today                              = ((buf[23] << 24) | (buf[22] << 16) | (buf[21] << 8) | buf[20]);
                this->voice_packets_sent_yesterday                          = ((buf[27] << 24) | (buf[26] << 16) | (buf[25] << 8) | buf[24]);
                this->voice_packets_lost_today                              = ((buf[31] << 24) | (buf[30] << 16) | (buf[29] << 8) | buf[28]);
                this->voice_packets_lost_yesterday                          = ((buf[35] << 24) | (buf[34] << 16) | (buf[33] << 8) | buf[32]);
                this->utterances_exceeding_packet_loss_threshold_today      = ((buf[39] << 24) | (buf[38] << 16) | (buf[37] << 8) | buf[36]);
                this->utterances_exceeding_packet_loss_threshold_yesterday  = ((buf[43] << 24) | (buf[42] << 16) | (buf[41] << 8) | buf[40]);
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db is too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_voice_metrics_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());
    char buf[VOICE_METRICS_LEN];

    memset(buf, 0, sizeof(buf));
    buf[0]  = (uint8_t)(this->voice_cmd_count_today);
    buf[1]  = (uint8_t)(this->voice_cmd_count_today >> 8);
    buf[2]  = (uint8_t)(this->voice_cmd_count_today >> 16);
    buf[3]  = (uint8_t)(this->voice_cmd_count_today >> 24);
    buf[4]  = (uint8_t)(this->voice_cmd_count_yesterday);
    buf[5]  = (uint8_t)(this->voice_cmd_count_yesterday >> 8);
    buf[6]  = (uint8_t)(this->voice_cmd_count_yesterday >> 16);
    buf[7]  = (uint8_t)(this->voice_cmd_count_yesterday >> 24);
    buf[8]  = (uint8_t)(this->voice_cmd_short_today);
    buf[9]  = (uint8_t)(this->voice_cmd_short_today >> 8);
    buf[10] = (uint8_t)(this->voice_cmd_short_today >> 16);
    buf[11] = (uint8_t)(this->voice_cmd_short_today >> 24);
    buf[12] = (uint8_t)(this->voice_cmd_short_yesterday);
    buf[13] = (uint8_t)(this->voice_cmd_short_yesterday >> 8);
    buf[14] = (uint8_t)(this->voice_cmd_short_yesterday >> 16);
    buf[15] = (uint8_t)(this->voice_cmd_short_yesterday >> 24);
    buf[16] = (uint8_t)(this->today);
    buf[17] = (uint8_t)(this->today >> 8);
    buf[18] = (uint8_t)(this->today >> 16);
    buf[19] = (uint8_t)(this->today >> 24);
    buf[20] = (uint8_t)(this->voice_packets_sent_today);
    buf[21] = (uint8_t)(this->voice_packets_sent_today >> 8);
    buf[22] = (uint8_t)(this->voice_packets_sent_today >> 16);
    buf[23] = (uint8_t)(this->voice_packets_sent_today >> 24);
    buf[24] = (uint8_t)(this->voice_packets_sent_yesterday);
    buf[25] = (uint8_t)(this->voice_packets_sent_yesterday >> 8);
    buf[26] = (uint8_t)(this->voice_packets_sent_yesterday >> 16);
    buf[27] = (uint8_t)(this->voice_packets_sent_yesterday >> 24);
    buf[28] = (uint8_t)(this->voice_packets_lost_today);
    buf[29] = (uint8_t)(this->voice_packets_lost_today >> 8);
    buf[30] = (uint8_t)(this->voice_packets_lost_today >> 16);
    buf[31] = (uint8_t)(this->voice_packets_lost_today >> 24);
    buf[32] = (uint8_t)(this->voice_packets_lost_yesterday);
    buf[33] = (uint8_t)(this->voice_packets_lost_yesterday >> 8);
    buf[34] = (uint8_t)(this->voice_packets_lost_yesterday >> 16);
    buf[35] = (uint8_t)(this->voice_packets_lost_yesterday >> 24);
    buf[36] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_today);
    buf[37] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_today >> 8);
    buf[38] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_today >> 16);
    buf[39] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_today >> 24);
    buf[40] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_yesterday);
    buf[41] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_yesterday >> 8);
    buf[42] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_yesterday >> 16);
    buf[43] = (uint8_t)(this->utterances_exceeding_packet_loss_threshold_yesterday >> 24);
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
