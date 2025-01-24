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

#ifndef _CTRLM_BLE_NETWORK_H_
#define _CTRLM_BLE_NETWORK_H_

// Includes

#include "../ctrlm.h"
#include "../ctrlm_network.h"
#include "ctrlm_hal_ble.h"
#include "ctrlm_ir_controller.h"
#include "ctrlm_ble_controller.h"
#include "ctrlm_ipc_ble.h"
#include <iostream>
#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <jansson.h>
#include "ctrlm_rfc.h"
#include "ctrlm_ir_rf_db.h"
#include "ctrlm_ble_rcu_interface.h"

// End Includes


typedef struct {
   ctrlm_main_queue_msg_header_t               header;
   ctrlm_iarm_call_GetRcuUnpairReason_params_t *params;
   sem_t *                                     semaphore;
} ctrlm_main_queue_msg_get_rcu_unpair_reason_t;

typedef struct {
   ctrlm_main_queue_msg_header_t               header;
   ctrlm_iarm_call_GetRcuRebootReason_params_t *params;
   sem_t *                                     semaphore;
} ctrlm_main_queue_msg_get_rcu_reboot_reason_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                header;
   ctrlm_iarm_call_GetRcuLastWakeupKey_params_t *params;
   sem_t *                                      semaphore;
} ctrlm_main_queue_msg_get_last_wakeup_key_t;

typedef struct {
   ctrlm_main_queue_msg_header_t          header;
   ctrlm_iarm_call_SendRcuAction_params_t *params;
   sem_t *                                semaphore;
} ctrlm_main_queue_msg_send_rcu_action_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                  header;
   bool                                           retry_all;
} ctrlm_main_queue_msg_continue_upgrade_t;

typedef struct {
   ctrlm_main_queue_msg_header_t             header;
   ctrlm_iarm_call_IrProtocol_params_t       *params;
   sem_t *                                   semaphore;
} ctrlm_main_queue_msg_ir_protocol_characteristic_t;

typedef struct {
   std::string                   product_name;
   std::string                   path;
   std::string                   image_filename;
   ctrlm_sw_version_t            version_software;
   ctrlm_sw_version_t            version_bootloader_min;
   ctrlm_hw_version_t            version_hardware_min;
   int                           size;
   int                           crc;
   bool                          force_update;
} ctrlm_ble_upgrade_image_info_t;


///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr BLE Controller Unpair Metrics Class
 * 
 * This class is used within ControlMgr to store data from the last RCU unpair event
 */
// Class ctrlm_ble_unpair_metrics_t
class ctrlm_ble_unpair_metrics_t {
public:
   ctrlm_ble_unpair_metrics_t();
   ctrlm_ble_unpair_metrics_t(ctrlm_obj_network_ble_t &network);

   void                       db_load();
   void                       write_rcu_unpair_event(uint64_t ieee);
   void                       write_rcu_unpair_event(uint64_t ieee, std::string reason);
   void                       log_rcu_unpair_event();

   std::shared_ptr<ctrlm_ieee_db_addr_t>   ieee_address_;
   std::shared_ptr<ctrlm_uint64_db_attr_t> unpaired_time_;
   std::shared_ptr<ctrlm_string_db_attr_t> reason_;
};


///////////////////////////////////////////////////////////////////////////////////////////

// Class ctrlm_obj_network_ble_t

class ctrlm_obj_network_ble_t : public ctrlm_obj_network_t {
public:
                                 ctrlm_obj_network_ble_t(ctrlm_network_type_t type, ctrlm_network_id_t id, const char *name, gboolean mask_key_codes, json_t *json_obj_net_ble, GThread *original_thread);
   virtual                      ~ctrlm_obj_network_ble_t();
   virtual ctrlm_hal_result_t    hal_init_request(GThread *ctrlm_main_thread);
   virtual void                  start(GMainLoop* main_loop);
   virtual std::string           db_name_get() const;
   void                          rfc_retrieved_handler(const ctrlm_rfc_attr_t &attr);


   void                          ind_rcu_status(ctrlm_hal_ble_RcuStatusData_t *params);
   void                          ind_rcu_paired(ctrlm_hal_ble_IndPaired_params_t *params);
   void                          ind_rcu_unpaired(ctrlm_hal_ble_IndUnPaired_params_t *params);
   void                          ind_keypress(ctrlm_hal_ble_IndKeypress_params_t *params);

   void                          ind_process_rcu_status(void *data, int size);
   void                          ind_process_paired(void *data, int size);
   void                          ind_process_unpaired(void *data, int size);
   void                          ind_process_keypress(void *data, int size);

   virtual void                  req_process_network_status(void *data, int size);
   virtual void                  req_process_controller_status(void *data, int size);
   
   virtual void                  req_process_voice_session_begin(void *data, int size);
   virtual void                  req_process_voice_session_end(void *data, int size);

   virtual void                  req_process_start_pairing(void *data, int size);
   virtual void                  req_process_pair_with_code(void *data, int size);
   virtual void                  req_process_ir_set_code(void *data, int size);
   virtual void                  req_process_ir_clear_codes(void *data, int size);
   virtual void                  req_process_find_my_remote(void *data, int size);
   void                          req_process_get_rcu_unpair_reason(void *data, int size);
   void                          req_process_get_rcu_reboot_reason(void *data, int size);
   void                          req_process_get_rcu_last_wakeup_key(void *data, int size);
   void                          req_process_send_rcu_action(void *data, int size);
   void                          req_process_write_rcu_wakeup_config(void *data, int size);
   void                          req_process_check_for_stale_remote(void *data, int size);
   void                          req_process_get_ir_protocol_support(void *data, int size);
   void                          req_process_set_ir_protocol_control(void *data, int size);

   virtual void                  req_process_network_managed_upgrade(void *data, int size);
   virtual void                  req_process_upgrade_controllers(void *data, int size);
   virtual json_t *              xconf_export_controllers();
   void                          addUpgradeImage(const ctrlm_ble_upgrade_image_info_t &image_info);
   void                          clearUpgradeImages();

   virtual void                  voice_command_status_set(void *data, int size);
   virtual void                  process_voice_controller_metrics(void *data, int size);
   virtual void                  ind_process_voice_session_end(void *data, int size);

   void                          ctrlm_ble_ProcessIndicateHAL_VoiceData(void *data, int size);

   void                          controllers_load();
   virtual void                  controller_list_get(std::vector<ctrlm_controller_id_t>& list) const;
   virtual bool                  controller_exists(ctrlm_controller_id_t controller_id);
   virtual std::vector<ctrlm_obj_controller_t *> get_controller_obj_list() const;

   bool                          getControllerId(unsigned long long ieee_address, ctrlm_controller_id_t *controller_id);

   void                          populate_rcu_status_message(ctrlm_iarm_RcuStatus_params_t *status);
   void                          printStatus();
   virtual void                  factory_reset();

   void                          power_state_change(gboolean waking_up);
   void                          thread_monitor_poll(ctrlm_thread_monitor_response_t *response) override;

   std::shared_ptr<ConfigSettings> getConfigSettings();

private:
   ctrlm_obj_network_ble_t();

   bool                                      controller_is_bound(ctrlm_controller_id_t controller_id) const;
   void                                      controller_remove(ctrlm_controller_id_t controller_id);
   ctrlm_controller_id_t                     controller_add(ctrlm_hal_ble_rcu_data_t &rcu_data);
   ctrlm_controller_id_t                     controller_id_assign(void);
   ctrlm_controller_id_t                     get_last_used_controller(void);
   bool                                      end_voice_session_for_controller(uint64_t ieee_address, ctrlm_voice_session_end_reason_t reason, uint32_t audioDuration = 0);

   json_t *                                  json_config_               = NULL;
   bool                                      upgrade_in_progress_       = false;
   bool                                      unpair_on_remote_request_  = true;
   ctrlm_ble_unpair_metrics_t                last_rcu_unpair_metrics_;

   std::map <ctrlm_controller_id_t, ctrlm_obj_controller_ble_t *> controllers_;
   std::map <std::string, ctrlm_ble_upgrade_image_info_t>         upgrade_images_;
   ctrlm_ir_rf_db_t                                               ir_rf_database_;

   std::shared_ptr<bool> is_alive_;
   std::shared_ptr<ctrlm_ble_rcu_interface_t> ble_rcu_interface_;
   std::shared_ptr<ConfigSettings> m_config;
};

// End Class ctrlm_obj_network_ble_t

#endif
