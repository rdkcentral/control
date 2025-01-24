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

// Includes

#include <string>
#include <string.h>
#include "ctrlm.h"
#include "ctrlm_utils.h"
#include "ctrlm_log.h"
#include "ctrlm_database.h"
#include "ctrlm_ble_network.h"
#include "ctrlm_ble_controller.h"
#include "ctrlm_ble_utils.h"
#include "ctrlm_controller.h"
#include "ctrlm_hal_ip.h"
#include "blercu/bleservices/blercuupgradeservice.h"

#include <sstream>
#include <iterator>
#include <iostream>
#include <algorithm>

using namespace std;

// End Includes

// Function Implementations

ctrlm_obj_controller_ble_t::ctrlm_obj_controller_ble_t(ctrlm_controller_id_t controller_id, ctrlm_obj_network_ble_t &network, unsigned long long ieee_address, ctrlm_ble_result_validation_t validation_result) : 
   ctrlm_obj_controller_t(controller_id, network, ieee_address),
   obj_network_ble_(&network),
   product_name_(std::make_shared<ctrlm_string_db_attr_t>("Product Name", "UNKNOWN", &network, controller_id, "controller_name")),
   serial_number_(std::make_shared<ctrlm_string_db_attr_t>("Serial Number", "UNKNOWN", &network, controller_id, "serial_number")),
   manufacturer_(std::make_shared<ctrlm_string_db_attr_t>("Manufacturer", "UNKNOWN", &network, controller_id, "controller_manufacturer")),
   model_(std::make_shared<ctrlm_string_db_attr_t>("Model", "UNKNOWN", &network, controller_id, "controller_model")),
   fw_revision_(std::make_shared<ctrlm_string_db_attr_t>("Firmware Revision", "UNKNOWN", &network, controller_id, "fw_revision")),
   sw_revision_(std::make_shared<ctrlm_ble_sw_version_t>(&network, controller_id)),
   hw_revision_(std::make_shared<ctrlm_ble_hw_version_t>(&network, controller_id)),
   battery_percent_(std::make_shared<ctrlm_uint64_db_attr_t>("Battery Percentage", 0xFF, &network, controller_id, "battery_percent")),
   validation_result_(validation_result),
   wakeup_custom_list_(),
   irdbs_supported_()
{
   XLOGD_INFO("constructor - controller id <%u>", controller_id);

   voice_metrics_->read_config();
   ieee_address_->set_num_bytes(6);
   last_key_time_->set_key("last_key_time");    //DB key needs to be updated for backwards compatibility
}

ctrlm_obj_controller_ble_t::ctrlm_obj_controller_ble_t() {
   XLOGD_INFO("default constructor");
}

void ctrlm_obj_controller_ble_t::db_create() {
   XLOGD_INFO("Creating database for controller %u", controller_id_get());
   ctrlm_db_ble_controller_create(network_id_get(), controller_id_get());
   stored_in_db_ = true;
}

void ctrlm_obj_controller_ble_t::db_destroy() {
   XLOGD_INFO("Destroying database for controller %u", controller_id_get());
   ctrlm_db_ble_controller_destroy(network_id_get(), controller_id_get());
   stored_in_db_ = false;
}

void ctrlm_obj_controller_ble_t::db_load() {
   ctrlm_obj_controller_t::db_load();

   ctrlm_db_attr_read(product_name_.get());
   setTypeFromProductName();
   ctrlm_db_attr_read(manufacturer_.get());
   ctrlm_db_attr_read(model_.get());
   ctrlm_db_attr_read(fw_revision_.get());
   ctrlm_db_attr_read(serial_number_.get());
   ctrlm_db_attr_read(sw_revision_.get());
   ctrlm_db_attr_read(hw_revision_.get());
   ctrlm_db_attr_read(battery_percent_.get());

   stored_in_db_ = true;
}

void ctrlm_obj_controller_ble_t::db_store() {
   ctrlm_obj_controller_t::db_store();

   ctrlm_db_attr_write(product_name_);
   ctrlm_db_attr_write(manufacturer_);
   ctrlm_db_attr_write(model_);
   ctrlm_db_attr_write(fw_revision_);
   ctrlm_db_attr_write(serial_number_);
   ctrlm_db_attr_write(sw_revision_);
   ctrlm_db_attr_write(hw_revision_);
   ctrlm_db_attr_write(battery_percent_);
}

void ctrlm_obj_controller_ble_t::validation_result_set(ctrlm_ble_result_validation_t validation_result) {
   if(CTRLM_BLE_RESULT_VALIDATION_SUCCESS == validation_result) {
      validation_result_ = validation_result;
      time_binding_->set_value((uint64_t)time(NULL));
      last_key_time_->set_value((uint64_t)time(NULL));
      db_store();
      return;
   }
   validation_result_ = validation_result;
}

ctrlm_ble_result_validation_t ctrlm_obj_controller_ble_t::validation_result_get() const {
   return validation_result_;
}

std::string ctrlm_obj_controller_ble_t::controller_type_str_get(void) {
   return(controller_type_str_);
}
void ctrlm_obj_controller_ble_t::setTypeFromProductName() {
   string name = product_name_->to_string();

   // Locate the config entry that matches the product name
   std::shared_ptr<ConfigSettings> config = obj_network_ble_->getConfigSettings();
   ConfigModelSettings settings = config->modelSettings(name);

   if(!settings.isValid()) {
      XLOGD_WARN("unhandled product name <%s>", product_name_->to_string().c_str());
      return;
   }

   controller_type_str_ = "BLE_CONTROLLER_";
   controller_type_str_.append(settings.name());
   
   ota_product_name_    = settings.otaProductName();

   XLOGD_DEBUG("type <%s> ota product name <%s>", controller_type_str_.c_str(), ota_product_name_.c_str());

   if(settings.typeZ()) {
      type_z_supported_                       = true;
   }

   std::string version = settings.connParamUpdateBeforeOtaVersion();
   if(!version.empty()) {
      conn_param_update_before_ota_supported_ = true;
      conn_param_update_before_ota_version_   = version;
   }

   version = settings.upgradePauseVersion();
   if(!version.empty()) {
      upgrade_pause_supported_                = true;
      upgrade_pause_version_                  = version;
   }

   version = settings.upgradeStuckVersion();
   if(!version.empty()) {
      upgrade_stuck_supported_                = true;
      upgrade_stuck_version_                  = version;
   }

   uint16_t keyCode = voice_key_code_;
   if(settings.voiceKeyCode(keyCode)) {
      voice_key_code_ = keyCode;
   }
}

bool ctrlm_obj_controller_ble_t::getOTAProductName(string &name) {
   if(ota_product_name_.empty()) {
      XLOGD_ERROR("controller of type %s not mapped to OTA product name", product_name_->to_string().c_str());
      return false;
   }
   name = ota_product_name_;
   if (is_controller_type_z()) {
      name.append("Z");
   }
   return true;
}

void ctrlm_obj_controller_ble_t::setConnected(bool connected) {
   XLOGD_DEBUG("Controller %u set connected to <%d>", controller_id_get(), connected);
   connected_ = connected;

   if (connected_ == true) {
      last_activity_time_->set_value((uint64_t)time(NULL));
      ctrlm_db_attr_write(last_activity_time_);
   }
}
bool ctrlm_obj_controller_ble_t::get_connected() const {
   return connected_;
}

void ctrlm_obj_controller_ble_t::setSerialNumber(const std::string &sn, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set serial number to %s", controller_id_get(), sn.c_str());
   serial_number_->set_value(sn);
   if (write_to_db) {
      ctrlm_db_attr_write(serial_number_);
   }
}
string ctrlm_obj_controller_ble_t::get_serial_number() const {
   return serial_number_->to_string();
}

void ctrlm_obj_controller_ble_t::setName(const std::string &controller_name, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set name to %s", controller_id_get(), controller_name.c_str());
   product_name_->set_value(controller_name);
   setTypeFromProductName();
   if (write_to_db) {
      ctrlm_db_attr_write(product_name_);
   }
}

string ctrlm_obj_controller_ble_t::get_name() const {
   return product_name_->to_string();
}

void ctrlm_obj_controller_ble_t::setManufacturer(const std::string &controller_manufacturer, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set manufacturer to %s", controller_id_get(), controller_manufacturer.c_str());
   manufacturer_->set_value(controller_manufacturer);
   if (write_to_db) {
      ctrlm_db_attr_write(manufacturer_);
   }
}
string ctrlm_obj_controller_ble_t::get_manufacturer() const {
   return manufacturer_->to_string();
}

void ctrlm_obj_controller_ble_t::setModel(const std::string &controller_model, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set model to %s", controller_id_get(), controller_model.c_str());
   model_->set_value(controller_model);
   if (write_to_db) {
      ctrlm_db_attr_write(model_);
   }
}
string ctrlm_obj_controller_ble_t::get_model() const {
   return model_->to_string();
}

void ctrlm_obj_controller_ble_t::setFwRevision(const std::string &rev, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set FW Revision to %s", controller_id_get(), rev.c_str());
   fw_revision_->set_value(rev);
   if (write_to_db) {
      ctrlm_db_attr_write(fw_revision_);
   }
}
string ctrlm_obj_controller_ble_t::get_fw_revision() const {
   return fw_revision_->to_string();
}

void ctrlm_obj_controller_ble_t::setHwRevision(const std::string &rev, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set HW Revision to %s", controller_id_get(), rev.c_str());
   hw_revision_->from_string(rev);
   if (write_to_db) {
      ctrlm_db_attr_write(hw_revision_);
   }
}
ctrlm_hw_version_t ctrlm_obj_controller_ble_t::get_hw_revision() const {
   return *hw_revision_;
}

void ctrlm_obj_controller_ble_t::setSwRevision(const std::string &rev, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set SW Revision to %s", controller_id_get(), rev.c_str());
   sw_revision_->from_string(rev);
   if (write_to_db) {
      ctrlm_db_attr_write(sw_revision_);
   }
}
ctrlm_sw_version_t ctrlm_obj_controller_ble_t::get_sw_revision() const {
   return *sw_revision_;
}

void ctrlm_obj_controller_ble_t::setBatteryPercent(uint8_t percent, bool write_to_db) {
   XLOGD_DEBUG("Controller %u set battery level percentage to %u%%", controller_id_get(), percent);
   battery_percent_->set_value(percent);
   if (write_to_db) {
      ctrlm_db_attr_write(battery_percent_);
   }
}
uint8_t ctrlm_obj_controller_ble_t::get_battery_percent() const {
   return (uint8_t)(battery_percent_->get_value() & 0xFF);
}

bool ctrlm_obj_controller_ble_t::swUpgradeRequired(ctrlm_sw_version_t newVersion, bool force) {
   if (ota_product_name_.empty()) {
      return false;
   } else {
      bool required = false;
      if (*sw_revision_ == newVersion) {
         XLOGD_WARN("Controller <%s> already has requested software installed", ieee_address_->to_string().c_str());
         ota_clear_all_failure_counters();
      } else if (upgrade_attempted_) {
         if (type_z_supported_) {
            // A type Z OTA failure is recognized by the OTA downloading 100% and completing successfully,
            // but the remote never reboots and updates its software revision field to the new firmware rev.
            // So if we've already attempted an upgrade during this upgrade procedure window but the revision is
            // still not updated then its a type Z failure.
            ota_failure_type_z_cnt_set(ota_failure_type_z_cnt_get() + 1);
         }
      } else if (force) {
         // force flag is true and versions aren't equal, need upgrade
         required = true;
      } else if (*sw_revision_ < newVersion) {
         // Current rev is older and we haven't attempted an upgrade on this controller this session, so need upgrade
         required = true;
      }
      XLOGD_DEBUG("Controller %u: current rev = <%s>, new rev = <%s>, force = <%s>, upgrade_attempted_ = <%s>, required = <%s>",
            controller_id_get(), sw_revision_->to_string().c_str(), newVersion.to_string().c_str(), force ? "TRUE" : "FALSE", upgrade_attempted_ ? "TRUE" : "FALSE", required ? "TRUE" : "FALSE");
      return required;
   }
}

void ctrlm_obj_controller_ble_t::setUpgradeAttempted(bool upgrade_attempted) {
   XLOGD_DEBUG("Controller %u set Upgrade Attempted to %d", controller_id_get(), upgrade_attempted);
   upgrade_attempted_ = upgrade_attempted;
}

bool ctrlm_obj_controller_ble_t::getUpgradeAttempted(void) {
   return upgrade_attempted_;
}

void ctrlm_obj_controller_ble_t::setUpgradeInProgress(bool upgrading) {
   upgrade_in_progress_ = upgrading;
}

bool ctrlm_obj_controller_ble_t::getUpgradeInProgress(void) {
   return upgrade_in_progress_;
}

bool ctrlm_obj_controller_ble_t::needsBLEConnParamUpdateBeforeOTA(ctrlm_hal_ble_connection_params_t &connParams) {
   if ( conn_param_update_before_ota_supported_ && (*sw_revision_ < conn_param_update_before_ota_version_) ) {
      connParams.minInterval = 7.5;
      connParams.maxInterval = 7.5;
      connParams.latency = 0;
      connParams.supvTimeout = 15000;
      XLOGD_WARN("Controller <%s> is running a software version that requires a BLE connection parameter update before an OTA.", ieee_address_->to_string().c_str());
      return true;
   }
   return false;
}

bool ctrlm_obj_controller_ble_t::getUpgradePauseSupported(void) {
   if ( upgrade_pause_supported_ && (*sw_revision_ < upgrade_pause_version_) ) {
      return false;
   }
   return true;
}

void ctrlm_obj_controller_ble_t::setUpgradePaused(bool paused) {
   upgrade_paused_ = paused;
}

bool ctrlm_obj_controller_ble_t::getUpgradePaused() {
   return upgrade_paused_;
}

void ctrlm_obj_controller_ble_t::setUpgradeError(std::string error_str) {
   if ( upgrade_stuck_supported_ && (*sw_revision_ < upgrade_stuck_version_) ) {
      if (error_str.find(BLE_RCU_UPGRADE_SERVICE_ERR_TIMEOUT_STR) != std::string::npos) {
         XLOGD_WARN("Controller %u received timeout error from OTA attempt, it may be in stuck state.", controller_id_get());
         upgrade_stuck_ = true;
      }
   }
   if (error_str.compare("Invalid hash error from RCU") == 0) {
      ota_failure_type_z_cnt_set(ota_failure_type_z_cnt_get() + 1);
   }
}

bool ctrlm_obj_controller_ble_t::getUpgradeStuck() {
   return upgrade_stuck_;
}

void ctrlm_obj_controller_ble_t::ota_failure_cnt_incr() {
   ctrlm_obj_controller_t::ota_failure_cnt_incr();
   ctrlm_db_attr_write(ota_failure_cnt_from_last_success_);
   XLOGD_DEBUG("Controller %s <%s> OTA failure count since last successful upgrade = %llu", controller_type_str_get().c_str(), ieee_address_->to_string().c_str(), ota_failure_cnt_from_last_success_->get_value());
}

void ctrlm_obj_controller_ble_t::ota_clear_all_failure_counters() {
   ctrlm_obj_controller_t::ota_clear_all_failure_counters();
   upgrade_stuck_ = false;
   ctrlm_db_attr_write(ota_failure_cnt_from_last_success_);
   XLOGD_DEBUG("Controller %s <%s> OTA failure count since last successful upgrade reset to 0.", controller_type_str_get().c_str(), ieee_address_->to_string().c_str());
}

void ctrlm_obj_controller_ble_t::ota_failure_cnt_session_clear() {
   ctrlm_obj_controller_t::ota_failure_cnt_session_clear();
   upgrade_stuck_ = false;
}

void ctrlm_obj_controller_ble_t::ota_failure_type_z_cnt_set(uint8_t ota_failures) {
   if (ota_failure_type_z_cnt_get() < ota_failures) {
      XLOGD_WARN("Controller %s <%s> type Z OTA failure suspected, incrementing counter.", controller_type_str_get().c_str(), ieee_address_->to_string().c_str());
   }
   bool is_type_z_before = is_controller_type_z();
   ctrlm_obj_controller_t::ota_failure_type_z_cnt_set(ota_failures);
   bool is_type_z_after = is_controller_type_z();
   if (is_type_z_before != is_type_z_after) {
      XLOGD_TELEMETRY("Controller %s <%s> switched from %s to %s", controller_type_str_get().c_str(), ieee_address_->to_string().c_str(), is_type_z_before ? "TYPE Z" : "NOT TYPE Z", is_type_z_after ? "TYPE Z" : "NOT TYPE Z");
   }
   ctrlm_db_ble_write_ota_failure_type_z_count(network_id_get(), controller_id_get(), ota_failure_type_z_cnt_get());
   XLOGD_INFO("Controller %s <%s> OTA type Z failure count updated to %d.... is %s", controller_type_str_get().c_str(), ieee_address_->to_string().c_str(), ota_failure_type_z_cnt_get(), is_controller_type_z() ? "TYPE Z" : "NOT TYPE Z");
}

uint8_t ctrlm_obj_controller_ble_t::ota_failure_type_z_cnt_get(void) const {
   return ctrlm_obj_controller_t::ota_failure_type_z_cnt_get();
}

bool ctrlm_obj_controller_ble_t::is_controller_type_z(void) const {
   return ctrlm_obj_controller_t::is_controller_type_z();
}

void ctrlm_obj_controller_ble_t::setIrCode(int ircode) {
   XLOGD_DEBUG("Controller %u set IR 5 digit code to %d", controller_id_get(), ircode);
   ir_code_ = ircode;
}

void ctrlm_obj_controller_ble_t::setAudioStreaming(bool streaming) {
   XLOGD_DEBUG("Controller %u set Audio Streaming to %s", controller_id_get(), streaming ? "TRUE" : "FALSE");
   audio_streaming_ = streaming;
}

void ctrlm_obj_controller_ble_t::setAudioGainLevel(guint8 gain) {
   XLOGD_DEBUG("Controller %u set Audio Gain level to %u", controller_id_get(), gain);
   audio_gain_level_ = gain;
}

void ctrlm_obj_controller_ble_t::setAudioCodecs(guint32 value) {
   XLOGD_DEBUG("Controller %u set Audio Codecs to 0x%X", controller_id_get(), value);
   audio_codecs_ = value;
}

void ctrlm_obj_controller_ble_t::setTouchMode(unsigned int param) {
   XLOGD_DEBUG("Controller %u set Touch Mode to %u", controller_id_get(), param);
   touch_mode_ = param;
}
void ctrlm_obj_controller_ble_t::setTouchModeSettable(bool param) {
   XLOGD_DEBUG("Controller %u set Touch Mode Settable to %s", controller_id_get(), param ? "TRUE" : "FALSE");
   touch_mode_settable_ = param;
}

void ctrlm_obj_controller_ble_t::setLastWakeupKey(guint16 code) {
   last_wakeup_key_code_ = code;
}
guint16 ctrlm_obj_controller_ble_t::get_last_wakeup_key() const {
   return last_wakeup_key_code_;
}

void ctrlm_obj_controller_ble_t::setWakeupConfig(uint8_t config) {
   if (config > CTRLM_RCU_WAKEUP_CONFIG_INVALID) {
      wakeup_config_ = CTRLM_RCU_WAKEUP_CONFIG_INVALID;
   } else {
      wakeup_config_ = (ctrlm_rcu_wakeup_config_t)config;
   }
}
ctrlm_rcu_wakeup_config_t ctrlm_obj_controller_ble_t::get_wakeup_config() const {
   return wakeup_config_;
}

void ctrlm_obj_controller_ble_t::setWakeupCustomList(int *list, int size) {
   if (NULL == list) {
      XLOGD_ERROR("list is NULL");
      return;
   }
   wakeup_custom_list_.clear();
   for (int i = 0; i < size; i++) {
      wakeup_custom_list_.push_back(list[i]);
   }
}
vector<uint16_t> ctrlm_obj_controller_ble_t::get_wakeup_custom_list() const {
   return wakeup_custom_list_;
}

std::string ctrlm_obj_controller_ble_t::wakeupCustomListToString() {
  std::ostringstream oss;
  if (!wakeup_custom_list_.empty()) {
    std::copy(wakeup_custom_list_.begin(), wakeup_custom_list_.end()-1, std::ostream_iterator<int>(oss, ","));
    // add the last element now to avoid trailing comma
    oss << wakeup_custom_list_.back();
  }
  return oss.str();
}

bool ctrlm_obj_controller_ble_t::is_stale(time_t stale_time_threshold) const {
   time_t last_activity_time = this->last_activity_time_get();
   if(last_activity_time > 0 && last_activity_time < stale_time_threshold) {
      XLOGD_DEBUG("Controller <%s> has not had activity since %s, which is earlier than the stale threshold %s", ieee_address_get().to_string().c_str(), ctrlm_utils_time_as_string(last_activity_time).c_str(), ctrlm_utils_time_as_string(stale_time_threshold).c_str());
      return true;
   }
   return false;
}

bool ctrlm_obj_controller_ble_t::isVoiceKey(uint16_t key_code) const {
   if(key_code == voice_key_code_) {
      return true;
   }
   return false;
}

void ctrlm_obj_controller_ble_t::setPressAndHoldSupport(bool supported) {
   press_and_hold_supported_ = supported;
}

bool ctrlm_obj_controller_ble_t::getPressAndHoldSupport() const {
   return(press_and_hold_supported_);
}

void ctrlm_obj_controller_ble_t::setVoiceStartTime(ctrlm_timestamp_t startTimeKey) {
   voice_start_time_key_ = startTimeKey;
   ctrlm_timestamp_get_monotonic(&voice_start_time_local_);
}

ctrlm_timestamp_t ctrlm_obj_controller_ble_t::getVoiceStartTimeKey() const {
   return(voice_start_time_key_);
}

ctrlm_timestamp_t ctrlm_obj_controller_ble_t::getVoiceStartTimeLocal() const {
   return(voice_start_time_local_);
}

void ctrlm_obj_controller_ble_t::setSupportedIrdbs(ctrlm_irdb_vendor_t* vendors, int num_supported) {
   if (vendors == NULL) {
      XLOGD_ERROR("vendors is NULL");
      return;
   }
   if (num_supported == 0)
      return;

   this->irdbs_supported_.clear();
   for (int i = 0; i < num_supported; i++) {
      this->irdbs_supported_.push_back(vendors[i]);
   }
   XLOGD_INFO("Controller <%s> IRDBs supported = <%s>",
         ieee_address_get().to_string().c_str(), ctrlm_ble_irdbs_supported_str(getSupportedIrdbs()).c_str());
}

std::vector<ctrlm_irdb_vendor_t> ctrlm_obj_controller_ble_t::getSupportedIrdbs() const {
   return this->irdbs_supported_;
}

bool ctrlm_obj_controller_ble_t::isSupportedIrdb(ctrlm_irdb_vendor_t vendor) {
   if (vendor == CTRLM_IRDB_VENDOR_INVALID) {
      XLOGD_ERROR("%s (%u) is not allowed", ctrlm_irdb_vendor_str(vendor), vendor);
      return false;
   }
   if (irdbs_supported_.empty()) {
      XLOGD_WARN("Controller %u likely does not support this feature yet - continuing...", controller_id_get());
      return true;
   }
   if (std::find(irdbs_supported_.begin(), irdbs_supported_.end(), vendor) != irdbs_supported_.end()) {
      XLOGD_INFO("Controller %u supports IRDB %s (%u)", controller_id_get(), ctrlm_irdb_vendor_str(vendor), vendor);
      return true;
   }
   XLOGD_WARN("Controller %u does not support IRDB %s (%u)", controller_id_get(), ctrlm_irdb_vendor_str(vendor), vendor);
   return false;
}

void ctrlm_obj_controller_ble_t::getStatus(ctrlm_controller_status_t *status) {
   errno_t safec_rc = -1;
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return;
   }

   //If the day has changed, store the values related to today and yesterday
   voice_metrics_->process_time(true);

   status->ieee_address                                         = ieee_address_->get_value();
   status->time_binding                                         = time_binding_->get_value();
   status->time_last_key                                        = this->last_key_time_get();
   status->last_key_status                                      = last_key_status_;
   status->last_key_code                                        = (ctrlm_key_code_t)this->last_key_code_get();
   status->voice_cmd_count_today                                = voice_metrics_->get_commands_today();
   status->voice_cmd_count_yesterday                            = voice_metrics_->get_commands_yesterday();
   status->voice_cmd_short_today                                = voice_metrics_->get_short_commands_today();
   status->voice_cmd_short_yesterday                            = voice_metrics_->get_short_commands_yesterday();
   status->voice_packets_sent_today                             = voice_metrics_->get_packets_sent_today();
   status->voice_packets_sent_yesterday                         = voice_metrics_->get_packets_sent_yesterday();
   status->voice_packets_lost_today                             = voice_metrics_->get_packets_lost_today();
   status->voice_packets_lost_yesterday                         = voice_metrics_->get_packets_lost_yesterday();
   status->voice_packet_loss_average_today                      = voice_metrics_->get_average_packet_loss_today();
   status->voice_packet_loss_average_yesterday                  = voice_metrics_->get_average_packet_loss_yesterday();
   status->utterances_exceeding_packet_loss_threshold_today     = voice_metrics_->get_packet_loss_exceeding_threshold_today();
   status->utterances_exceeding_packet_loss_threshold_yesterday = voice_metrics_->get_packet_loss_exceeding_threshold_yesterday();

   safec_rc = strncpy_s(status->manufacturer, sizeof(status->manufacturer), manufacturer_->to_string().c_str(),CTRLM_RCU_MAX_MANUFACTURER_LENGTH-1);
   ERR_CHK(safec_rc);
   status->manufacturer[CTRLM_RCU_MAX_MANUFACTURER_LENGTH - 1] = '\0';
   safec_rc = strncpy_s(status->type, sizeof(status->type), product_name_->to_string().c_str(), CTRLM_RCU_MAX_USER_STRING_LENGTH - 1);
   ERR_CHK(safec_rc);
   status->type[CTRLM_RCU_MAX_USER_STRING_LENGTH - 1] = '\0';

   status->battery_level_percent = get_battery_percent();

   // ctrlm_print_controller_status(__FUNCTION__, status);
}

void ctrlm_obj_controller_ble_t::print_status() {
   string ota_product_name;
   getOTAProductName(ota_product_name);

   XLOGD_WARN("------------------------------------------------------------");
   XLOGD_INFO("Controller ID                : %u", controller_id_get());
   XLOGD_INFO("Friendly Name                : %s", product_name_->to_string().c_str());
   XLOGD_INFO("OTA Product Name             : %s", ota_product_name.c_str());
   XLOGD_INFO("Manufacturer                 : %s", manufacturer_->to_string().c_str());
   XLOGD_INFO("Model                        : %s", model_->to_string().c_str());
   XLOGD_INFO("MAC Address                  : %s", ieee_address_->to_string().c_str());
   XLOGD_INFO("Device Minor ID              : %d", device_minor_id_);
   XLOGD_INFO("Battery Level                : %u%%", get_battery_percent());
   XLOGD_INFO("HW Revision                  : %s", hw_revision_->to_string().c_str());
   XLOGD_INFO("FW Revision                  : %s", fw_revision_->to_string().c_str());
   XLOGD_INFO("SW Revision                  : %s", sw_revision_->to_string().c_str());
   XLOGD_INFO("Serial Number                : %s", serial_number_->to_string().c_str());
   XLOGD_INFO("");
   XLOGD_INFO("Connected                    : %s", (connected_==true) ? "true" : "false");
   XLOGD_INFO("Last Activity Time           : %s", ctrlm_utils_time_as_string(this->last_activity_time_get()).c_str());
   XLOGD_INFO("Bound Time                   : %s", ctrlm_utils_time_as_string(this->time_binding_get()).c_str());
   XLOGD_INFO("");
   XLOGD_INFO("Last Key Code                : %u (%s key)", this->last_key_code_get(), ctrlm_linux_key_code_str(this->last_key_code_get(), false));
   XLOGD_INFO("Last Key Time                : %s", ctrlm_utils_time_as_string(this->last_key_time_get()).c_str());
   XLOGD_INFO("");
   XLOGD_INFO("Last Wakeup Key              : %u (%s key)", last_wakeup_key_code_, ctrlm_linux_key_code_str(last_wakeup_key_code_, false));
   XLOGD_INFO("Wakeup Config                : %s", ctrlm_rcu_wakeup_config_str(wakeup_config_));
   if (wakeup_config_ == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
   XLOGD_INFO("Wakeup Config Custom List    : %s", wakeupCustomListToString().c_str());
   }
   XLOGD_INFO("");
   XLOGD_INFO("IR Database Supported        : %s", ctrlm_ble_irdbs_supported_str(irdbs_supported_).c_str());
   XLOGD_INFO("Programmed TV IRDB Code      : %s", irdb_entry_id_name_tv_->to_string().c_str());
   XLOGD_INFO("Programmed AVR IRDB Code     : %s", irdb_entry_id_name_avr_->to_string().c_str());
   XLOGD_INFO("");
   voice_metrics_->print(__FUNCTION__);
   XLOGD_WARN("------------------------------------------------------------");
}

// End Function Implementations
