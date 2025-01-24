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

#ifndef _CTRLM_RF4CE_NETWORK_H_
#define _CTRLM_RF4CE_NETWORK_H_

#include <string>
#include <map>
#include <vector>
#include <semaphore.h>
#if CTRLM_HAL_RF4CE_API_VERSION >= 15 && !defined(CTRLM_HOST_DECRYPTION_NOT_SUPPORTED)
#include <openssl/ssl.h>
#endif
#include "ctrlm.h"
#include "ctrlm_network.h"
#include "ctrlm_rf4ce_controller.h"
#include "ctrlm_rf4ce_utils.h"
#include "ctrlm_device_update.h"
#include "json_config.h"
#include "ctrlm_ir_rf_db.h"
#include "network/attributes/ctrlm_rf4ce_network_attr_config.h"
#include "ctrlm_rfc.h"

#define CTRLM_RF4CE_AUTOBIND_OCTET       ((JSON_INT_VALUE_NETWORK_RF4CE_AUTOBIND_CONFIG_QTY_FAIL << 3) | JSON_INT_VALUE_NETWORK_RF4CE_AUTOBIND_CONFIG_QTY_PASS)
#define CTRLM_RF4CE_AUTOBIND_OCTET_RESET (0x40 | (JSON_INT_VALUE_NETWORK_RF4CE_AUTOBIND_CONFIG_QTY_FAIL << 3) | JSON_INT_VALUE_NETWORK_RF4CE_AUTOBIND_CONFIG_QTY_PASS)
#define CTRLM_RF4CE_AUTOBIND_OCTET_MENU  ((JSON_INT_VALUE_NETWORK_RF4CE_BINDING_MENU_MODE_AUTOBIND_CONFIG_QTY_FAIL << 3) | JSON_INT_VALUE_NETWORK_RF4CE_BINDING_MENU_MODE_AUTOBIND_CONFIG_QTY_PASS)

#define CTRLM_RF4CE_MAX_DEVICE_QTY (8)
#define CTRLM_RF4CE_MAX_PAYLOAD_LEN (95)

#define MSO_VOICE_CMD_ID_VOICE_SESSION_REQUEST  (0x01)
#define MSO_VOICE_CMD_ID_VOICE_SESSION_RESPONSE (0x02)
#define MSO_VOICE_CMD_ID_CHANNEL_CHECK_REQUEST  (0x03)
#define MSO_VOICE_CMD_ID_VOICE_SESSION_STOP     (0x04)
#define MSO_VOICE_CMD_ID_DATA_ADPCM_BEGIN       (0x20)
#define MSO_VOICE_CMD_ID_DATA_ADPCM_END         (0x3F)

#define CTRLM_RF4CE_DISCOVERY_USER_STRING_EXPIRATION_TIME_MS (300000) // 5 min in msec
#define CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS             (2000)   // 2 sec in msec
#ifdef ASB
#define CTRLM_RF4CE_DISCOVERY_ASB_EXPIRATION_TIME_MS         (2000)    // 250ms
#define CTRLM_RF4CE_DISCOVERY_ASB_OCTET_ENABLED              (0x80)
#endif

#define IR_RF_DATABASE_STATUS_FORCE_DOWNLOAD            (0x01)
#define IR_RF_DATABASE_STATUS_DOWNLOAD_TV_5_DIGIT_CODE  (0x02)
#define IR_RF_DATABASE_STATUS_DOWNLOAD_AVR_5_DIGIT_CODE (0x04)
#define IR_RF_DATABASE_STATUS_TX_IR_DESCRIPTOR          (0x08)
#define IR_RF_DATABASE_STATUS_CLEAR_ALL_5_DIGIT_CODES   (0x10)
#define IR_RF_DATABASE_STATUS_DB_DOWNLOAD_NO            (0x40)
#define IR_RF_DATABASE_STATUS_DB_DOWNLOAD_YES           (0x80)
#define IR_RF_DATABASE_STATUS_RESERVED                  (0x20)

typedef enum {
   CTRLM_RF4CE_DEVICE_TYPE_STB         = 0x09,
   CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND    = 0xD0,
   CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND = 0xD1,
   CTRLM_RF4CE_DEVICE_TYPE_ANY         = 0xFF
} ctrlm_rf4ce_device_type_t;

typedef enum {
   RF4CE_FRAME_CONTROL_USER_CONTROL_PRESSED      = 0x01,
   RF4CE_FRAME_CONTROL_USER_CONTROL_REPEATED     = 0x02,
   RF4CE_FRAME_CONTROL_USER_CONTROL_RELEASED     = 0x03,
   RF4CE_FRAME_CONTROL_CHECK_VALIDATION_REQUEST  = 0x20,
   RF4CE_FRAME_CONTROL_CHECK_VALIDATION_RESPONSE = 0x21,
   RF4CE_FRAME_CONTROL_SET_ATTRIBUTE_REQUEST     = 0x22,
   RF4CE_FRAME_CONTROL_SET_ATTRIBUTE_RESPONSE    = 0x23,
   RF4CE_FRAME_CONTROL_GET_ATTRIBUTE_REQUEST     = 0x24,
   RF4CE_FRAME_CONTROL_GET_ATTRIBUTE_RESPONSE    = 0x25,
   RF4CE_FRAME_CONTROL_KEY_COMBO                 = 0x30,
   RF4CE_FRAME_CONTROL_GHOST_CODE                = 0x31,
   RF4CE_FRAME_CONTROL_HEARTBEAT                 = 0x32,
   RF4CE_FRAME_CONTROL_HEARTBEAT_RESPONSE        = 0x33,
   RF4CE_FRAME_CONTROL_CONFIGURATION_COMPLETE    = 0x34
} ctrlm_rf4ce_frame_control_t;

typedef enum {
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_CHECK_REQUEST     = 0x01,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_CHECK_RESPONSE    = 0x02,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_AVAILABLE         = 0x03,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_DATA_REQUEST      = 0x04,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_DATA_RESPONSE     = 0x05,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_LOAD_REQUEST      = 0x06,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_LOAD_RESPONSE     = 0x07,
   RF4CE_DEVICE_UPDATE_CMD_IMAGE_DOWNLOAD_COMPLETE = 0x08
} ctrlm_rf4ce_device_update_cmd_t;

typedef enum {
   RF4CE_AUDIO_FORMAT_ADPCM_16K      = 0x00,
   RF4CE_AUDIO_FORMAT_PCM_16K        = 0x01,
   RF4CE_AUDIO_FORMAT_OPUS_16K       = 0x02,
   RF4CE_AUDIO_FORMAT_INVALID        = 0x03
} ctrlm_rf4ce_audio_format_t;

typedef struct {
   sem_t *                            semaphore;
   ctrlm_hal_rf4ce_rsp_discovery_t    cb;
   void *                             cb_data;
   ctrlm_hal_rf4ce_ind_disc_params_t  params;
   ctrlm_hal_rf4ce_rsp_disc_params_t *rsp_params;
} ctrlm_main_queue_msg_rf4ce_ind_discovery_t;

typedef struct {
   sem_t *                            semaphore;
   ctrlm_hal_rf4ce_rsp_pair_t         cb;
   void *                             cb_data;
   ctrlm_hal_rf4ce_ind_pair_params_t  params;
   ctrlm_hal_rf4ce_rsp_pair_params_t *rsp_params;
} ctrlm_main_queue_msg_rf4ce_ind_pair_t;

typedef struct {
   ctrlm_controller_id_t         controller_id;
   unsigned long long            ieee_address;
   ctrlm_hal_result_pair_t       result;
} ctrlm_main_queue_msg_rf4ce_ind_pair_result_t;

typedef struct {
   sem_t *                              semaphore;
   ctrlm_hal_rf4ce_rsp_unpair_t         cb;
   void *                               cb_data;
   ctrlm_hal_rf4ce_ind_unpair_params_t  params;
   ctrlm_hal_rf4ce_rsp_unpair_params_t *rsp_params;
} ctrlm_main_queue_msg_rf4ce_ind_unpair_t;

typedef struct {
   ctrlm_controller_id_t         controller_id;
   ctrlm_timestamp_t             timestamp;
   ctrlm_hal_rf4ce_profile_id_t  profile_id;
   unsigned char                 length;
   unsigned char                 data[CTRLM_RF4CE_MAX_PAYLOAD_LEN];
} ctrlm_main_queue_msg_rf4ce_ind_data_t;

typedef struct {
   ctrlm_controller_id_t             controller_id;
   guint8                            action;
   char                              data[5];
} ctrlm_main_queue_msg_rf4ce_polling_action_t;

typedef struct {
   ctrlm_main_queue_msg_header_t         header;
   ctrlm_controller_id_t                 controller_id;
} ctrlm_main_queue_msg_bind_validation_failed_timeout_t;

typedef struct {
   ctrlm_hal_rf4ce_result_t              result;
   ctrlm_timestamp_t                     timestamp;
} ctrlm_main_queue_msg_voice_session_response_confirm_t;

typedef struct {
   ctrlm_controller_id_t controller_id;
   ctrlm_key_code_t      key_code;
} ctrlm_main_queue_msg_rf4ce_timeout_key_release_t;

typedef struct {
   bool enabled;
   bool require_line_of_sight;
} ctrlm_rf4ce_discovery_config_t;

typedef struct {
   bool     enabled;
   guchar   threshold_pass;
   guchar   threshold_fail;
   guchar   octet;
} ctrlm_rf4ce_autobind_config_t;

typedef struct {
   guint  disc_respond;
   guint  disc_ignore;
} ctrlm_rf4ce_autobind_status_t;

typedef struct {
   unsigned char      rf_channel_number;
   unsigned char      rf_channel_quality;
} ctrlm_rf4ce_rf_channel_info_t;

typedef struct {
   char               user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
   ctrlm_timestamp_t  timestamp;
} ctrlm_rf4ce_user_string_t;
typedef std::map <ctrlm_hal_rf4ce_ieee_address_t, ctrlm_rf4ce_user_string_t> discovered_user_strings_t;

typedef struct {
   ctrlm_timestamp_t  timestamp;
} ctrlm_rf4ce_discovery_deadline_t;
typedef std::map <ctrlm_hal_rf4ce_ieee_address_t, ctrlm_rf4ce_discovery_deadline_t> discovery_deadlines_t;

typedef struct {
  ctrlm_controller_id_t controller_id;
  ctrlm_network_id_t    network_id;
  guint timer_id;
} ctrlm_bind_validation_timeout_t;
typedef std::vector<ctrlm_bind_validation_timeout_t*> ctrlm_bind_validation_failed_timeout_t;

typedef struct {
   bool                               is_blackout_enabled;
   guint                              pairing_fail_threshold;
   guint                              pairing_fail_count;
   guint                              blackout_reboot_threshold;
   guint                              blackout_count;
   guint                              blackout_time;
   guint                              blackout_time_increment;
   guint                              blackout_tag;
   bool                               is_blackout;
   bool                               force_blackout_settings;
} ctrlm_rf4ce_pairing_blackout_t;

typedef struct {
   bool     enabled;
   guint16  mic_delay;
   guint16  mic_duration;
   guint16  sweep_delay;
   guint16  haptic_delay;
   guint16  haptic_duration;
   guint16  reset_delay;
} ctrlm_rf4ce_mfg_test_t;

typedef struct {
   ctrlm_network_id_t    *network_id;
   ctrlm_controller_id_t  controller_id;
   guchar                 response[5];
   guchar                 response_len;
   ctrlm_timestamp_t      timestamp_hal;
   ctrlm_timestamp_t      timestamp_rsp_req;
   ctrlm_timestamp_t      timestamp_begin;
   ctrlm_timestamp_t      timestamp_end;
   guint8                 retries;
} ctrlm_rf4ce_voice_session_rsp_params_t;

typedef void *(*ctrlm_hal_rf4ce_network_main_t)(ctrlm_hal_rf4ce_main_init_t *main_init);

ctrlm_hal_result_t ctrlm_hal_rf4ce_cfm_init(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_cfm_init_params_t params);
ctrlm_hal_result_t ctrlm_hal_rf4ce_ind_discovery(ctrlm_network_id_t id, ctrlm_hal_rf4ce_ind_disc_params_t params, ctrlm_hal_rf4ce_rsp_disc_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_discovery_t cb, void *cb_data);
ctrlm_hal_result_t ctrlm_hal_rf4ce_ind_pair(ctrlm_network_id_t id, ctrlm_hal_rf4ce_ind_pair_params_t params, ctrlm_hal_rf4ce_rsp_pair_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_pair_t cb, void *cb_data);
ctrlm_hal_result_t ctrlm_hal_rf4ce_ind_pair_result(ctrlm_network_id_t id, ctrlm_hal_rf4ce_ind_pair_result_params_t params);
ctrlm_hal_result_t ctrlm_hal_rf4ce_ind_unpair(ctrlm_network_id_t id, ctrlm_hal_rf4ce_ind_unpair_params_t params, ctrlm_hal_rf4ce_rsp_unpair_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_unpair_t cb, void *cb_data);
ctrlm_hal_result_t ctrlm_hal_rf4ce_ind_data(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_hal_rf4ce_ind_data_params_t params);
ctrlm_hal_result_t ctrlm_voice_ind_data_rf4ce(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_timestamp_t timestamp, guchar command_id, unsigned long data_length, guchar *data, ctrlm_hal_rf4ce_data_read_t cb_data_read, void *cb_data_param, unsigned char lqi, ctrlm_hal_frequency_agility_t *frequency_agility);

void ctrlm_rf4ce_polling_action_push(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 action, char* data, size_t len);

class ctrlm_obj_network_rf4ce_t : public ctrlm_obj_network_t
{
public:
   ctrlm_obj_network_rf4ce_t(ctrlm_network_type_t type, ctrlm_network_id_t id, const char *name, gboolean mask_key_codes, json_t *json_obj_net_rf4ce, GThread *original_thread);
   ctrlm_obj_network_rf4ce_t();
   virtual ~ctrlm_obj_network_rf4ce_t();

   // External methods
   ctrlm_hal_result_t                   hal_init_request(GThread *ctrlm_main_thread);
   void                                 hal_init_confirm(ctrlm_hal_rf4ce_cfm_init_params_t params);
   void                                 hal_api_main_set(ctrlm_hal_rf4ce_network_main_t main);
   void                                 hal_rf4ce_api_set(ctrlm_hal_rf4ce_req_pair_t                   pair,
                                                          ctrlm_hal_rf4ce_req_unpair_t                 unpair,
                                                          ctrlm_hal_rf4ce_req_data_t                   data,
                                                          ctrlm_hal_rf4ce_rib_data_import_t            rib_data_import,
                                                          ctrlm_hal_rf4ce_rib_data_export_t            rib_data_export);
   ctrlm_hal_result_t                   req_data(ctrlm_rf4ce_profile_id_t profile_id, ctrlm_controller_id_t controller_id, ctrlm_timestamp_t tx_window_start, unsigned char length, guchar *data, ctrlm_hal_rf4ce_data_read_t cb_data_read, void *cb_data_param, bool tx_indirect=false, bool single_channel=false, ctrlm_hal_rf4ce_cfm_data_t cb_confirm=NULL, void *cb_confirm_param=NULL);

   virtual void                         hal_init_complete();
   ctrlm_hal_result_t                   network_init(GThread *ctrlm_main_thread);
   void                                 network_destroy();
   virtual std::string                  db_name_get() const;
   void                                 discovery_config_get(ctrlm_controller_discovery_config_t *config);
   bool                                 discovery_config_set(ctrlm_controller_discovery_config_t config);
   virtual void                         configure_rib();
   ctrlm_rf4ce_rib_t                   *get_rib();
   void                                 controllers_load();
   bool                                 controller_exists(ctrlm_controller_id_t controller_id);
   bool                                 controller_is_bound(ctrlm_controller_id_t controller_id);
   void                                 controller_set_binding_security_type(ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_security_type_t type);
   void                                 process_event_key(ctrlm_controller_id_t controller_id, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code);
   virtual void                         controller_list_get(std::vector<ctrlm_controller_id_t>& list) const;
   virtual ctrlm_rcu_controller_type_t  ctrlm_controller_type_get(ctrlm_controller_id_t controller_id);
   virtual ctrlm_rcu_binding_type_t     ctrlm_binding_type_get(ctrlm_controller_id_t controller_id);
   virtual void                         ctrlm_controller_status_get(ctrlm_controller_id_t controller_id, void *status);
   void                                 pan_id_get(guint16  *pan_id);
   void                                 ieee_address_get(unsigned long long  *ieee_address);
   void                                 short_address_get(ctrlm_hal_rf4ce_short_address_t  *short_address);
   void                                 rf_channel_info_get(ctrlm_rf4ce_rf_channel_info_t *rf_channel_info);
   bool                                 binding_config_set(ctrlm_controller_bind_config_t conf);
   void                                 cs_values_set(const ctrlm_cs_values_t *values, bool db_load);
   void                                 factory_reset();
   void                                 controller_unbind(ctrlm_controller_id_t controller_id, ctrlm_unbind_reason_t reason);
   ctrlm_rf4ce_controller_type_t        controller_type_get(ctrlm_controller_id_t controller_id);
   ctrlm_rf4ce_controller_type_t        controller_type_from_user_string(guchar *user_string);
   const char *                         chipset_get();
   unsigned char                        ctrlm_battery_level_percent(ctrlm_controller_id_t controller_id, unsigned char voltage_loaded);
   bool                                 is_importing_controller() const;

   void                                 ind_process_discovery(void *data, int size);  
   void                                 ind_process_pair(void *data, int size);       
   void                                 ind_process_pair_result(void *data, int size);
   void                                 ind_process_unpair(void *data, int size);     
   void                                 ind_process_data(void *data, int size);
   void                                 ind_process_voice_session_request(void *data, int size);
   void                                 ind_process_voice_session_stop(void *data, int size);
   void                                 ind_process_voice_session_end(void *data, int size);
   void                                 process_voice_controller_metrics(void *data, int size);

   void                                 hal_init_cfm(void *data, int size);

   void                                 process_pair_result(ctrlm_controller_id_t controller_id, unsigned long long ieee_address, ctrlm_hal_result_pair_t result);
   void                                 bind_validation_begin(ctrlm_main_queue_msg_bind_validation_begin_t *dqm);
   void                                 bind_validation_end(ctrlm_main_queue_msg_bind_validation_end_t *dqm);
   bool                                 bind_validation_timeout(ctrlm_controller_id_t controller_id);
   bool                                 remove_validation_failed_controller(ctrlm_controller_id_t controller_id);
   bool                                 analyze_assert_reason(const char *assert_info);

   void                                 power_state_change(gboolean waking_up);

   void                                 rfc_fetch_completed_handler(void);
   void                                 rfc_retrieved_handler(const ctrlm_rfc_attr_t& attr);
   void                                 rsp_time_updated_handler(const ctrlm_rf4ce_rsp_time_t& attr);
   void                                 notify_controllers_polling_configuration(void *data, size_t size);

//   void                                 req_process_voice_settings_update(ctrlm_main_queue_msg_voice_settings_update_t *dqm);
   void req_process_rib_set(void *data, int size);
   void req_process_rib_get(void *data, int size);
   ctrlm_rib_request_cmd_result_t req_process_rib_export(ctrlm_controller_id_t controller_id, uint8_t identifier, unsigned char index, unsigned char length, unsigned char *data);
   void req_process_controller_status(void *dqm, int size);
   void req_process_controller_product_name(void *data, int size);
   void req_process_network_status(void *data, int size);
   void req_process_chip_status(void *data, int size);
   void req_process_controller_link_key(void *data, int size);
   void req_process_dpi_control(void *data, int size);
   void req_process_polling_action_push(void *data, int size);
   virtual ctrlm_controller_status_cmd_result_t  req_process_reverse_cmd(ctrlm_main_queue_msg_rcu_reverse_cmd_t *dqm);


   void                                 controller_bind_update(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_controller_id_t controller_id, ctrlm_hal_rf4ce_result_t &status);

   void                                 recovery_set(ctrlm_recovery_type_t recovery);
   ctrlm_recovery_type_t                recovery_get();
   bool                                 backup_hal_nvm();

   #ifndef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint32                              short_rf_retry_period_get(void);
   guint16                              utterance_duration_max_get(void);
   guchar                               voice_data_retry_max_get(void);
   guchar                               voice_csma_backoff_max_get(void);
   guchar                               voice_data_backoff_exp_min_get(void);
   guint16                              rib_update_check_interval_get(void);
   guint16                              auto_check_validation_period_get(void);
   guint16                              link_lost_wait_time_get(void);
   guint16                              update_polling_period_get(void);
   guint16                              data_request_wait_time_get(void);
   guint16                              audio_profiles_targ_get(void);
   voice_command_encryption_t           voice_command_encryption_get(void);
   #endif
   guchar                               property_read_ir_rf_database(guchar index, guchar *data, guchar length);
   guchar                               property_write_ir_rf_database(guchar index, guchar *data, guchar length);
   void                                 ir_rf_database_read_from_db();
   void                                 attributes_from_db();
   guchar                               property_read_target_irdb_status(guchar *data, guchar length);
   guchar                               property_write_target_irdb_status(guchar *data, guchar length);
   void                                 push_ir_codes_to_voice_assistants_from_target_irdb_status();
   guchar                               write_target_irdb_status(guchar *data, guchar length);
   gboolean                             target_irdb_status_read_from_db();
   void                                 target_irdb_status_set(ctrlm_rf4ce_controller_irdb_status_t controller_irdb_status);
   guchar                               target_irdb_status_flags_get();
   ctrlm_rf4ce_controller_irdb_status_t most_recent_controller_irdb_status_get();
   virtual void                         disable_hal_calls();

   // Bastille 37 vulnerability public functions
   void                                 blackout_bind_success();
   void                                 blackout_bind_fail();
   void                                 blackout_reset(void *data, int size);
   void                                 blackout_tag_reset();

   json_t *                             xconf_export_controllers();
   void                                 check_if_update_file_still_needed(ctrlm_main_queue_msg_update_file_check_t *msg);
   void                                 voice_command_status_set(void *data, int size);
   gboolean                             mfg_test_enabled();
   guchar                               property_read_mfg_test(guchar *data, guchar length);
   guchar                               property_read_mfg_test_result(guchar *data, guchar length);
   guchar                               property_write_mfg_test(guchar *data, guchar length);
   guchar                               property_write_mfg_test_result(guchar *data, guchar length);
   // Polling Functions
   gboolean                             polling_configuration_by_controller_type(ctrlm_rf4ce_controller_type_t, guint8 *polling_methods, ctrlm_rf4ce_polling_configuration_t *configurations);
   void                                 notify_firmware(ctrlm_rf4ce_controller_type_t controller_type, ctrlm_rf4ce_device_update_image_type_t image_type, bool force_update, version_software_t version_software, version_hardware_t version_hardware_min, version_software_t version_bootloader_min);
   void                                 polling_action_push(void *data, int size);
   virtual bool                         is_fmr_supported() const;
   // End Polling Functions
#ifdef ASB
   // ASB Functions
   bool                                 rf4ce_asb_init(void *data, int size);
   void                                 asb_configuration(json_config *conf);
   bool                                 is_asb_enabled();
   bool                                 is_asb_force_settings();
   void                                 asb_enable_set(bool asb_enabled);
   asb_key_derivation_method_t          key_derivation_method_get(asb_key_derivation_bitmask_t bitmask_controller);
   void                                 controller_set_key_derivation_method(ctrlm_controller_id_t controller_id, asb_key_derivation_method_t method);
   void                                 asb_link_key_derivation_perform(void *data, int size);
   void                                 asb_link_key_validation_timeout(void *data, int size);
   void                                 rf4ce_asb_destroy(void *data, int size);
   // End ASB Functions
#endif
   void                                 open_chime_enable_set(bool open_chime_enabled);
   bool                                 open_chime_enable_get();
   void                                 close_chime_enable_set(bool close_chime_enabled);
   bool                                 close_chime_enable_get();
   void                                 privacy_chime_enable_set(bool privacy_chime_enabled);
   bool                                 privacy_chime_enable_get();
   void                                 conversational_mode_set(unsigned char conversational_mode);
   unsigned char                        conversational_mode_get();
   void                                 chime_volume_set(ctrlm_chime_volume_t chime_volume);
   ctrlm_chime_volume_t                 chime_volume_get();
   void                                 ir_command_repeats_set(unsigned char ir_command_repeats);
   unsigned char                        ir_command_repeats_get();

   void                                 set_timers();
   void                                 xconf_configuration();
   void                                 update_far_field_configuration(ctrlm_rf4ce_polling_action_t action = RF4CE_POLLING_ACTION_CONFIGURATION);
   guchar                               property_read_far_field_configuration(guchar *data, guchar length);
   guchar                               property_write_far_field_configuration(guchar *data, guchar length);
   guchar                               property_read_dsp_configuration(guchar *data, guchar length);
   guchar                               property_write_dsp_configuration(guchar *data, guchar length);

   void                                 process_controller_voice_metrics(ctrlm_main_queue_msg_controller_voice_metrics_t *dqm);
 
   ctrlm_rf4ce_polling_configuration_t  controller_polling_configuration_heartbeat_get(ctrlm_rf4ce_controller_type_t controller_type);
   ctrlm_rf4ce_polling_generic_config_t controller_generic_polling_configuration_get();

   std::vector<rf4ce_device_update_session_resume_info_t> *device_update_session_resume_list_get();
   guint32                              device_update_session_timeout_get();

   void                                 cfm_voice_session_rsp(void *data, int size);

   void                                 req_process_ir_set_code(void *data, int size);
   void                                 req_process_ir_clear_codes(void *data, int size);
   virtual void                         req_process_start_pairing(void *data, int size) override;
   virtual void                         req_process_find_my_remote(void *data, int size);
   virtual std::vector<ctrlm_obj_controller_t *> get_controller_obj_list() const;
   void                                 rcu_timeout_key_release(void *data, int data_size);

protected:
   virtual gboolean                     key_event_hook(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code);

private:
   ctrlm_hal_rf4ce_network_main_t     hal_api_main_;
   ctrlm_hal_rf4ce_req_pair_t         hal_api_pair_;
   ctrlm_hal_rf4ce_req_unpair_t       hal_api_unpair_;
   ctrlm_hal_rf4ce_req_data_t         hal_api_data_;
   ctrlm_hal_rf4ce_rib_data_import_t  hal_api_rib_data_import_;
   ctrlm_hal_rf4ce_rib_data_export_t  hal_api_rib_data_export_;

   ctrlm_hal_rf4ce_pan_id_t           pan_id_;
   ctrlm_hal_rf4ce_ieee_address_t     ieee_address_;
   ctrlm_hal_rf4ce_short_address_t    short_address_;
   std::string                        user_string_;
   std::string                        chipset_;
   ctrlm_rf4ce_discovery_config_t     discovery_config_normal_;
   ctrlm_rf4ce_discovery_config_t     discovery_config_menu_;
   ctrlm_rf4ce_autobind_status_t      autobind_status_;
   ctrlm_rf4ce_autobind_config_t      autobind_config_normal_;
   ctrlm_rf4ce_autobind_config_t      autobind_config_menu_;
   ctrlm_rf4ce_rib_t                  rib_;
   target_irdb_status_t               target_irdb_status_;
   guchar                             class_inc_line_of_sight_;
   guchar                             class_inc_recently_booted_;
   guchar                             class_inc_binding_button_;
   guchar                             class_inc_xr_;
   guchar                             class_inc_bind_table_empty_;
   guchar                             class_inc_bind_menu_active_;
   guint32                            timeout_key_release_;
   #ifndef CONTROLLER_SPECIFIC_NETWORK_ATTRIBUTES
   guint32                            short_rf_retry_period_;
   guint16                            utterance_duration_max_;
   guchar                             voice_data_retry_max_;
   guchar                             voice_csma_backoff_max_;
   guchar                             voice_data_backoff_exp_min_;
   guint16                            rib_update_check_interval_;
   guint16                            auto_check_validation_period_;
   guint16                            link_lost_wait_time_;
   guint16                            update_polling_period_;
   guint16                            data_request_wait_time_;
   voice_command_encryption_t         voice_command_encryption_;
   bool                               host_decryption_;
   bool                               single_channel_rsp_;
   #endif
   guint16                            audio_profiles_targ_;
   bool                               is_import_;
   ctrlm_controller_id_t              controller_id_to_remove_;
   ctrlm_recovery_type_t              recovery_;
   guchar                            *nvm_backup_data_;
   gulong                             nvm_backup_len_;

   ctrlm_hal_network_property_network_stats_t network_stats_cache;
   bool                                       network_stats_is_cached;

   std::map <ctrlm_controller_id_t, ctrlm_obj_controller_rf4ce_t *> controllers_;
   discovered_user_strings_t         discovered_user_strings_;
   discovery_deadlines_t             discovery_deadlines_autobind_;
   discovery_deadlines_t             discovery_deadlines_screen_bind_;
   ctrlm_bind_validation_failed_timeout_t bind_validation_failed_timeout_;

   ctrlm_ir_rf_db_t                  ir_rf_database_;

   // Bastille 37 vulnerability attribute
   ctrlm_rf4ce_pairing_blackout_t    blackout_;

   ctrlm_rf4ce_mfg_test_t              mfg_test_;

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 35
   volatile gint              binding_in_progress_;
#else
   gint                       binding_in_progress_;
#endif
   guint                               binding_in_progress_tag_;
   guint                               binding_in_progress_timeout_;

#if (CTRLM_HAL_RF4CE_API_VERSION >= 11)
   ctrlm_hal_rf4ce_deepsleep_arguments_t *dpi_args_;
#endif

   // Polling Variables - Array, one for each remote type
   guint8                                  controller_polling_methods_[RF4CE_CONTROLLER_TYPE_INVALID];
   ctrlm_rf4ce_polling_configuration_t     controller_polling_configuration_heartbeat_[RF4CE_CONTROLLER_TYPE_INVALID];
   ctrlm_rf4ce_polling_configuration_t     controller_polling_configuration_mac_[RF4CE_CONTROLLER_TYPE_INVALID];
   ctrlm_rf4ce_polling_generic_config_t    controller_generic_polling_configuration_;
   // End Polling Variables

#ifdef ASB
   // ASB Variables
   bool                                    asb_enabled_;
   asb_key_derivation_bitmask_t            asb_key_derivation_methods_;
   gint                                    asb_fallback_count_;
   gint                                    asb_fallback_count_threshold_;
   discovery_deadlines_t                   asb_discovery_deadlines_;
   bool                                    asb_force_settings_;
   // End ASB Variables
#endif

   ctrlm_voice_session_rsp_confirm_t       voice_session_rsp_confirm_;
   void *                                  voice_session_rsp_confirm_param_;
   ctrlm_rf4ce_voice_session_rsp_params_t  voice_session_rsp_params_;
   guint32                                 voice_session_active_count_;
   voice_session_response_stream_t         stream_begin_;
   gint16                                  stream_offset_;

   guint32                                 device_update_session_timeout_;

   far_field_configuration_t               ff_configuration_;
   dsp_configuration_t                     dsp_configuration_;
   bool                                    force_dsp_configuration_;
   std::vector<ctrlm_controller_id_t>      reverse_cmd_end_event_pending_;
   std::vector<ctrlm_controller_id_t>      reverse_cmd_not_found_event_pending_;
   sem_t                                   reverse_cmd_event_pending_semaphore_;
   guint                                   reverse_cmd_end_event_timer_id_;
   unsigned short                          chime_timeout_;
   guint8                                  polling_methods_;
   guint                                   response_idle_time_ff_;
   guint32                                 max_fmr_controllers_;
   bool                                    ir_rf_database_new_;
#if CTRLM_HAL_RF4CE_API_VERSION >= 15 && !defined(CTRLM_HOST_DECRYPTION_NOT_SUPPORTED)
   EVP_CIPHER_CTX*                         ctx_;
#endif
   static ctrlm_obj_network_rf4ce_t       *instance;
   ctrlm_rf4ce_rsp_time_t                  rsp_time_;

   static gboolean       bind_validation_failed(gpointer user_data);
   gboolean              load_config(json_t *json_obj_net_rf4ce);
   void                  process_discovery_stb(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params);
   void                  process_discovery_autobind(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params);
   void                  process_discovery_screen_bind(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params);
   ctrlm_controller_id_t controller_id_assign(void);
   ctrlm_controller_id_t controller_id_get_by_ieee(unsigned long long ieee_address);
   ctrlm_controller_id_t controller_id_get_last_recently_used(void);
   void                  controller_import(ctrlm_controller_id_t controller_id, unsigned long long ieee_address, bool validated);
   void                  controller_insert(ctrlm_controller_id_t controller_id, unsigned long long ieee_address, bool db_create);
   void                  controller_user_string_set(ctrlm_controller_id_t controller_id, guchar *user_string);
   void                  controller_autobind_in_progress_set(ctrlm_controller_id_t controller_id, bool in_progress);
   bool                  controller_autobind_in_progress_get(ctrlm_controller_id_t controller_id);
   void                  controller_binding_button_in_progress_set(ctrlm_controller_id_t controller_id, bool in_progress);
   bool                  controller_binding_button_in_progress_get(ctrlm_controller_id_t controller_id);
   void                  controller_screen_bind_in_progress_set(ctrlm_controller_id_t controller_id, bool in_progress);
   bool                  controller_screen_bind_in_progress_get(ctrlm_controller_id_t controller_id);
   gint                 *controller_binding_in_progress_get_pointer();
   void                  controller_binding_in_progress_tag_reset();
   void                  controller_stats_update(ctrlm_controller_id_t controller_id);
   void                  controller_remove(ctrlm_controller_id_t controller_id, bool db_destroy);
   void                  controller_backup(ctrlm_controller_id_t controller_id, void *data);
   void                  controller_restore(ctrlm_controller_id_t controller_id);
   ctrlm_hal_result_t    controller_unpair(ctrlm_controller_id_t controller_id);
   gboolean              is_attribute_network_wide(ctrlm_rf4ce_rib_attr_id_t attribute_id);
   gboolean              rf4ce_rib_set_target(ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guchar length, guchar *data, gboolean *rib_entries_updated);
   gboolean              is_xr11_hardware_version(version_hardware_t version_hardware);
   gboolean              is_xr15_hardware_version(version_hardware_t version_hardware);
   gboolean              is_autobind_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address);
   gboolean              is_voice_session_in_progress();
   bool                  is_key_adjacent(ctrlm_controller_id_t controller_id, unsigned long key_code);

   unsigned int          restore_hal_nvm(unsigned char **nvm_backup_data);
   gboolean              is_screen_bind_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address);
   // Bastille 37 vulnerability private function
   gboolean              blackout_settings_get_rfc();
   static gboolean       binding_in_progress_timeout(gpointer user_data);
   void                  default_polling_configuration();
   void                  polling_config_read(json_config *conf);
   void                  polling_config_tr181_read();
   void                  process_xconf();

   void                  dsp_configuration_xconf();

#ifdef ASB
   gboolean              is_asb_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address);
   static gboolean       asb_link_validation_timeout(gpointer user_data);
#endif

   bool                  reverse_cmd_begin_event(void*, int data_size);
   bool                  reverse_cmd_end_event(void*, int data_size);

   template<bool (ctrlm_obj_network_rf4ce_t::*event_func)(void*,int)>
   static gboolean       reverse_cmd_event_timer_proc(gpointer user_data);
   void                  indirect_tx_interval_set();

   void                  ind_process_pair_stb(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status);
   void                  ind_process_pair_autobind(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status);
   void                  ind_process_pair_binding_button(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status);
   void                  ind_process_pair_screen_bind(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status);
   void                  ind_process_data_rcu(ctrlm_main_queue_msg_rf4ce_ind_data_t *dqm);
   void                  ind_process_data_voice(ctrlm_main_queue_msg_rf4ce_ind_data_t *dqm);
   void                  ind_process_data_device_update(ctrlm_main_queue_msg_rf4ce_ind_data_t *dqm);
#if CTRLM_HAL_RF4CE_API_VERSION >= 15 && !defined(CTRLM_HOST_DECRYPTION_NOT_SUPPORTED)
   bool                  sec_init();
   void                  sec_deinit();
   ctrlm_hal_result_t        hal_rf4ce_decrypt(ctrlm_hal_rf4ce_decrypt_params_t* param);
   static ctrlm_hal_result_t hal_rf4ce_decrypt_callback(ctrlm_hal_rf4ce_decrypt_params_t* param);
#endif
   void                  controller_init_uinput(ctrlm_controller_id_t controller_id);
};
#endif
