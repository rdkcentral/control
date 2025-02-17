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
#ifndef _CTRLM_PRIVATE_H_
#define _CTRLM_PRIVATE_H_

#include <glib.h>
#include <gio/gio.h>
#include <semaphore.h>
#include <vector>
#include <string>
#include <memory>
#include <string.h>
#include "ctrlm_hal.h"
#include "ctrlm_hal_rf4ce.h"
#include "ctrlm_hal_ble.h"
#include "ctrlm_hal_ip.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_rcu.h"
#include "ctrlm_ipc_voice.h"
#include "ctrlm_ipc_device_update.h"
#include "libIBus.h"
#include "safec_lib.h"

#define CTRLM_MAIN_QUEUE_MSG_TYPE_GLOBAL (0x100)
#define CTRLM_THREAD_SYNC_DELAY          (10000)
#define JSON_PATH_SEPERATOR "."

#define CTRLM_JSON_ARRAY_INDEX_PRD (0)
#define CTRLM_JSON_ARRAY_INDEX_DEV (1)

class ctrlm_obj_network_t;
class ctrlm_obj_network_rf4ce_t;
class ctrlm_obj_controller_t;
class ctrlm_voice_t;
class ctrlm_voice_endpoint_t;
class ctrlm_irdb_t;
class ctrlm_auth_t;
class ctrlm_telemetry_t;
typedef enum {
   CTRLM_THREAD_MONITOR_RESPONSE_DEAD  = 0,
   CTRLM_THREAD_MONITOR_RESPONSE_ALIVE = 1
} ctrlm_thread_monitor_response_t;

typedef enum {
   // Network based messages
   CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_BEGIN                 = 0,
   CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_END,
   CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_CONFIGURATION_COMPLETE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_NETWORK_PROPERTY_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_FAILED_TIMEOUT,
   CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE_VOICE_SESSION,
   CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE_GET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_REVERSE_CMD,
   CTRLM_MAIN_QUEUE_MSG_TYPE_RCU_POLLING_ACTION,
   // End network based messages

   // Vendor messages
   CTRLM_MAIN_QUEUE_MSG_TYPE_VENDOR_FIRST                          = 150,
   CTRLM_MAIN_QUEUE_MSG_TYPE_VENDOR_LAST                           = (CTRLM_MAIN_QUEUE_MSG_TYPE_GLOBAL - 1),
   // End vendor messages

   // Global messages
   CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE                             = CTRLM_MAIN_QUEUE_MSG_TYPE_GLOBAL + 1,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STATUS,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_GET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_CONFIG_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_AUTOBIND_CONFIG_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PRECOMMISSION_CONFIG_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_FACTORY_RESET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROLLER_UNBIND,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_LINE_OF_SIGHT,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_AUTOBIND,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_BINDING_BUTTON,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_BUTTON,
   CTRLM_MAIN_QUEUE_MSG_TYPE_DPI_CONTROL,
   CTRLM_MAIN_QUEUE_MSG_TYPE_CHECK_UPDATE_FILE_NEEDED,
   CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_AUTHSERVICE_POLL,
   CTRLM_MAIN_QUEUE_MSG_TYPE_NOTIFY_FIRMWARE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_IR_REMOTE_USAGE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_LAST_KEY_INFO,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_SCREEN,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_ONE_TOUCH_AUTOBIND,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_BIND_STATUS_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_REMOTE_TYPE_SET,
   CTRLM_MAIN_QUEUE_MSG_TYPE_PAIRING_METRICS,
   CTRLM_MAIN_QUEUE_MSG_TYPE_BATTERY_MILESTONE_EVENT,
   CTRLM_MAIN_QUEUE_MSG_TYPE_REMOTE_REBOOT_EVENT,
   CTRLM_MAIN_QUEUE_MSG_TYPE_NTP_CHECK,
   CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER,
   CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_THREAD_MONITOR_POLL,
   CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_START,
   CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_STOP,
   CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST,
   CTRLM_MAIN_QUEUE_MSG_TYPE_ACCOUNT_ID_UPDATE,
   CTRLM_MAIN_QUEUE_MSG_TYPE_STARTUP,
   CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER_NEW
   // End global messages
} ctrlm_main_queue_msg_type_t;

typedef enum
{
   CTRLM_REMOTE_KEYPAD_CONFIG_HAS_SETUP_KEY_WITH_NUMBER_KEYS,
   CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NUMBER_KEYS,
   CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NO_NUMBER_KEYS,
   CTRLM_REMOTE_KEYPAD_CONFIG_NA,
   CTRLM_REMOTE_KEYPAD_CONFIG_VOICE_ASSISTANT,
   CTRLM_REMOTE_KEYPAD_CONFIG_INVALID,
} ctrlm_remote_keypad_config;

typedef enum
{
   CTRLM_CLOSE_PAIRING_WINDOW_REASON_PAIRING_SUCCESS,
   CTRLM_CLOSE_PAIRING_WINDOW_REASON_END,
   CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT,
} ctrlm_close_pairing_window_reason;

typedef enum {
   CTRLM_DEVICE_TYPE_STB_IP  = 0,
   CTRLM_DEVICE_TYPE_STB_QAM = 1,
   CTRLM_DEVICE_TYPE_TV      = 2,
   CTRLM_DEVICE_TYPE_INVALID = 3
} ctrlm_device_type_t;

typedef struct {
   ctrlm_main_queue_msg_type_t type;
   ctrlm_network_id_t          network_id;
} ctrlm_main_queue_msg_header_t;

typedef struct {
   ctrlm_main_queue_msg_type_t      type;
   ctrlm_network_id_t               network_id;
   ctrlm_obj_network_t             *obj_network;
   ctrlm_thread_monitor_response_t *response;
} ctrlm_thread_monitor_msg_t;

typedef enum {
   CTRLM_MAIN_STATUS_REQUEST_PENDING = 0,
   CTRLM_MAIN_STATUS_REQUEST_SUCCESS = 1,
   CTRLM_MAIN_STATUS_REQUEST_ERROR   = 2
} ctrlm_main_status_cmd_result_t;

typedef struct {
   ctrlm_main_queue_msg_header_t   header;
   ctrlm_main_iarm_call_status_t * status;
   sem_t *                         semaphore;
   ctrlm_main_status_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_main_status_t;

typedef struct {
   ctrlm_main_iarm_call_network_status_t *status;
   sem_t *                                semaphore;
   ctrlm_main_status_cmd_result_t *       cmd_result;
} ctrlm_main_queue_msg_main_network_status_t;

typedef struct {
   ctrlm_main_iarm_call_chip_status_t     *status;
   sem_t *                                semaphore;
} ctrlm_main_queue_msg_main_chip_status_t;

typedef struct {
   ctrlm_main_queue_msg_header_t    header;
   ctrlm_main_iarm_call_property_t *property;
   sem_t *                          semaphore;
   ctrlm_main_status_cmd_result_t * cmd_result;
} ctrlm_main_queue_msg_main_property_t;

typedef struct {
   ctrlm_main_queue_msg_header_t            header;
   ctrlm_main_iarm_call_discovery_config_t *config;
   sem_t *                                  semaphore;
   ctrlm_main_status_cmd_result_t *         cmd_result;
} ctrlm_main_queue_msg_main_discovery_config_t;

typedef struct {
   ctrlm_main_queue_msg_header_t           header;
   ctrlm_main_iarm_call_autobind_config_t *config;
   sem_t *                                 semaphore;
   ctrlm_main_status_cmd_result_t *        cmd_result;
} ctrlm_main_queue_msg_main_autobind_config_t;

typedef struct {
   ctrlm_main_queue_msg_header_t               header;
   ctrlm_main_iarm_call_precommision_config_t *config;
   sem_t *                                     semaphore;
   ctrlm_main_status_cmd_result_t *            cmd_result;
} ctrlm_main_queue_msg_main_precommision_config_t;

typedef struct {
   ctrlm_iarm_call_result_t result;
   ctrlm_network_id_t       network_id;
} ctrlm_main_iarm_call_factory_reset_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_main_iarm_call_factory_reset_t *reset;
   sem_t *                               semaphore;
   ctrlm_main_status_cmd_result_t *      cmd_result;
} ctrlm_main_queue_msg_main_factory_reset_t;

typedef struct {
   ctrlm_main_queue_msg_header_t             header;
   ctrlm_main_iarm_call_controller_unbind_t *unbind;
   sem_t *                                   semaphore;
   ctrlm_main_status_cmd_result_t *          cmd_result;
} ctrlm_main_queue_msg_main_controller_unbind_t;

typedef struct {
   union {
      ctrlm_hal_rf4ce_cfm_init_params_t rf4ce;
      ctrlm_hal_ip_cfm_init_params_t    ip;
      ctrlm_hal_ble_cfm_init_params_t   ble;
   } params;
   sem_t                            *semaphore;
} ctrlm_main_queue_msg_hal_cfm_init_t;

typedef struct {
   ctrlm_controller_id_t             controller_id;
   gboolean                         *controller_exists;
   sem_t                            *semaphore;
} ctrlm_main_queue_msg_controller_exists_t;

typedef struct {
   ctrlm_main_queue_msg_type_t       type;
} ctrlm_main_queue_msg_authservice_poll_t;

typedef struct {
   ctrlm_main_queue_msg_header_t   header;
   ctrlm_controller_id_t           controller_id;
   int                             status;
   guint8                          flags;
   guint8                          value;
} ctrlm_main_queue_msg_voice_cmd_status_t;

typedef struct {
   ctrlm_main_queue_msg_type_t       type;
   ctrlm_controller_id_t             controller_id;
   char                             *product_name;
   sem_t                            *semaphore;
   ctrlm_main_status_cmd_result_t   *cmd_result;
} ctrlm_main_queue_msg_product_name_t;

typedef struct {
   ctrlm_main_queue_msg_type_t             type;
   ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage;
   sem_t                                  *semaphore;
   ctrlm_main_status_cmd_result_t         *cmd_result;
} ctrlm_main_queue_msg_ir_remote_usage_t;

typedef struct {
   ctrlm_main_queue_msg_type_t             type;
   ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics;
   sem_t                                  *semaphore;
   ctrlm_main_status_cmd_result_t         *cmd_result;
} ctrlm_main_queue_msg_pairing_metrics_t;

typedef struct {
   ctrlm_main_queue_msg_type_t            type;
   guint32                               *today;
   ctrlm_main_iarm_call_last_key_info_t  *last_key_info;
   sem_t                                 *semaphore;
   ctrlm_main_status_cmd_result_t        *cmd_result;
} ctrlm_main_queue_msg_last_key_info_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                    header;
   ctrlm_main_iarm_call_control_service_settings_t *settings;
   sem_t *                                          semaphore;
   ctrlm_main_status_cmd_result_t *                 cmd_result;
} ctrlm_main_queue_msg_main_control_service_settings_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                              header;
   ctrlm_main_iarm_call_control_service_can_find_my_remote_t *can_find_my_remote;
   sem_t *                                                    semaphore;
   ctrlm_main_status_cmd_result_t *                           cmd_result;
} ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                        header;
   ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing;
   sem_t *                                              semaphore;
   ctrlm_main_status_cmd_result_t *                     cmd_result;
} ctrlm_main_queue_msg_main_control_service_pairing_mode_t;

typedef struct {
   ctrlm_main_queue_msg_type_t                          type;
   ctrlm_network_id_t                                   network_id;
   ctrlm_close_pairing_window_reason                    reason;
} ctrlm_main_queue_msg_close_pairing_window_t;

typedef struct {
   ctrlm_main_queue_msg_type_t                          type;
   ctrlm_bind_status_t                                  bind_status;
} ctrlm_main_queue_msg_pairing_window_bind_status_t;

typedef struct {
   ctrlm_main_queue_msg_type_t                          type;
   char                                                 remote_type_str[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
} ctrlm_main_queue_msg_discovery_remote_type_t;

typedef struct {
   ctrlm_main_queue_msg_type_t                          type;
   ctrlm_network_id_t                                   network_id;
   ctrlm_controller_id_t                                controller_id;
   ctrlm_rcu_battery_event_t                            battery_event;
   guchar                                               percent;
} ctrlm_main_queue_msg_rf4ce_battery_milestone_t;

typedef struct {
   ctrlm_main_queue_msg_type_t                          type;
   ctrlm_network_id_t                                   network_id;
   ctrlm_controller_id_t                                controller_id;
   guchar                                               voltage;
   controller_reboot_reason_t                           reason;
   guint32                                              assert_number;
} ctrlm_main_queue_msg_remote_reboot_t;

typedef struct {
   ctrlm_controller_id_t                   controller_id;
   uint8_t                                 short_utterance;
   uint32_t                                packets_total;
   uint32_t                                packets_lost;
} ctrlm_main_queue_msg_controller_voice_metrics_t;

typedef struct {
   ctrlm_main_queue_msg_header_t   header;
   char                            account_id[1024]; // We don't know the size contraints for Account ID, but this should be plenty.
} ctrlm_main_queue_msg_account_id_update_t;

typedef enum {
   VOICE_COMMAND_VOICE_SESSION_TERMINATE_SPECIAL_KEY_PRESS = 0,
   VOICE_COMMAND_VOICE_SESSION_TERMINATE_UNKNOWN = 5
} ctrlm_voice_session_termination_reason_t;


typedef struct {
   ctrlm_main_queue_msg_header_t            header;
   ctrlm_controller_id_t                    controller_id;
   ctrlm_voice_session_termination_reason_t reason;
} ctrlm_main_queue_msg_terminate_voice_session_t;

typedef enum {
   CTRLM_HANDLER_NETWORK,
   CTRLM_HANDLER_CONTROLLER,
   CTRLM_HANDLER_VOICE
} ctrlm_handler_type_t;

typedef void (ctrlm_obj_network_t::*ctrlm_msg_handler_network_t)(void*,int);
typedef void (ctrlm_obj_controller_t::*ctrlm_msg_handler_controller_t)(void*,int);
typedef void (ctrlm_voice_endpoint_t::*ctrlm_msg_handler_voice_t)(void*,int);
typedef struct {
   ctrlm_main_queue_msg_header_t  header;
   ctrlm_handler_type_t           type;
   sem_t *                        semaphore;
   void                          *obj;
   union {
      ctrlm_msg_handler_network_t    n;
      ctrlm_msg_handler_controller_t c;
      ctrlm_msg_handler_voice_t      v;
   } msg_handler;
   int                            data_len;
   unsigned char                  data[1];
} ctrlm_main_queue_msg_handler_t;

struct ctrlm_main_queue_msg_handler_new_t {
   ctrlm_main_queue_msg_header_t  header;
   ctrlm_handler_type_t           type;
   sem_t *                        semaphore;
   void                          *obj;
   union {
      ctrlm_msg_handler_network_t    n;
      ctrlm_msg_handler_controller_t c;
      ctrlm_msg_handler_voice_t      v;
   } msg_handler;
   int                            data_len;
   std::shared_ptr<void>          data;
};

typedef struct {
   ctrlm_main_queue_msg_header_t header;
   ctrlm_audio_container_t       container;
   char                          file_path[128];
   bool                          raw_mic_enable;
} ctrlm_main_queue_msg_audio_capture_start_t;

typedef struct {
   ctrlm_main_queue_msg_header_t header;
} ctrlm_main_queue_msg_audio_capture_stop_t;

typedef struct {
   ctrlm_main_queue_msg_header_t header;
   ctrlm_power_state_t           new_state;
} ctrlm_main_queue_power_state_change_t;

typedef struct {
   time_t last_key_time;
   uint16_t last_key_code;
} ctrlm_ir_last_keypress_t;

gboolean                           ctrlm_main_iarm_init(void);
void                               ctrlm_main_iarm_terminate(void);
gboolean                           ctrlm_is_production_build(void);
void                               ctrlm_network_list_get(std::vector<ctrlm_network_id_t> *list);
gboolean                           ctrlm_network_id_is_valid(ctrlm_network_id_t network_id);
gboolean                           ctrlm_controller_id_is_valid(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
ctrlm_network_type_t               ctrlm_network_type_get(ctrlm_network_id_t network_id);
std::vector<ctrlm_network_type_t>  ctrlm_network_types_get(void);
ctrlm_network_id_t                 ctrlm_network_id_get(ctrlm_network_type_t network_type);
gboolean                           ctrlm_precomission_lookup(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
void                               ctrlm_main_queue_msg_push(gpointer msg);
void                               ctrlm_stop_binding_button(void);
void                               ctrlm_stop_binding_screen(void);
void                               ctrlm_stop_one_touch_autobind(void);
void                               ctrlm_close_pairing_window(ctrlm_close_pairing_window_reason reason);
gboolean                           ctrlm_is_recently_booted(void);
gboolean                           ctrlm_is_line_of_sight(void);
gboolean                           ctrlm_is_autobind_active(void);
gboolean                           ctrlm_is_binding_button_pressed(void);
gboolean                           ctrlm_is_binding_screen_active(void);
gboolean                           ctrlm_is_one_touch_autobind_active(void);
gboolean                           ctrlm_is_binding_table_empty(void);
gboolean                           ctrlm_is_binding_table_full(void);
bool                               ctrlm_is_pii_mask_enabled(void);
gboolean                           ctrlm_main_has_receiver_id_get(void);
gboolean                           ctrlm_main_has_device_id_get(void);
gboolean                           ctrlm_main_has_device_type_get(void);
gboolean                           ctrlm_main_has_service_account_id_get(void);
gboolean                           ctrlm_main_has_partner_id_get(void);
gboolean                           ctrlm_main_has_experience_get(void);
gboolean                           ctrlm_main_needs_service_access_token_get(void);
void                               ctrlm_main_invalidate_service_access_token(void);
void                               ctrlm_main_sat_enabled_set(gboolean sat_enabled);
gboolean                           ctrlm_main_successful_init_get(void);
time_t                             ctrlm_shutdown_time_get(void);
gboolean                           ctrlm_pairing_window_active_get(void);
void                               ctrlm_pairing_window_bind_status_set(ctrlm_bind_status_t bind_status);
void                               ctrlm_discovery_remote_type_set(const char *remote_type_str);
ctrlm_pairing_restrict_by_remote_t restrict_pairing_by_remote_get();
void                               ctrlm_event_handler_ir(const char *owner, IARM_EventId_t event_id, void *data, size_t len);
void                               ctrlm_quit_main_loop();
gboolean                           ctrlm_power_state_change(ctrlm_power_state_t power_state);

gboolean ctrlm_main_iarm_call_status_get(ctrlm_main_iarm_call_status_t *status);
gboolean ctrlm_main_iarm_call_network_status_get(ctrlm_main_iarm_call_network_status_t *status);
gboolean ctrlm_main_iarm_call_property_set(ctrlm_main_iarm_call_property_t *property);
void     ctrlm_main_iarm_call_property_set_(ctrlm_main_iarm_call_property_t *property);
gboolean ctrlm_main_iarm_call_property_get(ctrlm_main_iarm_call_property_t *property);
gboolean ctrlm_main_iarm_call_discovery_config_set(ctrlm_main_iarm_call_discovery_config_t *config);
gboolean ctrlm_main_iarm_call_autobind_config_set(ctrlm_main_iarm_call_autobind_config_t *config);
gboolean ctrlm_main_iarm_call_precommission_config_set(ctrlm_main_iarm_call_precommision_config_t *config);
gboolean ctrlm_main_iarm_call_factory_reset(ctrlm_main_iarm_call_factory_reset_t *reset);
gboolean ctrlm_main_iarm_call_controller_unbind(ctrlm_main_iarm_call_controller_unbind_t *unbind);
gboolean ctrlm_main_iarm_call_ir_remote_usage_get(ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage);
gboolean ctrlm_main_iarm_call_pairing_metrics_get(ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics);
gboolean ctrlm_main_iarm_call_last_key_info_get(ctrlm_main_iarm_call_last_key_info_t *last_key_info);
gboolean ctrlm_main_iarm_call_control_service_set_values(ctrlm_main_iarm_call_control_service_settings_t *settings);
gboolean ctrlm_main_iarm_call_control_service_get_values(ctrlm_main_iarm_call_control_service_settings_t *settings);
gboolean ctrlm_main_iarm_call_control_service_can_find_my_remote(ctrlm_main_iarm_call_control_service_can_find_my_remote_t *can_find_my_remote);
gboolean ctrlm_main_iarm_call_control_service_start_pairing_mode(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing);
void     ctrlm_main_iarm_call_control_service_start_pairing_mode_(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing);
gboolean ctrlm_main_iarm_call_control_service_end_pairing_mode(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing);
gboolean ctrlm_main_iarm_call_chip_status_get(ctrlm_main_iarm_call_chip_status_t *status);

ctrlm_power_state_t ctrlm_main_iarm_call_get_power_state(void);
ctrlm_power_state_t ctrlm_main_get_power_state(void);
#ifdef DEEP_SLEEP_ENABLED
gboolean ctrlm_main_iarm_networked_standby(void);
gboolean ctrlm_main_iarm_wakeup_reason_voice(void);
#endif

void        ctrlm_main_iarm_event_binding_line_of_sight(gboolean active);
void        ctrlm_main_iarm_event_autobind_line_of_sight(gboolean active);
void        ctrlm_main_iarm_event_binding_button(gboolean active);
const char* ctrlm_minidump_path_get();
void        ctrlm_on_network_assert(ctrlm_network_id_t network_id);
void        ctrlm_on_network_assert(ctrlm_network_id_t network_id, const char* assert_info);
ctrlm_network_id_t network_id_get_next(ctrlm_network_type_t network_type);
void        ctrlm_update_last_key_info(int controller_id, guchar source_type, guint32 source_key_code, const char *source_name, gboolean is_screen_bind_mode, gboolean write_last_key_info);
ctrlm_irdb_t* ctrlm_main_irdb_get();
ctrlm_auth_t* ctrlm_main_auth_get();
void          ctrlm_main_auth_start_poll();
std::string ctrlm_receiver_id_get();
std::string ctrlm_device_id_get();
std::string ctrlm_stb_name_get();
std::string ctrlm_device_mac_get();
ctrlm_controller_id_t ctrlm_last_used_controller_get(ctrlm_network_type_t network_type);

ctrlm_voice_t *ctrlm_get_voice_obj();
ctrlm_telemetry_t *ctrlm_get_telemetry_obj();

void ctrlm_main_ir_last_keypress_get(ctrlm_ir_last_keypress_t *last_key_info);
std::string ctrlm_main_ir_controller_name_get();


template <typename T>
void ctrlm_main_queue_handler_push(ctrlm_handler_type_t type, T handler, void *data, int size, void *obj = NULL, ctrlm_network_id_t network_id = CTRLM_MAIN_NETWORK_ID_INVALID, bool synchronous = false) {
   int msg_size = sizeof(ctrlm_main_queue_msg_handler_t) + size;
   if (size > 1) {
      --msg_size;
   }
   errno_t safec_rc = -1;
   ctrlm_main_queue_msg_handler_t *msg = (ctrlm_main_queue_msg_handler_t *)g_malloc(msg_size);

   if(NULL == msg) {
      g_assert(0);
      return;
   }
   sem_t semaphore;
   if(synchronous) {
      sem_init(&semaphore, 0, 0);
   }
   safec_rc = memset_s(msg, msg_size, 0, msg_size);
   ERR_CHK(safec_rc);
   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER;
   msg->header.network_id = network_id;
   msg->obj               = obj;
   msg->type              = type;
   msg->semaphore         = synchronous ? &semaphore : NULL;
   switch(type) {
      case CTRLM_HANDLER_NETWORK:    msg->msg_handler.n = (ctrlm_msg_handler_network_t)handler;    break;
      case CTRLM_HANDLER_CONTROLLER: msg->msg_handler.c = (ctrlm_msg_handler_controller_t)handler; break;
      case CTRLM_HANDLER_VOICE:      msg->msg_handler.v = (ctrlm_msg_handler_voice_t)handler;      break;
      default:                       msg->msg_handler.n = NULL;                                    break;
   }
   msg->data_len          = size;
   if (size > 0) {
      safec_rc = memcpy_s(msg->data, msg->data_len, data, size);
      ERR_CHK(safec_rc);
   }
   ctrlm_main_queue_msg_push(msg);

   if(synchronous) { // Wait for the result semaphore to be signaled
      sem_wait(&semaphore);
      sem_destroy(&semaphore);
   }
}

template <typename T1, typename T2>
void ctrlm_main_queue_handler_push_new(ctrlm_handler_type_t type, T1 handler, std::shared_ptr<T2> data, void *obj = NULL, ctrlm_network_id_t network_id = CTRLM_MAIN_NETWORK_ID_INVALID, bool synchronous = false) {
   ctrlm_main_queue_msg_handler_new_t *msg = new (std::nothrow) ctrlm_main_queue_msg_handler_new_t();
   if(NULL == msg) {
      g_assert(0);
      return;
   }
   sem_t semaphore;
   if(synchronous) {
      sem_init(&semaphore, 0, 0);
   }

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER_NEW;
   msg->header.network_id = network_id;
   msg->obj               = obj;
   msg->type              = type;
   msg->semaphore         = synchronous ? &semaphore : NULL;
   msg->data_len          = sizeof(T2);
   msg->data              = data;

   switch(type) {
      case CTRLM_HANDLER_NETWORK:    msg->msg_handler.n = (ctrlm_msg_handler_network_t)handler;    break;
      case CTRLM_HANDLER_CONTROLLER: msg->msg_handler.c = (ctrlm_msg_handler_controller_t)handler; break;
      case CTRLM_HANDLER_VOICE:      msg->msg_handler.v = (ctrlm_msg_handler_voice_t)handler;      break;
      default:                       msg->msg_handler.n = NULL;                                    break;
   }

   ctrlm_main_queue_msg_push(msg);

   if(synchronous) { // Wait for the result semaphore to be signaled
      sem_wait(&semaphore);
      sem_destroy(&semaphore);
   }
}

#endif
