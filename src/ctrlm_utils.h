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
#ifndef _CTRLM_UTILS_H_
#define _CTRLM_UTILS_H_

#include <semaphore.h>
#include <string>
#include <ctrlm.h>
#include <glib.h>
#include <zlib.h>
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_rcu.h"
#include "ctrlm_ipc_voice.h"
#include "ctrlm_ipc_device_update.h"
#include "ctrlm_rcu.h"
#include "ctrlm_hal.h"
#include "ctrlm_hal_rf4ce.h"
#include "ctrlm_irdb.h"
#include "ctrlm_log.h"
#include "libIBus.h"
#include "libIBusDaemon.h"
#include <jansson.h>
#ifdef ENABLE_DEEP_SLEEP
#include "deepSleepMgr.h"
#endif

#define TIMEOUT_TAG_INVALID (0)

#define Q_NOTATION_TO_DOUBLE(input, bits) ((double)((double)input) / (double)(1 << bits))

typedef struct {
   uint32_t voice_cmd_count_today;
   uint32_t voice_cmd_count_yesterday;
   uint32_t voice_cmd_short_today;
   uint32_t voice_cmd_short_yesterday;
   uint32_t voice_packets_sent_today;
   uint32_t voice_packets_sent_yesterday;
   uint32_t voice_packets_lost_today;
   uint32_t voice_packets_lost_yesterday;
   uint32_t utterances_exceeding_packet_loss_threshold_today;
   uint32_t utterances_exceeding_packet_loss_threshold_yesterday;
} ctrlm_voice_util_stats_t;

typedef enum
{
   CTRLM_IR_REMOTE_TYPE_XR11V2,
   CTRLM_IR_REMOTE_TYPE_XR15V1,
   CTRLM_IR_REMOTE_TYPE_NA,
   CTRLM_IR_REMOTE_TYPE_UNKNOWN,
   CTRLM_IR_REMOTE_TYPE_COMCAST,
   CTRLM_IR_REMOTE_TYPE_PLATCO,
   CTRLM_IR_REMOTE_TYPE_XR15V2,
   CTRLM_IR_REMOTE_TYPE_XR16V1,
   CTRLM_IR_REMOTE_TYPE_XRAV1,
   CTRLM_IR_REMOTE_TYPE_XR20V1,
   CTRLM_IR_REMOTE_TYPE_UNDEFINED
} ctrlm_ir_remote_type;

typedef struct {
   const char *   name;
   pthread_t      id;
   bool           running;
} ctrlm_thread_t;

template<typename T>
bool ctrlm_json_to_iarm_call_data_result(json_t *obj, T iarm)
{
    if (!obj || !iarm) {
        XLOGD_ERROR("Null parameter");
        return false;
    }

    char *ret_str = json_dumps(obj, JSON_COMPACT);
    if (!ret_str) {
        XLOGD_ERROR("JSON dump failed");
        return false;
    }

    if(strlen(ret_str) >= sizeof(iarm->result)) {
        XLOGD_ERROR("JSON payload larger than iarm response");
        return false;
    }
    errno_t safec_rc = sprintf_s(iarm->result, sizeof(iarm->result), "%s", ret_str);
    if (safec_rc < EOK) {
        ERR_CHK(safec_rc);
    }

    if(ret_str) {
        free(ret_str);
        ret_str = NULL;
    }

    if (obj) {
        json_decref(obj);
    }
    return true;
}

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef BREAKPAD_SUPPORT
void ctrlm_crash_ctrlm_device_update(void);
#ifdef CTRLM_RF4CE_HAL_QORVO
void ctrlm_crash_rf4ce_qorvo(void);
#else
void ctrlm_crash_rf4ce_ti(void);
#endif
void ctrlm_crash_ble(void);
void ctrlm_crash_vsdk(void);
void ctrlm_crash_ctrlm_main(void);
void ctrlm_crash_ctrlm_database(void);

void ctrlm_crash(void);
#endif

const char *ctrlm_invalid_return(int value);

guint ctrlm_timeout_create(guint timeout, GSourceFunc function, gpointer user_data);
//guint ctrlm_timeout_update(guint timeout_tag, guint timeout);
void ctrlm_timeout_destroy(guint *p_timeout_tag);

void ctrlm_print_data_hex(const char *prefix, guchar *data, unsigned int length, unsigned int width);
void ctrlm_print_controller_status(const char *prefix, ctrlm_controller_status_t *status);

const char *ctrlm_main_queue_msg_type_str(ctrlm_main_queue_msg_type_t type);
const char *ctrlm_controller_status_cmd_result_str(ctrlm_controller_status_cmd_result_t result);


const char *ctrlm_key_status_str(ctrlm_key_status_t key_status);
const char *ctrlm_key_code_str(ctrlm_key_code_t key_code);
const char *ctrlm_linux_key_code_str(uint16_t code, bool mask);
const char *ctrlm_iarm_call_result_str(ctrlm_iarm_call_result_t result);
const char *ctrlm_iarm_result_str(IARM_Result_t result);
const char *ctrlm_access_type_str(ctrlm_access_type_t access_type);
std::string ctrlm_network_type_str(ctrlm_network_type_t network_type);
std::string ctrlm_controller_name_str(ctrlm_controller_id_t controller);
const char *ctrlm_unbind_reason_str(ctrlm_unbind_reason_t reason);

const char *ctrlm_rcu_validation_result_str(ctrlm_rcu_validation_result_t validation_result);
const char *ctrlm_rcu_configuration_result_str(ctrlm_rcu_configuration_result_t configuration_result);
const char *ctrlm_rcu_rib_attr_id_str(ctrlm_rcu_rib_attr_id_t attribute_id);
const char *ctrlm_rcu_binding_type_str(ctrlm_rcu_binding_type_t binding_type);
const char *ctrlm_rcu_validation_type_str(ctrlm_rcu_validation_type_t validation_type);
const char *ctrlm_rcu_binding_security_type_str(ctrlm_rcu_binding_security_type_t security_type);
const char *ctrlm_rcu_ghost_code_str(ctrlm_rcu_ghost_code_t ghost_code);
const char *ctrlm_rcu_function_str(ctrlm_rcu_function_t function);
const char *ctrlm_rcu_controller_type_str(ctrlm_rcu_controller_type_t controller_type);
const char *ctrlm_rcu_reverse_cmd_result_str(ctrlm_rcu_reverse_cmd_result_t result);
const char *ctrlm_rcu_ir_remote_types_str(ctrlm_ir_remote_type controller_type);

const char *ctrlm_voice_session_result_str(ctrlm_voice_session_result_t result);
const char *ctrlm_voice_session_end_reason_str(ctrlm_voice_session_end_reason_t reason);
const char *ctrlm_voice_session_abort_reason_str(ctrlm_voice_session_abort_reason_t reason);
const char *ctrlm_voice_internal_error_str(ctrlm_voice_internal_error_t error);
const char *ctrlm_voice_reset_type_str(ctrlm_voice_reset_type_t reset_type);

const char *ctrlm_device_update_iarm_load_type_str(ctrlm_device_update_iarm_load_type_t load_type);
const char *ctrlm_device_update_iarm_load_result_str(ctrlm_device_update_iarm_load_result_t load_result);
const char *ctrlm_device_update_image_type_str(ctrlm_device_update_image_type_t image_type);
const char *ctrlm_bind_status_str(ctrlm_bind_status_t bind_status);
const char *ctrlm_close_pairing_window_reason_str(ctrlm_close_pairing_window_reason reason);
const char *ctrlm_battery_event_str(ctrlm_rcu_battery_event_t event);
const char *ctrlm_dsp_event_str(ctrlm_rcu_dsp_event_t event);
const char *ctrlm_rf4ce_reboot_reason_str(controller_reboot_reason_t reboot_reason);

const char *ctrlm_ir_state_str(ctrlm_ir_state_t state);

const char *ctrlm_power_state_str(ctrlm_power_state_t state);
const char *ctrlm_device_type_str(ctrlm_device_type_t device_type);

#ifdef ENABLE_DEEP_SLEEP
const char *ctrlm_wakeup_reason_str(DeepSleep_WakeupReason_t wakeup_reason);
#endif
const char *ctrlm_rcu_wakeup_config_str(ctrlm_rcu_wakeup_config_t config);

const char *ctrlm_irdb_vendor_str(ctrlm_irdb_vendor_t vendor);
const char *ctrlm_rf_pair_state_str(ctrlm_rf_pair_state_t state);

bool        ctrlm_file_copy(const char* src, const char* dst, bool overwrite, bool follow_symbolic_link);
bool        ctrlm_file_delete(const char* path, bool follow_symbolic_link);
bool        ctrlm_file_get_symlink_target(const char *path, std::string &link_target);
bool        ctrlm_file_exists(const char* path);
bool        ctrlm_file_timestamp_get(const char *path, guint64 *ts);
bool        ctrlm_file_timestamp_set(const char *path, guint64  ts);

char       *ctrlm_get_file_contents(const char *path);
char       *ctrlm_do_regex(char *re, char *str);

bool        ctrlm_dsmgr_init();
bool        ctrlm_dsmgr_mute_audio(bool mute);
bool        ctrlm_dsmgr_duck_audio(bool enable, bool relative, double vol);
bool        ctrlm_dsmgr_LED(bool on);
bool        ctrlm_dsmgr_deinit();

bool        ctrlm_is_voice_assistant(ctrlm_rcu_controller_type_t controller_type);
ctrlm_remote_keypad_config ctrlm_get_remote_keypad_config(const char *remote_type);

unsigned long long ctrlm_convert_mac_string_to_long (const char* ascii_mac);
std::string        ctrlm_convert_mac_long_to_string (const unsigned long long ieee_address);

bool        ctrlm_archive_extract(const std::string &file_path_archive, const std::string &tmp_dir_path, const std::string &archive_file_name);
void        ctrlm_archive_remove(const std::string &dir);
bool        ctrlm_tar_archive_extract(const std::string &file_path_archive, const std::string &dest_path);
bool        ctrlm_utils_rm_rf(const std::string &path);
void        ctrlm_utils_sem_wait();
void        ctrlm_utils_sem_post();
void        ctrlm_archive_extract_tmp_dir_make(const std::string &tmp_dir_path);
void        ctrlm_archive_extract_ble_tmp_dir_make(const std::string &tmp_dir_path);
bool        ctrlm_archive_extract_ble_check_dir_exists(const std::string &path);
std::string ctrlm_xml_tag_text_get(const std::string &xml, const std::string &tag);

ctrlm_power_state_t ctrlm_iarm_power_state_map(IARM_Bus_PowerState_t iarm_power_state);

bool ctrlm_utils_calc_crc32( const char *filename, uLong *crc_ret );
bool ctrlm_utils_move_file_to_secure_nvm(const char *path);

json_t *ctrlm_utils_json_from_path(json_t *root, const std::string &path, bool add_ref);
std::string ctrlm_utils_json_string_from_path(json_t *root, const std::string &path);

bool ctrlm_utils_thread_create(ctrlm_thread_t *thread, void *(*start_routine) (void *), void *arg, pthread_attr_t *attr = NULL);
bool ctrlm_utils_thread_join(ctrlm_thread_t *thread, uint32_t timeout_secs);

bool ctrlm_utils_message_queue_open(int *msgq, uint8_t max_msg, size_t msgsize);
void ctrlm_utils_message_queue_close(int *msgq);
bool ctrlm_utils_queue_msg_push(int msgq, const char *msg, size_t msg_len);
std::string ctrlm_utils_time_as_string(time_t time);

ctrlm_fmr_alarm_level_t ctrlm_utils_str_to_fmr_level(const std::string &level);
ctrlm_rcu_wakeup_config_t ctrlm_utils_str_to_wakeup_config(const std::string &wakeup_config);
int ctrlm_utils_custom_key_str_to_array(const std::string &custom_keys, int *custom_list);

#ifdef __cplusplus
}
#endif

#endif
