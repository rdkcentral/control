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
#include "../ctrlm_voice_packet_analysis.h"
#include "../ctrlm_log.h"


class stubs_ctrlm_voice_packet_analysis : public ctrlm_voice_packet_analysis {
   public:
      stubs_ctrlm_voice_packet_analysis();
      virtual ~stubs_ctrlm_voice_packet_analysis();
      virtual void reset();
      virtual ctrlm_voice_packet_analysis_result_t packet_check(const void* header, unsigned long header_len, const void* data, unsigned long data_len);
      virtual void stats_get(ctrlm_voice_packet_analysis_stats_t& stats) const;
   private:
      uint32_t total_packets;
};

stubs_ctrlm_voice_packet_analysis::stubs_ctrlm_voice_packet_analysis() : total_packets(0) {
   XLOGD_INFO("STUB Constructor");
}

stubs_ctrlm_voice_packet_analysis::~stubs_ctrlm_voice_packet_analysis() {
   XLOGD_INFO("STUB Destructor");
}

void stubs_ctrlm_voice_packet_analysis::reset() {
   XLOGD_INFO("STUB");
   total_packets = 0;
}

ctrlm_voice_packet_analysis_result_t stubs_ctrlm_voice_packet_analysis::packet_check(const void* header, unsigned long header_len, const void* data, unsigned long data_len) {
   XLOGD_INFO("STUB");
   ++total_packets;
   return CTRLM_VOICE_PACKET_ANALYSIS_GOOD;
}

void stubs_ctrlm_voice_packet_analysis::stats_get(ctrlm_voice_packet_analysis_stats_t& stats) const {
   XLOGD_INFO("STUB");
   stats.total_packets = total_packets;
   stats.bad_packets = 0;
   stats.duplicated_packets = 0;
   stats.lost_packets = 0;
   stats.sequence_error_count = 0;
}

ctrlm_voice_packet_analysis* ctrlm_voice_packet_analysis_factory() {
   return new stubs_ctrlm_voice_packet_analysis;
}
