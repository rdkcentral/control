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
#include <time.h>
#include <glib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <uuid/uuid.h>
#include <sys/sysmacros.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "ctrlm_rf4ce_network.h"
#include "ctrlm_device_update.h"
#include "ctrlm_database.h"
#include "ctrlm_validation.h"
#ifdef ASB
#include "ctrlm_asb.h"
#endif
#include "irMgr.h"

using namespace std;

//#define USE_LOCAL_IR_RF_DB_CODES
#ifdef USE_LOCAL_IR_RF_DB_CODES

static char samsung_0x6B[] = {0x88, 0x00, 0x29, 0x04, 0x13, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x33, 0x23, 0x33, 0x33, 0x32, 0x32, 0x22, 0x22, 0x20};
static char samsung_0x6C[] = {0x8C, 0x21, 0x4C, 0x02, 0x31, 0x01, 0x4F, 0x29, 0x04, 0x13, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x32, 0x33, 0x22, 0x33, 0x23, 0x22, 0x33, 0x22, 0x30};
static char samsung_0x6D[] = {0x8C, 0x21, 0x4C, 0x02, 0x31, 0x02, 0x4F, 0x29, 0x04, 0x13, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x33, 0x33, 0x22, 0x33, 0x22, 0x22, 0x33, 0x22, 0x30};
static char samsung_0x41[] = {0x8C, 0x01, 0x4C, 0x02, 0x31, 0x00, 0x00, 0x29, 0x04, 0x11, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x33, 0x33, 0x22, 0x22, 0x20};
static char samsung_0x42[] = {0x8C, 0x01, 0x4C, 0x02, 0x31, 0x00, 0x00, 0x29, 0x04, 0x11, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x32, 0x23, 0x23, 0x33, 0x33, 0x32, 0x32, 0x22, 0x20};
static char samsung_0x43[] = {0x8C, 0x01, 0x4C, 0x02, 0x31, 0x00, 0x00, 0x29, 0x04, 0x11, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x32, 0x22, 0x23, 0x33, 0x33, 0x33, 0x32, 0x22, 0x20};
static char samsung_0x34[] = {0x88, 0x00, 0x29, 0x04, 0x11, 0x04, 0x00, 0x22, 0x00, 0xD2, 0x00, 0x90, 0x00, 0x04, 0x2D, 0x65, 0x04, 0x65, 0x04, 0x90, 0x00, 0xA6, 0x01, 0x90, 0x00, 0x8D, 0x00, 0x12, 0x22, 0x33, 0x33, 0x32, 0x22, 0x33, 0x33, 0x32, 0x33, 0x33, 0x33, 0x33, 0x22, 0x22, 0x22, 0x20};

#endif

#define NUM_XR11V2_HARDWARE_VERSIONS       1
#define NUM_XR15V1_HARDWARE_VERSIONS       1
#define NUM_XR15V2_HARDWARE_VERSIONS       2
#define NUM_XR11V2_HARDWARE_FIX_INDEX      0
#define NUM_XR15V1_HARDWARE_FIX_INDEX      0
#define NUM_XR15V2_HARDWARE_FIX_INDEX      0
#define XR11V2_IEEE_PREFIX                 0x00124B0000000000
#define XR15V1_IEEE_PREFIX                 0x00155F0000000000
#define XR15V2_IEEE_PREFIX                 0x00155F0000000000
#define XR15V2_IEEE_PREFIX_UP_TO_FEB_2018  0x48D0CF0000000000
#define XR15V2_IEEE_PREFIX_THIRD           0x00CC3F0000000000


const guchar xr11v2_hardware_versions[NUM_XR11V2_HARDWARE_VERSIONS][CTRLM_RF4CE_RIB_ATTR_LEN_VERSIONING] = { {0x22, 0x01, 0x00, 0x00}                           };
const guchar xr15v1_hardware_versions[NUM_XR15V1_HARDWARE_VERSIONS][CTRLM_RF4CE_RIB_ATTR_LEN_VERSIONING] = { {0x23, 0x01, 0x00, 0x00}                           };
const guchar xr15v2_hardware_versions[NUM_XR15V2_HARDWARE_VERSIONS][CTRLM_RF4CE_RIB_ATTR_LEN_VERSIONING] = { {0x23, 0x02, 0x00, 0x00}, {0x13, 0x02, 0x00, 0x00} };

ctrlm_obj_controller_rf4ce_t::ctrlm_obj_controller_rf4ce_t(ctrlm_controller_id_t controller_id, ctrlm_obj_network_rf4ce_t &network, unsigned long long ieee_address, ctrlm_rf4ce_result_validation_t result_validation, ctrlm_rcu_configuration_result_t result_configuration) :
   ctrlm_obj_controller_t(controller_id, network, ieee_address),
   obj_network_rf4ce_(&network),
   loading_db_(false),
   stored_in_db_(false),
   pairing_data_(NULL),
   short_address_(0),
   validation_result_(result_validation),
   configuration_result_(result_configuration),
   rib_(),
   version_software_(std::make_shared<ctrlm_rf4ce_sw_version_t>(&network, controller_id)),
   version_dsp_(std::make_shared<ctrlm_rf4ce_dsp_version_t>(&network, controller_id)),
   version_keyword_model_(std::make_shared<ctrlm_rf4ce_keyword_model_version_t>(&network, controller_id)),
   version_arm_(std::make_shared<ctrlm_rf4ce_arm_version_t>(&network, controller_id)),
   version_irdb_(std::make_shared<ctrlm_rf4ce_irdb_version_t>(&network, controller_id)),
   version_bootloader_(std::make_shared<ctrlm_rf4ce_bootloader_version_t>(&network, controller_id)),
   version_golden_(std::make_shared<ctrlm_rf4ce_golden_version_t>(&network, controller_id)),
   version_audio_data_(std::make_shared<ctrlm_rf4ce_audio_data_version_t>(&network, controller_id)),
   version_hardware_(std::make_shared<ctrlm_rf4ce_hw_version_t>(&network, this, controller_id)),
   version_build_id_(std::make_shared<ctrlm_rf4ce_sw_build_id_t>(&network, controller_id)),
   version_dsp_build_id_(std::make_shared<ctrlm_rf4ce_dsp_build_id_t>(&network, controller_id)),
   battery_status_(std::make_shared<ctrlm_rf4ce_battery_status_t>(&network, this, controller_id)),
   battery_milestones_(std::make_shared<ctrlm_rf4ce_battery_milestones_t>(&network, controller_id)),
   voice_command_status_(this),
   voice_command_length_(),
   firmware_updated_(RF4CE_FIRMWARE_UPDATED_NO),
   audio_profiles_ctrl_(std::make_shared<ctrlm_rf4ce_controller_audio_profiles_t>(&network, controller_id)),
   voice_statistics_(std::make_shared<ctrlm_rf4ce_voice_statistics_t>(&network, controller_id)),
   voice_session_statistics_(network.network_id_get(), controller_id),
   rib_entries_updated_(),
   product_name_(std::make_shared<ctrlm_rf4ce_product_name_t>(&network, controller_id)),
   controller_type_(RF4CE_CONTROLLER_TYPE_UNKNOWN),
   capabilities_(std::make_shared<ctrlm_rf4ce_controller_capabilities_t>(&network, controller_id)),
   download_rate_(DOWNLOAD_RATE_BACKGROUND),
   controller_irdb_status_(std::make_shared<ctrlm_rf4ce_controller_irdb_status_t>(&network, controller_id)),
   controller_irdb_status_is_new_(false),
   ir_rf_database_status_(this),
   reboot_reason_(CONTROLLER_REBOOT_OTHER),
   reboot_voltage_level_(0),
   reboot_assert_number_(0),
   memory_available_(0),
   memory_largest_(0),
   download_in_progress_(false),
   download_image_id_(RF4CE_DOWNLOAD_IMAGE_ID_INVALID),
   autobind_in_progress_(false),
   binding_button_in_progress_(false),
   binding_type_(CTRLM_RCU_BINDING_TYPE_INVALID),
   validation_type_(CTRLM_RCU_VALIDATION_TYPE_INVALID),
   binding_security_type_(CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL),
   backup_binding_type_(CTRLM_RCU_BINDING_TYPE_INVALID),
   backup_validation_type_(CTRLM_RCU_VALIDATION_TYPE_INVALID),
   backup_binding_security_type_(CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL),
   time_last_checkin_for_device_update_(0),
   manual_poll_firmware_(false),
   manual_poll_audio_data_(false),
   audio_theme_(RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID),
   memory_dump_(this),
   uinput_writer_(new ctrlm_input_event_writer()),
   configuration_complete_failure_(false),
   privacy_(0),
   time_metrics_(0),
   polling_methods_(0),
   time_last_heartbeat_(0),
   rib_configuration_complete_status_(RF4CE_RIB_CONFIGURATION_COMPLETE_PAIRING_INCOMPLETE),
#ifdef ASB
   asb_key_derivation_method_used_(ASB_KEY_DERIVATION_NONE),
#endif
   metrics_tag_ (0),
#ifdef XR15_704
   needs_reset_(false),
   did_reset_(false),
#endif
   mfg_test_result_(1)
{
   XLOGD_INFO("constructor - %u", controller_id);

#ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   short_rf_retry_period_        = JSON_INT_VALUE_SHORT_RF_RETRY_PERIOD;
   utterance_duration_max_       = JSON_INT_VALUE_UTTERANCE_DURATION_MAX;
   voice_data_retry_max_         = JSON_INT_VALUE_VOICE_DATA_RETRY_MAX;
   voice_csma_backoff_max_       = JSON_INT_VALUE_VOICE_CSMA_BACKOFF_MAX;
   voice_data_backoff_exp_min_   = JSON_INT_VALUE_VOICE_DATA_BACKOFF_EXP_MIN;
   rib_update_check_interval_    = JSON_INT_VALUE_RIB_UPDATE_CHECK_INTERVAL;
   auto_check_validation_period_ = JSON_INT_VALUE_AUTO_CHECK_VALIDATION_PERIOD;
   link_lost_wait_time_          = JSON_INT_VALUE_LINK_LOST_WAIT_TIME;
   update_polling_period_        = JSON_INT_VALUE_UPDATE_POLLING_PERIOD;
   data_request_wait_time_       = JSON_INT_VALUE_DATA_REQUEST_WAIT_TIME;
   voice_command_encryption_     = (voice_command_encryption_t)JSON_INT_VALUE_VOICE_COMMAND_ENCRYPTION;
#endif

   #ifdef USE_LOCAL_IR_RF_DB_CODES
   // These codes will be received over iarm and set to each controller.  store them on the network object.
   property_write_ir_rf_database(0x6B, samsung_0x6B, sizeof(samsung_0x6B));
   property_write_ir_rf_database(0x6C, samsung_0x6C, sizeof(samsung_0x6C));
   property_write_ir_rf_database(0x6D, samsung_0x6D, sizeof(samsung_0x6D));
   property_write_ir_rf_database(0x41, samsung_0x41, sizeof(samsung_0x41));
   property_write_ir_rf_database(0x42, samsung_0x42, sizeof(samsung_0x42));
   property_write_ir_rf_database(0x43, samsung_0x43, sizeof(samsung_0x43));
   property_write_ir_rf_database(0x34, samsung_0x34, sizeof(samsung_0x34));
   #endif

   print_firmware_on_button_press = true;

   has_battery_                                               = false;
   has_dsp_                                                   = false;
   
   // Battery milestones change when battery status is updated
   battery_status_->set_updated_listener(std::bind(&ctrlm_rf4ce_battery_milestones_t::battery_status_updated, battery_milestones_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   // target IRDB status needs to be updated when controller IRDB status changes
   controller_irdb_status_->set_updated_listener(std::bind(&ctrlm_obj_controller_rf4ce_t::controller_irdb_status_updated, this, std::placeholders::_1));
   // controller needs to update some internal variables dependent on product name
   product_name_->set_updated_listener(std::bind(&ctrlm_obj_controller_rf4ce_t::controller_product_name_updated, this, std::placeholders::_1));
   // rib entries updated gets changed when voice command length is updated
   voice_command_length_.set_updated_listener(std::bind(&ctrlm_rf4ce_rib_entries_updated_t::voice_command_length_updated, &rib_entries_updated_, std::placeholders::_1));

   // Far Field
   errno_t safec_rc = memset_s(&ff_metrics_, sizeof(ff_metrics_), 0, sizeof(ff_metrics_));
   ERR_CHK(safec_rc);
   safec_rc = memset_s(&dsp_metrics_, sizeof(dsp_metrics_), 0, sizeof(dsp_metrics_));
   ERR_CHK(safec_rc);

   // Uptime / Privacy Mode
   safec_rc = memset_s(&uptime_privacy_info_, sizeof(uptime_privacy_info_), 0, sizeof(uptime_privacy_info_));
   ERR_CHK(safec_rc);
   time_since_last_saved_ = 0;

   // Polling Init
   safec_rc = memset_s(&polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT], sizeof(ctrlm_rf4ce_polling_configuration_t), 0, sizeof(ctrlm_rf4ce_polling_configuration_t));
   ERR_CHK(safec_rc);
   safec_rc = memset_s(&polling_configurations_[RF4CE_POLLING_METHOD_MAC], sizeof(ctrlm_rf4ce_polling_configuration_t), 0, sizeof(ctrlm_rf4ce_polling_configuration_t));
   ERR_CHK(safec_rc);
   polling_actions_ = g_async_queue_new_full(ctrlm_rf4ce_polling_action_free);
   safec_rc = memset_s(&checkin_time_, sizeof(checkin_time_), 0, sizeof(checkin_time_));
   ERR_CHK(safec_rc);

   // Read from config file (will seperate to seperate function if list grows)
   voice_metrics_->read_config();

   // Configure RIB
   this->configure_rib();
}

ctrlm_obj_controller_rf4ce_t::ctrlm_obj_controller_rf4ce_t() {
   XLOGD_INFO("constructor - default");
}

ctrlm_obj_controller_rf4ce_t::~ctrlm_obj_controller_rf4ce_t() {
   XLOGD_INFO("deconstructor");

   if (metrics_tag_ != 0) {
      XLOGD_WARN("metrics timer destroyed");
      ctrlm_timeout_destroy(&metrics_tag_);
      metrics_tag_ = 0;
   }

   if(pairing_data_ != NULL) {
      ctrlm_hal_free(pairing_data_);
   }
   if(polling_actions_ != NULL) {
      g_async_queue_unref(polling_actions_);
   }

   uinput_writer_->shutdown();
}
 
void ctrlm_obj_controller_rf4ce_t::controller_destroy() {
   //Save the uptime/privacy info
   if(ctrlm_is_voice_assistant((ctrlm_rcu_controller_type_t)controller_type_)) {
      ctrlm_db_rf4ce_write_uptime_privacy_info(network_id_get(), controller_id_get(), (guchar *)&uptime_privacy_info_, sizeof(uptime_privacy_info_t)) ;
   }
}

#ifndef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
guint32                    ctrlm_obj_controller_rf4ce_t::short_rf_retry_period_get(void)        { return(obj_network_rf4ce_->short_rf_retry_period_get());        }
guint16                    ctrlm_obj_controller_rf4ce_t::utterance_duration_max_get(void)       { return(obj_network_rf4ce_->utterance_duration_max_get());       }
guchar                     ctrlm_obj_controller_rf4ce_t::voice_data_retry_max_get(void)         { return(obj_network_rf4ce_->voice_data_retry_max_get());         }
guchar                     ctrlm_obj_controller_rf4ce_t::voice_csma_backoff_max_get(void)       { return(obj_network_rf4ce_->voice_csma_backoff_max_get());       }
guchar                     ctrlm_obj_controller_rf4ce_t::voice_data_backoff_exp_min_get(void)   { return(obj_network_rf4ce_->voice_data_backoff_exp_min_get());   }
guint16                    ctrlm_obj_controller_rf4ce_t::rib_update_check_interval_get(void)    { return(obj_network_rf4ce_->rib_update_check_interval_get());    }
guint16                    ctrlm_obj_controller_rf4ce_t::auto_check_validation_period_get(void) { return(obj_network_rf4ce_->auto_check_validation_period_get()); }
guint16                    ctrlm_obj_controller_rf4ce_t::link_lost_wait_time_get(void)          { return(obj_network_rf4ce_->link_lost_wait_time_get());          }
guint16                    ctrlm_obj_controller_rf4ce_t::update_polling_period_get(void)        { return(obj_network_rf4ce_->update_polling_period_get());        }
guint16                    ctrlm_obj_controller_rf4ce_t::data_request_wait_time_get(void)       { return(obj_network_rf4ce_->data_request_wait_time_get());       }
voice_command_encryption_t ctrlm_obj_controller_rf4ce_t::voice_command_encryption_get(void)     { return(obj_network_rf4ce_->voice_command_encryption_get());     }
#endif
guint16                    ctrlm_obj_controller_rf4ce_t::audio_profiles_targ_get(void)          { return(obj_network_rf4ce_->audio_profiles_targ_get());          }


void ctrlm_obj_controller_rf4ce_t::stats_update(void) {
   ctrlm_hal_network_property_controller_stats_t controller_stats;
   controller_stats.controller_id = controller_id_get();
   controller_stats.short_address = 0;

   if(CTRLM_HAL_RESULT_SUCCESS != network_property_get(CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_STATS, (void **)&controller_stats)) {
      XLOGD_ERROR("Unable to update controller stats");
   } else {
      short_address_ = controller_stats.short_address;
#if CTRLM_HAL_RF4CE_API_VERSION >= 14
      checkin_time_ = controller_stats.checkin_time;    ///< OUT - Timestamp indicating the most recent poll indication of the controller
#endif
   }
}

ctrlm_timestamp_t ctrlm_obj_controller_rf4ce_t::last_mac_poll_checkin_time_get() {
   stats_update();
   return checkin_time_;
}

ctrlm_rf4ce_controller_type_t rf4ce_controller_type_from_product_name(std::string product_name) {
   ctrlm_rf4ce_controller_type_t ret = RF4CE_CONTROLLER_TYPE_UNKNOWN;
   if(product_name.find("XR2-")    != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR2;
   } else if(product_name.find("XR5-")   != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR5;
   } else if(product_name.find("XR11-")  != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR11;
   } else if(product_name.find("XR15-1") != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR15;
   } else if(product_name.find("XR15-2") != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR15V2;
   } else if(product_name.find("XR16-")  != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR16;
   } else if(product_name.find("XR18-")  != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR18;
   } else if(product_name.find("XR19-")  != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XR19;
   } else if(product_name.find("XRA-")   != std::string::npos) {
      ret = RF4CE_CONTROLLER_TYPE_XRA;
   } else {
      XLOGD_ERROR("Unsupported controller type <%s>", product_name.c_str());
   }
   return(ret);
}


void ctrlm_obj_controller_rf4ce_t::user_string_set(guchar *user_string) {
   std::string name = std::string((char *)user_string);
   controller_type_ = obj_network_rf4ce_->controller_type_from_user_string((uint8_t *)name.c_str());

   // If string is comcast, assume XR2
   if(name.find("COMCAST") != std::string::npos) {
      XLOGD_WARN("Assuming XR2.");
      name = "XR2-10";
   } else if(controller_type_ == RF4CE_CONTROLLER_TYPE_UNKNOWN) {
      XLOGD_WARN("Assuming XR5.");
      name = "XR5-40";
   }

   product_name_->set_product_name(name);
}

std::string ctrlm_obj_controller_rf4ce_t::product_name_get(void) const {
   return (product_name_->to_string());
}

const ctrlm_rf4ce_battery_status_t& ctrlm_obj_controller_rf4ce_t::battery_status_get() const {
   return *battery_status_;
}

void ctrlm_obj_controller_rf4ce_t::autobind_in_progress_set(bool in_progress) {
   autobind_in_progress_ = in_progress;
}

bool ctrlm_obj_controller_rf4ce_t::autobind_in_progress_get(void) {
   return(autobind_in_progress_);
}

void ctrlm_obj_controller_rf4ce_t::binding_button_in_progress_set(bool in_progress) {
   binding_button_in_progress_ = in_progress;
}

bool ctrlm_obj_controller_rf4ce_t::binding_button_in_progress_get(void) {
   return(binding_button_in_progress_);
}

void ctrlm_obj_controller_rf4ce_t::screen_bind_in_progress_set(bool in_progress) {
   screen_bind_in_progress_ = in_progress;
}

bool ctrlm_obj_controller_rf4ce_t::screen_bind_in_progress_get(void) {
   return(screen_bind_in_progress_);
}

ctrlm_rf4ce_controller_type_t ctrlm_obj_controller_rf4ce_t::controller_type_get(void) {
   return(controller_type_);
}

ctrlm_rcu_binding_type_t ctrlm_obj_controller_rf4ce_t::binding_type_get(void) {
   return(binding_type_);
}

void ctrlm_obj_controller_rf4ce_t::last_key_time_update(void) {
   last_key_time_->set_value((uint64_t)time(NULL));
   if(validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS && this->last_key_time_get() > last_key_time_flush_) {
      XLOGD_INFO("Flush last key press to the DB");
      ctrlm_db_attr_write(last_key_time_);
      ctrlm_db_attr_write(last_key_code_);

      last_key_time_flush_ = this->last_key_time_get() + LAST_KEY_DATABASE_FLUSH_INTERVAL;
   }
}

void ctrlm_obj_controller_rf4ce_t::time_last_heartbeat_update(void) {
    if(validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) {
      ctrlm_rf4ce_polling_generic_config_t controller_generic_polling_configuration  = obj_network_rf4ce_->controller_generic_polling_configuration_get();
      time_t time_current                                                            = time(NULL);
      gint32 diff                                                                    = 0;
  
      //Account for boxes with already paired remotes before this code was added
      if(time_last_heartbeat_ == 0) {
         time_last_heartbeat_ = time_current;
      }
      //Only count the time diff if the time is valid (startup time is usually off)...
      if(time_current > time_last_heartbeat_) {
         diff = time_current-time_last_heartbeat_;

         //If this is XR19 or some other voice assistant
         if(ctrlm_is_voice_assistant((ctrlm_rcu_controller_type_t)controller_type_)) {
            //Get the heartbeat polling config
            ctrlm_rf4ce_polling_configuration_t controller_polling_configuration_heartbeat = obj_network_rf4ce_->controller_polling_configuration_heartbeat_get(controller_type_);
            guint32 time_interval                                                          = controller_polling_configuration_heartbeat.time_interval / 1000;

            //Make sure there is a start time for uptime.  Should be same as paired time but may be 0 for XR19's that were paired before this code was written
            if(uptime_privacy_info_.time_uptime_start == 0){
              uptime_privacy_info_.time_uptime_start = time_current;
            }
            //If within the allowed heartbeat threshold, increment the uptime and privacy time if in privacy mode
            if(diff <= (gint32)(time_interval * controller_generic_polling_configuration.uptime_multiplier)) {
               uptime_privacy_info_.uptime_seconds += diff;
               if(privacy_) {
                  uptime_privacy_info_.privacy_time_seconds += diff;
               }
               XLOGD_DEBUG("Uptime Start Time <%ld>, Uptime in seconds <%lu>, Privacy Time in seconds <%lu>", uptime_privacy_info_.time_uptime_start, uptime_privacy_info_.uptime_seconds, uptime_privacy_info_.privacy_time_seconds);
            }
         }
  
         //Update with the new heartbeat
         time_last_heartbeat_ = time_current;

         //Check if it is time to save the data
         time_since_last_saved_ += diff;
         if(time_since_last_saved_ >= controller_generic_polling_configuration.hb_time_to_save) {
            ctrlm_network_id_t       network_id = network_id_get();
            ctrlm_controller_id_t controller_id = controller_id_get();
            ctrlm_db_rf4ce_write_time_last_heartbeat(network_id, controller_id, time_last_heartbeat_);

            //If this is XR19 or some other voice assistant...
            if(ctrlm_is_voice_assistant((ctrlm_rcu_controller_type_t)controller_type_)) {
               XLOGD_INFO("Saving to DB: Time Last Heartbeat <%ld>, Uptime Start Time <%ld>, Uptime in seconds <%lu>, Privacy Time in seconds <%lu>", time_last_heartbeat_, uptime_privacy_info_.time_uptime_start, uptime_privacy_info_.uptime_seconds, uptime_privacy_info_.privacy_time_seconds);
               ctrlm_db_rf4ce_write_uptime_privacy_info(network_id, controller_id, (guchar *)&uptime_privacy_info_, sizeof(uptime_privacy_info_t)) ;
            } else {
               XLOGD_INFO("Saving to DB: Time Last Heartbeat <%ld>", time_last_heartbeat_);
            }
            time_since_last_saved_ = 0;
         }
      }
   }
}

void ctrlm_obj_controller_rf4ce_t::manual_poll_firmware(void) {
   manual_poll_firmware_ = true;
}

void ctrlm_obj_controller_rf4ce_t::manual_poll_audio_data(void) {
   manual_poll_audio_data_ = true;
}

void ctrlm_obj_controller_rf4ce_t::audio_theme_set(ctrlm_rf4ce_device_update_audio_theme_t audio_theme) {
   if(audio_theme >= RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID) {
      XLOGD_ERROR("Invalid audio theme (%u)", audio_theme);
   } else {
      audio_theme_ = audio_theme;
   }
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_controller_status(ctrlm_controller_status_t *status) {
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return;
   }

   errno_t safec_rc = memset_s(status, sizeof(ctrlm_controller_status_t), 0, sizeof(ctrlm_controller_status_t));
   ERR_CHK(safec_rc);

   //If the day has changed, store the values related to today and yesterday
   voice_metrics_->process_time(true);

   status->ieee_address                                         = ieee_address_->get_value();
   status->short_address                                        = short_address_;
   status->time_binding                                         = time_binding_->get_value();
   status->binding_type                                         = binding_type_;
   status->validation_type                                      = validation_type_;
   status->security_type                                        = binding_security_type_;
   status->command_count                                        = battery_status_->get_codes_txd_rf();
   status->link_quality                                         = 0;
   status->link_quality_percent                                 = (status->link_quality * 100) / 255;
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
   status->firmware_updated                                     = firmware_updated_;

   safec_rc = strcpy_s(status->manufacturer, sizeof(status->manufacturer), version_hardware_->get_manufacturer_name().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->chipset, sizeof(status->chipset), product_name_->get_predicted_chipset().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_build_id, sizeof(status->version_build_id), version_build_id_->to_string().c_str());
   ERR_CHK(safec_rc);
   status->version_build_id[CTRLM_RCU_VERSION_LENGTH-1] = '\0';

   safec_rc = strcpy_s(status->version_dsp_build_id, sizeof(status->version_dsp_build_id), version_dsp_build_id_->to_string().c_str());
   ERR_CHK(safec_rc);
   status->version_dsp_build_id[CTRLM_RCU_VERSION_LENGTH-1] = '\0';

   safec_rc = strcpy_s(status->version_software, sizeof(status->version_software), version_software_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_dsp, sizeof(status->version_dsp), version_dsp_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_keyword_model, sizeof(status->version_keyword_model), version_keyword_model_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_arm, sizeof(status->version_arm), version_arm_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_hardware, sizeof(status->version_arm), version_hardware_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_irdb, sizeof(status->version_irdb), version_irdb_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_bootloader, sizeof(status->version_bootloader), version_bootloader_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_golden, sizeof(status->version_golden), version_golden_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->version_audio_data, sizeof(status->version_audio_data), version_audio_data_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strncpy_s(status->type, sizeof(status->type),product_name_->to_string().c_str(), CTRLM_RCU_MAX_USER_STRING_LENGTH-1);
   ERR_CHK(safec_rc);
   status->type[CTRLM_RCU_MAX_USER_STRING_LENGTH - 1] = '\0';

   if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_DB_TYPE)) {
      status->ir_db_type = CTRLM_RCU_IR_DB_TYPE_REMOTEC;
   } else {
      status->ir_db_type = CTRLM_RCU_IR_DB_TYPE_UEI;
   }

   if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::LOAD_5_DIGIT_CODE_SUPPORT)) {
      status->ir_db_code_download_supported = true;
   } else {
      status->ir_db_code_download_supported = false;
   }

   safec_rc = strcpy_s(status->ir_db_code_tv, sizeof(status->ir_db_code_tv),"00000");
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->ir_db_code_avr, sizeof(status->ir_db_code_avr),"00000");
   ERR_CHK(safec_rc);

   if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::NO_IR_PROGRAMMED)) {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_NO_CODES;
   } else if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_RF_DB)) {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_IR_RF_DB_CODES;
   } else if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_DB_CODE_TV) &&
             controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_DB_CODE_AVR)) {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_TV_AVR_CODES;
      safec_rc = strcpy_s(status->ir_db_code_tv, sizeof(status->ir_db_code_tv),controller_irdb_status_->get_tv_code_str().c_str());
      ERR_CHK(safec_rc);

      safec_rc = strcpy_s(status->ir_db_code_avr, sizeof(status->ir_db_code_avr),controller_irdb_status_->get_avr_code_str().c_str());
      ERR_CHK(safec_rc);
   } else if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_DB_CODE_TV)) {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_TV_CODE;
      safec_rc = strcpy_s(status->ir_db_code_tv, sizeof(status->ir_db_code_tv),controller_irdb_status_->get_tv_code_str().c_str());
      ERR_CHK(safec_rc);
   } else if(controller_irdb_status_->is_flag_set(ctrlm_rf4ce_controller_irdb_status_t::flag::IR_DB_CODE_AVR)) {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_AVR_CODE;
      safec_rc = strcpy_s(status->ir_db_code_avr, sizeof(status->ir_db_code_avr),controller_irdb_status_->get_avr_code_str().c_str());
      ERR_CHK(safec_rc);
   } else {
      status->ir_db_state = CTRLM_RCU_IR_DB_STATE_NO_CODES;
   }

   safec_rc = strcpy_s(status->irdb_entry_id_name_tv, sizeof(status->irdb_entry_id_name_tv), irdb_entry_id_name_tv_->to_string().c_str());
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->irdb_entry_id_name_avr, sizeof(status->irdb_entry_id_name_avr), irdb_entry_id_name_avr_->to_string().c_str());
   ERR_CHK(safec_rc);

   //Check whether the remote checked in in the last x hours
   time_t now = time(NULL);
   status->checkin_for_device_update = (((now - time_last_checkin_for_device_update_) / RF4CE_CHECKIN_FOR_DEVICE_UPDATE_SECONDS_TO_HOURS) < RF4CE_CHECKIN_FOR_DEVICE_UPDATE_HOURS);

   status->time_last_heartbeat = time_last_heartbeat_;

   //Battery members
   status->has_battery                            = has_battery_;
   status->battery_voltage_loaded                 = VOLTAGE_CALC(battery_milestones_->get_loaded_voltage_battery_last_good());
   status->battery_voltage_unloaded               = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_last_good());
   status->battery_level_percent                  = battery_milestones_->get_actual_battery_percent_last_good();
   get_last_battery_event(status->battery_event, status->time_battery_update);

   status->time_battery_changed                   = battery_milestones_->get_timestamp_battery_changed();
   status->battery_changed_actual_percentage      = battery_milestones_->get_actual_battery_changed();
   status->battery_changed_unloaded_voltage       = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_changed());
   status->time_battery_75_percent                = battery_milestones_->get_timestamp_battery_percent75();
   status->battery_75_percent_actual_percentage   = battery_milestones_->get_actual_battery_percent75();
   status->battery_75_percent_unloaded_voltage    = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_percent75());
   status->time_battery_50_percent                = battery_milestones_->get_timestamp_battery_percent50();
   status->battery_50_percent_actual_percentage   = battery_milestones_->get_actual_battery_percent50();
   status->battery_50_percent_unloaded_voltage    = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_percent50());
   status->time_battery_25_percent                = battery_milestones_->get_timestamp_battery_percent25();
   status->battery_25_percent_actual_percentage   = battery_milestones_->get_actual_battery_percent25();
   status->battery_25_percent_unloaded_voltage    = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_percent25());
   status->time_battery_5_percent                 = battery_milestones_->get_timestamp_battery_percent5();
   status->battery_5_percent_actual_percentage    = battery_milestones_->get_actual_battery_percent5();
   status->battery_5_percent_unloaded_voltage     = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_percent5());
   status->time_battery_0_percent                 = battery_milestones_->get_timestamp_battery_percent0();
   status->battery_0_percent_actual_percentage    = battery_milestones_->get_actual_battery_percent0();
   status->battery_0_percent_unloaded_voltage     = VOLTAGE_CALC(battery_milestones_->get_unloaded_voltage_battery_percent0());

   status->battery_voltage_large_jump_counter     = battery_milestones_->get_battery_voltage_large_jump_counter();
   status->battery_voltage_large_decline_detected = battery_milestones_->get_battery_voltage_large_decline_detected();

   //DSP members
   status->has_dsp                              = has_dsp_;
   status->average_time_in_privacy_mode         = 0;
   status->in_privacy_mode                      = privacy_ & PRIVACY_FLAGS_ENABLED;
   status->average_snr                          = dsp_metrics_.average_snr;
   status->average_keyword_confidence           = dsp_metrics_.average_keyword_confidence;
   status->total_number_of_mics_working         = dsp_metrics_.total_working_mics;
   status->total_number_of_speakers_working     = dsp_metrics_.total_working_speakers;
   status->end_of_speech_initial_timeout_count  = dsp_metrics_.eos_initial_timeout_count;
   status->end_of_speech_timeout_count          = dsp_metrics_.eos_timeout_count;
   status->time_uptime_start                    = uptime_privacy_info_.time_uptime_start;
   status->uptime_seconds                       = uptime_privacy_info_.uptime_seconds;
   status->privacy_time_seconds                 = uptime_privacy_info_.privacy_time_seconds;

   //reboot members
   status->reboot_reason                        = reboot_reason_;
   status->reboot_voltage                       = reboot_voltage_level_;
   status->reboot_assert_number                 = reboot_assert_number_;
   status->reboot_timestamp                     = reboot_time_;

//   ctrlm_print_controller_status(__FUNCTION__, status);
}

void ctrlm_obj_controller_rf4ce_t::req_data(ctrlm_rf4ce_profile_id_t profile_id, ctrlm_timestamp_t tx_window_start, unsigned char length, guchar *data, ctrlm_hal_rf4ce_data_read_t cb_data_read, void *cb_data_param, bool tx_indirect, bool single_channel) {
   obj_network_rf4ce_->req_data(profile_id, controller_id_get(), tx_window_start, length, data, cb_data_read, cb_data_param, tx_indirect, single_channel);
}

ctrlm_hal_result_t ctrlm_obj_controller_rf4ce_t::network_property_get(ctrlm_hal_network_property_t property, void **value) {
   return(obj_network_rf4ce_->property_get(property, value));
}

ctrlm_hal_result_t ctrlm_obj_controller_rf4ce_t::network_property_set(ctrlm_hal_network_property_t property, void *value) {
   return(obj_network_rf4ce_->property_set(property, value));
}

bool ctrlm_obj_controller_rf4ce_t::is_bound(void) const {
   return(stored_in_db_);
}

void ctrlm_obj_controller_rf4ce_t::backup_pairing(void *data) {
   if(pairing_data_ != NULL) {
      ctrlm_hal_free(pairing_data_);
   }

   pairing_data_ = data;
   // Backup previous binding and validation type so they can also be restored
   backup_validation_type_ = validation_type_;
   backup_binding_type_    = binding_type_;
   backup_binding_security_type_ = binding_security_type_;
}

void ctrlm_obj_controller_rf4ce_t::restore_pairing(void) {
   if(pairing_data_ != NULL) {
      ctrlm_hal_network_property_pairing_table_entry_t entry;
      entry.controller_id = controller_id_get();
      entry.pairing_data  = pairing_data_;
      network_property_set(CTRLM_HAL_NETWORK_PROPERTY_PAIRING_TABLE_ENTRY, &entry);

      // Free the pairing data since we no longer need it
      ctrlm_hal_free(pairing_data_);
      pairing_data_ = NULL;

      // Restore previous binding and validation type so they can also be restored
      validation_type_ = backup_validation_type_;
      binding_type_    = backup_binding_type_;
      binding_security_type_ = backup_binding_security_type_;
      validation_result_     = CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS;
   }
}

void ctrlm_obj_controller_rf4ce_t::db_create() {
   // Create the database entry
   ctrlm_db_rf4ce_controller_create(network_id_get(), controller_id_get());
   stored_in_db_ = true;
}

void ctrlm_obj_controller_rf4ce_t::db_destroy() {
   // Destroy the database entry
   ctrlm_db_rf4ce_controller_destroy(network_id_get(), controller_id_get());
   stored_in_db_ = false;
}

void ctrlm_obj_controller_rf4ce_t::db_load() {
   // Set the loading flag so the entries are not rewritten to the DB
   loading_db_ = true;

   // Load the database entry
   guchar *data = NULL;
   guint32 length;
   ctrlm_network_id_t    network_id    = network_id_get();
   ctrlm_controller_id_t controller_id = controller_id_get();
   
   ctrlm_db_attr_read(ieee_address_.get());
   ctrlm_db_attr_read(time_binding_.get());
   ctrlm_db_attr_read(last_key_time_.get());
   ctrlm_db_attr_read(last_key_code_.get());

   ctrlm_db_rf4ce_read_binding_type(network_id, controller_id, &binding_type_);
   ctrlm_db_rf4ce_read_validation_type(network_id, controller_id, &validation_type_);
   ctrlm_db_rf4ce_read_binding_security_type(network_id, controller_id, &binding_security_type_);
#ifdef ASB
   ctrlm_db_rf4ce_read_asb_key_derivation_method(network_id, controller_id, &asb_key_derivation_method_used_);
#endif

   ctrlm_db_rf4ce_read_peripheral_id(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Peripheral Id");
   } else {
      property_write_peripheral_id(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }
   
   ctrlm_db_rf4ce_read_rf_statistics(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - RF Statistics");
   } else {
      property_write_rf_statistics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_attr_read(product_name_.get()); // This also sets the controller type etc
   ctrlm_db_attr_read(version_software_.get());
   ctrlm_db_attr_read(version_dsp_.get());
   ctrlm_db_attr_read(version_keyword_model_.get());
   ctrlm_db_attr_read(version_arm_.get());
   ctrlm_db_attr_read(version_irdb_.get());
   ctrlm_db_attr_read(version_bootloader_.get());
   ctrlm_db_attr_read(version_golden_.get());
   ctrlm_db_attr_read(version_audio_data_.get());
   ctrlm_db_attr_read(version_hardware_.get());
   ctrlm_db_attr_read(version_build_id_.get());
   ctrlm_db_attr_read(version_dsp_build_id_.get());
   ctrlm_db_attr_read(battery_status_.get());
   battery_milestones_->update_last_good(*battery_status_); // Update Last Good variables with data from battery status
   ctrlm_db_attr_read(battery_milestones_.get());
   ctrlm_db_attr_read(audio_profiles_ctrl_.get());
   ctrlm_db_attr_read(voice_statistics_.get());
   ctrlm_db_attr_read(controller_irdb_status_.get());
   ctrlm_db_attr_read(voice_metrics_.get());
   ctrlm_db_attr_read(capabilities_.get());
   ctrlm_db_attr_read(irdb_entry_id_name_tv_.get());
   ctrlm_db_attr_read(irdb_entry_id_name_avr_.get());

   ctrlm_db_rf4ce_read_firmware_updated(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Firmware Updated");
   } else {
      property_write_firmware_updated(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_reboot_diagnostics(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Reboot Diagnostics");
   } else {
      property_write_reboot_diagnostics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_memory_statistics(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Memory Statistics");
   } else {
      property_write_memory_statistics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_time_last_checkin_for_device_update(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Time Last Checkin for Device Update");
   } else {
      property_write_time_last_checkin_for_device_update(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_polling_configuration_heartbeat(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Heartbeat Polling Configuration");
   } else {
      property_write_polling_configuration_heartbeat(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_polling_configuration_mac(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Mac Polling Configuration");
   } else {
      property_write_polling_configuration_mac(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_privacy(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Privacy");
   } else {
      property_write_privacy(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_far_field_metrics(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Field Metrics");
   } else {
      property_write_far_field_metrics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_dsp_metrics(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - DSP Metrics");
   } else {
      property_write_dsp_metrics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }
 
   ctrlm_db_rf4ce_read_uptime_privacy_info(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Uptime Privacy Info");
   } else {
      property_write_uptime_privacy_info(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_time_metrics(network_id, controller_id, &time_metrics_);

   ctrlm_db_rf4ce_read_polling_methods(network_id, controller_id, &polling_methods_);
   ctrlm_db_rf4ce_read_rib_configuration_complete(network_id, controller_id, (int *)&rib_configuration_complete_status_);
   ctrlm_db_rf4ce_read_time_last_heartbeat(network_id, controller_id, &time_last_heartbeat_);

   ctrlm_db_rf4ce_read_mfg_test_result(network_id, controller_id, &data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Mfg Test Result");
   } else {
      property_write_mfg_test_result(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   ctrlm_db_rf4ce_read_ota_failures_type_z_count(network_id, controller_id, &data, &length);
   if (data == NULL) {
      XLOGD_WARN("Not read from DB - OTA failure count");
   } else {
      ota_failure_type_z_cnt_ = (uint8_t) data[0];
      ctrlm_db_free(data);
      data = NULL;
   }

   // Clear the loading flag so subsequent updates are rewritten to the DB
   loading_db_ = false;

   // Indicate that the controller exists in the DB
   stored_in_db_ = true;
}

void ctrlm_obj_controller_rf4ce_t::db_store() {
   // Store the database entries
   guchar data[CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE];
   ctrlm_network_id_t    network_id    = network_id_get();
   ctrlm_controller_id_t controller_id = controller_id_get();

   XLOGD_INFO("controller id %u ieee address 0x%016llX", controller_id, ieee_address_->get_value());

   ctrlm_db_attr_write(ieee_address_);
   ctrlm_db_attr_write(time_binding_);
   ctrlm_db_attr_write(last_key_time_);
   ctrlm_db_attr_write(last_key_code_);

   ctrlm_db_rf4ce_write_binding_type(network_id, controller_id, binding_type_);
   ctrlm_db_rf4ce_write_validation_type(network_id, controller_id, validation_type_);
   ctrlm_db_rf4ce_write_binding_security_type(network_id, controller_id, binding_security_type_);
#ifdef ASB
   ctrlm_db_rf4ce_write_asb_key_derivation_method(network_id, controller_id, asb_key_derivation_method_used_);
#endif

   if(CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID              == property_read_peripheral_id(data, CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID)) {
      ctrlm_db_rf4ce_write_peripheral_id(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS              == property_read_rf_statistics(data, CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS)) {
      ctrlm_db_rf4ce_write_rf_statistics(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS);
   }
   
   rf4ce_rib_export_api_t export_api(std::bind(&ctrlm_obj_network_rf4ce_t::req_process_rib_export, this->obj_network_rf4ce_, this->controller_id_get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_4, std::placeholders::_3));

   ctrlm_db_attr_write(version_software_); version_software_->export_rib(export_api);
   ctrlm_db_attr_write(version_dsp_);
   ctrlm_db_attr_write(version_keyword_model_);
   ctrlm_db_attr_write(version_arm_);
   ctrlm_db_attr_write(version_irdb_);          version_irdb_->export_rib(export_api);
   ctrlm_db_attr_write(version_bootloader_);    version_bootloader_->export_rib(export_api);
   ctrlm_db_attr_write(version_golden_);        version_golden_->export_rib(export_api);
   ctrlm_db_attr_write(version_audio_data_);    version_audio_data_->export_rib(export_api);
   ctrlm_db_attr_write(version_hardware_);      version_hardware_->export_rib(export_api);
   ctrlm_db_attr_write(version_build_id_);
   ctrlm_db_attr_write(version_dsp_build_id_);
   ctrlm_db_attr_write(battery_status_);        battery_status_->export_rib(export_api);
   ctrlm_db_attr_write(audio_profiles_ctrl_);   audio_profiles_ctrl_->export_rib(export_api);
   ctrlm_db_attr_write(voice_statistics_);      voice_statistics_->export_rib(export_api);
   ctrlm_db_attr_write(product_name_);          product_name_->export_rib(export_api);
   ctrlm_db_attr_write(controller_irdb_status_);
   ctrlm_db_attr_write(voice_metrics_);
   ctrlm_db_attr_write(capabilities_);
   ctrlm_db_attr_write(irdb_entry_id_name_tv_);
   ctrlm_db_attr_write(irdb_entry_id_name_avr_);

   if(CTRLM_RF4CE_LEN_FIRMWARE_UPDATED                    == property_read_firmware_updated(data, CTRLM_RF4CE_LEN_FIRMWARE_UPDATED)) {
      ctrlm_db_rf4ce_write_firmware_updated(network_id, controller_id, data, CTRLM_RF4CE_LEN_FIRMWARE_UPDATED);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS         == property_read_reboot_diagnostics(data, CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS)) {
      ctrlm_db_rf4ce_write_reboot_diagnostics(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS          == property_read_memory_statistics(data, CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS)) {
      ctrlm_db_rf4ce_write_memory_statistics(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS);
   }

   if(CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE == property_read_time_last_checkin_for_device_update(data, CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE)) {
      ctrlm_db_rf4ce_write_time_last_checkin_for_device_update(network_id, controller_id, data, CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS == property_read_polling_methods(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS)) {
      ctrlm_db_rf4ce_write_polling_methods(network_id, controller_id, (guint8)data[0]);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION == property_read_polling_configuration_heartbeat(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION)) {
      ctrlm_db_rf4ce_write_polling_configuration_heartbeat(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION == property_read_polling_configuration_mac(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION)) {
      ctrlm_db_rf4ce_write_polling_configuration_mac(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY == property_read_privacy(data, CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY)) {
      ctrlm_db_rf4ce_write_privacy(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS == property_read_far_field_metrics(data, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS)) {
      ctrlm_db_rf4ce_write_far_field_metrics(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS == property_read_dsp_metrics(data, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS)) {
      ctrlm_db_rf4ce_write_dsp_metrics(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS);
   }

   if(CTRLM_RF4CE_RIB_ATTR_LEN_UPTIME_PRIVACY_INFO == property_read_uptime_privacy_info(data, CTRLM_RF4CE_RIB_ATTR_LEN_UPTIME_PRIVACY_INFO)) {
      ctrlm_db_rf4ce_write_uptime_privacy_info(network_id, controller_id, (guchar *)&uptime_privacy_info_, sizeof(uptime_privacy_info_t)) ;
   }

   ctrlm_db_rf4ce_write_time_metrics(network_id, controller_id, time_metrics_);

   ctrlm_db_rf4ce_write_rib_configuration_complete(network_id, controller_id, (int)rib_configuration_complete_status_);
   ctrlm_db_rf4ce_write_time_last_heartbeat(network_id, controller_id, time_last_heartbeat_);

   if(CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT == property_read_mfg_test_result(data, CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT)) {
      ctrlm_db_rf4ce_write_mfg_test_result(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT);
   }
}

void ctrlm_obj_controller_rf4ce_t::configure_rib() {
   // Versioning
   this->rib_.add_attribute(version_software_.get());
   this->rib_.add_attribute(version_dsp_.get());
   this->rib_.add_attribute(version_keyword_model_.get());
   this->rib_.add_attribute(version_arm_.get());
   this->rib_.add_attribute(version_irdb_.get());
   this->rib_.add_attribute(version_bootloader_.get());
   this->rib_.add_attribute(version_golden_.get());
   this->rib_.add_attribute(version_audio_data_.get());
   this->rib_.add_attribute(version_hardware_.get());
   this->rib_.add_attribute(version_build_id_.get());
   this->rib_.add_attribute(version_dsp_build_id_.get());

   this->rib_.add_attribute(battery_status_.get());
   this->rib_.add_attribute(controller_irdb_status_.get());
   this->rib_.add_attribute(&memory_dump_);
   this->rib_.add_attribute(audio_profiles_ctrl_.get());
   this->rib_.add_attribute(voice_statistics_.get());
   this->rib_.add_attribute(&voice_session_statistics_);
   this->rib_.add_attribute(product_name_.get());
   this->rib_.add_attribute(&ir_rf_database_status_);
   this->rib_.add_attribute(&voice_command_status_);
   this->rib_.add_attribute(&voice_command_length_);
   this->rib_.add_attribute(&rib_entries_updated_);
   this->rib_.add_attribute(capabilities_.get());
}

void ctrlm_obj_controller_rf4ce_t::validation_result_set(ctrlm_rcu_binding_type_t binding_type, ctrlm_rcu_validation_type_t validation_type, ctrlm_rf4ce_result_validation_t result, time_t time_binding, time_t time_last_key) {

   if(validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_PENDING && result == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) {
      // Result changed from pending to success
      XLOGD_INFO("Create and store DB entries");

      if(time_binding == 0) {
         time_binding_->set_value((uint64_t)time(NULL));
      } else {
         time_binding_->set_value((uint64_t)time_binding);
      }
      if(time_last_key == 0) {
         last_key_time_->set_value(time_binding_->get_value());
      } else {
         last_key_time_->set_value((uint64_t)time_last_key);
      }
      time_metrics_ = this->time_binding_get();
 
      //reset uptime metrics
      uptime_privacy_info_.time_uptime_start    = this->time_binding_get();
      uptime_privacy_info_.uptime_seconds       = 0;
      uptime_privacy_info_.privacy_time_seconds = 0;
      time_since_last_saved_                    = 0;
      time_last_heartbeat_                      = uptime_privacy_info_.time_uptime_start;
      battery_first_write_                      = true;

      binding_type_     = binding_type;
      validation_type_  = validation_type;
      db_create();
      db_store();
      // HACK for XR15-704, possible duplicate pairing
#ifdef XR15_704
      needs_reset_ = false;
      did_reset_   = false;
#endif
      // HACK for XR15-704

      // Telemetry needs to keep track of binding.  
      log_binding_for_telemetry();

      // Update Polling Configuration
      update_polling_configurations(false);

      // ROBIN -- Check Target IRDB status
      if(RF4CE_CONTROLLER_TYPE_XR19 == controller_type_) {
         push_ir_codes();
      }
   }
   if(result == CTRLM_RF4CE_RESULT_VALIDATION_PENDING) {
      // Validation set back to pending due to repairing a controller.  Change configuration back to pending.
      configuration_result_ = CTRLM_RCU_CONFIGURATION_RESULT_PENDING;
   }

   validation_result_ = result;
}

ctrlm_rf4ce_result_validation_t ctrlm_obj_controller_rf4ce_t::validation_result_get() {
   return(validation_result_);
}

ctrlm_rcu_configuration_result_t ctrlm_obj_controller_rf4ce_t::configuration_result_get(void) {
   return(configuration_result_);
}

void ctrlm_obj_controller_rf4ce_t::property_write_peripheral_id() {

   //TODO
   //peripheral_id_

   //if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
   //   ctrlm_db_rf4ce_write_peripheral_id(network_id_get(), controller_id_get(), data, length);
   //}

   XLOGD_INFO("NOT IMPLEMENTED");
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_peripheral_id(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   //TODO
   //peripheral_id_
   
   //if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
   //   ctrlm_db_rf4ce_write_peripheral_id(network_id_get(), controller_id_get(), data, length);
   //}
   
   XLOGD_INFO("NOT IMPLEMENTED");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_peripheral_id(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   XLOGD_ERROR("NOT IMPLEMENTED");
   return(0);
}

void ctrlm_obj_controller_rf4ce_t::property_write_rf_statistics() {
   //TODO
   //rf_statistics_

   //if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
   //   ctrlm_db_rf4ce_write_rf_statistics(network_id_get(), controller_id_get(), data, length);
   //}

   XLOGD_INFO("NOT IMPLEMENTED");
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_rf_statistics(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS) {
      XLOGD_ERROR("RF Statistics - INVALID PARAMETERS");
      return(0);
   }
   
   //TODO
   //rf_statistics_
   
   //if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
   //   ctrlm_db_rf4ce_write_rf_statistics(network_id_get(), controller_id_get(), data, length);
   //}
   
   XLOGD_INFO("NOT IMPLEMENTED");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_rf_statistics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   XLOGD_ERROR("NOT IMPLEMENTED");
   return(0);
}

gboolean ctrlm_obj_controller_rf4ce_t::send_remote_reboot_event(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar voltage, controller_reboot_reason_t reason, guint32 assert_number) {
   XLOGD_INFO("");
   // Send message to MAIN QUEUE
   ctrlm_main_queue_msg_remote_reboot_t *msg = (ctrlm_main_queue_msg_remote_reboot_t *)g_malloc(sizeof(ctrlm_main_queue_msg_remote_reboot_t));
   if(NULL == msg) {
      XLOGD_ERROR("Couldn't allocate memory");
      g_assert(0);
      return(FALSE);
   }

   msg->type              = CTRLM_MAIN_QUEUE_MSG_TYPE_REMOTE_REBOOT_EVENT;
   msg->network_id        = network_id;
   msg->controller_id     = controller_id;
   msg->voltage           = voltage;
   msg->reason            = reason;
   msg->assert_number     = assert_number;
   ctrlm_main_queue_msg_push(msg);
   return(FALSE);
}

void ctrlm_obj_controller_rf4ce_t::property_write_short_rf_retry_period(guint32 short_rf_retry_period) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   // Store the data on the controller object
   short_rf_retry_period_ = short_rf_retry_period;

   XLOGD_INFO("%u us", short_rf_retry_period_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_short_rf_retry_period(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint32 short_rf_retry_period = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
   property_write_short_rf_retry_period(short_rf_retry_period);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_short_rf_retry_period(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint32 short_rf_retry_period = short_rf_retry_period_;
   #else
   guint32 short_rf_retry_period = short_rf_retry_period_get();
   #endif
   // Load the data from the controller object
   data[0]  = (guchar) (short_rf_retry_period);
   data[1]  = (guchar) (short_rf_retry_period >>  8);
   data[2]  = (guchar) (short_rf_retry_period >> 16);
   data[3]  = (guchar) (short_rf_retry_period >> 24);

   XLOGD_INFO("%u us", short_rf_retry_period);
   
   return(CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD);
}

void ctrlm_obj_controller_rf4ce_t::property_write_firmware_updated(void) {
   guchar data[CTRLM_RF4CE_LEN_FIRMWARE_UPDATED];
   data[0]  = (guchar)(firmware_updated_);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_firmware_updated(network_id_get(), controller_id_get(), data, CTRLM_RF4CE_LEN_FIRMWARE_UPDATED);
   }
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_firmware_updated(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_LEN_FIRMWARE_UPDATED) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   unsigned char firmware_updated = data[0];
   
   if(firmware_updated_ != firmware_updated) {
      // Store the data on the controller object
      firmware_updated_ = (ctrlm_rf4ce_firmware_updated_t)firmware_updated;
      if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
         ctrlm_db_rf4ce_write_firmware_updated(network_id_get(), controller_id_get(), data, length);
      }
   }

   XLOGD_INFO("Firmware Updated <%s>", ctrlm_rf4ce_firmware_updated_str((ctrlm_rf4ce_firmware_updated_t)firmware_updated));
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_firmware_updated(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_LEN_FIRMWARE_UPDATED) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   // Load the data from the controller object
   data[0]  = (char)(firmware_updated_);
   
   XLOGD_INFO("Firmware Updated <%s>", ctrlm_rf4ce_firmware_updated_str((ctrlm_rf4ce_firmware_updated_t)firmware_updated_));
   
   return(CTRLM_RF4CE_LEN_FIRMWARE_UPDATED);
}

void ctrlm_obj_controller_rf4ce_t::property_write_reboot_diagnostics(void) {
   time_t reboot_time_ = time(NULL);
   guchar data[CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS];
   data[0]  = reboot_reason_;
   data[1]  = reboot_voltage_level_;
   data[2]  = (guchar)(reboot_assert_number_);
   data[3]  = (guchar)(reboot_assert_number_ >> 8);
   data[4]  = (guchar)(reboot_assert_number_ >> 16);
   data[5]  = (guchar)(reboot_assert_number_ >> 24);
   data[6]  = (guchar)(reboot_time_);
   data[7]  = (guchar)(reboot_time_ >> 8);
   data[8]  = (guchar)(reboot_time_ >> 16);
   data[9]  = (guchar)(reboot_time_ >> 24);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_reboot_diagnostics(network_id_get(), controller_id_get(), data, CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS);
      send_remote_reboot_event(network_id_get(), controller_id_get(), reboot_voltage_level_, reboot_reason_, reboot_assert_number_);
   }
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_reboot_diagnostics(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   guchar  reboot_reason          = data[0];
   guchar  reboot_voltage_level   = data[1];
   guint32 reboot_assert_number   = (data[5] <<24) | (data[4] << 16) | (data[3] << 8) | data[2];
   time_t reboot_time             = (data[9] <<24) | (data[8] << 16) | (data[7] << 8) | data[6];
   
   if((reboot_reason_ != reboot_reason) || (reboot_voltage_level_ != reboot_voltage_level) || (reboot_assert_number_ != reboot_assert_number) || (reboot_time_ != reboot_time)) {
      // Store the data on the controller object
      reboot_reason_          = (controller_reboot_reason_t)reboot_reason;
      reboot_voltage_level_   = reboot_voltage_level;
      reboot_assert_number_   = reboot_assert_number;
      reboot_time_            = reboot_time;
      if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
         ctrlm_db_rf4ce_write_reboot_diagnostics(network_id_get(), controller_id_get(), data, length);
      }
   }

   if(reboot_reason_ == CONTROLLER_REBOOT_ASSERT_NUMBER) {
      XLOGD_INFO("Reboot Reason <%s>, Reboot Voltage Level <%d>, Reboot Time <%lu>, Reboot Assert Number <%u>", ctrlm_rf4ce_reboot_reason_str((controller_reboot_reason_t)reboot_reason_), reboot_voltage_level_, reboot_time_, reboot_assert_number_);
   } else {
      XLOGD_INFO("Reboot Reason <%s>, Reboot Voltage Level <%d>, Reboot Time <%lu>", ctrlm_rf4ce_reboot_reason_str((controller_reboot_reason_t)reboot_reason_), reboot_voltage_level_, reboot_time_);
   }
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_reboot_diagnostics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   // Load the data from the controller object
   data[0]  = reboot_reason_;
   data[1]  = reboot_voltage_level_;
   data[2]  = (guchar)(reboot_assert_number_);
   data[3]  = (guchar)(reboot_assert_number_ >> 8);
   data[4]  = (guchar)(reboot_assert_number_ >> 16);
   data[5]  = (guchar)(reboot_assert_number_ >> 24);
   data[6]  = (guchar)(reboot_time_);
   data[7]  = (guchar)(reboot_time_ >> 8);
   data[8]  = (guchar)(reboot_time_ >> 16);
   data[9]  = (guchar)(reboot_time_ >> 24);
   
   if(reboot_reason_ == CONTROLLER_REBOOT_ASSERT_NUMBER) {
      XLOGD_INFO("Reboot Reason <%s>, Reboot Voltage Level <%d>, Reboot Assert Number <%u>", ctrlm_rf4ce_reboot_reason_str((controller_reboot_reason_t)reboot_reason_), reboot_voltage_level_, reboot_assert_number_);
   } else {
      XLOGD_INFO("Reboot Reason <%s>, Reboot Voltage Level <%d>", ctrlm_rf4ce_reboot_reason_str((controller_reboot_reason_t)reboot_reason_), reboot_voltage_level_);
   }
   
   return(CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS);
}

void ctrlm_obj_controller_rf4ce_t::property_write_memory_statistics(void) {
   guchar data[CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS];
   data[0]  = (guchar)(memory_available_);
   data[1]  = (guchar)(memory_available_ >> 8);
   data[2]  = (guchar)(memory_largest_);
   data[3]  = (guchar)(memory_largest_ >> 8);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_memory_statistics(network_id_get(), controller_id_get(), data, CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS);
   }
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_memory_statistics(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   guint16 memory_available = (data[1] << 8) | data[0];
   guint16 memory_largest   = (data[3] << 8) | data[2];
   
   if((memory_available_ != memory_available) || (memory_largest_ != memory_largest)) {
      // Store the data on the controller object
      memory_available_  = memory_available;
      memory_largest_    = memory_largest;
      if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
         ctrlm_db_rf4ce_write_memory_statistics(network_id_get(), controller_id_get(), data, length);
      }
   }

   XLOGD_INFO("Memory Available <%u> Largest <%u>", memory_available_, memory_largest_);
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_memory_statistics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   // Load the data from the controller object
   data[0]  = (guchar)(memory_available_);
   data[1]  = (guchar)(memory_available_ >> 8);
   data[2]  = (guchar)(memory_largest_);
   data[3]  = (guchar)(memory_largest_ >> 8);
   
   XLOGD_INFO("Memory Available <%u> Largest <%u>", memory_available_, memory_largest_);
   
   return(CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS);
}

void ctrlm_obj_controller_rf4ce_t::property_write_time_last_checkin_for_device_update(void) {
   guchar data[CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE];
   data[0]  = (guchar)(time_last_checkin_for_device_update_);
   data[1]  = (guchar)(time_last_checkin_for_device_update_ >> 8);
   data[2]  = (guchar)(time_last_checkin_for_device_update_ >> 16);
   data[3]  = (guchar)(time_last_checkin_for_device_update_ >> 24);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_time_last_checkin_for_device_update(network_id_get(), controller_id_get(), data, CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE);
   }
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_time_last_checkin_for_device_update(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   time_t time_last_checkin_for_device_update = ((data[3]  << 24) | (data[2]  << 16) | (data[1]  << 8) | data[0]);
   
   if(time_last_checkin_for_device_update_ != time_last_checkin_for_device_update) {
      // Store the data on the controller object
      time_last_checkin_for_device_update_ = time_last_checkin_for_device_update;

      if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
         property_write_time_last_checkin_for_device_update();
      }
   }

   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_time_last_checkin_for_device_update(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   // Load the data from the controller object
   data[0]  = (guchar)(time_last_checkin_for_device_update_);
   data[1]  = (guchar)(time_last_checkin_for_device_update_ >> 8);
   data[2]  = (guchar)(time_last_checkin_for_device_update_ >> 16);
   data[3]  = (guchar)(time_last_checkin_for_device_update_ >> 24);
   
   XLOGD_INFO("Time Last Checkin For Device Update <%ld>", time_last_checkin_for_device_update_);

   return(CTRLM_RF4CE_LEN_TIME_LAST_CHECKIN_FOR_DEVICE_UPDATE);
}

void ctrlm_obj_controller_rf4ce_t::property_write_maximum_utterance_length(guint16 maximum_utterance_length) {
   if(maximum_utterance_length > 0 && maximum_utterance_length < 100) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return;
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 utterance_duration_max = utterance_duration_max_;
   // Store the data on the controller object
   utterance_duration_max_ = maximum_utterance_length;
   if(utterance_duration_max != maximum_utterance_length) {
      rib_entries_updated_ = true;
   }

   XLOGD_INFO("%u ms", utterance_duration_max_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_maximum_utterance_length(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 maximum_utterance_length = (data[1] << 8) | (data[0]);

   property_write_maximum_utterance_length(maximum_utterance_length);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_maximum_utterance_length(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 utterance_duration_max = utterance_duration_max_;
   #else
   guint16 utterance_duration_max = utterance_duration_max_get();
   #endif
   // Load the data from the controller object
   data[0]  = (guchar) (utterance_duration_max);
   data[1]  = (guchar) (utterance_duration_max >> 8);

   XLOGD_INFO("%u ms", utterance_duration_max);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH);
}

void ctrlm_obj_controller_rf4ce_t::property_write_voice_command_encryption(voice_command_encryption_t voice_command_encryption) {
   if(voice_command_encryption > VOICE_COMMAND_ENCRYPTION_DEFAULT) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return;
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   if(voice_command_encryption_ != voice_command_encryption) {
      rib_entries_updated_ = true;
   }
   
   // Store the data on the controller object
   voice_command_encryption_ = voice_command_encryption;
   XLOGD_INFO("<%s>", ctrlm_rf4ce_voice_command_encryption_str(voice_command_encryption));
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_voice_command_encryption(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   voice_command_encryption_t voice_command_encryption = (voice_command_encryption_t)data[0];

   property_write_voice_command_encryption(voice_command_encryption);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_voice_command_encryption(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   voice_command_encryption_t voice_command_encryption = voice_command_encryption_;
   #else
   voice_command_encryption_t voice_command_encryption = voice_command_encryption_get();
   #endif

   // Load the data from the controller object
   data[0] = voice_command_encryption;
   XLOGD_INFO("<%s>", ctrlm_rf4ce_voice_command_encryption_str(voice_command_encryption));

   return(CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION);
}

void ctrlm_obj_controller_rf4ce_t::property_write_max_voice_data_retry(guchar max_voice_data_retry_attempts) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   if(voice_data_retry_max_ != max_voice_data_retry_attempts) {
      rib_entries_updated_ = true;
   }
   
   // Store the data on the controller object
   voice_data_retry_max_ = max_voice_data_retry_attempts;

   XLOGD_INFO("%u attempts", voice_data_retry_max_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_max_voice_data_retry(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar max_voice_data_retry_attempts = data[0];
   property_write_max_voice_data_retry(max_voice_data_retry_attempts);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_max_voice_data_retry(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar voice_data_retry_max = voice_data_retry_max_;
   #else
   guchar voice_data_retry_max = voice_data_retry_max_get();
   #endif
   // Load the data from the controller object
   data[0] = voice_data_retry_max;

   XLOGD_INFO("%u attempts", voice_data_retry_max);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY);
}

void ctrlm_obj_controller_rf4ce_t::property_write_max_voice_csma_backoff(guchar max_voice_csma_backoffs) {
   if(max_voice_csma_backoffs > 5) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return;
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   if(voice_csma_backoff_max_ != max_voice_csma_backoffs) {
      rib_entries_updated_ = true;
   }
   
   // Store the data on the controller object
   voice_csma_backoff_max_ = max_voice_csma_backoffs;

   XLOGD_INFO("%u backoffs", voice_csma_backoff_max_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_max_voice_csma_backoff(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar max_voice_csma_backoffs = data[0];
   property_write_max_voice_csma_backoff(max_voice_csma_backoffs);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_max_voice_csma_backoff(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar voice_csma_backoff_max = voice_csma_backoff_max_;
   #else
   guchar voice_csma_backoff_max = voice_csma_backoff_max_get();
   #endif
   // Load the data from the controller object
   data[0] = voice_csma_backoff_max;

   XLOGD_INFO("%u backoffs", voice_csma_backoff_max);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF);
}

void ctrlm_obj_controller_rf4ce_t::property_write_min_voice_data_backoff(guchar min_voice_data_backoff_exp) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   if(voice_data_backoff_exp_min_ != min_voice_data_backoff_exp) {
      rib_entries_updated_ = true;
   }
   
   // Store the data on the controller object
   voice_data_backoff_exp_min_ = min_voice_data_backoff_exp;

   XLOGD_INFO("backoff exponent %u", voice_data_backoff_exp_min_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_min_voice_data_backoff(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar min_voice_data_backoff_exp = data[0];
   property_write_min_voice_data_backoff(min_voice_data_backoff_exp);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_min_voice_data_backoff(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guchar voice_data_backoff_exp_min = voice_data_backoff_exp_min_;
   #else
   guchar voice_data_backoff_exp_min = voice_data_backoff_exp_min_get();
   #endif
   // Load the data from the controller object
   data[0] = voice_data_backoff_exp_min;

   XLOGD_INFO("backoff exponent %u", voice_data_backoff_exp_min);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_voice_targ_audio_profiles(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_CTRL_AUDIO_PROFILES) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   guint16 audio_profiles_ctrl = VOICE_AUDIO_PROFILE_ADPCM_16BIT_16KHZ;
   guint16 audio_profiles_ctrl_support = (guint16)audio_profiles_ctrl_->get_profiles();

   guint16 audio_profiles_targ = audio_profiles_targ_get(); // RF4CE Network profiles supported

   if(audio_profiles_targ & audio_profiles_ctrl_support & VOICE_AUDIO_PROFILE_OPUS_16BIT_16KHZ)  { // network and controller support OPUS
      audio_profiles_ctrl = VOICE_AUDIO_PROFILE_OPUS_16BIT_16KHZ;
   }

   // Tell the controller which format to use
   data[0]  = (guchar) (audio_profiles_ctrl);
   data[1]  = (guchar) (audio_profiles_ctrl >> 8);
   
   XLOGD_INFO("<%s>", voice_audio_profile_str((voice_audio_profile_t)audio_profiles_ctrl));

   return(CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_CTRL_AUDIO_PROFILES);
}

void ctrlm_obj_controller_rf4ce_t::property_write_rib_update_check_interval(guint16 rib_update_check_interval) {
   if(rib_update_check_interval > 8760) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return;
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   if(rib_update_check_interval_ != rib_update_check_interval) {
      rib_entries_updated_ = true;
   }
   
   // Store the data on the controller object
   rib_update_check_interval_ = rib_update_check_interval;

   XLOGD_INFO("%u hours", rib_update_check_interval_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_rib_update_check_interval(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 rib_update_check_interval = (data[1] << 8) | data[0];
   property_write_rib_update_check_interval(rib_update_check_interval);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_rib_update_check_interval(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 rib_update_check_interval = rib_update_check_interval_;
   #else
   guint16 rib_update_check_interval = rib_update_check_interval_get();
   #endif
   // Load the data from the controller object
   data[0] = rib_update_check_interval;
   data[1] = (rib_update_check_interval >> 8);

   XLOGD_INFO("%u hours", rib_update_check_interval);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_opus_encoding_params(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   XLOGD_INFO("NOT SUPPORTED");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_opus_encoding_params(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   voice_params_opus_encoder_t params;
   ctrlm_get_voice_obj()->voice_params_opus_encoder_get(&params);
   errno_t safec_rc = memcpy_s(data, CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE, params.data, CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS);
   ERR_CHK(safec_rc);
   return(CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_voice_session_qos(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   XLOGD_INFO("NOT SUPPORTED");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_voice_session_qos(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   voice_params_qos_t params;
   ctrlm_get_voice_obj()->voice_params_qos_get(&params);

   data[0] = (params.timeout_packet_initial);
   data[1] = (params.timeout_packet_initial >> 8);
   data[2] = (params.timeout_packet_subsequent);
   data[3] = (params.timeout_packet_subsequent >> 8);
   data[4] = (params.bitrate_minimum);
   data[5] = (params.time_threshold);
   data[6] = (params.time_threshold >> 8);

   XLOGD_INFO("Timeout First %u Inter %u Bitrate min %u Time Threshold %u", params.timeout_packet_initial, params.timeout_packet_subsequent, params.bitrate_minimum, params.time_threshold);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS);
}

void ctrlm_obj_controller_rf4ce_t::property_write_download_rate(download_rate_t download_rate) {
   switch(download_rate) {
      case DOWNLOAD_RATE_IMMEDIATE:  XLOGD_INFO("IMMEDIATE"); break;
      case DOWNLOAD_RATE_BACKGROUND: XLOGD_INFO("BACKGROUND"); break;
      default:                       XLOGD_WARN("UNKNOWN 0x%02X", download_rate); return;
   }
   // Store the data on the controller object
   download_rate_ = download_rate;
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_download_rate(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   download_rate_t download_rate = (download_rate_t)data[0];
   property_write_download_rate(download_rate);
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_download_rate(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   // Load the data from the controller object
   data[0] = download_rate_;
   
   switch(download_rate_) {
      case DOWNLOAD_RATE_IMMEDIATE:  XLOGD_INFO("IMMEDIATE"); break;
      case DOWNLOAD_RATE_BACKGROUND: XLOGD_INFO("BACKGROUND"); break;
      default:                       XLOGD_WARN("UNKNOWN 0x%02X", download_rate_); break;
   }

   return(CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE);
}

void ctrlm_obj_controller_rf4ce_t::property_write_update_polling_period(guint16 update_polling_period) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   // Store the data on the controller object
   update_polling_period_ = update_polling_period;
   
   XLOGD_INFO("%u hours", update_polling_period_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_update_polling_period(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 update_polling_period = (data[1] << 8) | (data[0]);
   property_write_update_polling_period(update_polling_period);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_update_polling_period(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 update_polling_period = update_polling_period_;
   #else
   guint16 update_polling_period = update_polling_period_get();
   #endif
   // Load the data from the controller object
   data[0]  = (guchar)update_polling_period;
   data[1]  = (guchar)(update_polling_period >> 8);
   
   XLOGD_INFO("%u hours", update_polling_period);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD);
}

void ctrlm_obj_controller_rf4ce_t::property_write_data_request_wait_time(guint16 data_request_wait_time) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   // Store the data on the controller object
   data_request_wait_time_ = data_request_wait_time;
   
   XLOGD_INFO("%u ms", data_request_wait_time_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_data_request_wait_time(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 data_request_wait_time = (data[1] << 8) | (data[0]);
   property_write_data_request_wait_time(data_request_wait_time);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_data_request_wait_time(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 data_request_wait_time = data_request_wait_time_;
   #else
   guint16 data_request_wait_time = data_request_wait_time_get();
   #endif
   // Load the data from the controller object
   data[0]  = (guchar)data_request_wait_time;
   data[1]  = (guchar)(data_request_wait_time >> 8);
   
   XLOGD_INFO("%u ms", data_request_wait_time);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME);
}

gboolean ir_rf_database_status_download_timeout(gpointer data) {
   XLOGD_INFO("");
   ctrlm_obj_controller_rf4ce_t *rf4ce_controller = (ctrlm_obj_controller_rf4ce_t *)data;
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_CONTROLLER, (ctrlm_msg_handler_controller_t)&ctrlm_obj_controller_rf4ce_t::ir_rf_database_status_download_reset, NULL, 0, rf4ce_controller);
   return(FALSE);
}

void ctrlm_obj_controller_rf4ce_t::ir_rf_database_status_download_reset(void *data, int size) {
   XLOGD_INFO("Resetting IR RF Status Download flag.");
   ir_rf_database_status_ = IR_RF_DATABASE_STATUS_DEFAULT;
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_ir_rf_database(guchar index, guchar *data, guchar length) {
   // ROBIN -- Only accept ir waveforms that we do not have
   if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
      guchar temp[CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE] = {'\0'};
      if(obj_network_rf4ce_->property_read_ir_rf_database(index, temp, sizeof(temp)) > 0) {
         if(!(temp[0] & IR_RF_DATABASE_ATTRIBUTE_FLAGS_USE_DEFAULT) && !is_ir_code_to_be_cleared(data, length)) {
            XLOGD_INFO("XRE set an IR code for XR19 which we have already, ignore..");
            return(length);
         }
      }
      ir_rf_database_status_ |=   ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES;
      ir_rf_database_status_ &= ~(ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO);
   }
   return(obj_network_rf4ce_->property_write_ir_rf_database(index, data, length));
}

bool ctrlm_obj_controller_rf4ce_t::is_ir_code_to_be_cleared(guchar *data, guchar length) {
   guchar *current_ptr = data + 1;
   guchar test_byte;
   if(length > CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE || data == NULL) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(false);
   }
   test_byte = IR_RF_DATABASE_ATTRIBUTE_FLAGS_PERMANENT + IR_RF_DATABASE_ATTRIBUTE_FLAGS_USE_DEFAULT;
   if(data[0] != test_byte) {
      XLOGD_DEBUG("First byte not 0x%x.  Do not clear.", test_byte);
      return false;
   }
   test_byte = 0;
   for(int i=0; i<length-1; i++) {
      if (current_ptr[i] != test_byte) {
         XLOGD_DEBUG("<%d> byte not 0x%02x.  Do not clear.", i, test_byte);
         return false;
      }
   }
   XLOGD_INFO("IR Code should be cleared");
   return true;
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_ir_rf_database(guchar index, guchar *data, guchar length) {
   guchar len = 0;
   // ROBIN -- IR-RF DB Read logic
   if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
      switch(index) {
         case CTRLM_KEY_CODE_TV_POWER_ON:
         case CTRLM_KEY_CODE_TV_POWER_OFF:
         case CTRLM_KEY_CODE_AVR_POWER_ON:
         case CTRLM_KEY_CODE_AVR_POWER_OFF: {
            XLOGD_INFO("Allowing XR19 to read discrete ir code");
            return(obj_network_rf4ce_->property_read_ir_rf_database(index, data, length));
            break;
         }
         case CTRLM_KEY_CODE_VOL_UP:
         case CTRLM_KEY_CODE_VOL_DOWN:
         case CTRLM_KEY_CODE_MUTE:
         case CTRLM_KEY_CODE_INPUT_SELECT: {
            if(!(this->obj_network_rf4ce_->target_irdb_status_flags_get() & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_TV || this->obj_network_rf4ce_->target_irdb_status_flags_get() & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_AVR)) {
               XLOGD_INFO("Allowing XR19 to read ir code");
               return(obj_network_rf4ce_->property_read_ir_rf_database(index, data, length));
            } else {
               XLOGD_INFO("Disallowing XR19 to read ir code, already a 5 digit code available");
               data[0] = IR_RF_DATABASE_ATTRIBUTE_FLAGS_USE_DEFAULT;
               return(1);
            }
            break;
         }
         case CTRLM_KEY_CODE_TV_POWER: {
            if(!(this->obj_network_rf4ce_->target_irdb_status_flags_get() & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_TV)) {
               XLOGD_INFO("Allowing XR19 to read ir code");
               return(obj_network_rf4ce_->property_read_ir_rf_database(index, data, length));
            } else {
               XLOGD_INFO("Disallowing XR19 to read ir code, already a 5 digit code available");
               data[0] = IR_RF_DATABASE_ATTRIBUTE_FLAGS_USE_DEFAULT;
               return(1);
            }
            break;
         }
         case CTRLM_KEY_CODE_AVR_POWER_TOGGLE: {
            if(!(this->obj_network_rf4ce_->target_irdb_status_flags_get() & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_AVR)) {
               XLOGD_INFO("Allowing XR19 to read ir code");
               return(obj_network_rf4ce_->property_read_ir_rf_database(index, data, length));
            } else {
               XLOGD_INFO("Disallowing XR19 to read ir code, already a 5 digit code available");
               data[0] = IR_RF_DATABASE_ATTRIBUTE_FLAGS_USE_DEFAULT;
               return(1);
            }
            break;
         }
         default: {
            break;
         }
      }
   }
   len = obj_network_rf4ce_->property_read_ir_rf_database(index, data, length);
   // HACK for XR15-704
#ifdef XR15_704
   if(needs_reset_) {
      if(!ctrlm_device_update_is_controller_updating(network_id_get(), controller_id_get(), true)) {
         if(len < 2) {
            len = 0x32;
            // If IR codes do not exist, inject some fake data
            data[0] = 0x8C;
            data[1] = 0x01;
            data[2] = 0x4C;
            data[3] = 0x02;
            data[4] = 0x31;
            data[5] = 0x06;
            data[6] = 0x00;
         }
         // Set the IR code length to 255
         data[7] = 0xFF;
         did_reset_ = true;
      }
      needs_reset_ = false;
      XLOGD_INFO("EXITING XR15 CRASH CODE: XR15 was reset <%s>", (did_reset_ ? "TRUE" : "FALSE"));
   }
#endif
   // HACK for XR15-704
   return(len);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_target_irdb_status(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_write_target_irdb_status(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_target_irdb_status(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_read_target_irdb_status(data, length));
}

void ctrlm_obj_controller_rf4ce_t::property_write_validation_configuration(guint16 auto_check_validation_period, guint16 link_lost_wait_time) {
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   // Store the data on the controller object
   auto_check_validation_period_ = auto_check_validation_period;
   link_lost_wait_time_          = link_lost_wait_time;

   XLOGD_INFO("auto check validation period %u ms", auto_check_validation_period_);
   XLOGD_INFO("link lost wait time %u ms", link_lost_wait_time_);
   #else
   XLOGD_WARN("Can't set network attribute");
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_validation_configuration(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 auto_check_validation_period = (data[1] << 8) | (data[0]);
   guint16 link_lost_wait_time          = (data[3] << 8) | (data[2]);
   property_write_validation_configuration(auto_check_validation_period, link_lost_wait_time);
   return(length);
   #else
   XLOGD_WARN("Can't set network attribute");
   return(0);
   #endif
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_validation_configuration(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   #ifdef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint16 auto_check_validation_period = auto_check_validation_period_;
   guint16 link_lost_wait_time          = link_lost_wait_time_;
   #else
   guint16 auto_check_validation_period = auto_check_validation_period_get();
   guint16 link_lost_wait_time          = link_lost_wait_time_get();
   #endif
   // Load the data from the controller object
   data[0] = (guchar) (auto_check_validation_period);
   data[1] = (guchar) (auto_check_validation_period >> 8);
   data[2] = (guchar) (link_lost_wait_time);
   data[3] = (guchar) (link_lost_wait_time >> 8);

   XLOGD_INFO("auto check validation period %u ms", auto_check_validation_period);
   XLOGD_INFO("link lost wait time %u ms", link_lost_wait_time);
   
   return(CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION);
}

void ctrlm_obj_controller_rf4ce_t::irdb_entry_id_name_set(ctrlm_irdb_dev_type_t type, ctrlm_irdb_ir_entry_id_t irdb_entry_id_name) {
   switch(type) {
      case CTRLM_IRDB_DEV_TYPE_TV:
         if (irdb_entry_id_name_tv_->to_string() != irdb_entry_id_name) {
            irdb_entry_id_name_tv_->set_value(irdb_entry_id_name);
            if (!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
               ctrlm_db_attr_write(irdb_entry_id_name_tv_);
            }
         }
         XLOGD_INFO("TV IRDB Code <%s>", irdb_entry_id_name_tv_->to_string().c_str());
         break;
      case CTRLM_IRDB_DEV_TYPE_AVR:
         if (irdb_entry_id_name_avr_->to_string() != irdb_entry_id_name) {
            irdb_entry_id_name_avr_->set_value(irdb_entry_id_name);
            if (!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
               ctrlm_db_attr_write(irdb_entry_id_name_avr_);
            }
         }
         XLOGD_INFO("AVR IRDB Code <%s>", irdb_entry_id_name_avr_->to_string().c_str());
         break;
      default:
         XLOGD_WARN("Invalid type <%d>", type);
         break;
   }
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_receiver_id(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   // We do not want to support overwriting the receiver id
   XLOGD_WARN("Wrting the receiver id is NOT allowed");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_receiver_id(guchar *data, guchar length) {
   std::string receiver_id;
   guchar      len;

   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   receiver_id = receiver_id_get();
   len = (receiver_id.length() > length ? length : receiver_id.length());

   // Copy receiver id to data buf
   errno_t safec_rc = strncpy_s((gchar *)data, CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE, receiver_id.c_str(),len);
   ERR_CHK(safec_rc);

   return(len);

}

guchar ctrlm_obj_controller_rf4ce_t::property_write_device_id(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   // We do not want to support overwriting the receiver id
   XLOGD_WARN("Wrting the device id is NOT allowed");
   return(0);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_device_id(guchar *data, guchar length) {
   std::string device_id;
   guchar      len;

   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   device_id = device_id_get();
   len = (device_id.length() > length ? length : device_id.length());

   // Copy receiver id to data buf
   errno_t safec_rc = strncpy_s((gchar *)data, CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE, device_id.c_str(),len);
   ERR_CHK(safec_rc);

   return(len);

}

guchar ctrlm_obj_controller_rf4ce_t::property_write_reboot_stats(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_STATS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   ctrlm_voice_reset_type_t reboot_reason = CTRLM_VOICE_RESET_TYPE_OTHER;
   reboot_reason_          = (controller_reboot_reason_t)data[0];
   reboot_voltage_level_   = data[1];
   reboot_assert_number_   = (data[5] <<24) | (data[4] << 16) | (data[3] << 8) | data[2];
   reboot_time_            = time(NULL);

   // Store the data on the controller object
   if(data[0] == CONTROLLER_REBOOT_POWER_ON) {
       XLOGD_TELEMETRY("Reset Reason - Power-on reset and brownout detection");
       reboot_reason = CTRLM_VOICE_RESET_TYPE_POWER_ON;
   } else if(data[0] == CONTROLLER_REBOOT_EXTERNAL) {
       XLOGD_INFO("Reset Reason - External reset");
       reboot_reason = CTRLM_VOICE_RESET_TYPE_EXTERNAL;
   } else if(data[0] == CONTROLLER_REBOOT_WATCHDOG) {
       XLOGD_INFO("Reset Reason - Watchdog Timer reset");
       reboot_reason = CTRLM_VOICE_RESET_TYPE_WATCHDOG;
   } else if(data[0] == CONTROLLER_REBOOT_CLOCK_LOSS) {
       XLOGD_INFO("Reset Reason - Clock loss reset");
       reboot_reason = CTRLM_VOICE_RESET_TYPE_CLOCK_LOSS;
   } else if(data[0] == CONTROLLER_REBOOT_BROWN_OUT) {
       XLOGD_INFO("Reset Reason - Brown out reset");
       reboot_reason = CTRLM_VOICE_RESET_TYPE_BROWN_OUT;
   } else if(data[0] == CONTROLLER_REBOOT_OTHER) {
       XLOGD_INFO("Reset Reason - Other reset");
   } else if(data[0] == CONTROLLER_REBOOT_ASSERT_NUMBER) {
       XLOGD_TELEMETRY("Reset Reason - Assert Number reset <%u>", reboot_assert_number_);
   } else {
       XLOGD_INFO("Reset Reason - UNKNOWN (%u)", data[0]);
   }
   XLOGD_INFO("voltage %.2fV", (((float)data[1]) * 4.0 / 255));

   print_firmware_on_button_press = true;
   print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_REBOOT);

   // Send data to voice object
   ctrlm_voice_t *obj = ctrlm_get_voice_obj();
   if(NULL != obj) {
      ctrlm_voice_stats_reboot_t  stats_reboot;

      stats_reboot.available          = 1;
      stats_reboot.reset_type         = reboot_reason;
      stats_reboot.voltage            = data[1];
      stats_reboot.battery_percentage = battery_level_percent(controller_type_, *version_hardware_, data[1]);

      obj->voice_session_stats(stats_reboot);
   }

   // Inform device update
   ctrlm_device_update_rf4ce_notify_reboot(network_id_get(), controller_id_get(), device_update_session_resume_support());
   //Save in the database
   property_write_reboot_diagnostics();
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_memory_stats(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   // Store the data on the controller object
   memory_available_ = (data[1] << 8) | data[0];
   memory_largest_   = (data[3] << 8) | data[2];
   XLOGD_INFO("Memory Available <%u> Largest <%u>", memory_available_, memory_largest_);
   //Save in the database
   property_write_memory_statistics();
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_mfg_test(guchar *data, guchar length) {
   if((!capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) && length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST) ||
      ( capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) && length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_HAPTICS)) {
      XLOGD_ERROR("INVALID Length for controller with HAPTICS <%s>", (capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) ? "ENABLED" : "DISABLED"));
      return(0);
   }

   return(obj_network_rf4ce_->property_read_mfg_test(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_mfg_test(guchar *data, guchar length) {
   if((!capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) && length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST) ||
      ( capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) && length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_HAPTICS)) {
      XLOGD_ERROR("INVALID Length for controller with HAPTICS <%s>", (capabilities_->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS) ? "ENABLED" : "DISABLED"));
      return(0);
   }

   return(obj_network_rf4ce_->property_write_mfg_test(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_mfg_test_result(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT ) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   if(!obj_network_rf4ce_->mfg_test_enabled()) {
      XLOGD_ERROR("Manufacturing testing is disabled");
      return(0);
   }

   // Load the data from the controller object
   data[0]  = mfg_test_result_;

   XLOGD_INFO("MFG Security Key Test Result <%u>", mfg_test_result_);
   return(CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_mfg_test_result(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   if(!obj_network_rf4ce_->mfg_test_enabled()) {
      XLOGD_ERROR("Manufacturing testing is disabled");
      return(0);
   }

   guint8 mfg_test_result = data[0];

   if(mfg_test_result_ != mfg_test_result) {
      // Store the data on the controller object
      mfg_test_result_ = mfg_test_result;
      if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
         ctrlm_db_rf4ce_write_mfg_test_result(network_id_get(), controller_id_get(), data, length);
      }
   }

   XLOGD_INFO("MFG Security Key Test Result <%u>", mfg_test_result_);
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_polling_methods(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   polling_methods_ = (guint8)data[0];

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_polling_methods(network_id_get(), controller_id_get(), polling_methods_);
   }

   XLOGD_INFO("polling methods <%s><%s>", ctrlm_rf4ce_controller_type_str(controller_type_get()), ctrlm_rf4ce_controller_polling_methods_str(polling_methods_));
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_polling_methods(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   // Load the data from the controller object
   data[0]  = polling_methods_;

   XLOGD_INFO("polling methods <%s><%s>", ctrlm_rf4ce_controller_type_str(controller_type_get()), ctrlm_rf4ce_controller_polling_methods_str(polling_methods_));
   return(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_polling_configuration_heartbeat(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].trigger       = (data[1] << 8) + (data[0]); // Not Little Endian -- Bitfield
   polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].kp_counter    = (data[2]);
   polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].time_interval = (data[3]) + (data[4] << 8) + (data[5] << 16) + (data[6] << 24);
   polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].reserved      = (data[7]);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_polling_configuration_heartbeat(network_id_get(), controller_id_get(), data, length);
   }

   ctrlm_rf4ce_controller_polling_configuration_print(__FUNCTION__, "Heartbeat", &polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT]);
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_polling_configuration_heartbeat(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   // Load the data from the controller object
   data[0]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].trigger)              & 0xFF;
   data[1]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].trigger       >> 8)   & 0xFF;
   data[2]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].kp_counter)           & 0xFF;
   data[3]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].time_interval)        & 0xFF;
   data[4]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].time_interval >> 8)   & 0xFF;
   data[5]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].time_interval >> 16)  & 0xFF;
   data[6]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].time_interval >> 24)  & 0xFF;
   data[7]  = (polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT].reserved)             & 0xFF;

   ctrlm_rf4ce_controller_polling_configuration_print(__FUNCTION__, "Heartbeat", &polling_configurations_[RF4CE_POLLING_METHOD_HEARTBEAT]);
   return(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_polling_configuration_mac(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   
   polling_configurations_[RF4CE_POLLING_METHOD_MAC].trigger       = (data[1] << 8) + (data[0]);
   polling_configurations_[RF4CE_POLLING_METHOD_MAC].kp_counter    = (data[2]);
   polling_configurations_[RF4CE_POLLING_METHOD_MAC].time_interval = (data[3]) + (data[4] << 8) + (data[5] << 16) + (data[6] << 24);
   polling_configurations_[RF4CE_POLLING_METHOD_MAC].reserved      = (data[7]);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_polling_configuration_mac(network_id_get(), controller_id_get(), data, length);
   }

   ctrlm_rf4ce_controller_polling_configuration_print(__FUNCTION__, "MAC", &polling_configurations_[RF4CE_POLLING_METHOD_MAC]);
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_polling_configuration_mac(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   // Load the data from the controller object
   data[0]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].trigger)              & 0xFF;
   data[1]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].trigger       >> 8)   & 0xFF;
   data[2]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].kp_counter)           & 0xFF;
   data[3]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].time_interval)        & 0xFF;
   data[4]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].time_interval >> 8)   & 0xFF;
   data[5]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].time_interval >> 16)  & 0xFF;
   data[6]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].time_interval >> 24)  & 0xFF;
   data[7]  = (polling_configurations_[RF4CE_POLLING_METHOD_MAC].reserved)             & 0xFF;

   ctrlm_rf4ce_controller_polling_configuration_print(__FUNCTION__, "MAC", &polling_configurations_[RF4CE_POLLING_METHOD_MAC]);
   return(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_privacy(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   privacy_ = data[0];

   XLOGD_INFO("Privacy Mode <%s>", (privacy_ & PRIVACY_FLAGS_ENABLED ? "ENABLED" : "DISABLED"));

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_privacy(network_id_get(), controller_id_get(), data, length);
   }
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_privacy(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   data[0]  =  privacy_ ;

   XLOGD_INFO("Privacy Mode <%s>", (privacy_ & PRIVACY_FLAGS_ENABLED ? "ENABLED" : "DISABLED"));

   return(CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_far_field_configuration(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_write_far_field_configuration(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_far_field_configuration(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_read_far_field_configuration(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_far_field_metrics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(&ff_metrics_, sizeof(ff_metrics_) , 0, sizeof(ff_metrics_));
   ERR_CHK(safec_rc);
   
   ff_metrics_.flags                = data[0];
   ff_metrics_.uptime               = data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24);
   ff_metrics_.privacy_time         = data[5] + (data[6] << 8) + (data[7] << 16) + (data[8] << 24);

   XLOGD_INFO("Flags <%02X>, Uptime <%u>, Privacy Time <%u>", ff_metrics_.flags, ff_metrics_.uptime, ff_metrics_.privacy_time);

   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_far_field_metrics(network_id_get(), controller_id_get(), data, length);
      time_metrics_ = time(NULL);
      ctrlm_db_rf4ce_write_time_metrics(network_id_get(), controller_id_get(), time_metrics_);
   }
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_far_field_metrics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(data, length , 0, length);
   ERR_CHK(safec_rc);

   // Load the data from the controller object
   data[0]  =  ff_metrics_.flags;
   data[1]  =  ff_metrics_.uptime                     & 0xFF;
   data[2]  = (ff_metrics_.uptime       >> 8)         & 0xFF;
   data[3]  = (ff_metrics_.uptime       >> 16)        & 0xFF;
   data[4]  = (ff_metrics_.uptime       >> 24)        & 0xFF;
   data[5]  =  ff_metrics_.privacy_time               & 0xFF;
   data[6]  = (ff_metrics_.privacy_time >> 8)         & 0xFF;
   data[7]  = (ff_metrics_.privacy_time >> 16)        & 0xFF;
   data[8]  = (ff_metrics_.privacy_time >> 24)        & 0xFF;

   XLOGD_INFO("Flags <%02X>, Uptime <%u>, Privacy Time <%u>", ff_metrics_.flags, ff_metrics_.uptime, ff_metrics_.privacy_time);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_dsp_configuration(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_write_dsp_configuration(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_dsp_configuration(guchar *data, guchar length) {
   return(obj_network_rf4ce_->property_read_dsp_configuration(data, length));
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_dsp_metrics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(&dsp_metrics_, sizeof(dsp_metrics_) , 0, sizeof(dsp_metrics_));
   ERR_CHK(safec_rc);

   dsp_metrics_.dropped_mic_frames = data[0] + (data[1] << 8);
   dsp_metrics_.dropped_speaker_frames = data[2] + (data[3] << 8);
   dsp_metrics_.keyword_detect_count = data[4] + (data[5] << 8);
   dsp_metrics_.average_snr = data[6];
   dsp_metrics_.average_keyword_confidence = data[7];
   dsp_metrics_.eos_initial_timeout_count = data[8] + (data[9] << 8);
   dsp_metrics_.eos_timeout_count = data[10] + (data[11] << 8);
   dsp_metrics_.mic_failure_count = data[12];
   dsp_metrics_.total_working_mics = data[13];
   dsp_metrics_.speaker_failure_count = data[14];
   dsp_metrics_.total_working_speakers = data[15];

   XLOGD_INFO("Dropped Mic/Speaker Frames <%u/%u>, Keyword Detect Count <%u>, Average SNR <%.02f>, Average Keyword Confidence <%.04f>, Initial EOS / EOS Timeout <%u/%u>, Total Working Mics/Speakers <%u/%u>, Mic/Speaker Failures <%u/%u>",
            dsp_metrics_.dropped_mic_frames, dsp_metrics_.dropped_speaker_frames, dsp_metrics_.keyword_detect_count, Q_NOTATION_TO_DOUBLE(dsp_metrics_.average_snr, 2),
            Q_NOTATION_TO_DOUBLE(dsp_metrics_.average_keyword_confidence, 7), dsp_metrics_.eos_initial_timeout_count, dsp_metrics_.eos_timeout_count, dsp_metrics_.total_working_mics,
            dsp_metrics_.total_working_speakers, dsp_metrics_.mic_failure_count, dsp_metrics_.speaker_failure_count);


   if(!loading_db_ && validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) { // write this data to the database
      ctrlm_db_rf4ce_write_dsp_metrics(network_id_get(), controller_id_get(), data, length);
      time_metrics_ = time(NULL);
      ctrlm_db_rf4ce_write_time_metrics(network_id_get(), controller_id_get(), time_metrics_);
   }
   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_dsp_metrics(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(data, length , 0, length);
   ERR_CHK(safec_rc);

   data[0]  =  dsp_metrics_.dropped_mic_frames       & 0xFF;
   data[1]  = (dsp_metrics_.dropped_mic_frames >> 8) & 0xFF;
   data[2]  =  dsp_metrics_.dropped_speaker_frames   & 0xFF;
   data[3]  = (dsp_metrics_.dropped_speaker_frames >> 8) & 0XFF;
   data[4]  =  dsp_metrics_.keyword_detect_count     & 0xFF;
   data[5]  = (dsp_metrics_.keyword_detect_count >> 8) & 0xFF;
   data[6]  =  dsp_metrics_.average_snr;
   data[7]  =  dsp_metrics_.average_keyword_confidence;
   data[8]  =  dsp_metrics_.eos_initial_timeout_count & 0xFF;
   data[9]  = (dsp_metrics_.eos_initial_timeout_count >> 8) & 0xFF;
   data[10] =  dsp_metrics_.eos_timeout_count & 0xFF;
   data[11] = (dsp_metrics_.eos_timeout_count >> 8) & 0xFF;
   data[12] =  dsp_metrics_.mic_failure_count;
   data[13] =  dsp_metrics_.total_working_mics;
   data[14] =  dsp_metrics_.speaker_failure_count;
   data[15] =  dsp_metrics_.total_working_speakers;

   XLOGD_INFO("Dropped Mic/Speaker Frames <%u/%u>, Keyword Detect Count <%u>, Average SNR <%.02f>, Average Keyword Confidence <%.04f>, Initial EOS / EOS Timeout <%u/%u>, Total Working Mics/Speakers <%u/%u>, Mic/Speaker Failures <%u/%u>",
            dsp_metrics_.dropped_mic_frames, dsp_metrics_.dropped_speaker_frames, dsp_metrics_.keyword_detect_count, Q_NOTATION_TO_DOUBLE(dsp_metrics_.average_snr, 2),
            Q_NOTATION_TO_DOUBLE(dsp_metrics_.average_keyword_confidence, 7), dsp_metrics_.eos_initial_timeout_count, dsp_metrics_.eos_timeout_count, dsp_metrics_.total_working_mics,
            dsp_metrics_.total_working_speakers, dsp_metrics_.mic_failure_count, dsp_metrics_.speaker_failure_count);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS);
}

guchar ctrlm_obj_controller_rf4ce_t::property_write_uptime_privacy_info(guchar *data, guchar length) {
   if(length != sizeof(uptime_privacy_info_t)) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(&uptime_privacy_info_, sizeof(uptime_privacy_info_t) , 0, sizeof(uptime_privacy_info_t));
   ERR_CHK(safec_rc);

   uptime_privacy_info_.time_uptime_start     = (data[3] << 24) + (data[2] << 16) + (data[1] << 8) + data[0];
   uptime_privacy_info_.uptime_seconds        = (data[7] << 24) + (data[6] << 16) + (data[5] << 8) + data[4];
   uptime_privacy_info_.privacy_time_seconds  = (data[11] << 24) + (data[10] << 16) + (data[9] << 8) + data[8];

   XLOGD_INFO("Uptime Start Time <%ld>, Uptime in seconds <%lu>, Privacy Time in seconds <%lu>", uptime_privacy_info_.time_uptime_start, uptime_privacy_info_.uptime_seconds, uptime_privacy_info_.privacy_time_seconds);

   return(length);
}

guchar ctrlm_obj_controller_rf4ce_t::property_read_uptime_privacy_info(guchar *data, guchar length) {
   if(length != CTRLM_RF4CE_RIB_ATTR_LEN_UPTIME_PRIVACY_INFO) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }

   errno_t safec_rc = memset_s(data, length , 0, length);
   ERR_CHK(safec_rc);

   data[0]   =  uptime_privacy_info_.time_uptime_start           & 0xFF;
   data[1]   = (uptime_privacy_info_.time_uptime_start >> 8)     & 0xFF;
   data[2]   = (uptime_privacy_info_.time_uptime_start >> 16)    & 0xFF;
   data[3]   = (uptime_privacy_info_.time_uptime_start >> 24)    & 0xFF;
   data[4]   =  uptime_privacy_info_.uptime_seconds              & 0xFF;
   data[5]   = (uptime_privacy_info_.uptime_seconds >> 8)        & 0xFF;
   data[6]   = (uptime_privacy_info_.uptime_seconds >> 16)       & 0xFF;
   data[7]   = (uptime_privacy_info_.uptime_seconds >> 24)       & 0xFF;
   data[8]   =  uptime_privacy_info_.privacy_time_seconds        & 0xFF;
   data[9]   = (uptime_privacy_info_.privacy_time_seconds >> 8)  & 0xFF;
   data[10]  = (uptime_privacy_info_.privacy_time_seconds >> 16) & 0xFF;
   data[11]  = (uptime_privacy_info_.privacy_time_seconds >> 24) & 0xFF;

   XLOGD_INFO("Uptime Start Time <%ld>, Uptime in seconds <%lu>, Privacy Time in seconds <%lu>", uptime_privacy_info_.time_uptime_start, uptime_privacy_info_.uptime_seconds, uptime_privacy_info_.privacy_time_seconds);

   return(CTRLM_RF4CE_RIB_ATTR_LEN_UPTIME_PRIVACY_INFO);
}

void ctrlm_obj_controller_rf4ce_t::set_firmware_updated() {
   if(firmware_updated_ != RF4CE_FIRMWARE_UPDATED_YES) {
      firmware_updated_ = RF4CE_FIRMWARE_UPDATED_YES;
      XLOGD_INFO("Firmware Updated <YES>");
      property_write_firmware_updated();
   }
}

gboolean ctrlm_obj_controller_rf4ce_t::is_firmeware_updated(){
   if(firmware_updated_==RF4CE_FIRMWARE_UPDATED_YES){
      return true;
   }
   return false;
}


void ctrlm_obj_controller_rf4ce_t::time_last_checkin_for_device_update_set() {
   time_last_checkin_for_device_update_ = time(NULL);
   property_write_time_last_checkin_for_device_update();
}

void ctrlm_obj_controller_rf4ce_t::time_last_checkin_for_device_update_get(time_t *time) {
   if(NULL == time) {
      XLOGD_ERROR("Parameter time is NULL");
      return;
   }

   *time = time_last_checkin_for_device_update_;
}

void ctrlm_obj_controller_rf4ce_t::get_last_battery_event(ctrlm_rcu_battery_event_t &battery_event, unsigned long &battery_event_timestamp) {
   if (battery_milestones_->get_timestamp_battery_percent0() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_0_PERCENT;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_percent0();
   } else if (battery_milestones_->get_timestamp_battery_percent5() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_PENDING_DOOM;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_percent5();
   } else if (battery_milestones_->get_timestamp_battery_percent25() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_25_PERCENT;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_percent25();
   } else if (battery_milestones_->get_timestamp_battery_percent50() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_50_PERCENT;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_percent50();
   } else if (battery_milestones_->get_timestamp_battery_percent75() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_75_PERCENT;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_percent75();
   } else if (battery_milestones_->get_timestamp_battery_changed() != 0) {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_REPLACED;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_changed();
   } else {
      battery_event           = CTRLM_RCU_BATTERY_EVENT_NONE;
      battery_event_timestamp = battery_milestones_->get_timestamp_battery_last_good();
   }
}

bool ctrlm_obj_controller_rf4ce_t::import_check_validation() {
   bool ret                = TRUE;
   ctrlm_sw_version_t zeros(0,0,0,0);
   ctrlm_sw_version_t invalid(0xFF, 0xFF, 0xFF, 0xFF);

   if(*version_software_   == zeros &&
      *version_irdb_       == zeros &&
      *version_bootloader_ == zeros) {
         XLOGD_ERROR("zeros..");
         ret = FALSE;
      }

   if(*version_software_   == invalid &&
      *version_irdb_       == invalid &&
      *version_bootloader_ == invalid) {
         XLOGD_ERROR("invalid..");
         ret = FALSE;
      }

   return ret;
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::version_software_get(){
   return *version_software_;
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::version_dsp_get(){
   return *version_dsp_;
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::version_keyword_model_get(){
   return *version_keyword_model_;
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::version_arm_get(){
   return *version_arm_;
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::version_audio_data_get(){
   return *version_audio_data_;
}

ctrlm_hw_version_t ctrlm_obj_controller_rf4ce_t::version_hardware_get(){
   return *version_hardware_;
}

ctrlm_controller_capabilities_t ctrlm_obj_controller_rf4ce_t::get_capabilities() const {
   return(*capabilities_);
}

ctrlm_rf4ce_controller_irdb_status_t ctrlm_obj_controller_rf4ce_t::controller_irdb_status_get() {
   return *controller_irdb_status_;
}

void ctrlm_obj_controller_rf4ce_t::controller_product_name_updated(const ctrlm_rf4ce_product_name_t& product_name) {
   controller_type_ = rf4ce_controller_type_from_product_name(product_name.to_string());
   has_battery_     = ctrlm_rf4ce_has_battery(controller_type_);
   has_dsp_         = ctrlm_rf4ce_has_dsp(controller_type_);
   XLOGD_INFO("Controller Type <%s> Has Battery <%s> Has DSP <%s>", ctrlm_rf4ce_controller_type_str(controller_type_), (has_battery_ ? "YES" : "NO"), (has_dsp_ ? "YES" : "NO"));
}

void ctrlm_obj_controller_rf4ce_t::controller_irdb_status_updated(const ctrlm_rf4ce_controller_irdb_status_t& status) {
   if(controller_type_ != RF4CE_CONTROLLER_TYPE_XR19) {
      obj_network_rf4ce_->target_irdb_status_set(*controller_irdb_status_);
   }
}

gboolean ctrlm_obj_controller_rf4ce_t::has_battery() {
   return has_battery_;
}

gboolean ctrlm_obj_controller_rf4ce_t::has_dsp() {
   return has_dsp_;
}

void ctrlm_obj_controller_rf4ce_t::log_binding_for_telemetry() {
   char time_binding_str[CTRLM_MAX_TIME_STR_LEN];
   ctrlm_controller_id_t controller_id = controller_id_get();

   if(this->time_binding_get() == 0) {
      errno_t safec_rc = strcpy_s(time_binding_str, sizeof(time_binding_str), "NEVER");
      ERR_CHK(safec_rc);
   } else {
      time_binding_str[0] = '\0';
      time_t time_binding = this->time_binding_get();
      strftime(time_binding_str, 20, "%F %T", localtime((time_t *)&time_binding));
   }
   XLOGD_INFO("Model <%s>, Binding <%s>, Remote Bound (%u,%u), Time <%s>", product_name_->to_string().c_str(), ctrlm_rcu_binding_type_str(binding_type_), network_id_get(), controller_id_get(), time_binding_str);
   ctrlm_update_last_key_info(controller_id, IARM_BUS_IRMGR_KEYSRC_RF, 0, product_name_->to_string().c_str(), false, true);
}

void ctrlm_obj_controller_rf4ce_t::log_unbinding_for_telemetry() {
   char   time_unbinding_str[CTRLM_MAX_TIME_STR_LEN];
   time_t time_unbinding = time(NULL);

   time_unbinding_str[0] = '\0';
   strftime(time_unbinding_str, 20, "%F %T", localtime((time_t *)&time_unbinding));
   XLOGD_INFO("Model <%s>, Remote Unbound (%u,%u), Time <%s>", product_name_->to_string().c_str(), network_id_get(), controller_id_get(), time_unbinding_str);
}

void ctrlm_obj_controller_rf4ce_t::print_remote_firmware_debug_info(ctrlm_rf4ce_controller_firmware_log_t log_type, string message){
   char remote_info[100] = {'\0'};

   errno_t safec_rc = sprintf_s(remote_info, sizeof(remote_info), "%s ID - %u. Current Firmware: <%s>. ", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), version_software_->to_string().c_str());
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   switch(log_type){
      case RF4CE_PRINT_FIRMWARE_LOG_BUTTON_PRESS:
         if(print_firmware_on_button_press){
            XLOGD_INFO("%s First key press.", remote_info);
            print_firmware_on_button_press = false;
         }
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_REBOOT:
         XLOGD_INFO("%s Remote reboot. %s", remote_info, ctrlm_rf4ce_reboot_reason_str(reboot_reason_));
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_UPGRADE_CHECK:
         XLOGD_INFO("%s Check for FW upgrade", remote_info);
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_STARTED:
         XLOGD_INFO("%s Image download Started. Downloading: %s", remote_info, message.c_str());
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_PENDING:
         XLOGD_INFO("%s Image download Pending: %s", remote_info, message.c_str());
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_COMPLETE:
         XLOGD_INFO("%s Image download complete. Downloaded: %s", remote_info, message.c_str());
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_REBOOT_SCHEDULE:
         XLOGD_INFO("%s Remote reboot scheduled. Downloaded: %s", remote_info, message.c_str());
         break;
      case RF4CE_PRINT_FIRMWARE_LOG_PAIRED_REMOTE:
         XLOGD_INFO("%s Remote Paired. Battery level: %u", remote_info, battery_level_percent(controller_type_, *version_hardware_, battery_status_->get_voltage_loaded()));
         break;
      default:
         XLOGD_INFO("INVALID MESSAGE TYPE");
         break;
   }
}


// These functions are HACKS for XR15-704
#ifdef XR15_704
void ctrlm_obj_controller_rf4ce_t::set_reset() {
   ctrlm_sw_version_t version_bug(XR15_DEVICE_UPDATE_BUG_FIRMWARE_MAJOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_MINOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_REVISION, XR15_DEVICE_UPDATE_BUG_FIRMWARE_PATCH);

   if(RF4CE_CONTROLLER_TYPE_XR15 == controller_type_ && *version_software_ < version_bug) {
      needs_reset_ = true;
   } else {
      needs_reset_ = false;
   }
}

bool ctrlm_obj_controller_rf4ce_t::needs_reset() {
   return(needs_reset_);
}
#endif
// These functions are HACKS for XR15-704

// Polling Functions
void ctrlm_obj_controller_rf4ce_t::polling_action_push(ctrlm_rf4ce_polling_action_msg_t *action) {
   if(action->action != RF4CE_POLLING_ACTION_NONE) {
      XLOGD_INFO("Adding action %s to the polling action queue for controller %u", ctrlm_rf4ce_polling_action_str(action->action), controller_id_get());
      g_async_queue_push_sorted(polling_actions_, (gpointer)action, ctrlm_rf4ce_polling_action_sort_function, NULL);
   }
}

bool ctrlm_obj_controller_rf4ce_t::polling_action_pop(ctrlm_rf4ce_polling_action_msg_t **action) {
   if(action == NULL) {
      XLOGD_ERROR("Action Pointer NULL");
      return(false);
   }

   *action = (ctrlm_rf4ce_polling_action_msg_t *)(g_async_queue_try_pop(polling_actions_)); // Try pop returns 0 which is action NONE
   return(g_async_queue_length(polling_actions_) ? true : false);
}

void ctrlm_obj_controller_rf4ce_t::update_polling_configurations(bool add_polling_action) {
   // Check Network for Polling configuration and if changed, add to polling actions
   if(obj_network_rf4ce_->polling_configuration_by_controller_type(controller_type_get(), &polling_methods_, polling_configurations_)) {
      // Store the new values in DB
      guchar data[CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE];
      ctrlm_network_id_t    network_id    = network_id_get();
      ctrlm_controller_id_t controller_id = controller_id_get();

      if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS == property_read_polling_methods(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS)) {
         ctrlm_db_rf4ce_write_polling_methods(network_id, controller_id, (guint8)data[0]);
      }
   
      if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION == property_read_polling_configuration_heartbeat(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION)) {
         ctrlm_db_rf4ce_write_polling_configuration_heartbeat(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
      }
   
      if(CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION == property_read_polling_configuration_mac(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION)) {
         ctrlm_db_rf4ce_write_polling_configuration_mac(network_id, controller_id, data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
      }      

      // Add polling action
      if(add_polling_action) {
          ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_POLL_CONFIGURATION, NULL, 0);
      }
   }
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_heartbeat(ctrlm_timestamp_t timestamp, guint16 trigger) {
   XLOGD_DEBUG("Controller %u Heartbeat: Trigger %s", controller_id_get(), ctrlm_rf4ce_controller_polling_trigger_str(trigger));
   guint8 flags  = 0x00;
   ctrlm_rf4ce_polling_action_t      action     = RF4CE_POLLING_ACTION_NONE;
   ctrlm_rf4ce_polling_action_msg_t *action_msg = NULL;
   guint8 response[3 + POLLING_RESPONSE_DATA_LEN] = {0};
   guint8 response_len = sizeof(response);
   errno_t safec_rc = -1;

#ifdef ASB
   bool link_key_validation = false;
#endif

   if(CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS == validation_result_) {
      // Set last heartbeat time
      time_last_heartbeat_update();
      switch(trigger) {
         case POLLING_TRIGGER_FLAG_VOICE_SESSION: {
            if(false == ctrlm_get_voice_obj()->voice_device_streaming(network_id_get(), controller_id_get())) {
               action = RF4CE_POLLING_ACTION_EOS;
            }
            break;
         }
         default: {
            // Get the action from the queue
            if(polling_action_pop(&action_msg)) {
               flags |= HEARTBEAT_RESPONSE_FLAG_POLL_AGAIN;
            }
            if(action_msg) {
               action = action_msg->action;
            }
            break;
         }
      }

      // Log Response
      if(action == RF4CE_POLLING_ACTION_NONE) {
         XLOGD_DEBUG("Controller %u Heartbeat Response: Action <%s> Poll Again <%s>", controller_id_get(), ctrlm_rf4ce_polling_action_str(action), (flags & HEARTBEAT_RESPONSE_FLAG_POLL_AGAIN ? "YES" : "NO"));
      } else {
         XLOGD_INFO("Controller %u Heartbeat Response: Action <%s> Poll Again <%s>", controller_id_get(), ctrlm_rf4ce_polling_action_str(action), (flags & HEARTBEAT_RESPONSE_FLAG_POLL_AGAIN ? "YES" : "NO"));
      }
   } 
#ifdef ASB
   else if(POLLING_TRIGGER_FLAG_STATUS == trigger && CTRLM_RF4CE_RESULT_VALIDATION_PENDING == validation_result_) {
      XLOGD_INFO("Controller %u Heartbeat for Link Key Validation, respond with NO ACTION", controller_id_get());
      // Cancel timeout
      ctrlm_timeout_destroy(&asb_tag_);
      asb_tag_ = 0;
      link_key_validation = true;
      response_len = 3; // Backwards compatiability for XR15v2
   }
#endif
   else {
      XLOGD_INFO("Heartbeat from controller that is not bound.. Ignore...");
      return;
   }

   // Ready the response
   // Determine when to send the response (50 ms after receipt)
   ctrlm_timestamp_add_ms(&timestamp, CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME);
   unsigned long delay = ctrlm_timestamp_until_us(timestamp);
   if(delay == 0) {
      ctrlm_timestamp_t now;
      ctrlm_timestamp_get(&now);
      long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
      if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
         XLOGD_WARN("LATE response packet - diff %ld ms", diff);
      }
   }
   // Add Data to response
   response[0] = RF4CE_FRAME_CONTROL_HEARTBEAT_RESPONSE;
   response[1] = flags;
   response[2] = (uint8_t)action;
   if(action_msg) {
      safec_rc = memcpy_s(&response[3], sizeof(response)-3, action_msg->data, POLLING_RESPONSE_DATA_LEN);
      ERR_CHK(safec_rc);
   }

   // Send the response back to the controller
   req_data(CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU, timestamp, response_len, response, NULL, NULL);
#ifdef ASB
   if(link_key_validation) {
      obj_network_rf4ce_->process_pair_result(controller_id_get(), ieee_address_->get_value(), CTRLM_HAL_RESULT_PAIR_SUCCESS);
   }
#endif
   if(NULL != action_msg) {
      free(action_msg);
      action_msg = NULL;
   }
}
void ctrlm_obj_controller_rf4ce_t::rib_configuration_complete(ctrlm_timestamp_t timestamp, ctrlm_rf4ce_rib_configuration_complete_status_t status) {
   XLOGD_INFO("Controller %u Configuration Complete: Status <%u>", controller_id_get(), status);
   switch(status) {
      case RF4CE_RIB_CONFIGURATION_COMPLETE_PAIRING_SUCCESS: {
         XLOGD_INFO("Configuration Complete Pairing Success");
         ctrlm_inform_configuration_complete(network_id_get(), controller_id_get(), CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS);
         break;
      }
      case RF4CE_RIB_CONFIGURATION_COMPLETE_PAIRING_INCOMPLETE: {
         XLOGD_INFO("Configuration Complete Pairing Incomplete");
         ctrlm_inform_configuration_complete(network_id_get(), controller_id_get(), CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS);
         if(configuration_complete_failure_ == false) {
            ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_CONFIGURATION, NULL, 0);
            configuration_complete_failure_ = true;
         }
         break;
      }
      case RF4CE_RIB_CONFIGURATION_COMPLETE_REBOOT_SUCCESS: {
         XLOGD_INFO("Configuration Complete Reboot Success");
         break;
      }
      case RF4CE_RIB_CONFIGURATION_COMPLETE_REBOOT_INCOMPLETE: {
         XLOGD_INFO("Configuration Complete Reboot Incomplete");
         if(configuration_complete_failure_ == false) {
            ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_CONFIGURATION, NULL, 0);
            configuration_complete_failure_ = true;
         }
         break;
      }
      default: {
         XLOGD_ERROR("Unexpected value for status");
         break;
      }
   }
   if(rib_configuration_complete_status_ != status) {
      rib_configuration_complete_status_ = status;
      ctrlm_db_rf4ce_write_rib_configuration_complete(network_id_get(), controller_id_get(), rib_configuration_complete_status_);
   }
}

void ctrlm_obj_controller_rf4ce_t::time_last_heartbeat_get(time_t *time) {
   if(time == NULL) {
      XLOGD_ERROR("NULL parameter");
      return;
   }
   *time = time_last_heartbeat_;
}
// End Polling Functions

void ctrlm_obj_controller_rf4ce_t::binding_security_type_set(ctrlm_rcu_binding_security_type_t type) {
   XLOGD_INFO("Binding Security Type set to %d", type);
   binding_security_type_ = type;
   // TODO need to flush the timestamp to the DB periodically
   if(validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) {
      ctrlm_db_rf4ce_write_binding_security_type(network_id_get(), controller_id_get(), binding_security_type_);
   }
}

ctrlm_rcu_binding_security_type_t ctrlm_obj_controller_rf4ce_t::binding_security_type_get() {
   return(binding_security_type_);
}

#ifdef ASB
typedef struct {
   ctrlm_network_id_t    network_id;
   ctrlm_controller_id_t controller_id;
} asb_link_key_validation_timout_t;

static void ctrlm_asb_link_key_validation_timeout_destroy(gpointer user_data) {
   if(NULL != user_data) {
      XLOGD_INFO("Free timeout data");
      free(user_data);
   }
}

static gboolean ctrlm_asb_link_key_validation_timeout(gpointer user_data) {
   XLOGD_INFO("Timeout");
   asb_link_key_validation_timout_t *data = (asb_link_key_validation_timout_t *)user_data;
   // Send message to MAIN QUEUE
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::asb_link_key_validation_timeout, &data->controller_id, sizeof(data->controller_id), NULL, data->network_id);
   return(FALSE);
}

void ctrlm_obj_controller_rf4ce_t::asb_key_derivation_method_set(asb_key_derivation_method_t method) {
   XLOGD_INFO("ASB Key Derivation method set to %d", method);
   asb_key_derivation_method_used_ = method;
   if(validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS) {
      ctrlm_db_rf4ce_write_asb_key_derivation_method(network_id_get(), controller_id_get(), asb_key_derivation_method_used_);
   }
}

asb_key_derivation_method_t ctrlm_obj_controller_rf4ce_t::asb_key_derivation_method_get() {
   return(asb_key_derivation_method_used_);
}

void ctrlm_obj_controller_rf4ce_t::asb_key_derivation_start(ctrlm_network_id_t network_id) {
   asb_link_key_validation_timout_t *timeout = (asb_link_key_validation_timout_t *)g_malloc(sizeof(asb_link_key_validation_timout_t));
   ctrlm_controller_id_t             controller_id = controller_id_get();
   // Set up timeout object
   if(NULL == timeout) {
      XLOGD_ERROR("Couldn't allocate memory");
      g_assert(0);
      return;
   }
   timeout->network_id = network_id;
   timeout->controller_id = controller_id;
   // Get timestamp of start of blackout period
   ctrlm_timestamp_get(&asb_key_derivation_ts_start_);
   // Start timeout timer
   asb_tag_ = g_timeout_add_full(G_PRIORITY_DEFAULT, CTRLM_RF4CE_CONST_ASB_BLACKOUT_TIME + CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME, ctrlm_asb_link_key_validation_timeout, (gpointer)timeout, ctrlm_asb_link_key_validation_timeout_destroy);
   // Send message to MAIN QUEUE
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::asb_link_key_derivation_perform, (void *)&controller_id, sizeof(controller_id), obj_network_rf4ce_);
}

void ctrlm_obj_controller_rf4ce_t::asb_key_derivation_perform() {
   ctrlm_timestamp_t asb_key_derivation_ts_end;
   unsigned char new_aes128_key[CTRLM_HAL_NETWORK_AES128_KEY_SIZE] = {0};
   ctrlm_hal_network_property_encryption_key_t property = {0};
   // Get Link key
   property.controller_id = controller_id_get();
   if(CTRLM_HAL_RESULT_SUCCESS != network_property_get(CTRLM_HAL_NETWORK_PROPERTY_ENCRYPTION_KEY, (void **)&property)) {
      XLOGD_ERROR("Failed to get Link Key from HAL");
      return;
   }
   // Perform link key derivation

   if(asb_key_derivation(property.aes128_key, new_aes128_key, asb_key_derivation_method_used_)) {
      XLOGD_ERROR("Failed to perform key derivation");
      asb_destroy();
      return;
   }
   // Set New Link key
   errno_t safec_rc = memcpy_s(property.aes128_key, sizeof(property.aes128_key), new_aes128_key, sizeof(property.aes128_key));
   ERR_CHK(safec_rc);
   if(CTRLM_HAL_RESULT_SUCCESS != network_property_set(CTRLM_HAL_NETWORK_PROPERTY_ENCRYPTION_KEY, &property)) {
      XLOGD_ERROR("Failed to set new link key");
      return;
   }

   // Print stats
   ctrlm_timestamp_get(&asb_key_derivation_ts_end);
   XLOGD_INFO("Key Derivation took %lldms", ctrlm_timestamp_subtract_ms(asb_key_derivation_ts_start_, asb_key_derivation_ts_end));
   if(ctrlm_timestamp_subtract_ms(asb_key_derivation_ts_start_, asb_key_derivation_ts_end) > CTRLM_RF4CE_CONST_ASB_BLACKOUT_TIME) {
      XLOGD_ERROR("Key Derivation took longer than blackout time!");
   }

   // Destroy ASB
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::rf4ce_asb_destroy, (void *)NULL, 0, obj_network_rf4ce_);
}
#endif

void ctrlm_obj_controller_rf4ce_t::metrics_tag_reset() {
   metrics_tag_ = 0;
}

static gboolean handle_controller_metrics_timeout(void *data) {
   ctrlm_obj_controller_rf4ce_t *controller = (ctrlm_obj_controller_rf4ce_t *)data;
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_CONTROLLER, (ctrlm_msg_handler_controller_t)&ctrlm_obj_controller_rf4ce_t::handle_controller_metrics, (void *)NULL, 0, (void *)controller);
   controller->metrics_tag_reset();
   return false;
}

void ctrlm_obj_controller_rf4ce_t::handle_controller_metrics(void *data, int size) {
   time_t current    = time(NULL);
   time_t since_last = current - time_metrics_;
   if(since_last >= 60*60*23) { // It's been 23 hours since last reported metrics.. Have to account for early timer.
      if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
         ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_METRICS, NULL, 0);
      }
      metrics_tag_ = ctrlm_timeout_create((60*60*24)*1000, handle_controller_metrics_timeout, this); // 24 hours in milliseconds
   } else {
      if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
         XLOGD_DEBUG("metrics timeout created");
         metrics_tag_ = ctrlm_timeout_create(((60*60*24)-since_last)*1000, handle_controller_metrics_timeout, this); // 24 hours - time since in milliseconds
      }
   }
}

void ctrlm_obj_controller_rf4ce_t::handle_voice_configuration() {
   ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_VOICE_CONFIGURATION, NULL, 0);
}

void ctrlm_obj_controller_rf4ce_t::handle_controller_battery_status() {
   ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_BATTERY_STATUS, NULL, 0);
}

void ctrlm_obj_controller_rf4ce_t::push_ir_codes(void) {
   // ROBIN -- Check Target IRDB status
   guchar flags = this->obj_network_rf4ce_->target_irdb_status_flags_get();
   if(flags & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_TV) {
      ir_rf_database_status_ |= ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE;
   }
   if(flags & CONTROLLER_IRDB_STATUS_FLAGS_IR_DB_CODE_AVR) {
      ir_rf_database_status_ |= ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE;
   }
   if(flags & CONTROLLER_IRDB_STATUS_FLAGS_IR_RF_DB) {
      ir_rf_database_status_ |=   ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES;
      ir_rf_database_status_ &= ~(ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO);
   }
   if(ir_rf_database_status_ != IR_RF_DATABASE_STATUS_DEFAULT) {
      ctrlm_rf4ce_polling_action_push(network_id_get(), controller_id_get(), RF4CE_POLLING_ACTION_IRRF_STATUS, NULL, 0);
   }
}

guint8 ctrlm_obj_controller_rf4ce_t::polling_methods_get() const {
   return polling_methods_;
}

gboolean ctrlm_obj_controller_rf4ce_t::is_batteries_changed(guchar new_voltage) {
   guchar battery_increase = (0.3 * 255 / 4);
   //If the new voltage goes up by 0.3v or more, consider this a batteries changed situation
   return(new_voltage >= (battery_status_->get_voltage_unloaded() + battery_increase));
}

gboolean ctrlm_obj_controller_rf4ce_t::is_batteries_large_voltage_jump(guchar new_voltage) {
   guchar battery_increase = (0.2 * 255 / 4);
   //If the new voltage goes up by 0.2v or more but less than 0.3v, don't set the new voltage but report as a large jump.
   return(new_voltage >= (battery_status_->get_voltage_unloaded() + battery_increase));
}

void ctrlm_obj_controller_rf4ce_t::ota_failure_type_z_cnt_set(uint8_t ota_failures) {
   if (controller_type_ != RF4CE_CONTROLLER_TYPE_XR15V2 && controller_type_ != RF4CE_CONTROLLER_TYPE_XR16) {
      return ;
   }
   ctrlm_obj_controller_t::ota_failure_type_z_cnt_set(ota_failures);
   ctrlm_db_rf4ce_write_ota_failures_type_z_count(network_id_get(), controller_id_get(), ota_failure_type_z_cnt_get());
   XLOGD_INFO("Controller <%s> id %d OTA failure count = %d", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), ota_failure_type_z_cnt_get());
}

uint8_t ctrlm_obj_controller_rf4ce_t::ota_failure_type_z_cnt_get(void) const {
   if (controller_type_ != RF4CE_CONTROLLER_TYPE_XR15V2 && controller_type_ != RF4CE_CONTROLLER_TYPE_XR16) {
      return 0;
   }
   return ctrlm_obj_controller_t::ota_failure_type_z_cnt_get();
}

bool ctrlm_obj_controller_rf4ce_t::is_controller_type_z(void) const {
   if (controller_type_ != RF4CE_CONTROLLER_TYPE_XR15V2 && controller_type_ != RF4CE_CONTROLLER_TYPE_XR16) {
      return false;
   }
   bool is_type_z = ctrlm_obj_controller_t::is_controller_type_z();
   XLOGD_INFO("Controller id %d (%s) is %s", controller_id_get(), ctrlm_rf4ce_controller_type_str(controller_type_), is_type_z ? "TYPE Z" : "NOT TYPE Z");
   return is_type_z;
}

bool ctrlm_obj_controller_rf4ce_t::get_connected() const {
    return is_bound();
}

std::string ctrlm_obj_controller_rf4ce_t::get_name() const {
    return product_name_get();
}

std::string ctrlm_obj_controller_rf4ce_t::get_manufacturer() const {
    return version_hardware_->get_manufacturer_name();
}

ctrlm_sw_version_t ctrlm_obj_controller_rf4ce_t::get_sw_revision() const {
    return *version_software_;
}

ctrlm_hw_version_t ctrlm_obj_controller_rf4ce_t::get_hw_revision() const {
    return *version_hardware_;
}

std::string ctrlm_obj_controller_rf4ce_t::get_fw_revision() const {
    return version_bootloader_->to_string();
}

uint8_t ctrlm_obj_controller_rf4ce_t::get_battery_percent() const {
    return battery_milestones_->get_actual_battery_percent_last_good();
}
uint16_t ctrlm_obj_controller_rf4ce_t::get_last_wakeup_key() const {
    return this->last_key_code_get();
}

bool ctrlm_obj_controller_rf4ce_t::init_uinput_writer() {
    bool ret = false;

    if (!uinput_writer_) {
        return ret;
    }

    std::string uinput_name = product_name_get() + " " + std::to_string(controller_id_get());
    ret = uinput_writer_->init(uinput_name, version_hardware_->get_manufacturer(), version_hardware_->get_model());
    if (!ret) {
        XLOGD_ERROR("Controller <%s><%d> failed to initialize a uinput device", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
        return ret;
    }

    struct stat meta_data = {};
    ret = uinput_writer_->get_meta_data(meta_data);
    if (!ret) {
        XLOGD_ERROR("Controller <%s><%d> could not read input device meta data", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
        return ret;
    }
    set_device_minor_id(minor(meta_data.st_rdev));

    return ret;
}

void ctrlm_obj_controller_rf4ce_t::process_event_key(ctrlm_key_status_t key_status, uint16_t key_code, bool mask) {
    uint16_t linux_code = uinput_writer_->write_event(static_cast<ctrlm_key_code_t>(key_code), key_status);

    if (linux_code == KEY_RESERVED) {
        XLOGD_ERROR("Something went wrong while trying to inject the %s key", ctrlm_key_code_str(static_cast<ctrlm_key_code_t>(key_code)));
        return;
    }

    ctrlm_obj_controller_t::process_event_key(key_status, linux_code, mask);
}

std::string ctrlm_obj_controller_rf4ce_t::controller_type_str_get(void) {
    return std::string("RF4CE_CONTROLLER_") + std::string(ctrlm_rf4ce_controller_type_str(controller_type_));
}

std::string ctrlm_obj_controller_rf4ce_t::get_model() const {
    return product_name_get();
}

ctrlm_rcu_wakeup_config_t ctrlm_obj_controller_rf4ce_t::get_wakeup_config() const {
    return CTRLM_RCU_WAKEUP_CONFIG_NONE;
}

std::vector<uint16_t> ctrlm_obj_controller_rf4ce_t::get_wakeup_custom_list() const {
    return std::vector<uint16_t>();
}
