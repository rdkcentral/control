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
#ifndef _CTRLM_RCU_H_
#define _CTRLM_RCU_H_

#include "ctrlm_ipc_rcu.h"

typedef enum {
   CTRLM_CONTROLLER_STATUS_REQUEST_PENDING = 0,
   CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS = 1,
   CTRLM_CONTROLLER_STATUS_REQUEST_ERROR   = 2
} ctrlm_controller_status_cmd_result_t;

typedef enum {
   CTRLM_RIB_REQUEST_PENDING = 0,
   CTRLM_RIB_REQUEST_SUCCESS = 1,
   CTRLM_RIB_REQUEST_ERROR   = 2
} ctrlm_rib_request_cmd_result_t;

typedef struct {
   ctrlm_controller_id_t                 controller_id;
   ctrlm_controller_status_t *           status;
   sem_t *                               semaphore;
   ctrlm_controller_status_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_controller_status_t;

typedef struct {
   ctrlm_controller_id_t                 controller_id;
   unsigned char *                       link_key;
   sem_t *                               semaphore;
   ctrlm_controller_status_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_controller_link_key_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
   sem_t *                               semaphore;
   ctrlm_rcu_controller_type_t *         controller_type;
   ctrlm_controller_status_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_controller_type_get_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
   sem_t *                               semaphore;
   unsigned char                         loaded_voltage;
   unsigned char *                       battery_percent;
   ctrlm_controller_status_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_battery_percent_get_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
   sem_t *                               semaphore;
   ctrlm_controller_status_cmd_result_t  *cmd_result;
   unsigned long                         total_size;  ///< ctrlm_main_queue_msg_rcu_reverse_cmd_t + data size
   ctrlm_main_iarm_call_rcu_reverse_cmd_t reverse_command;
} ctrlm_main_queue_msg_rcu_reverse_cmd_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
   unsigned char                         attribute_id;
   unsigned char                         attribute_index;
   unsigned char                         length;
   unsigned char *                       length_out;
   unsigned char *                       data;
   sem_t *                               semaphore;
   ctrlm_rib_request_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_rib_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
   guint8                                action;
   char                                  data[CTRLM_RCU_POLLING_RESPONSE_DATA_LEN];
   sem_t *                               semaphore;
   ctrlm_controller_status_cmd_result_t  *cmd_result;
} ctrlm_main_queue_msg_rcu_polling_action_t;

typedef struct {
   ctrlm_pairing_modes_t mode;
   union {
      struct {
         bool    enable;
         uint8_t pass_threshold;
         uint8_t fail_threshold;
      } autobind;
   } data;
} ctrlm_controller_bind_config_t;

typedef struct {
   bool enabled;
   bool require_line_of_sight;
} ctrlm_controller_discovery_config_t;

typedef struct {
   bool                 asb_enable;
   bool                 chime_open_enable;
   bool                 chime_close_enable;
   bool                 chime_privacy_enable;
   uint8_t              conversational_mode;
   ctrlm_chime_volume_t chime_volume;
   uint8_t              ir_repeats;
} ctrlm_cs_values_t;


#ifdef __cplusplus
extern "C"
{
#endif

void     ctrlm_rcu_init(void);
gboolean ctrlm_rcu_init_iarm(void);
void     ctrlm_rcu_terminate(void);
void     ctrlm_rcu_terminate_iarm(void);
void     ctrlm_rcu_iarm_event_key_press(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code);
void     ctrlm_rcu_iarm_event_key_press_validation(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code);
void     ctrlm_rcu_iarm_event_function(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_function_t function, unsigned long value);
void     ctrlm_rcu_iarm_event_key_ghost(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_remote_keypad_config remote_keypad_config, ctrlm_rcu_ghost_code_t ghost_code);
void     ctrlm_rcu_iarm_event_control(int controller_id, const char *event_source, const char *event_type, const char *event_data, int event_value, int spare_value);
void     ctrlm_rcu_iarm_event_rib_access_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_rib_attr_id_t identifier, guchar index, ctrlm_access_type_t access_type);
void     ctrlm_rcu_iarm_event_battery_milestone(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_battery_event_t battery_event, guchar percent);
void     ctrlm_rcu_iarm_event_remote_reboot(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar voltage, controller_reboot_reason_t reason, guint32 assert_number);
void     ctrlm_rcu_iarm_event_reverse_cmd(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_main_iarm_event_t event, ctrlm_rcu_reverse_cmd_result_t result, int result_data_size, const unsigned char* result_data);

gboolean ctrlm_rcu_validation_finish(ctrlm_rcu_iarm_call_validation_finish_t *params);
gboolean ctrlm_rcu_controller_status(ctrlm_rcu_iarm_call_controller_status_t *params);
gboolean ctrlm_rcu_rib_request_get(ctrlm_rcu_iarm_call_rib_request_t *params);
gboolean ctrlm_rcu_rib_request_set(ctrlm_rcu_iarm_call_rib_request_t *params);
gboolean ctrlm_rcu_controller_link_key(ctrlm_rcu_iarm_call_controller_link_key_t *params);
gboolean ctrlm_rcu_reverse_cmd(ctrlm_main_iarm_call_rcu_reverse_cmd_t *params);
gboolean ctrlm_rcu_controller_type_get(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t *type);
gboolean ctrlm_rcu_rf4ce_polling_action(ctrlm_rcu_iarm_call_rf4ce_polling_action_t *params);

#ifdef __cplusplus
}
#endif

#endif
