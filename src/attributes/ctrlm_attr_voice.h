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
#ifndef __CTRLM_ATTR_VOICE_H__
#define __CTRLM_ATTR_VOICE_H__
#include "ctrlm_attr.h"
#include "ctrlm_rfc_attr.h"
#include "ctrlm_db_attr.h"
#include "ctrlm_config_attr.h"


/**
 * @brief ControlMgr RF4CE Voice Metrics Class
 * 
 * This class contains the controller's voice metrics for the current day and previous day
 */
class ctrlm_voice_metrics_t : public ctrlm_attr_t, public ctrlm_db_attr_t, public ctrlm_config_attr_t {
public:
    /**
     * RF4CE Voice Metrics Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_voice_metrics_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF, std::string db_key = "voice_cmd_counts");
    /**
     * RF4CE Voice Metrics Destructor
     */
    virtual ~ctrlm_voice_metrics_t();

public:
    /**
     * Function is called to check whether it is a new day and handle the day change
     * @param write_db True if caller wants the new data to be written to the DB if day has changed, False if no DB write is needed
     */
    void process_time(bool write_db = false);
    /**
     * Function is called to update the metrics for a normal voice command. This function writes the new data to the DB.
     * @param sent The number of packets received from the remote
     * @param lost The calculated number of lost packets
     */
    void increment_voice_count(uint32_t sent, uint32_t lost);
    /**
     * Function is called to update the metrics for a short voice command. This function writes the new data to the DB.
     * @param sent The number of packets received from the remote
     * @param lost The calculated number of lost packets
     */
    void increment_short_voice_count(uint32_t sent, uint32_t lost);

protected:
    /**
     * Internal helper function to handle the packet data.
     * @param sent The number of packets received from the remote
     * @param lost The calculated number of lost packets
     */
    void add_packets(uint32_t sent, uint32_t lost);

public:
    /**
     * Getter function for normal voice commands for the current day
     * @return The number of voice commands for current day.
     */
    uint32_t get_commands_today() const;
    /**
     * Getter function for normal voice commands for the previous day
     * @return The number of voice commands for previous day.
     */
    uint32_t get_commands_yesterday() const;
    /**
     * Getter function for short voice commands for the current day
     * @return The number of voice commands for current day.
     */
    uint32_t get_short_commands_today() const;
    /**
     * Getter function for short voice commands for the previous day
     * @return The number of voice commands for previous day.
     */
    uint32_t get_short_commands_yesterday() const;
    /**
     * Getter function for packets sent for the current day
     * @return The number of packets sent for current day.
     */
    uint32_t get_packets_sent_today() const;
    /**
     * Getter function for packets sent for the previous day
     * @return The number of packets sent for previous day.
     */
    uint32_t get_packets_sent_yesterday() const;
    /**
     * Getter function for packets lost for the current day
     * @return The number of packets lost for current day.
     */
    uint32_t get_packets_lost_today() const;
    /**
     * Getter function for packets lost for the previous day
     * @return The number of packets lost for previous day.
     */
    uint32_t get_packets_lost_yesterday() const;
    /**
     * Getter function for number of sessions that were above the packet loss threshold for the current day.
     * @return The number of sessions that were above the packet loss threshold for current day.
     */
    uint32_t get_packet_loss_exceeding_threshold_today() const;
    /**
     * Getter function for number of sessions that were above the packet loss threshold for the previous day.
     * @return The number of sessions that were above the packet loss threshold for previous day.
     */
    uint32_t get_packet_loss_exceeding_threshold_yesterday() const;
    /**
     * Getter function for the average packet loss for the current day.
     * @return The the average packet loss for the current day.
     */
    float    get_average_packet_loss_today() const;
    /**
     * Getter function for the average packet loss for the previous day.
     * @return The the average packet loss for the previous day.
     */
    float    get_average_packet_loss_yesterday() const;

    void     print(const char *prefix = __FUNCTION__, bool single_line = false) const;

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the voice command count info
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

public:
    /**
     * Interface implementation to read config values
     * @see ctrlm_config_attr_t::read_config
     */
    virtual bool read_config();
    /**
     * Function to handle RFC parameter updates
     */
    void rfc_retrieved_handler(const ctrlm_rfc_attr_t &attr);

public:
    /**
     * Interface implementation to read the data from DB
     * @see ctrlm_db_attr_t::read_db
     */
    virtual bool read_db(ctrlm_db_ctx_t ctx);
    /**
     * Interface implementation to write the data to DB
     * @see ctrlm_db_attr_t::write_db
     */
    virtual bool write_db(ctrlm_db_ctx_t ctx);

private:
    uint32_t voice_cmd_count_today;
    uint32_t voice_cmd_count_yesterday;
    uint32_t voice_cmd_short_today;
    uint32_t voice_cmd_short_yesterday;
    uint32_t today;
    uint32_t voice_packets_sent_today;
    uint32_t voice_packets_sent_yesterday;
    uint32_t voice_packets_lost_today;
    uint32_t voice_packets_lost_yesterday;
    uint32_t utterances_exceeding_packet_loss_threshold_today;
    uint32_t utterances_exceeding_packet_loss_threshold_yesterday;

    int      packet_loss_threshold;
};
#endif
