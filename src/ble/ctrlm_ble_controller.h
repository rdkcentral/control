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
#ifndef _CTRLM_BLE_CONTROLLER_H_
#define _CTRLM_BLE_CONTROLLER_H_

//////////////////////////////////////////
// Includes
//////////////////////////////////////////

#include <string>
#include "ctrlm_hal_ble.h"
#include "../ctrlm_controller.h"
#include "../ctrlm_ipc_rcu.h"
#include "ctrlm_ble_controller_attr_version.h"

//////////////////////////////////////////
// defines
//////////////////////////////////////////

#define CTRLM_BLE_LEN_VOICE_METRICS                      (44)


//////////////////////////////////////////
// Enumerations
//////////////////////////////////////////

typedef enum {
    CTRLM_BLE_RESULT_VALIDATION_SUCCESS          = 0x00,
    CTRLM_BLE_RESULT_VALIDATION_PENDING          = 0x01,
    CTRLM_BLE_RESULT_VALIDATION_FAILURE          = 0x02
} ctrlm_ble_result_validation_t;


//////////////////////////////////////////
// Class ctrlm_obj_controller_ble_t
//////////////////////////////////////////

class ctrlm_obj_network_ble_t;

class ctrlm_obj_controller_ble_t : public ctrlm_obj_controller_t {

public:
   ctrlm_obj_controller_ble_t(ctrlm_controller_id_t controller_id, ctrlm_obj_network_ble_t &network, unsigned long long ieee_address, ctrlm_ble_result_validation_t validation_result);
   ctrlm_obj_controller_ble_t();

   void db_create();
   void db_destroy();
   void db_load();
   void db_store();

   void                             getStatus(ctrlm_controller_status_t *status);

   void                             validation_result_set(ctrlm_ble_result_validation_t validation_result);
   ctrlm_ble_result_validation_t    validation_result_get() const;

   std::string                      controller_type_str_get(void);

   void                             setTypeFromProductName();
   bool                             getOTAProductName(std::string &name);
   void                             setName(const std::string &controller_name, bool write_to_db = false);
   virtual std::string              get_name() const;
   void                             setBatteryPercent(uint8_t percent, bool write_to_db = false);
   virtual uint8_t                  get_battery_percent() const;
   void                             setConnected(bool connected);
   virtual bool                     get_connected() const;
   void                             setSerialNumber(const std::string &sn, bool write_to_db = false);
   virtual std::string              get_serial_number() const;
   void                             setManufacturer(const std::string &controller_manufacturer, bool write_to_db = false);
   virtual std::string              get_manufacturer() const;
   void                             setModel(const std::string &controller_model, bool write_to_db = false);
   virtual std::string              get_model() const;
   void                             setFwRevision(const std::string &rev, bool write_to_db = false);
   virtual std::string              get_fw_revision() const;
   void                             setHwRevision(const std::string &rev, bool write_to_db = false);
   virtual ctrlm_hw_version_t       get_hw_revision() const;
   void                             setSwRevision(const std::string &rev, bool write_to_db = false);
   virtual ctrlm_sw_version_t       get_sw_revision() const;

   bool                             swUpgradeRequired(ctrlm_sw_version_t newVersion, bool force);

   void                             setUpgradeInProgress(bool upgrading);
   bool                             getUpgradeInProgress(void);
   void                             setUpgradeAttempted(bool upgrade_attempted);
   bool                             getUpgradeAttempted(void);

   virtual void                     ota_failure_cnt_incr();
   virtual void                     ota_clear_all_failure_counters();
   virtual void                     ota_failure_cnt_session_clear();

   virtual void                     ota_failure_type_z_cnt_set(uint8_t ota_failures);
   virtual uint8_t                  ota_failure_type_z_cnt_get(void) const;
   virtual bool                     is_controller_type_z(void) const;

   void                             setIrCode(int code);
   void                             setAudioGainLevel(guint8 gain);
   void                             setAudioCodecs(guint32 value);
   void                             setAudioStreaming(bool streaming);
   void                             setTouchMode(unsigned int param);
   void                             setTouchModeSettable(bool param);

   void                             setLastWakeupKey(guint16 code);
   virtual uint16_t                 get_last_wakeup_key() const;

   void                             setWakeupConfig(uint8_t config);
   virtual ctrlm_rcu_wakeup_config_t get_wakeup_config() const;
   void                             setWakeupCustomList(int *list, int size);
   virtual std::vector<uint16_t>    get_wakeup_custom_list() const;
   std::string                      wakeupCustomListToString();

   void                             setUpgradePaused(bool paused);
   bool                             getUpgradePaused();
   bool                             getUpgradePauseSupported(void);
   bool                             getUpgradeStuck();
   bool                             needsBLEConnParamUpdateBeforeOTA(ctrlm_hal_ble_connection_params_t &connParams);

   virtual void                     set_upgrade_error(const std::string &error_str);

   void                             setSupportedIrdbs(ctrlm_irdb_vendor_t* vendors, int num_supported);
   std::vector<ctrlm_irdb_vendor_t> getSupportedIrdbs() const;
   bool                             isSupportedIrdb(ctrlm_irdb_vendor_t vendor);

   void                             print_status();

   virtual bool                     is_stale(time_t stale_time_threshold) const;
   bool                             isVoiceKey(uint16_t key_code) const;

   void                             setPressAndHoldSupport(bool supported);
   bool                             getPressAndHoldSupport() const;

   void                             setVoiceStartTime(ctrlm_timestamp_t startTimeKey);
   ctrlm_timestamp_t                getVoiceStartTimeKey() const;
   ctrlm_timestamp_t                getVoiceStartTimeLocal() const;

private:
   ctrlm_obj_network_ble_t                *obj_network_ble_ = NULL;

   bool                                    connected_       = false;
   
   std::shared_ptr<ctrlm_string_db_attr_t> product_name_;
   std::shared_ptr<ctrlm_string_db_attr_t> serial_number_;
   std::shared_ptr<ctrlm_string_db_attr_t> manufacturer_;
   std::shared_ptr<ctrlm_string_db_attr_t> model_;
   std::shared_ptr<ctrlm_string_db_attr_t> fw_revision_;
   std::shared_ptr<ctrlm_ble_sw_version_t> sw_revision_;
   std::shared_ptr<ctrlm_ble_hw_version_t> hw_revision_;
   std::shared_ptr<ctrlm_uint64_db_attr_t> battery_percent_;

   bool                                    upgrade_in_progress_  = false;
   bool                                    upgrade_attempted_    = false;
   bool                                    upgrade_paused_       = false;
   bool                                    upgrade_stuck_        = false;

   int                                     ir_code_              = 0;

   uint8_t                                 audio_gain_level_     = 0;
   uint32_t                                audio_codecs_         = 0;
   bool                                    audio_streaming_      = false;

   unsigned int                            touch_mode_           = 0;
   bool                                    touch_mode_settable_  = false;

   bool                                    stored_in_db_         = false;
   ctrlm_ble_result_validation_t           validation_result_    = CTRLM_BLE_RESULT_VALIDATION_PENDING;

   uint16_t                                last_wakeup_key_code_ = CTRLM_KEY_CODE_INVALID;
   uint16_t                                voice_key_code_       = KEY_F8;

   bool                                    press_and_hold_supported_ = false;
   ctrlm_timestamp_t                       voice_start_time_key_;
   ctrlm_timestamp_t                       voice_start_time_local_;

   ctrlm_rcu_wakeup_config_t               wakeup_config_        = CTRLM_RCU_WAKEUP_CONFIG_INVALID;
   std::vector<uint16_t>                   wakeup_custom_list_;

   std::vector<ctrlm_irdb_vendor_t>        irdbs_supported_;

   std::string                             ota_product_name_;
   std::string                             controller_type_str_;
   
   bool                                    type_z_supported_     = false;

   bool                                    conn_param_update_before_ota_supported_ = false;
   ctrlm_sw_version_t                      conn_param_update_before_ota_version_;
   
   bool                                    upgrade_pause_supported_ = false;
   ctrlm_sw_version_t                      upgrade_pause_version_;

   bool                                    upgrade_stuck_supported_ = false;
   ctrlm_sw_version_t                      upgrade_stuck_version_;

};
// End Class ctrlm_obj_controller_ble_t

#endif
