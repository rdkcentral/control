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
#include <string.h>
#include <glib.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_rcu.h"
#include "ctrlm_utils.h"
#include "ctrlm_rf4ce_network.h"
#include "ctrlm_rf4ce_controller.h"
#include "ctrlm_rf4ce_utils.h"

#define CTRLM_INVALID_STR_LEN (24)
#define NUM_OF_USER_STRINGS (sizeof(ctrlm_rf4ce_user_strings)/sizeof(ctrlm_rf4ce_user_strings[0]))

//user strings
const char *ctrlm_rf4ce_user_strings[] = {
"XR11-20",
"XR15-10",
"XR15-20",
"XR16-10",
"XRA-10"
};


/* Below helper function used for better optinamization of multiple if- else cases */
bool ctrlm_rf4ce_is_voice_remote_from_user_string(const char *name) {

   errno_t safec_rc = -1;
   int ind = -1;
   unsigned int i = 0;
   int strsize = 0;

   if(name == NULL)
      return false;

   strsize = strlen(name);

   for (i = 0 ; i < NUM_OF_USER_STRINGS ; ++i) {
      safec_rc = strcmp_s(name, strsize, ctrlm_rf4ce_user_strings[i], &ind);
      ERR_CHK(safec_rc);
      if((safec_rc == EOK) && (!ind)) {
         return true;
       }
   }
   return false;
}

const char *ctrlm_rf4ce_result_validation_str(ctrlm_rf4ce_result_validation_t result) {
   switch(result) {
      case CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS:         return("SUCCESS");
      case CTRLM_RF4CE_RESULT_VALIDATION_PENDING:         return("PENDING");
      case CTRLM_RF4CE_RESULT_VALIDATION_TIMEOUT:         return("TIMEOUT");
      case CTRLM_RF4CE_RESULT_VALIDATION_COLLISION:       return("COLLISION");
      case CTRLM_RF4CE_RESULT_VALIDATION_FAILURE:         return("FAILURE");
      case CTRLM_RF4CE_RESULT_VALIDATION_ABORT:           return("ABORT");
      case CTRLM_RF4CE_RESULT_VALIDATION_FULL_ABORT:      return("FULL_ABORT");
      case CTRLM_RF4CE_RESULT_VALIDATION_FAILED:          return("FAILED");
      case CTRLM_RF4CE_RESULT_VALIDATION_BIND_TABLE_FULL: return("BIND_TABLE_FULL");
      case CTRLM_RF4CE_RESULT_VALIDATION_IN_PROGRESS:     return("IN_PROGRESS");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_rf4ce_device_update_audio_theme_str(ctrlm_rf4ce_device_update_audio_theme_t audio_theme) {
   switch(audio_theme) {
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_NONE:    return("NONE");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_DEFAULT: return("DEFAULT");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_2:       return("2");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_3:       return("3");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_4:       return("4");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_5:       return("5");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_6:       return("6");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_7:       return("7");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_8:       return("8");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_9:       return("9");
      case RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(audio_theme));
}

const char *ctrlm_rf4ce_device_update_image_type_str(ctrlm_rf4ce_device_update_image_type_t image_type) {
   switch(image_type) {
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE:      return("FIRMWARE");
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1:  return("AUDIO_DATA_1");
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_2:  return("AUDIO_DATA_2");
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_DSP:           return("DSP");
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_KEYWORD_MODEL: return("KEYWORD_MODEL");
   }
   return(ctrlm_invalid_return(image_type));
}

const char *ctrlm_rf4ce_controller_type_str(ctrlm_rf4ce_controller_type_t controller_type) {
   switch(controller_type) {
   case RF4CE_CONTROLLER_TYPE_XR2:     return("XR2");
   case RF4CE_CONTROLLER_TYPE_XR5:     return("XR5");
   case RF4CE_CONTROLLER_TYPE_XR11:    return("XR11");
   case RF4CE_CONTROLLER_TYPE_XR15:    return("XR15");
   case RF4CE_CONTROLLER_TYPE_XR15V2:  return("XR15V2");
   case RF4CE_CONTROLLER_TYPE_XR16:    return("XR16");
   case RF4CE_CONTROLLER_TYPE_XR18:    return("XR18");
   case RF4CE_CONTROLLER_TYPE_XR19:    return("XR19");
   case RF4CE_CONTROLLER_TYPE_XRA:     return("XRA");
   case RF4CE_CONTROLLER_TYPE_UNKNOWN: return("UNKNOWN");
   case RF4CE_CONTROLLER_TYPE_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(controller_type));
}

const char *ctrlm_rf4ce_voice_command_encryption_str(voice_command_encryption_t encryption) {
   switch(encryption) {
   case VOICE_COMMAND_ENCRYPTION_DISABLED: return("DISABLED");
   case VOICE_COMMAND_ENCRYPTION_ENABLED:  return("ENABLED");
   case VOICE_COMMAND_ENCRYPTION_DEFAULT:  return("DEFAULT");
   }
   return(ctrlm_invalid_return(encryption));
}

const char *ctrlm_rf4ce_binding_initiation_indicator_str(ctrlm_rf4ce_binding_initiation_indicator_t indicator) {
   switch(indicator) {
   case RF4CE_BINDING_INIT_IND_DEDICATED_KEY_COMBO_BIND: return("DEDICATED_KEY_COMBO_BIND");
   case RF4CE_BINDING_INIT_IND_ANY_BUTTON_BIND:          return("ANY_BUTTON_BIND");
   }
   return(ctrlm_invalid_return(indicator));
}

const char *ctrlm_rf4ce_firmware_updated_str(ctrlm_rf4ce_firmware_updated_t updated) {
   switch(updated) {
   case RF4CE_FIRMWARE_UPDATED_YES: return("YES");
   case RF4CE_FIRMWARE_UPDATED_NO:  return("NO");
   }
   return(ctrlm_invalid_return(updated));
}

const char *ctrlm_rf4ce_controller_polling_configuration_str(ctrlm_rf4ce_controller_type_t controller_type) {
   switch(controller_type) {
   case RF4CE_CONTROLLER_TYPE_XR2:     return("xr2");
   case RF4CE_CONTROLLER_TYPE_XR5:     return("xr5");
   case RF4CE_CONTROLLER_TYPE_XR11:    return("xr11v2");
   case RF4CE_CONTROLLER_TYPE_XR15:    return("xr15v1");
   case RF4CE_CONTROLLER_TYPE_XR15V2:  return("xr15v2");
   case RF4CE_CONTROLLER_TYPE_XR16:    return("xr16v1");
   case RF4CE_CONTROLLER_TYPE_XR18:    return("xr18");
   case RF4CE_CONTROLLER_TYPE_XR19:    return("xr19v1");
   case RF4CE_CONTROLLER_TYPE_XRA:     return("xrav1");
   case RF4CE_CONTROLLER_TYPE_UNKNOWN: return("unknown");
   default:                            break;
   }
   return(ctrlm_invalid_return(controller_type));
}

const char *ctrlm_rf4ce_controller_polling_trigger_str(uint16_t trigger) {
   switch(trigger) {
      case POLLING_TRIGGER_FLAG_TIME:          return("TIME");
      case POLLING_TRIGGER_FLAG_KEY_PRESS:     return("KEYPRESS");
      case POLLING_TRIGGER_FLAG_ACCELEROMETER: return("ACCELEROMETER");
      case POLLING_TRIGGER_FLAG_REBOOT:        return("REBOOT");
      case POLLING_TRIGGER_FLAG_VOICE:         return("VOICE");
      case POLLING_TRIGGER_FLAG_POLL_AGAIN:    return("POLL AGAIN");
      case POLLING_TRIGGER_FLAG_STATUS:        return("STATUS");
      default:                                 break;
   }
   return(ctrlm_invalid_return(trigger));
}

const char *ctrlm_rf4ce_discovery_type_str(uint8_t dev_type) {
   switch(dev_type) {
      case CTRLM_RF4CE_DEVICE_TYPE_STB:         return("STB");
      case CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND:    return("AUTOBIND");
      case CTRLM_RF4CE_DEVICE_TYPE_ANY:         return("ANY");
      case CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND: return("SCREEN_BIND");
      default:                                  break;
   }
   return(ctrlm_invalid_return(dev_type));
}

const char *voice_audio_profile_str(voice_audio_profile_t profile) {
   switch(profile) {
      case VOICE_AUDIO_PROFILE_NONE:              return("NONE");
      case VOICE_AUDIO_PROFILE_ADPCM_16BIT_16KHZ: return("ADPCM_16BIT_16KHZ");
      case VOICE_AUDIO_PROFILE_PCM_16BIT_16KHZ:   return("PCM_16BIT_16KHZ");
      case VOICE_AUDIO_PROFILE_OPUS_16BIT_16KHZ:  return("OPUS_16BIT_16KHZ");
   }
   return(ctrlm_invalid_return(profile));
}

const char *controller_irdb_status_load_status_str(guchar status) {
   status >>= 4;
   switch(status) {
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_NONE:                            return("NONE");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_CODE_CLEARING_SUCCESS:           return("CODE CLEARING SUCCEEDED");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_CODE_SETTING_SUCCESS:            return("CODE SETTING SUCCEEDED");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_SET_AND_CLEAR_ERROR:             return("ERROR - BOTH SET AND CLEAR REQUESTED");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_CODE_CLEAR_FAILED:               return("ERROR - CODE CLEAR FAILED");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_CODE_SET_FAILED_INVALID_CODE:    return("ERROR - CODE SET FAILED DUE TO INVALID CODE");
      case CONTROLLER_IRDB_STATUS_LOAD_STATUS_CODE_SET_FAILED_UNKNOWN_REASON:  return("ERROR - CODE SET FAILED DUE TO UNKNOWN REASON");
   }
   return(ctrlm_invalid_return(status));
}

const char *ctrlm_rf4ce_controller_polling_methods_str(guchar methods) {
   if(methods == 0) {
      return("NONE");
   }
   if(methods & ~(POLLING_METHODS_FLAG_HEARTBEAT | POLLING_METHODS_FLAG_MAC)) {
      return(ctrlm_invalid_return(methods));
   }

   if(methods & POLLING_METHODS_FLAG_HEARTBEAT) {
      if(methods & POLLING_METHODS_FLAG_MAC) {
         return("HEARTBEAT, MAC");
      }
      return("HEARTBEAT");
   }

   if(methods & POLLING_METHODS_FLAG_MAC) {
      return("MAC");
   }
   return(ctrlm_invalid_return(methods));
}

void ctrlm_rf4ce_controller_polling_configuration_print(const char *function, const char *type, ctrlm_rf4ce_polling_configuration_t *configuration) {
   const xlog_args_t xlog_args_info = {.options = XLOG_OPTS_DEFAULT, .color = XLOG_COLOR_NONE, .function = function, .line = XLOG_LINE_NONE, .level = XLOG_LEVEL_INFO, .id = XLOG_MODULE_ID, .size_max = XLOG_BUF_SIZE_DEFAULT};
   if(configuration == NULL) {
      xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "%s Poll Configuration: NULL", type);
      return;
   }
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "%s Poll Configuration: Trigger <%u>, KP Counter <%u>, Time Interval <%u>, Reserved <%u>", type, configuration->trigger, configuration->kp_counter, configuration->time_interval, configuration->reserved);
}

int ctrlm_rf4ce_polling_action_sort_function(const void *a, const void *b, void *user_data) {
   ctrlm_rf4ce_polling_action_msg_t *a_msg = (ctrlm_rf4ce_polling_action_msg_t *)a;
   ctrlm_rf4ce_polling_action_msg_t *b_msg = (ctrlm_rf4ce_polling_action_msg_t *)b;
   int a_priority = 0;
   int b_priority = 0;

   // Get the actual priority
   a_priority = ctrlm_rf4ce_polling_action_priority((ctrlm_rf4ce_polling_action_t)a_msg->action);
   b_priority = ctrlm_rf4ce_polling_action_priority((ctrlm_rf4ce_polling_action_t)b_msg->action);

   return((int)(a_priority-b_priority));
}

int ctrlm_rf4ce_polling_action_priority(ctrlm_rf4ce_polling_action_t action) {
   switch(action) {
      case RF4CE_POLLING_ACTION_ALERT: {
         return(0);
      }
      case RF4CE_POLLING_ACTION_REPAIR:
      case RF4CE_POLLING_ACTION_CONFIGURATION:
      case RF4CE_POLLING_ACTION_OTA:
      case RF4CE_POLLING_ACTION_IRDB_STATUS:
      case RF4CE_POLLING_ACTION_POLL_CONFIGURATION:
      case RF4CE_POLLING_ACTION_VOICE_CONFIGURATION: {
         return(1);
      }
      case RF4CE_POLLING_ACTION_REBOOT: {
         return(2);
      }
      case RF4CE_POLLING_ACTION_NONE:
      default: {
         return(3);
      }
   }

   return(0);
}

const char *ctrlm_rf4ce_pairing_restrict_by_remote_str(ctrlm_pairing_restrict_by_remote_t pairing_restrict_by_remote) {
   switch(pairing_restrict_by_remote) {
      case CTRLM_PAIRING_RESTRICT_NONE:                 return("PAIRING_RESTRICT_NONE");
      case CTRLM_PAIRING_RESTRICT_TO_VOICE_REMOTES:     return("PAIRING_RESTRICT_TO_VOICE_REMOTES");
      case CTRLM_PAIRING_RESTRICT_TO_VOICE_ASSISTANTS:  return("PAIRING_RESTRICT_TO_VOICE_ASSISTANTS");
      case CTRLM_PAIRING_RESTRICT_MAX:                  return("PAIRING_RESTRICT_MAX");
   }
   return(ctrlm_invalid_return(pairing_restrict_by_remote));
}

gboolean ctrlm_rf4ce_is_voice_remote(const char * user_string) {
   if(user_string == NULL) {
      XLOGD_ERROR("user_string is NULL");
      return(false);
   }

   if(ctrlm_rf4ce_is_voice_remote_from_user_string(user_string)) {
         return(true);
     }
   return(false);
}

gboolean ctrlm_rf4ce_is_voice_assistant(const char * user_string) {
   if(user_string == NULL) {
      XLOGD_ERROR("user_string is NULL");
      return(false);
   }

   int ind = -1;

   errno_t safec_rc = strcmp_s("XR19-10", strlen("XR19-10"), user_string, &ind);
   ERR_CHK(safec_rc);
   if((safec_rc == EOK) && (ind == 0)) {
         return(true);
     }
   return(false);
}

void ctrlm_rf4ce_polling_action_free(void *data) {
   ctrlm_rf4ce_polling_action_msg_t *msg = (ctrlm_rf4ce_polling_action_msg_t *)data;
   if(msg) {
      free(msg);
   }
}

const char *ctrlm_rf4ce_polling_action_str(ctrlm_rf4ce_polling_action_t action) {
   switch(action) {
      case RF4CE_POLLING_ACTION_NONE:                  return("NONE");
      case RF4CE_POLLING_ACTION_REBOOT:                return("REBOOT");
      case RF4CE_POLLING_ACTION_REPAIR:                return("REPAIR");
      case RF4CE_POLLING_ACTION_CONFIGURATION:         return("CONFIGURATION");
      case RF4CE_POLLING_ACTION_OTA:                   return("OTA");
      case RF4CE_POLLING_ACTION_ALERT:                 return("ALERT");
      case RF4CE_POLLING_ACTION_IRDB_STATUS:           return("IRDB STATUS");
      case RF4CE_POLLING_ACTION_POLL_CONFIGURATION:    return("POLLING CONFIGURATION");
      case RF4CE_POLLING_ACTION_VOICE_CONFIGURATION:   return("VOICE CONFIGURATION");
      case RF4CE_POLLING_ACTION_DSP_CONFIGURATION:     return("DSP CONFIGURATION");
      case RF4CE_POLLING_ACTION_METRICS:               return("METRICS");
      case RF4CE_POLLING_ACTION_EOS:                   return("EOS");
      case RF4CE_POLLING_ACTION_BATTERY_STATUS:        return("BATTERY STATUS");
      case RF4CE_POLLING_ACTION_PROFILE_CONFIGURATION: return("PROFILE CONFIGURATION");
      case RF4CE_POLLING_ACTION_IRRF_STATUS:           return("IRRF STATUS");
   }
   return("");
}

gboolean ctrlm_rf4ce_has_battery(ctrlm_rf4ce_controller_type_t controller_type) {
   switch(controller_type) {
   case RF4CE_CONTROLLER_TYPE_XR2:
   case RF4CE_CONTROLLER_TYPE_XR5:
   case RF4CE_CONTROLLER_TYPE_XR11:
   case RF4CE_CONTROLLER_TYPE_XR15:
   case RF4CE_CONTROLLER_TYPE_XR15V2:
   case RF4CE_CONTROLLER_TYPE_XR16:
   case RF4CE_CONTROLLER_TYPE_XRA:
      return(true);
   case RF4CE_CONTROLLER_TYPE_XR18:
   case RF4CE_CONTROLLER_TYPE_XR19:
   case RF4CE_CONTROLLER_TYPE_UNKNOWN:
   case RF4CE_CONTROLLER_TYPE_INVALID:
      return(false);
   }
   return(false);
}

gboolean ctrlm_rf4ce_has_dsp(ctrlm_rf4ce_controller_type_t controller_type) {
   switch(controller_type) {
   case RF4CE_CONTROLLER_TYPE_XR19:
      return(true);
   case RF4CE_CONTROLLER_TYPE_XR2:
   case RF4CE_CONTROLLER_TYPE_XR5:
   case RF4CE_CONTROLLER_TYPE_XR11:
   case RF4CE_CONTROLLER_TYPE_XR15:
   case RF4CE_CONTROLLER_TYPE_XR15V2:
   case RF4CE_CONTROLLER_TYPE_XR16:
   case RF4CE_CONTROLLER_TYPE_XR18:
   case RF4CE_CONTROLLER_TYPE_XRA:
   case RF4CE_CONTROLLER_TYPE_UNKNOWN:
   case RF4CE_CONTROLLER_TYPE_INVALID:
      return(false);
   }
   return(false);
}

ctrlm_remote_keypad_config ctrlm_rf4ce_get_remote_keypad_config(const char *remote_type) {
   ctrlm_remote_keypad_config remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_INVALID;
   if((strncmp("XR2-", remote_type, 4)  == 0) ||
         (strncmp("XR5-", remote_type, 4)  == 0) ||
         (strncmp("XR11-", remote_type, 5) == 0)) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_SETUP_KEY_WITH_NUMBER_KEYS;
   } else if (strncmp("XR15-", remote_type, 5) == 0) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NUMBER_KEYS;
   } else if(strncmp("XR16-", remote_type, 5) == 0) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NO_NUMBER_KEYS;
   } else if(strncmp("XR19-", remote_type, 5) == 0) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_VOICE_ASSISTANT;
   } else if(strncmp("XRA-", remote_type, 4) == 0) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NUMBER_KEYS;
   }
   return(remote_keypad_config);
}
