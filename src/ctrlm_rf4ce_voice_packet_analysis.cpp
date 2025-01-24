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

#include "ctrlm_voice_packet_analysis.h"
#include "ctrlm_log.h"

#define SEQUENCE_NUM_INVALID  (0xFF)

class ctrlm_voice_packet_analysis_rf4ce_t : public ctrlm_voice_packet_analysis {
   public:
      ctrlm_voice_packet_analysis_rf4ce_t();
      virtual void reset();
      virtual ctrlm_voice_packet_analysis_result_t packet_check(const void* header, unsigned long header_len, const void* data, unsigned long data_len);
      virtual void stats_get(ctrlm_voice_packet_analysis_stats_t& stats) const;
   private:
      uint8_t                          sequence_num_last;
      uint32_t                         total_packets;
      uint32_t                         duplicated_packets;
      uint32_t                         lost_packets;
      uint32_t                         sequence_error_count;
};



ctrlm_voice_packet_analysis_rf4ce_t::ctrlm_voice_packet_analysis_rf4ce_t() :
   sequence_num_last(SEQUENCE_NUM_INVALID),
   total_packets(0),
   duplicated_packets(0),
   lost_packets(0),
   sequence_error_count(0)
   {
   }

void ctrlm_voice_packet_analysis_rf4ce_t::reset() {
   sequence_num_last = 0x20;
   total_packets = 0;
   duplicated_packets = 0;
   lost_packets = 0;
   sequence_error_count = 0;
}

ctrlm_voice_packet_analysis_result_t ctrlm_voice_packet_analysis_rf4ce_t::packet_check(const void* header, unsigned long header_len, const void* data, unsigned long data_len) {
   ctrlm_voice_packet_analysis_result_t result = CTRLM_VOICE_PACKET_ANALYSIS_GOOD;
   // sanity check
   if (header_len != sizeof(uint8_t)) {
      XLOGD_INFO("header_len != 1");
      return (result = CTRLM_VOICE_PACKET_ANALYSIS_ERROR);
   }

   uint8_t seqnum_act  = *(uint8_t*)header;
   uint8_t seqnum_exp = sequence_num_last+1 > 0x3F ? 0x20 : sequence_num_last+1;

   if(seqnum_act == sequence_num_last) {
       XLOGD_INFO("Sequence duplicate: rec:%x exp:%x", (unsigned)seqnum_act, (unsigned)seqnum_exp);
       ++total_packets;
       ++duplicated_packets;
       return (result = CTRLM_VOICE_PACKET_ANALYSIS_DUPLICATE); // don't propagate repeated voice packets
   }

   if(seqnum_act != seqnum_exp) {
       XLOGD_INFO("Sequence discontinuity: rec:%x exp:%x", (unsigned)seqnum_act, (unsigned)seqnum_exp);
       ++sequence_error_count;
       uint8_t missing_packets;
       if(seqnum_act > seqnum_exp) {
          missing_packets = seqnum_act - seqnum_exp;
       } else {
          missing_packets = 0x20 - seqnum_exp + seqnum_act;
       }
       lost_packets  += missing_packets;
       total_packets += missing_packets;
       result = CTRLM_VOICE_PACKET_ANALYSIS_DISCONTINUITY;
   }

   sequence_num_last = seqnum_act;
   total_packets++;
   return result;
}

void ctrlm_voice_packet_analysis_rf4ce_t::stats_get(ctrlm_voice_packet_analysis_stats_t& stats) const {
   stats.total_packets = total_packets;
   stats.bad_packets = 0;
   stats.duplicated_packets = duplicated_packets;
   stats.lost_packets = lost_packets;
   stats.sequence_error_count = sequence_error_count;
}

ctrlm_voice_packet_analysis* ctrlm_voice_packet_analysis_factory() {
   return new ctrlm_voice_packet_analysis_rf4ce_t;
}





