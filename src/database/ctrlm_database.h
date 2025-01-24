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
#ifndef _CTRLM_DATABASE_H_
#define _CTRLM_DATABASE_H_

#include <vector>
#include <string>
#include <glib.h>
#include <ctrlm.h>
#include <ctrlm_ipc.h>
#include <ctrlm_ipc_rcu.h>
#include "ctrlm_db_attr.h"
#include <memory>

#ifdef __cplusplus
extern "C"
{
#endif

gboolean ctrlm_db_init(const char *db_path);
gboolean ctrlm_db_created_default(void);
void     ctrlm_db_terminate(void);
void     ctrlm_db_queue_msg_push(gpointer msg);
void     ctrlm_db_queue_msg_push_front(gpointer msg);
bool     ctrlm_db_backup();
void     ctrlm_db_power_state_change(gboolean waking_up);

void ctrlm_db_ir_controller_create(std::string &table);
void ctrlm_db_version_write(int version);
void ctrlm_db_version_read(int *version);
void ctrlm_db_device_update_session_id_write(unsigned char session_id);
void ctrlm_db_device_update_session_id_read(unsigned char *session_id);
void ctrlm_db_voice_settings_write(guchar *data, guint32 length);
void ctrlm_db_voice_settings_read(guchar **data, guint32 *length);
void ctrlm_db_ir_rf_database_write(ctrlm_key_code_t key_code, guchar *data, guint32 length);
void ctrlm_db_ir_rf_database_read(ctrlm_key_code_t key_code, guchar **data, guint32 *length);
void ctrlm_db_ir_rf_database_delete(ctrlm_key_code_t key_code);
void ctrlm_db_target_irdb_status_write(guchar *data, guint32 length);
void ctrlm_db_target_irdb_status_read(guchar **data, guint32 *length);
void ctrlm_db_voice_settings_remove();
void ctrlm_db_free(guchar *data);
void ctrlm_db_ir_remote_usage_write(guchar *data, guint32 length);
void ctrlm_db_ir_remote_usage_read(guchar **data, guint32 *length);
void ctrlm_db_pairing_metrics_write(guchar *data, guint32 length);
void ctrlm_db_pairing_metrics_read(guchar **data, guint32 *length);
void ctrlm_db_last_key_info_write(guchar *data, guint32 length);
void ctrlm_db_last_key_info_read(guchar **data, guint32 *length);
void ctrlm_db_shutdown_time_write(guchar *data, guint32 length);
void ctrlm_db_shutdown_time_read(guchar **data, guint32 *length);
#ifdef ASB
void ctrlm_db_asb_enabled_write(guchar *data, guint32 length);
void ctrlm_db_asb_enabled_read(guchar **data, guint32 *length);
#endif
void ctrlm_db_open_chime_enabled_write(guchar *data, guint32 length);
void ctrlm_db_open_chime_enabled_read(guchar **data, guint32 *length);
void ctrlm_db_close_chime_enabled_write(guchar *data, guint32 length);
void ctrlm_db_close_chime_enabled_read(guchar **data, guint32 *length);
void ctrlm_db_privacy_chime_enabled_write(guchar *data, guint32 length);
void ctrlm_db_privacy_chime_enabled_read(guchar **data, guint32 *length);
void ctrlm_db_conversational_mode_write(guchar *data, guint32 length);
void ctrlm_db_conversational_mode_read(guchar **data, guint32 *length);
void ctrlm_db_chime_volume_write(guchar *data, guint32 length);
void ctrlm_db_chime_volume_read(guchar **data, guint32 *length);
void ctrlm_db_ir_command_repeats_write(guchar *data, guint32 length);
void ctrlm_db_ir_command_repeats_read(guchar **data, guint32 *length);
void ctrlm_db_tv_ir_code_id_write(const std::string id);
void ctrlm_db_tv_ir_code_id_read(std::string &id);
void ctrlm_db_avr_ir_code_id_write(const std::string id);
void ctrlm_db_avr_ir_code_id_read(std::string &id);

void ctrlm_db_rf4ce_networks_list(std::vector<ctrlm_network_id_t> *network_ids);
void ctrlm_db_rf4ce_controllers_list(ctrlm_network_id_t network_id, std::vector<ctrlm_controller_id_t> *controller_ids);
void ctrlm_db_rf4ce_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
void ctrlm_db_rf4ce_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

void ctrlm_db_rf4ce_write_dsp_configuration_xr19(ctrlm_network_id_t network_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_read_dsp_configuration_xr19(ctrlm_network_id_t network_id, guchar **data, guint32 *length);

void ctrlm_db_rf4ce_read_ieee_address(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long long *ieee_address);
void ctrlm_db_rf4ce_read_binding_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t *binding_type);
void ctrlm_db_rf4ce_read_validation_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_validation_type_t *validation_type);
void ctrlm_db_rf4ce_read_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_binding);
void ctrlm_db_rf4ce_read_peripheral_id(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_rf_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_firmware_updated(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_reboot_diagnostics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_memory_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_time_last_checkin_for_device_update(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_time_last_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_last_heartbeat);
void ctrlm_db_rf4ce_read_polling_methods(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 *polling_methods);
void ctrlm_db_rf4ce_read_polling_configuration_mac(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_polling_configuration_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_rib_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int *status);
void ctrlm_db_rf4ce_read_binding_security_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_security_type_t *type);
void ctrlm_db_rf4ce_read_privacy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_controller_capabilities(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_far_field_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_dsp_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_time_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_metrics);
#ifdef ASB
void ctrlm_db_rf4ce_read_asb_key_derivation_method(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char *method);
#endif
void ctrlm_db_rf4ce_read_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_read_ota_failures_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);

void ctrlm_db_rf4ce_write_binding_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t binding_type);
void ctrlm_db_rf4ce_write_validation_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_validation_type_t validation_type);
void ctrlm_db_rf4ce_write_peripheral_id(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_rf_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_firmware_updated(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_reboot_diagnostics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_memory_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_time_last_checkin_for_device_update(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_time_last_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_last_heartbeat);
void ctrlm_db_rf4ce_write_polling_methods(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 polling_methods);
void ctrlm_db_rf4ce_write_polling_configuration_mac(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_polling_configuration_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_rib_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int status);
void ctrlm_db_rf4ce_write_binding_security_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_security_type_t type);
void ctrlm_db_rf4ce_write_privacy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_controller_capabilities(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, const guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_far_field_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_dsp_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_time_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_metrics);
void ctrlm_db_rf4ce_read_uptime_privacy_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_write_uptime_privacy_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
#ifdef ASB
void ctrlm_db_rf4ce_write_asb_key_derivation_method(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char method);
#endif
void ctrlm_db_rf4ce_write_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_file(const char *path, guchar *data, guint32 length);
void ctrlm_db_rf4ce_read_mfg_test_result(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_rf4ce_write_mfg_test_result(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_rf4ce_write_ota_failures_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 ota_failures);

void ctrlm_db_rf4ce_destroy_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
bool ctrlm_db_rf4ce_exists_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

void ctrlm_db_ip_controllers_list(ctrlm_network_id_t network_id, std::vector<ctrlm_controller_id_t> *controller_ids);
void ctrlm_db_ip_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
void ctrlm_db_ip_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

void ctrlm_db_ip_read_controller_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_ip_write_controller_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_ip_read_controller_manufacturer(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_ip_write_controller_manufacturer(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_ip_read_controller_model(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_ip_write_controller_model(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_ip_read_authentication_token(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length);
void ctrlm_db_ip_write_authentication_token(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length);
void ctrlm_db_ip_read_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_binding);
void ctrlm_db_ip_write_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_binding);
void ctrlm_db_ip_read_time_last_key(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_last_key);
void ctrlm_db_ip_write_time_last_key(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_last_key);
void ctrlm_db_ip_read_permissions(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char *permissions);
void ctrlm_db_ip_write_permissions(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char permissions);

void ctrlm_db_ble_controllers_list(ctrlm_network_id_t network_id, std::vector<ctrlm_controller_id_t> *controller_ids);
void ctrlm_db_ble_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
void ctrlm_db_ble_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

#ifdef __cplusplus
}
#endif

void ctrlm_db_ble_read_ota_failure_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 &value);
void ctrlm_db_ble_write_ota_failure_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 value);

bool ctrlm_db_voice_valid();
void ctrlm_db_voice_read_guide_language(std::string &lang);
void ctrlm_db_voice_read_aspect_ratio(std::string &ratio);
void ctrlm_db_voice_read_utterance_duration_min(unsigned long &stream_time_min);
void ctrlm_db_voice_read_device_status(int device, int *status); // Use int * because it's technically an enum, which causes an issue when using int &
void ctrlm_db_voice_read_audio_ducking_beep_enable(bool &enable);
void ctrlm_db_voice_read_par_voice_status(bool &status);

void ctrlm_db_voice_write_guide_language(std::string lang);
void ctrlm_db_voice_write_aspect_ratio(std::string ratio);
void ctrlm_db_voice_write_utterance_duration_min(unsigned long stream_time_min);
void ctrlm_db_voice_write_device_status(int device, int status);
void ctrlm_db_voice_write_audio_ducking_beep_enable(bool enable);
void ctrlm_db_voice_write_par_voice_status(bool status);

void ctrlm_db_attr_write(std::weak_ptr<ctrlm_db_attr_t> attr);
bool ctrlm_db_attr_read(ctrlm_db_attr_t *attr);

#endif
