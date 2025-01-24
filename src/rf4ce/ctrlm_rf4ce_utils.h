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
#ifndef _CTRLM_RF4CE_UTILS_H_
#define _CTRLM_RF4CE_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

const char *ctrlm_rf4ce_result_validation_str(ctrlm_rf4ce_result_validation_t result);
const char *ctrlm_rf4ce_device_update_audio_theme_str(ctrlm_rf4ce_device_update_audio_theme_t audio_theme);
const char *ctrlm_rf4ce_device_update_image_type_str(ctrlm_rf4ce_device_update_image_type_t image_type);
const char *ctrlm_rf4ce_controller_type_str(ctrlm_rf4ce_controller_type_t controller_type);
const char *ctrlm_rf4ce_voice_command_encryption_str(voice_command_encryption_t encryption);
const char *ctrlm_rf4ce_binding_initiation_indicator_str(ctrlm_rf4ce_binding_initiation_indicator_t indicator);
const char *ctrlm_rf4ce_firmware_updated_str(ctrlm_rf4ce_firmware_updated_t updated);
const char *ctrlm_rf4ce_controller_polling_configuration_str(ctrlm_rf4ce_controller_type_t controller_type);
const char *ctrlm_rf4ce_controller_polling_trigger_str(uint16_t trigger);
const char *ctrlm_rf4ce_discovery_type_str(uint8_t dev_type);
const char *voice_audio_profile_str(voice_audio_profile_t profile);
const char *controller_irdb_status_load_status_str(guchar status);
const char *ctrlm_rf4ce_pairing_restrict_by_remote_str(ctrlm_pairing_restrict_by_remote_t pairing_restrict_by_remote);
gboolean    ctrlm_rf4ce_is_voice_remote(const char * user_string);
gboolean    ctrlm_rf4ce_is_voice_assistant(const char * user_string);
gboolean    ctrlm_rf4ce_has_battery(ctrlm_rf4ce_controller_type_t controller_type);
gboolean    ctrlm_rf4ce_has_dsp(ctrlm_rf4ce_controller_type_t controller_type);
const char *ctrlm_rf4ce_controller_polling_methods_str(guchar methods);
void        ctrlm_rf4ce_controller_polling_configuration_print(const char *function, const char *type, ctrlm_rf4ce_polling_configuration_t *configuration);
int         ctrlm_rf4ce_polling_action_priority(ctrlm_rf4ce_polling_action_t action);
int         ctrlm_rf4ce_polling_action_sort_function(const void *a, const void *b, void *user_data);
void        ctrlm_rf4ce_polling_action_free(void *data);
const char *ctrlm_rf4ce_polling_action_str(ctrlm_rf4ce_polling_action_t action);

ctrlm_remote_keypad_config ctrlm_rf4ce_get_remote_keypad_config(const char *remote_type);



#ifdef __cplusplus
}
#endif

#endif
