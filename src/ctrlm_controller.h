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
#ifndef _CTRLM_CONTROLLER_H_
#define _CTRLM_CONTROLLER_H_

#include <sstream>
#include <vector>
#include "ctrlm_ipc_rcu.h"
#include "ctrlm_irdb.h"
#include "ctrlm_attr_general.h"
#include "ctrlm_attr_voice.h"
#include "ctrlm_version.h"

#define LAST_KEY_DATABASE_FLUSH_INTERVAL (2 * 60 * 60) // in seconds

class ctrlm_obj_network_t;

class ctrlm_obj_controller_t
{
public:
   ctrlm_obj_controller_t(ctrlm_controller_id_t controller_id, ctrlm_obj_network_t &network, unsigned long long ieee_address = 0);
   ctrlm_obj_controller_t();
   virtual ~ctrlm_obj_controller_t();

   virtual void            db_load();
   virtual void            db_store();

   virtual std::string     controller_type_str_get(void);

   ctrlm_ieee_db_addr_t    ieee_address_get(void) const;
   time_t                  time_binding_get() const;
   time_t                  last_activity_time_get() const;
   time_t                  last_key_time_get() const;
   void                    last_key_time_set(time_t val);
   uint16_t                last_key_code_get() const;
   void                    last_key_code_set(uint16_t val);
   virtual void            last_key_time_update();
   virtual void            process_event_key(ctrlm_key_status_t key_status, uint16_t key_code, bool mask);

   void                    send_to(unsigned long delay, unsigned long length, char *data);
   virtual void            irdb_entry_id_name_set(ctrlm_irdb_dev_type_t type, ctrlm_irdb_ir_entry_id_t irdb_ir_entry_id);
   std::string             get_irdb_entry_id_name_tv() const;
   std::string             get_irdb_entry_id_name_avr() const;

   ctrlm_controller_id_t   controller_id_get() const;
   ctrlm_network_id_t      network_id_get() const;
   std::string             receiver_id_get() const;
   std::string             device_id_get() const;
   std::string             service_account_id_get() const;
   std::string             partner_id_get() const;
   std::string             experience_get() const;
   std::string             stb_name_get() const;
   void                    set_device_minor_id(int device_minor_id);
   int                     get_device_minor_id() const;

   virtual ctrlm_controller_capabilities_t get_capabilities() const;

   virtual void            ota_failure_cnt_incr();
   virtual void            ota_clear_all_failure_counters();
   virtual void            ota_failure_cnt_session_clear();
   bool                    retry_ota() const;

   virtual void            ota_failure_type_z_cnt_set(uint8_t ota_failures);
   virtual uint8_t         ota_failure_type_z_cnt_get(void) const;
   virtual bool            is_controller_type_z(void) const;

   virtual bool            is_stale(time_t stale_time_threshold) const;

   virtual bool                      get_connected() const;
   virtual std::string               get_name() const;
   virtual std::string               get_manufacturer() const;
   virtual std::string               get_model() const;
   virtual ctrlm_sw_version_t        get_sw_revision() const;
   virtual ctrlm_hw_version_t        get_hw_revision() const;
   virtual std::string               get_fw_revision() const;
   virtual std::string               get_serial_number() const;
   virtual uint8_t                   get_battery_percent() const;
   virtual uint16_t                  get_last_wakeup_key() const;
   virtual ctrlm_rcu_wakeup_config_t get_wakeup_config() const;
   virtual std::vector<uint16_t>     get_wakeup_custom_list() const;

   virtual void                      set_upgrade_progress(uint8_t progress);
   virtual uint8_t                   get_upgrade_progress() const;
   virtual void                      set_upgrade_state(ctrlm_rcu_upgrade_state_t state);
   virtual ctrlm_rcu_upgrade_state_t get_upgrade_state() const;
   virtual void                      set_upgrade_error(const std::string &error_str);
   virtual std::string               get_upgrade_error() const;
   virtual void                      set_upgrade_session_uuid(bool generate = true);
   virtual std::string               get_upgrade_session_uuid() const;
   virtual void                      set_upgrade_increment(uint8_t increment);
   virtual uint8_t                   get_upgrade_increment() const;
   virtual bool                      is_upgrade_progress_at_increment() const;

   void                    update_voice_metrics(bool is_short_utterance, uint32_t voice_packets_sent, uint32_t voice_packets_lost);

private:
   ctrlm_controller_id_t      controller_id_ = CTRLM_MAIN_CONTROLLER_ID_INVALID;
   ctrlm_obj_network_t       *obj_network_   = NULL;

protected:
   std::shared_ptr<ctrlm_ieee_db_addr_t>   ieee_address_;

   std::shared_ptr<ctrlm_uint64_db_attr_t> time_binding_;
   std::shared_ptr<ctrlm_uint64_db_attr_t> last_activity_time_;

   std::shared_ptr<ctrlm_uint64_db_attr_t> last_key_time_;
   std::shared_ptr<ctrlm_uint64_db_attr_t> last_key_code_;
   time_t                                  last_key_time_flush_ = 0;
   ctrlm_key_status_t                      last_key_status_     = CTRLM_KEY_STATUS_INVALID;

   std::shared_ptr<ctrlm_string_db_attr_t> irdb_entry_id_name_tv_;
   std::shared_ptr<ctrlm_string_db_attr_t> irdb_entry_id_name_avr_;

   std::shared_ptr<ctrlm_voice_metrics_t>  voice_metrics_;

   uint8_t                                 ota_failure_type_z_cnt_  = 0;
   uint8_t                                 ota_failure_cnt_session_ = 0;
   std::shared_ptr<ctrlm_uint64_db_attr_t> ota_failure_cnt_from_last_success_;

   int                                     device_minor_id_      = 0;

   uint8_t                                 upgrade_progress_  = -1;
   ctrlm_rcu_upgrade_state_t               upgrade_state_     = CTRLM_RCU_UPGRADE_STATE_INVALID;
   std::string                             upgrade_error_msg_ = "";
   std::string                             upgrade_session_uuid_ = "";
   uint8_t                                 upgrade_increment_ = -1;
};

#endif
