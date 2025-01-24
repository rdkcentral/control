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
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_network.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"

using namespace std;

#define OTA_MAX_RETRIES (2)

ctrlm_obj_controller_t::ctrlm_obj_controller_t(ctrlm_controller_id_t controller_id, ctrlm_obj_network_t &network, unsigned long long ieee_address) :
   controller_id_(controller_id),
   obj_network_(&network),
   ieee_address_(std::make_shared<ctrlm_ieee_db_addr_t>(ieee_address, &network, controller_id)),
   time_binding_(std::make_shared<ctrlm_uint64_db_attr_t>("Binding Time", 0, &network, controller_id, "time_binding")),
   last_activity_time_(std::make_shared<ctrlm_uint64_db_attr_t>("Last Activity Time", 0, &network, controller_id, "last_activity_time")),
   last_key_time_(std::make_shared<ctrlm_uint64_db_attr_t>("Last Keypress Time", 0, &network, controller_id, "time_last_key")),
   last_key_code_(std::make_shared<ctrlm_uint64_db_attr_t>("Last Keypress Code", CTRLM_KEY_CODE_INVALID, &network, controller_id, "last_key_code")),
   irdb_entry_id_name_tv_(std::make_shared<ctrlm_string_db_attr_t>("TV IRDB Code", "0", &network, controller_id, "irdb_entry_id_name_tv")),
   irdb_entry_id_name_avr_(std::make_shared<ctrlm_string_db_attr_t>("AVR IRDB Code", "0", &network, controller_id, "irdb_entry_id_name_avr")),
   voice_metrics_(std::make_shared<ctrlm_voice_metrics_t>(&network, controller_id)),
   ota_failure_cnt_from_last_success_(std::make_shared<ctrlm_uint64_db_attr_t>("OTA Failure Count From Last Success", 0, &network, controller_id, "ota_failure_cnt_last_success"))
{
   XLOGD_INFO("constructor - %u", controller_id_);
}

ctrlm_obj_controller_t::ctrlm_obj_controller_t() {
   XLOGD_INFO("constructor - default");
}

ctrlm_obj_controller_t::~ctrlm_obj_controller_t() {
   XLOGD_INFO("deconstructor - %u", controller_id_);
}

void ctrlm_obj_controller_t::db_load() {
   ctrlm_db_attr_read(ieee_address_.get());
   ctrlm_db_attr_read(time_binding_.get());

   ctrlm_db_attr_read(last_activity_time_.get());
   if (last_activity_time_->get_value() == 0) {
      XLOGD_INFO("Controller %s <%s> last activity time is empty, initializing with the current time.", controller_type_str_get().c_str(), ieee_address_->to_string().c_str());
      last_activity_time_->set_value((uint64_t)time(NULL));
      ctrlm_db_attr_write(last_activity_time_);
   }
   
   ctrlm_db_attr_read(last_key_time_.get());
   ctrlm_db_attr_read(last_key_code_.get());
   ctrlm_db_attr_read(irdb_entry_id_name_tv_.get());
   ctrlm_db_attr_read(irdb_entry_id_name_avr_.get());
   ctrlm_db_attr_read(voice_metrics_.get());

   ctrlm_db_attr_read(ota_failure_cnt_from_last_success_.get());
   ctrlm_db_ble_read_ota_failure_type_z_count(network_id_get(), controller_id_get(), ota_failure_type_z_cnt_);
   XLOGD_INFO("Controller %s <%s> OTA total failures since last successful upgrade = %llu", controller_type_str_get().c_str(), ieee_address_->to_string().c_str(), ota_failure_cnt_from_last_success_->get_value());
   XLOGD_INFO("Controller %s <%s> OTA type Z failure count = %d.... is %s", controller_type_str_get().c_str(), ieee_address_->to_string().c_str(), ota_failure_type_z_cnt_get(), is_controller_type_z() ? "TYPE Z" : "NOT TYPE Z");
}


void ctrlm_obj_controller_t::db_store() {
   ctrlm_db_attr_write(ieee_address_);
   ctrlm_db_attr_write(time_binding_);
   ctrlm_db_attr_write(last_key_time_);
   ctrlm_db_attr_write(last_key_code_);
   ctrlm_db_attr_write(irdb_entry_id_name_tv_);
   ctrlm_db_attr_write(irdb_entry_id_name_avr_);
   ctrlm_db_attr_write(voice_metrics_);
}

std::string ctrlm_obj_controller_t::controller_type_str_get(void) {
   XLOGD_WARN("not implemented.");
   return "";
}

bool ctrlm_obj_controller_t::is_stale(time_t stale_time_threshold) const {
   XLOGD_WARN("not implemented.");
   return false;
}

ctrlm_controller_id_t ctrlm_obj_controller_t::controller_id_get() const {
   return(controller_id_);
}

ctrlm_network_id_t ctrlm_obj_controller_t::network_id_get() const {
   return(obj_network_->network_id_get());
}

string ctrlm_obj_controller_t::receiver_id_get() const {
   return(obj_network_->receiver_id_get());
}

string ctrlm_obj_controller_t::device_id_get() const {
   return(obj_network_->device_id_get());
}

string ctrlm_obj_controller_t::service_account_id_get() const {
   return(obj_network_->service_account_id_get());
}

string ctrlm_obj_controller_t::partner_id_get() const {
   return(obj_network_->partner_id_get());
}

string ctrlm_obj_controller_t::experience_get() const {
   return(obj_network_->experience_get());
}

string ctrlm_obj_controller_t::stb_name_get() const {
   return(obj_network_->stb_name_get());
}

ctrlm_ieee_db_addr_t ctrlm_obj_controller_t::ieee_address_get(void) const {
   return(*ieee_address_);
}

time_t ctrlm_obj_controller_t::time_binding_get() const {
   return (time_t)(time_binding_->get_value());
}

time_t ctrlm_obj_controller_t::last_activity_time_get() const {
   return (time_t)(last_activity_time_->get_value());
}

time_t ctrlm_obj_controller_t::last_key_time_get() const {
   return (time_t)(last_key_time_->get_value());
}

void ctrlm_obj_controller_t::last_key_time_set(time_t val) {
   last_key_time_->set_value(val);
}

uint16_t ctrlm_obj_controller_t::last_key_code_get() const {
   return (uint16_t)(last_key_code_->get_value());
}

void ctrlm_obj_controller_t::last_key_code_set(uint16_t val) {
   last_key_code_->set_value(val);
}

void ctrlm_obj_controller_t::last_key_time_update() {
   uint64_t current_time = (uint64_t)time(NULL);
   last_key_time_->set_value(current_time);
   last_activity_time_->set_value(current_time);

   if(this->last_key_time_get() > last_key_time_flush_) {
      last_key_time_flush_ = this->last_key_time_get() + LAST_KEY_DATABASE_FLUSH_INTERVAL;
      ctrlm_db_attr_write(last_key_time_);
      ctrlm_db_attr_write(last_key_code_);
      ctrlm_db_attr_write(last_activity_time_);
   }
}

void ctrlm_obj_controller_t::process_event_key(ctrlm_key_status_t key_status, uint16_t key_code, bool mask) {
   last_key_status_ = key_status;
   last_key_code_->set_value((uint64_t)key_code);
   last_key_time_update();

   XLOGD_TELEMETRY("ind_process_keypress: %s - MAC Address <%s>, code = <%d> (%s key), status = <%s>", controller_type_str_get().c_str(),
                                                                                 ieee_address_get().to_string().c_str(),
                                                                                 mask ? -1 : key_code,
                                                                                 ctrlm_linux_key_code_str(key_code, mask),
                                                                                 ctrlm_key_status_str(key_status));
}

ctrlm_controller_capabilities_t ctrlm_obj_controller_t::get_capabilities() const {
   return(ctrlm_controller_capabilities_t()); // return empty capabilities object
}

void ctrlm_obj_controller_t::irdb_entry_id_name_set(ctrlm_irdb_dev_type_t type, ctrlm_irdb_ir_entry_id_t irdb_entry_id_name) {
   switch(type) {
      case CTRLM_IRDB_DEV_TYPE_TV:
         if (irdb_entry_id_name_tv_->to_string() != irdb_entry_id_name) {
            irdb_entry_id_name_tv_->set_value(irdb_entry_id_name);
            ctrlm_db_attr_write(irdb_entry_id_name_tv_);
         }
         XLOGD_INFO("TV IRDB Code <%s>", irdb_entry_id_name_tv_->to_string().c_str());
         break;
      case CTRLM_IRDB_DEV_TYPE_AVR:
         if (irdb_entry_id_name_avr_->to_string() != irdb_entry_id_name) {
            irdb_entry_id_name_avr_->set_value(irdb_entry_id_name);
            ctrlm_db_attr_write(irdb_entry_id_name_avr_);
         }
         XLOGD_INFO("AVR IRDB Code <%s>", irdb_entry_id_name_avr_->to_string().c_str());
         break;
      default:
         XLOGD_WARN("Invalid type <%d>", type);
         break;
   }
}

std::string ctrlm_obj_controller_t::get_irdb_entry_id_name_tv() const {
   return irdb_entry_id_name_tv_->to_string();
}

std::string ctrlm_obj_controller_t::get_irdb_entry_id_name_avr() const {
   return irdb_entry_id_name_avr_->to_string();
}


void ctrlm_obj_controller_t::ota_failure_cnt_incr() {
   ota_failure_cnt_session_++;
   ota_failure_cnt_from_last_success_->set_value(ota_failure_cnt_from_last_success_->get_value() + 1);
   XLOGD_DEBUG("ota_failure_cnt_session_ = %hu, ota_failure_cnt_from_last_success_ = %llu", ota_failure_cnt_session_, ota_failure_cnt_from_last_success_->get_value());
}

void ctrlm_obj_controller_t::ota_clear_all_failure_counters() {
   ota_failure_cnt_session_ = 0;
   ota_failure_cnt_from_last_success_->set_value(0);
   ota_failure_type_z_cnt_set(0);
   XLOGD_DEBUG("all OTA failure counters reset to 0");
}

void ctrlm_obj_controller_t::ota_failure_cnt_session_clear() {
   ota_failure_cnt_session_ = 0;
   XLOGD_DEBUG("ota_failure_cnt_session_ reset to 0");
}

bool ctrlm_obj_controller_t::retry_ota() const {
   bool retry = ota_failure_cnt_session_ < OTA_MAX_RETRIES;
   XLOGD_WARN("ota_failure_cnt_session_ = <%d>, retry OTA = <%s>",ota_failure_cnt_session_,retry ? "TRUE":"FALSE");
   return retry;
}

void ctrlm_obj_controller_t::ota_failure_type_z_cnt_set(uint8_t ota_failures) {
   ota_failure_type_z_cnt_  = (ota_failures >= 4) ? 0 : ota_failures;
}

uint8_t ctrlm_obj_controller_t::ota_failure_type_z_cnt_get(void) const {
   return ota_failure_type_z_cnt_;
}

bool ctrlm_obj_controller_t::is_controller_type_z(void) const {
   return (ota_failure_type_z_cnt_ >= 2) ? true : false;
}

void ctrlm_obj_controller_t::update_voice_metrics(bool is_short_utterance, uint32_t voice_packets_sent, uint32_t voice_packets_lost) {
   voice_metrics_->process_time(false);

   if(is_short_utterance) {
      voice_metrics_->increment_short_voice_count(voice_packets_sent, voice_packets_lost);
   } else {
      voice_metrics_->increment_voice_count(voice_packets_sent, voice_packets_lost);
   }
   voice_metrics_->print(__FUNCTION__, true);
}

void ctrlm_obj_controller_t::set_device_minor_id(int device_minor_id) {
    XLOGD_DEBUG("Controller %u set device ID to %d", controller_id_get(), device_minor_id);
    device_minor_id_ = device_minor_id;
}

int ctrlm_obj_controller_t::get_device_minor_id() const {
    return device_minor_id_;
}

bool ctrlm_obj_controller_t::get_connected() const {
    XLOGD_WARN("not implemented.");
    return false;
}

std::string ctrlm_obj_controller_t::get_name() const {
    XLOGD_WARN("not implemented.");
    return "";
}

std::string ctrlm_obj_controller_t::get_manufacturer() const {
    XLOGD_WARN("not implemented.");
    return "";
}

std::string ctrlm_obj_controller_t::get_model() const {
    XLOGD_WARN("not implemented.");
    return "";
}

ctrlm_sw_version_t ctrlm_obj_controller_t::get_sw_revision() const {
    XLOGD_WARN("not implemented.");
    return ctrlm_sw_version_t();
}

ctrlm_hw_version_t ctrlm_obj_controller_t::get_hw_revision() const {
    XLOGD_WARN("not implemented.");
    return ctrlm_hw_version_t();
}

std::string ctrlm_obj_controller_t::get_fw_revision() const {
    XLOGD_WARN("not implemented.");
    return "";
}
std::string ctrlm_obj_controller_t::get_serial_number() const {
    XLOGD_WARN("not implemented.");
    return "";
}

uint8_t ctrlm_obj_controller_t::get_battery_percent() const {
    XLOGD_WARN("not implemented.");
    return 0;
}

uint16_t ctrlm_obj_controller_t::get_last_wakeup_key() const {
    XLOGD_WARN("not implemented.");
    return 0;
}

ctrlm_rcu_wakeup_config_t ctrlm_obj_controller_t::get_wakeup_config() const {
    XLOGD_WARN("not implemented.");
    return CTRLM_RCU_WAKEUP_CONFIG_INVALID;
}

std::vector<uint16_t> ctrlm_obj_controller_t::get_wakeup_custom_list() const {
    XLOGD_WARN("not implemented.");
    return {};
}
