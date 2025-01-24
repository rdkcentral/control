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
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <archive.h>
#include <algorithm>
#include <cctype>
#include "ctrlm.h"
#include "ctrlm_utils.h"
#include <xr_mq.h>
#include <map>
#include <linux/input.h>

// dsMgr includes
#include "host.hpp"
#include "exception.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioOutputPort.hpp"
#include "frontPanelIndicator.hpp"
#include "manager.hpp"
#include "dsMgr.h"
#include "dsRpc.h"
#include "dsDisplay.h"
// end dsMgr includes

#define BLOCK_SIZE     (1024 * 4 * 10) /* bytes */
#define MAX_RECURSE_DEPTH 20

#define CTRLM_INVALID_STR_LEN (24)

#define CRC_BUFF_SIZE         (1024)
#define CTRLM_NVM_SECURE_PATH "/opt/secure/"

static char ctrlm_invalid_str[CTRLM_INVALID_STR_LEN];

#ifdef BREAKPAD_SUPPORT

void ctrlm_crash_ctrlm_device_update(void) {
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

#ifdef CTRLM_RF4CE_HAL_QORVO
void ctrlm_crash_rf4ce_qorvo(void) {
#else
void ctrlm_crash_rf4ce_ti(void) {
#endif
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

void ctrlm_crash_ble(void) {
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

void ctrlm_crash_vsdk(void) {
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

void ctrlm_crash_ctrlm_main(void) {
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

void ctrlm_crash_ctrlm_database(void) {
   XLOGD_TELEMETRY("crash");
   ctrlm_crash();
}

void ctrlm_crash(void)
{
  volatile int* a = (int*)(NULL);
  *a = 1;
}
#endif

void *ctrlm_hal_malloc(unsigned long size) {
   return(g_malloc(size));
}

void ctrlm_hal_free(void *ptr) {
   g_free(ptr);
}

guint ctrlm_timeout_create(guint timeout, GSourceFunc function, gpointer user_data) {
   return g_timeout_add(timeout, function, user_data);
}

void ctrlm_timeout_destroy(guint *p_timeout_tag) {
   g_assert(p_timeout_tag);
   if(NULL != p_timeout_tag && 0 != *p_timeout_tag) {
      g_source_remove(*p_timeout_tag);
      *p_timeout_tag = 0;
   }
}

void ctrlm_timestamp_get(ctrlm_timestamp_t *timestamp) {
   if(timestamp == NULL || clock_gettime(CLOCK_REALTIME, timestamp)) {
      XLOGD_ERROR("Unable to get time.");
   }
}

void ctrlm_timestamp_get_monotonic(ctrlm_timestamp_t *timestamp) {
   if(timestamp == NULL || clock_gettime(CLOCK_MONOTONIC_RAW, timestamp)) {
      XLOGD_ERROR("Unable to get time.");
   }
}

//  1 : one is greater than two
//  0 : one and two are equal
// -1 : one is less than two
int ctrlm_timestamp_cmp(ctrlm_timestamp_t one, ctrlm_timestamp_t two) {
   if(one.tv_sec > two.tv_sec) {
      return(1);
   } else if(one.tv_sec < two.tv_sec) {
      return(-1);
   }
   if(one.tv_nsec > two.tv_nsec) {
      return(1);
   } else if(one.tv_nsec < two.tv_nsec) {
      return(-1);
   }
   return(0);
}

// Subtract timestamp one from two ... (two - one)
signed long long ctrlm_timestamp_subtract_ns(ctrlm_timestamp_t one, ctrlm_timestamp_t two) {
   int cmp = ctrlm_timestamp_cmp(one, two);

   if(cmp > 0) { // one is greater than two
      if(one.tv_sec - two.tv_sec) {
         return(((one.tv_sec - two.tv_sec)*(long long)-1000000000) + one.tv_nsec - two.tv_nsec);
      }
      return(two.tv_nsec - one.tv_nsec);
   } else if(cmp < 0) { // one is less than two
      if(two.tv_sec - one.tv_sec) {
         return(((two.tv_sec - one.tv_sec)*(long long)1000000000) - one.tv_nsec + two.tv_nsec);
      }
      return(two.tv_nsec - one.tv_nsec);
   }
   return 0;
}

signed long long ctrlm_timestamp_subtract_us(ctrlm_timestamp_t one, ctrlm_timestamp_t two) {
   return(ctrlm_timestamp_subtract_ns(one, two) / 1000);
}

signed long long ctrlm_timestamp_subtract_ms(ctrlm_timestamp_t one, ctrlm_timestamp_t two) {
   return(ctrlm_timestamp_subtract_ns(one, two) / 1000000);
}

void ctrlm_timestamp_add_ns(ctrlm_timestamp_t *timestamp, unsigned long nanoseconds) {
   if(timestamp == NULL) {
      XLOGD_ERROR("Invalid timestamp");
      return;
   }
   unsigned long long nsecs = timestamp->tv_nsec + nanoseconds;

   timestamp->tv_nsec  = nsecs % 1000000000;
   timestamp->tv_sec  += nsecs / 1000000000;
}

void ctrlm_timestamp_add_us(ctrlm_timestamp_t *timestamp, unsigned long microseconds) {
   ctrlm_timestamp_add_ns(timestamp, microseconds * 1000);
}

void ctrlm_timestamp_add_ms(ctrlm_timestamp_t *timestamp, unsigned long milliseconds) {
   ctrlm_timestamp_add_ns(timestamp, milliseconds * 1000000);
}

void ctrlm_timestamp_add_secs(ctrlm_timestamp_t *timestamp, unsigned long seconds) {
   if(timestamp == NULL) {
      XLOGD_ERROR("Invalid timestamp");
      return;
   }
   timestamp->tv_sec += seconds;
}

unsigned long long ctrlm_timestamp_until_ns(ctrlm_timestamp_t timestamp) {
   ctrlm_timestamp_t now;
   ctrlm_timestamp_get(&now);
   if(ctrlm_timestamp_cmp(timestamp, now) <= 0) {
      return(0);
   }

   return((unsigned long long)ctrlm_timestamp_subtract_ns(now, timestamp));
}

unsigned long long ctrlm_timestamp_until_us(ctrlm_timestamp_t timestamp) {
   return(ctrlm_timestamp_until_ns(timestamp) / 1000);
}

unsigned long long ctrlm_timestamp_until_ms(ctrlm_timestamp_t timestamp) {
   return(ctrlm_timestamp_until_ns(timestamp) / 1000000);
}

unsigned long long ctrlm_timestamp_since_ns(ctrlm_timestamp_t timestamp) {
   ctrlm_timestamp_t now;
   ctrlm_timestamp_get(&now);
   if(ctrlm_timestamp_cmp(timestamp, now) >= 0) {
      return(0);
   }

   return((unsigned long long)ctrlm_timestamp_subtract_ns(timestamp, now));
}

unsigned long long ctrlm_timestamp_since_us(ctrlm_timestamp_t timestamp) {
   return(ctrlm_timestamp_since_ns(timestamp) / 1000);
}

unsigned long long ctrlm_timestamp_since_ms(ctrlm_timestamp_t timestamp)  {
   return(ctrlm_timestamp_since_ns(timestamp) / 1000000);
}

static char nibble_to_hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void ctrlm_print_data_hex(const char *prefix, guchar *data, unsigned int length, unsigned int width) {
   #define MAX_WIDTH (64)
   if(prefix == NULL || data == NULL || length == 0 || width < 4 || width > MAX_WIDTH || width % 4) {
      XLOGD_ERROR("Invalid parameters %p %p %u %u", prefix, data, length, width);
      return;
   }
   const xlog_args_t xlog_args = {.options = XLOG_OPTS_DEFAULT, .color = XLOG_COLOR_NONE, .function = prefix, .line = XLOG_LINE_NONE, .level = XLOG_LEVEL_INFO, .id = XLOG_MODULE_ID, .size_max = XLOG_BUF_SIZE_DEFAULT};
   char buffer[MAX_WIDTH / 4 * 9];
   for(unsigned int index = 0; index < length; index += width) {
      char *p = buffer;
      for(unsigned int j = 0; j < width && index + j < length; j++) {
         guchar byte = data[index + j];
         if((j + 1) % 4) {
            *p++ = nibble_to_hex[byte >> 4];
            *p++ = nibble_to_hex[byte & 0xF];
         } else {
            *p++ = nibble_to_hex[byte >> 4];
            *p++ = nibble_to_hex[byte & 0xF];
            *p++ = ' ';
         }
      }
      *p = '\0';

      xlog_fprintf(&xlog_args, XLOGD_OUTPUT, "0x%04X: 0x%s", index, buffer);
   }
   #undef MAX_WIDTH
}


void ctrlm_print_controller_status(const char *prefix, ctrlm_controller_status_t *status) {
   if(prefix == NULL || status == NULL) {
      return;
   }
   char time_binding_str[CTRLM_MAX_TIME_STR_LEN];
   char time_last_key_str[CTRLM_MAX_TIME_STR_LEN];
   char time_battery_update_str[CTRLM_MAX_TIME_STR_LEN];
   errno_t safec_rc = -1;

   if(status->time_binding == 0) {
      safec_rc = strcpy_s(time_binding_str, sizeof(time_binding_str), "NEVER");
      ERR_CHK(safec_rc);
   } else {
      time_binding_str[0]        = '\0';
      strftime(time_binding_str,        20, "%F %T", localtime((time_t *)&status->time_binding));
   }
   if(status->time_last_key == 0) {
      safec_rc = strcpy_s(time_last_key_str, sizeof(time_last_key_str), "NEVER");
      ERR_CHK(safec_rc);
   } else {
      time_last_key_str[0]       = '\0';
      strftime(time_last_key_str,       20, "%F %T", localtime((time_t *)&status->time_last_key));
   }
   if(status->time_battery_update == 0) {
      safec_rc = strcpy_s(time_battery_update_str, sizeof(time_battery_update_str), "NEVER");
      ERR_CHK(safec_rc);
   } else {
      time_battery_update_str[0] = '\0';
      strftime(time_battery_update_str, 20, "%F %T", localtime((time_t *)&status->time_battery_update));
   }

   const xlog_args_t xlog_args_info = {.options = XLOG_OPTS_DEFAULT, .color = XLOG_COLOR_NONE, .function = prefix, .line = XLOG_LINE_NONE, .level = XLOG_LEVEL_INFO, .id = XLOG_MODULE_ID, .size_max = XLOG_BUF_SIZE_DEFAULT};
   const xlog_args_t xlog_args_tele = {.options = XLOG_OPTS_DEFAULT, .color = XLOG_COLOR_BLU,  .function = prefix, .line = XLOG_LINE_NONE, .level = XLOG_LEVEL_INFO, .id = XLOG_MODULE_ID, .size_max = XLOG_BUF_SIZE_DEFAULT};

   // XLOGD_TELEMETRY
   xlog_fprintf(&xlog_args_tele, XLOGD_OUTPUT, "Type <%s> Version Sw <%s> Hw <%s> Irdb Ver <%s> IEEE 0x%016llx", status->type, status->version_software, status->version_hardware, status->version_irdb, status->ieee_address);
   if(!strncmp(status->type, "XR19-10", 7)) {
      xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Version Dsp <%s> Keyword Model <%s>", status->version_dsp, status->version_keyword_model);
   }
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Version BuildID <%s>", status->version_build_id);
   // XLOGD_TELEMETRY
   xlog_fprintf(&xlog_args_tele, XLOGD_OUTPUT, "Binding <%s> Validation <%s> Security <%s> Time <%s>", ctrlm_rcu_binding_type_str(status->binding_type), ctrlm_rcu_validation_type_str(status->validation_type), ctrlm_rcu_binding_security_type_str(status->security_type), time_binding_str);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Command Count %lu Last Key Code <%s> Status <%s> Time <%s>", status->command_count, ctrlm_key_code_str(status->last_key_code), ctrlm_key_status_str(status->last_key_status), time_last_key_str);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Link Quality %u", status->link_quality);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Battery Voltage Loaded %4.2f V Unloaded %4.2f V Last updated <%s>", status->battery_voltage_loaded, status->battery_voltage_unloaded, time_battery_update_str);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Count Today %lu", status->voice_cmd_count_today);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Count Yesterday %lu", status->voice_cmd_count_yesterday);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Short Today %lu", status->voice_cmd_short_today);
   xlog_fprintf(&xlog_args_info, XLOGD_OUTPUT, "Voice Cmd Short Yesterday %lu", status->voice_cmd_short_yesterday);
}

const char *ctrlm_invalid_return(int value) {
   errno_t safec_rc = sprintf_s(ctrlm_invalid_str, CTRLM_INVALID_STR_LEN, "INVALID(%d)", value);
   if(safec_rc < EOK) {
       ERR_CHK(safec_rc);
   }
   return(ctrlm_invalid_str);
}

const char *ctrlm_main_queue_msg_type_str(ctrlm_main_queue_msg_type_t type) {
   switch(type) {
      case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_BEGIN:                   return("BIND_VALIDATION_BEGIN");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_END:                     return("BIND_VALIDATION_END");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_FAILED_TIMEOUT:          return("BIND_VALIDATION_FAILED_TIMEOUT");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_CONFIGURATION_COMPLETE:             return("BIND_CONFIGURATION_COMPLETE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_NETWORK_PROPERTY_SET:                    return("NETWORK_PROPERTY_SET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE:                               return("TERMINATE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STATUS:                             return("MAIN_STATUS");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_SET:                       return("MAIN_PROPERTY_SET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_GET:                       return("MAIN_PROPERTY_GET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_CONFIG_SET:               return("MAIN_DISCOVERY_CONFIG_SET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_AUTOBIND_CONFIG_SET:                return("MAIN_AUTOBIND_CONFIG_SET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PRECOMMISSION_CONFIG_SET:           return("MAIN_PRECOMMISSION_CONFIG_SET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_FACTORY_RESET:                      return("MAIN_FACTORY_RESET");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROLLER_UNBIND:                  return("MAIN_CONTROLLER_UNBIND");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_LINE_OF_SIGHT:              return("MAIN_TIMEOUT_LINE_OF_SIGHT");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_AUTOBIND:                   return("MAIN_TIMEOUT_AUTOBIND");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_BINDING_BUTTON:             return("MAIN_TIMEOUT_BINDING_BUTTON");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_BUTTON:                return("MAIN_STOP_BINDING_BUTTON");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE:                      return("POWER STATE CHANGE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_CHECK_UPDATE_FILE_NEEDED:                return("MAIN_CHECK_UPDATE_FILE_NEED");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE:                         return("CONTROLLER_TYPE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_AUTHSERVICE_POLL:                        return("AUTHSERVICE_POLL");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE:                                  return("TICKLE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_THREAD_MONITOR_POLL:                     return("THREAD_MONITOR_POLL");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_NOTIFY_FIRMWARE:                         return("NOTIFY_FIRMWARE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_IR_REMOTE_USAGE:                         return("IR_REMOTE_USAGE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_LAST_KEY_INFO:                           return("LAST_KEY_INFO");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_SCREEN:                return("MAIN_STOP_BINDING_SCREEN");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES:         return("CONTROL_SERVICE_SET_VALUES");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES:         return("CONTROL_SERVICE_GET_VALUES");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE: return("CONTROL_SERVICE_CAN_FIND_MY_REMOTE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE: return("CONTROL_SERVICE_START_PAIRING_MODE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE:   return("CONTROL_SERVICE_END_PAIRING_MODE");
      case CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST:                  return("EXPORT_CONTROLLER_LIST");
      default: if (type >= CTRLM_MAIN_QUEUE_MSG_TYPE_VENDOR_FIRST && type <= CTRLM_MAIN_QUEUE_MSG_TYPE_VENDOR_LAST) {
         return("VENDOR SPECIFIC MESSAGE");
      }
   }
   return(ctrlm_invalid_return(type));
}

const char *ctrlm_controller_status_cmd_result_str(ctrlm_controller_status_cmd_result_t result) {
   switch(result) {
      case CTRLM_CONTROLLER_STATUS_REQUEST_PENDING: return("PENDING");
      case CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS: return("SUCCESS");
      case CTRLM_CONTROLLER_STATUS_REQUEST_ERROR:   return("ERROR");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_iarm_call_result_str(ctrlm_iarm_call_result_t result) {
   switch(result) {
      case CTRLM_IARM_CALL_RESULT_SUCCESS:                 return("SUCCESS");
      case CTRLM_IARM_CALL_RESULT_ERROR:                   return("ERROR");
      case CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY:         return("ERROR_READ_ONLY");
      case CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER: return("ERROR_INVALID_PARAMETER");
      case CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION:      return("ERROR_API_REVISION");
      case CTRLM_IARM_CALL_RESULT_ERROR_NOT_SUPPORTED:     return("ERROR_NOT_SUPPORTED");
      case CTRLM_IARM_CALL_RESULT_INVALID:                 return("INVALID");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_iarm_result_str(IARM_Result_t result) {
   switch(result) {
      case IARM_RESULT_SUCCESS:       return("IARM_CALL_SUCCESS");
      case IARM_RESULT_INVALID_PARAM: return("IARM_CALL_ERROR_INVALID_PARAMETER");
      case IARM_RESULT_INVALID_STATE: return("IARM_CALL_ERROR_INVALID_STATE");
      case IARM_RESULT_IPCCORE_FAIL:  return("IARM_CALL_ERROR_IPCCORE_FAIL");
      case IARM_RESULT_OOM:           return("IARM_CALL_ERROR_OOM");
      default:                        return("IARM_CALL_UNKNOWN");
   }
}

const char *ctrlm_access_type_str(ctrlm_access_type_t access_type) {
   switch(access_type) {
      case CTRLM_ACCESS_TYPE_READ:    return("READ");
      case CTRLM_ACCESS_TYPE_WRITE:   return("WRITE");
      case CTRLM_ACCESS_TYPE_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(access_type));
}

std::string ctrlm_network_type_str(ctrlm_network_type_t network_type) {
   switch(network_type) {
      case CTRLM_NETWORK_TYPE_RF4CE:        return("RF4CE");
      case CTRLM_NETWORK_TYPE_BLUETOOTH_LE: return("BLE");
      case CTRLM_NETWORK_TYPE_IP:           return("IP");
      case CTRLM_NETWORK_TYPE_DSP:          return("DSP");
      case CTRLM_NETWORK_TYPE_INVALID:      return("INVALID");
      default: {
         std::ostringstream str;
         str << "Network type " << network_type;
         return str.str();
      }
   }
}

std::string ctrlm_controller_name_str(ctrlm_controller_id_t controller) {
   switch(controller) {
      case CTRLM_MAIN_CONTROLLER_ID_ALL:       return("ALL");
      case CTRLM_MAIN_CONTROLLER_ID_LAST_USED: return("LAST USED");
      case CTRLM_MAIN_CONTROLLER_ID_INVALID:   return("INVALID");
      default: {
         std::ostringstream str;
         str << (int)controller;
         return str.str();
      }
   }
}

const char *ctrlm_unbind_reason_str(ctrlm_unbind_reason_t reason) {
   switch(reason) {
      case CTRLM_UNBIND_REASON_CONTROLLER:         return("CONTROLLER");
      case CTRLM_UNBIND_REASON_TARGET_USER:        return("TARGET_USER");
      case CTRLM_UNBIND_REASON_TARGET_NO_SPACE:    return("TARGET_NO_SPACE");
      case CTRLM_UNBIND_REASON_FACTORY_RESET:      return("FACTORY_RESET");
      case CTRLM_UNBIND_REASON_CONTROLLER_RESET:   return("CONTROLLER_RESET");
      case CTRLM_UNBIND_REASON_INVALID_VALIDATION: return("INVALID_VALIDATION");
      case CTRLM_UNBIND_REASON_MAX:                return("MAX");
   }
   return(ctrlm_invalid_return(reason));
}

const char *ctrlm_rcu_validation_result_str(ctrlm_rcu_validation_result_t validation_result) {
   switch(validation_result) {
      case CTRLM_RCU_VALIDATION_RESULT_SUCCESS:         return("SUCCESS");
      case CTRLM_RCU_VALIDATION_RESULT_PENDING:         return("PENDING");
      case CTRLM_RCU_VALIDATION_RESULT_TIMEOUT:         return("TIMEOUT");
      case CTRLM_RCU_VALIDATION_RESULT_COLLISION:       return("COLLISION");
      case CTRLM_RCU_VALIDATION_RESULT_FAILURE:         return("FAILURE");
      case CTRLM_RCU_VALIDATION_RESULT_ABORT:           return("ABORT");
      case CTRLM_RCU_VALIDATION_RESULT_FULL_ABORT:      return("FULL_ABORT");
      case CTRLM_RCU_VALIDATION_RESULT_FAILED:          return("FAILED");
      case CTRLM_RCU_VALIDATION_RESULT_BIND_TABLE_FULL: return("BIND_TABLE_FULL");
      case CTRLM_RCU_VALIDATION_RESULT_IN_PROGRESS:     return("IN_PROGRESS");
      case CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART:   return("CTRLM_RESTART");
      case CTRLM_RCU_VALIDATION_RESULT_MAX:             return("MAX");
   }
   return(ctrlm_invalid_return(validation_result));
}

const char *ctrlm_rcu_configuration_result_str(ctrlm_rcu_configuration_result_t configuration_result) {
   switch(configuration_result) {
      case CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS: return("SUCCESS");
      case CTRLM_RCU_CONFIGURATION_RESULT_PENDING: return("PENDING");
      case CTRLM_RCU_CONFIGURATION_RESULT_TIMEOUT: return("TIMEOUT");
      case CTRLM_RCU_CONFIGURATION_RESULT_FAILURE: return("FAILED");
      case CTRLM_RCU_CONFIGURATION_RESULT_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(configuration_result));
}

const char *ctrlm_rcu_rib_attr_id_str(ctrlm_rcu_rib_attr_id_t attribute_id) {
   switch(attribute_id) {
      case CTRLM_RCU_RIB_ATTR_ID_PERIPHERAL_ID:             return("PERIPHERAL_ID");
      case CTRLM_RCU_RIB_ATTR_ID_RF_STATISTICS:             return("RF_STATISTICS");
      case CTRLM_RCU_RIB_ATTR_ID_VERSIONING:                return("VERSIONING");
      case CTRLM_RCU_RIB_ATTR_ID_BATTERY_STATUS:            return("BATTERY_STATUS");
      case CTRLM_RCU_RIB_ATTR_ID_SHORT_RF_RETRY_PERIOD:     return("SHORT_RF_RETRY_PERIOD");
      case CTRLM_RCU_RIB_ATTR_ID_TARGET_ID_DATA:            return("TARGET_ID_DATA");
      case CTRLM_RCU_RIB_ATTR_ID_POLLING_METHODS:           return("POLLING_METHODS");
      case CTRLM_RCU_RIB_ATTR_ID_POLLING_CONFIGURATION:     return("POLLING_CONFIGURATION");
      case CTRLM_RCU_RIB_ATTR_ID_PRIVACY:                   return("PRIVACY");
      case CTRLM_RCU_RIB_ATTR_ID_CONTROLLER_CAPABILITIES:   return("CONTROLLER_CAPABILITIES");
      case CTRLM_RCU_RIB_ATTR_ID_RESPONSE_TIME:             return("RESPONSE_TIME");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_STATUS:      return("VOICE_COMMAND_STATUS");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_LENGTH:      return("VOICE_COMMAND_LENGTH");
      case CTRLM_RCU_RIB_ATTR_ID_MAXIMUM_UTTERANCE_LENGTH:  return("MAXIMUM_UTTERANCE_LENGTH");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_ENCRYPTION:  return("VOICE_COMMAND_ENCRYPTION");
      case CTRLM_RCU_RIB_ATTR_ID_MAX_VOICE_DATA_RETRY:      return("MAX_VOICE_DATA_RETRY");
      case CTRLM_RCU_RIB_ATTR_ID_MAX_VOICE_CSMA_BACKOFF:    return("MAX_VOICE_CSMA_BACKOFF");
      case CTRLM_RCU_RIB_ATTR_ID_MIN_VOICE_DATA_BACKOFF:    return("MIN_VOICE_DATA_BACKOFF");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_CTRL_AUDIO_PROFILES: return("VOICE_CTRL_AUDIO_PROFILES");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_TARG_AUDIO_PROFILES: return("VOICE_TARG_AUDIO_PROFILES");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_STATISTICS:          return("VOICE_STATISTICS");
      case CTRLM_RCU_RIB_ATTR_ID_RIB_ENTRIES_UPDATED:       return("RIB_ENTRIES_UPDATED");
      case CTRLM_RCU_RIB_ATTR_ID_RIB_UPDATE_CHECK_INTERVAL: return("RIB_UPDATE_CHECK_INTERVAL");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_SESSION_STATISTICS:  return("VOICE_SESSION_STATISTICS");
      case CTRLM_RCU_RIB_ATTR_ID_OPUS_ENCODING_PARAMS:      return("OPUS_ENCODING_PARAMS");
      case CTRLM_RCU_RIB_ATTR_ID_VOICE_SESSION_QOS:         return("VOICE_SESSION_QOS");
      case CTRLM_RCU_RIB_ATTR_ID_UPDATE_VERSIONING:         return("UPDATE_VERSIONING");
      case CTRLM_RCU_RIB_ATTR_ID_PRODUCT_NAME:              return("PRODUCT_NAME");
      case CTRLM_RCU_RIB_ATTR_ID_DOWNLOAD_RATE:             return("DOWNLOAD_RATE");
      case CTRLM_RCU_RIB_ATTR_ID_UPDATE_POLLING_PERIOD:     return("UPDATE_POLLING_PERIOD");
      case CTRLM_RCU_RIB_ATTR_ID_DATA_REQUEST_WAIT_TIME:    return("DATA_REQUEST_WAIT_TIME");
      case CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE_STATUS:     return("IR_RF_DATABASE_STATUS");
      case CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE:            return("IR_RF_DATABASE");
      case CTRLM_RCU_RIB_ATTR_ID_VALIDATION_CONFIGURATION:  return("VALIDATION_CONFIGURATION");
      case CTRLM_RCU_RIB_ATTR_ID_CONTROLLER_IRDB_STATUS:    return("CONTROLLER_IRDB_STATUS");
      case CTRLM_RCU_RIB_ATTR_ID_TARGET_IRDB_STATUS:        return("TARGET_IRDB_STATUS");
      case CTRLM_RCU_RIB_ATTR_ID_FAR_FIELD_CONFIGURATION:   return("FARFIELD CONFIGURATION");
      case CTRLM_RCU_RIB_ATTR_ID_FAR_FIELD_METRICS:         return("FARFIELD METRICS");
      case CTRLM_RCU_RIB_ATTR_ID_DSP_CONFIGURATION:         return("DSP CONFIGURATION");
      case CTRLM_RCU_RIB_ATTR_ID_DSP_METRICS:               return("DSP METRICS");
      case CTRLM_RCU_RIB_ATTR_ID_MFG_TEST:                  return("MFG_TEST");
      case CTRLM_RCU_RIB_ATTR_ID_MEMORY_DUMP:               return("MEMORY_DUMP");
      case CTRLM_RCU_RIB_ATTR_ID_GENERAL_PURPOSE:           return("GENERAL_PURPOSE");
   }
   return(ctrlm_invalid_return(attribute_id));
}

const char *ctrlm_rcu_binding_type_str(ctrlm_rcu_binding_type_t binding_type) {
   switch(binding_type) {
      case CTRLM_RCU_BINDING_TYPE_INTERACTIVE: return("INTERACTIVE");
      case CTRLM_RCU_BINDING_TYPE_AUTOMATIC:   return("AUTOMATIC");
      case CTRLM_RCU_BINDING_TYPE_BUTTON:      return("BUTTON");
      case CTRLM_RCU_BINDING_TYPE_SCREEN_BIND: return("SCREENBIND");
      case CTRLM_RCU_BINDING_TYPE_INVALID:     return("INVALID");
   }
   return(ctrlm_invalid_return(binding_type));
}

const char *ctrlm_rcu_validation_type_str(ctrlm_rcu_validation_type_t validation_type) {
   switch(validation_type) {
      case CTRLM_RCU_VALIDATION_TYPE_APPLICATION:   return("APPLICATION");
      case CTRLM_RCU_VALIDATION_TYPE_INTERNAL:      return("INTERNAL");
      case CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC:     return("AUTOMATIC");
      case CTRLM_RCU_VALIDATION_TYPE_BUTTON:        return("BUTTON");
      case CTRLM_RCU_VALIDATION_TYPE_PRECOMMISSION: return("PRECOMMISSION");
      case CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND:   return("SCREENBIND");
      case CTRLM_RCU_VALIDATION_TYPE_INVALID:       return("INVALID");
   }
   return(ctrlm_invalid_return(validation_type));
}

const char *ctrlm_rcu_binding_security_type_str(ctrlm_rcu_binding_security_type_t security_type) {
   switch(security_type) {
      case CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL:   return("NORMAL");
      case CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED: return("ADVANCED");
   }
   return(ctrlm_invalid_return(security_type));
}

const char *ctrlm_rcu_ghost_code_str(ctrlm_rcu_ghost_code_t ghost_code) {
   switch(ghost_code) {
      case CTRLM_RCU_GHOST_CODE_VOLUME_UNITY_GAIN: return("VOLUME_UNITY_GAIN");
      case CTRLM_RCU_GHOST_CODE_POWER_OFF:         return("POWER_OFF");
      case CTRLM_RCU_GHOST_CODE_POWER_ON:          return("POWER_ON");
      case CTRLM_RCU_GHOST_CODE_IR_POWER_TOGGLE:   return("IR_POWER_TOGGLE");
      case CTRLM_RCU_GHOST_CODE_IR_POWER_OFF:      return("IR_POWER_OFF");
      case CTRLM_RCU_GHOST_CODE_IR_POWER_ON:       return("IR_POWER_ON");
      case CTRLM_RCU_GHOST_CODE_VOLUME_UP:         return("VOLUME_UP");
      case CTRLM_RCU_GHOST_CODE_VOLUME_DOWN:       return("VOLUME_DOWN");
      case CTRLM_RCU_GHOST_CODE_MUTE:              return("MUTE");
      case CTRLM_RCU_GHOST_CODE_INPUT:             return("INPUT");
      case CTRLM_RCU_GHOST_CODE_FIND_MY_REMOTE:    return ("FIND_MY_REMOTE");
      case CTRLM_RCU_GHOST_CODE_INVALID:           return("INVALID");
   }
   return(ctrlm_invalid_return(ghost_code));
}

const char *ctrlm_rcu_function_str(ctrlm_rcu_function_t function) {
   switch(function) {
      case CTRLM_RCU_FUNCTION_SETUP:                  return("SETUP");
      case CTRLM_RCU_FUNCTION_BACKLIGHT:              return("BACKLIGHT");
      case CTRLM_RCU_FUNCTION_POLL_FIRMWARE:          return("POLL_FIRMWARE");
      case CTRLM_RCU_FUNCTION_POLL_AUDIO_DATA:        return("POLL_AUDIO_DATA");
      case CTRLM_RCU_FUNCTION_RESET_SOFT:             return("RESET_SOFT");
      case CTRLM_RCU_FUNCTION_RESET_FACTORY:          return("RESET_FACTORY");
      case CTRLM_RCU_FUNCTION_BLINK_SOFTWARE_VERSION: return("BLINK_SOFTWARE_VERSION");
      case CTRLM_RCU_FUNCTION_BLINK_AVR_CODE:         return("BLINK_AVR_CODE");
      case CTRLM_RCU_FUNCTION_RESET_IR:               return("RESET_IR");
      case CTRLM_RCU_FUNCTION_RESET_RF:               return("RESET_RF");
      case CTRLM_RCU_FUNCTION_BLINK_TV_CODE:          return("BLINK_TV_CODE");
      case CTRLM_RCU_FUNCTION_IR_DB_TV_SEARCH:        return("IR_DB_TV_SEARCH");
      case CTRLM_RCU_FUNCTION_IR_DB_AVR_SEARCH:       return("IR_DB_AVR_SEARCH");
      case CTRLM_RCU_FUNCTION_KEY_REMAPPING:          return("KEY_REMAPPING");
      case CTRLM_RCU_FUNCTION_BLINK_IR_DB_VERSION:    return("BLINK_IR_DB_VERSION");
      case CTRLM_RCU_FUNCTION_BLINK_BATTERY_LEVEL:    return("BLINK_BATTERY_LEVEL");
      case CTRLM_RCU_FUNCTION_DISCOVERY:              return("DISCOVERY");
      case CTRLM_RCU_FUNCTION_MODE_IR_CLIP:           return("MODE_IR_CLIP");
      case CTRLM_RCU_FUNCTION_MODE_IR_MOT:            return("MODE_IR_MOT");
      case CTRLM_RCU_FUNCTION_MODE_IR_CIS:            return("MODE_IR_CIS");
      case CTRLM_RCU_FUNCTION_MODE_CLIP_DISCOVERY:    return("MODE_CLIP_DISCOVERY");
      case CTRLM_RCU_FUNCTION_IR_DB_TV_SELECT:        return("IR_DB_TV_SELECT");
      case CTRLM_RCU_FUNCTION_IR_DB_AVR_SELECT:       return("IR_DB_AVR_SELECT");
      case CTRLM_RCU_FUNCTION_INVALID_KEY_COMBO:      return("INVALID_KEY_COMBO");
      case CTRLM_RCU_FUNCTION_INVALID:                return("INVALID");
   }
   return(ctrlm_invalid_return(function));
}

const char *ctrlm_rcu_controller_type_str(ctrlm_rcu_controller_type_t controller_type) {
   switch(controller_type) {
      case CTRLM_RCU_CONTROLLER_TYPE_XR2:     return("XR2");
      case CTRLM_RCU_CONTROLLER_TYPE_XR5:     return("XR5");
      case CTRLM_RCU_CONTROLLER_TYPE_XR11:    return("XR11");
      case CTRLM_RCU_CONTROLLER_TYPE_XR15:    return("XR15");
      case CTRLM_RCU_CONTROLLER_TYPE_XR15V2:  return("XR15V2");
      case CTRLM_RCU_CONTROLLER_TYPE_XR16:    return("XR16");
      case CTRLM_RCU_CONTROLLER_TYPE_XR18:    return("XR18");
      case CTRLM_RCU_CONTROLLER_TYPE_XR19:    return("XR19");
      case CTRLM_RCU_CONTROLLER_TYPE_XRA:     return("XRA");
      case CTRLM_RCU_CONTROLLER_TYPE_UNKNOWN: return("UNKNOWN");
      case CTRLM_RCU_CONTROLLER_TYPE_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(controller_type));
}

const char *ctrlm_rcu_ir_remote_types_str(ctrlm_ir_remote_type controller_type) {
   switch(controller_type) {
      case CTRLM_IR_REMOTE_TYPE_XR11V2:    return("XR11-20");
      case CTRLM_IR_REMOTE_TYPE_XR15V1:    return("XR15-10");
      case CTRLM_IR_REMOTE_TYPE_NA:        return("N/A");
      case CTRLM_IR_REMOTE_TYPE_UNKNOWN:   return("unknown");
      case CTRLM_IR_REMOTE_TYPE_COMCAST:   return("COMCAST");
      case CTRLM_IR_REMOTE_TYPE_PLATCO:    return("PLATCO");
      case CTRLM_IR_REMOTE_TYPE_XR15V2:    return("XR15-20");
      case CTRLM_IR_REMOTE_TYPE_XR16V1:    return("XR16-10");
      case CTRLM_IR_REMOTE_TYPE_XRAV1:     return("XRA-10");
      case CTRLM_IR_REMOTE_TYPE_XR20V1:    return("XR20-10");
      case CTRLM_IR_REMOTE_TYPE_UNDEFINED: return("UNDEFINED");
   }
   return(ctrlm_invalid_return(controller_type));
}

const char *ctrlm_rcu_reverse_cmd_result_str(ctrlm_rcu_reverse_cmd_result_t result) {
   switch(result) {
      case CTRLM_RCU_REVERSE_CMD_SUCCESS:                return("SUCCESS");
      case CTRLM_RCU_REVERSE_CMD_FAILURE:                return("FAILURE");
      case CTRLM_RCU_REVERSE_CMD_CONTROLLER_NOT_CAPABLE: return("CONTROLLER_NOT_CAPABLE");
      case CTRLM_RCU_REVERSE_CMD_CONTROLLER_NOT_FOUND:   return("CONTROLLER_NOT_FOUND");
      case CTRLM_RCU_REVERSE_CMD_CONTROLLER_FOUND:       return("CONTROLLER_FOUND");
      case CTRLM_RCU_REVERSE_CMD_USER_INTERACTION:       return("USER_INTERACTION");
      case CTRLM_RCU_REVERSE_CMD_DISABLED:               return("DISABLED");
      case CTRLM_RCU_REVERSE_CMD_INVALID:                return("INVALID");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_voice_session_result_str(ctrlm_voice_session_result_t result) {
   switch(result) {
      case CTRLM_VOICE_SESSION_RESULT_SUCCESS: return("SUCCESS");
      case CTRLM_VOICE_SESSION_RESULT_FAILURE: return("FAILURE");
      case CTRLM_VOICE_SESSION_RESULT_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_voice_session_end_reason_str(ctrlm_voice_session_end_reason_t reason) {
   switch(reason) {
      case CTRLM_VOICE_SESSION_END_REASON_DONE:                 return("DONE");
      case CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_FIRST_PACKET: return("TIMEOUT_FIRST_PACKET");
      case CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_INTERPACKET:  return("TIMEOUT_INTERPACKET");
      case CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_MAXIMUM:      return("TIMEOUT_MAXIMUM");
      case CTRLM_VOICE_SESSION_END_REASON_ADJACENT_KEY_PRESSED: return("ADJACENT_KEY_PRESSED");
      case CTRLM_VOICE_SESSION_END_REASON_OTHER_KEY_PRESSED:    return("OTHER_KEY_PRESSED");
      case CTRLM_VOICE_SESSION_END_REASON_NEW_SESSION:          return("NEW_SESSION");
      case CTRLM_VOICE_SESSION_END_REASON_OTHER_ERROR:          return("OTHER_ERROR");
      case CTRLM_VOICE_SESSION_END_REASON_MINIMUM_QOS:          return("MINIMUM_QOS");
      case CTRLM_VOICE_SESSION_END_REASON_MAX:                  return("MAX");
   }
   return(ctrlm_invalid_return(reason));
}

const char *ctrlm_voice_session_abort_reason_str(ctrlm_voice_session_abort_reason_t reason) {
   switch(reason) {
      case CTRLM_VOICE_SESSION_ABORT_REASON_BUSY:                  return("BUSY");
      case CTRLM_VOICE_SESSION_ABORT_REASON_SERVER_NOT_READY:      return("SERVER_NOT_READY");
      case CTRLM_VOICE_SESSION_ABORT_REASON_AUDIO_FORMAT:          return("AUDIO_FORMAT");
      case CTRLM_VOICE_SESSION_ABORT_REASON_DEVICE_UPDATE:         return("DEVICE_UPDATE");
      case CTRLM_VOICE_SESSION_ABORT_REASON_FAILURE:               return("FAILURE");
      case CTRLM_VOICE_SESSION_ABORT_REASON_VOICE_DISABLED:        return("VOICE_DISABLED");
      case CTRLM_VOICE_SESSION_ABORT_REASON_NO_RECEIVER_ID:        return("NO RECEIVER_ID");
      case CTRLM_VOICE_SESSION_ABORT_REASON_NEW_SESSION:           return("NEW_SESSION");
      case CTRLM_VOICE_SESSION_ABORT_REASON_INVALID_CONTROLLER_ID: return("INVALID_CONTROLLER_ID");
      case CTRLM_VOICE_SESSION_ABORT_REASON_APPLICATION_RESTART:   return("APPLICATION_RESTART");
      case CTRLM_VOICE_SESSION_ABORT_REASON_MAX:                   return("MAX");
   }
   return(ctrlm_invalid_return(reason));
}

const char *ctrlm_voice_internal_error_str(ctrlm_voice_internal_error_t error) {
   switch(error) {
      case CTRLM_VOICE_INTERNAL_ERROR_NONE:          return("NONE");
      case CTRLM_VOICE_INTERNAL_ERROR_EXCEPTION:     return("EXCEPTION");
      case CTRLM_VOICE_INTERNAL_ERROR_THREAD_CREATE: return("THREAD_CREATE");
      case CTRLM_VOICE_INTERNAL_ERROR_MAX:           return("MAX");
   }
   return(ctrlm_invalid_return(error));
}

const char *ctrlm_voice_reset_type_str(ctrlm_voice_reset_type_t reset_type) {
   switch(reset_type) {
      case CTRLM_VOICE_RESET_TYPE_POWER_ON:   return("POWER_ON");
      case CTRLM_VOICE_RESET_TYPE_EXTERNAL:   return("EXTERNAL");
      case CTRLM_VOICE_RESET_TYPE_WATCHDOG:   return("WATCHDOG");
      case CTRLM_VOICE_RESET_TYPE_CLOCK_LOSS: return("CLOCK_LOSS");
      case CTRLM_VOICE_RESET_TYPE_BROWN_OUT:  return("BROWN_OUT");
      case CTRLM_VOICE_RESET_TYPE_OTHER:      return("OTHER");
      case CTRLM_VOICE_RESET_TYPE_MAX:        return("MAX");
   }
   return(ctrlm_invalid_return(reset_type));
}
const char *ctrlm_device_update_iarm_load_type_str(ctrlm_device_update_iarm_load_type_t load_type) {
   switch(load_type) {
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_DEFAULT: return("DEFAULT");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_NORMAL:  return("NORMAL");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_POLL:    return("POLL");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_ABORT:   return("ABORT");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(load_type));
}

const char *ctrlm_device_update_iarm_load_result_str(ctrlm_device_update_iarm_load_result_t load_result) {
   switch(load_result) {
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_SUCCESS:        return("SUCCESS");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REQUEST:  return("ERROR_REQUEST");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_CRC:      return("ERROR_CRC");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_ABORT:    return("ERROR_ABORT");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REJECT:   return("ERROR_REJECT");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_TIMEOUT:  return("ERROR_TIMEOUT");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_BAD_HASH: return("ERROR_BAD_HASH");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_OTHER:    return("ERROR_OTHER");
      case CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_MAX:            return("MAX");
   }
   return(ctrlm_invalid_return(load_result));
}

const char *ctrlm_device_update_image_type_str(ctrlm_device_update_image_type_t image_type) {
   switch(image_type) {
      case CTRLM_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE:   return("FIRMWARE");
      case CTRLM_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA: return("AUDIO_DATA");
      case CTRLM_DEVICE_UPDATE_IMAGE_TYPE_OTHER:      return("OTHER");
      case CTRLM_DEVICE_UPDATE_IMAGE_TYPE_MAX:        return("MAX");
   }
   return(ctrlm_invalid_return(image_type));
}

const char *ctrlm_hal_result_str(ctrlm_hal_result_t result) {
   switch(result) {
      case CTRLM_HAL_RESULT_SUCCESS:                 return("SUCCESS");
      case CTRLM_HAL_RESULT_ERROR:                   return("ERROR");
      case CTRLM_HAL_RESULT_ERROR_NETWORK_ID:        return("ERROR_NETWORK_ID");
      case CTRLM_HAL_RESULT_ERROR_NETWORK_NOT_READY: return("ERROR_NETWORK_NOT_READY");
      case CTRLM_HAL_RESULT_ERROR_CONTROLLER_ID:     return("ERROR_CONTROLLER_ID");
      case CTRLM_HAL_RESULT_ERROR_OUT_OF_MEMORY:     return("ERROR_OUT_OF_MEMORY");
      case CTRLM_HAL_RESULT_ERROR_BIND_TABLE_FULL:   return("ERROR_BIND_TABLE_FULL");
      case CTRLM_HAL_RESULT_ERROR_INVALID_PARAMS:    return("ERROR_INVALID_PARAMS");
      case CTRLM_HAL_RESULT_ERROR_INVALID_NVM:       return("ERROR_INVALID_NVM");
      case CTRLM_HAL_RESULT_ERROR_NOT_IMPLEMENTED:   return("ERROR_NOT_IMPLEMENTED");
      case CTRLM_HAL_RESULT_MAX:                     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_hal_frequency_agility_str(ctrlm_hal_frequency_agility_t frequency_agility) {
   switch(frequency_agility) {
      case CTRLM_HAL_FREQUENCY_AGILITY_DISABLE:   return("DISABLE");
      case CTRLM_HAL_FREQUENCY_AGILITY_ENABLE:    return("ENABLE");
      case CTRLM_HAL_FREQUENCY_AGILITY_NO_CHANGE: return("NO CHANGE");
      case CTRLM_HAL_FREQUENCY_AGILITY_MAX:       return("MAX");
   }
   return(ctrlm_invalid_return(frequency_agility));
}

const char *ctrlm_hal_result_discovery_str(ctrlm_hal_result_discovery_t result) {
   switch(result) {
      case CTRLM_HAL_RESULT_DISCOVERY_PENDING: return("PENDING");
      case CTRLM_HAL_RESULT_DISCOVERY_RESPOND: return("RESPOND");
      case CTRLM_HAL_RESULT_DISCOVERY_IGNORE:  return("IGNORE");
      case CTRLM_HAL_RESULT_DISCOVERY_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_hal_result_pair_request_str(ctrlm_hal_result_pair_request_t result) {
   switch(result) {
      case CTRLM_HAL_RESULT_PAIR_REQUEST_PENDING: return("PENDING");
      case CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND: return("RESPOND");
      case CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE:  return("IGNORE");
      case CTRLM_HAL_RESULT_PAIR_REQUEST_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_hal_result_pair_str(ctrlm_hal_result_pair_t result) {
   switch(result) {
      case CTRLM_HAL_RESULT_PAIR_SUCCESS: return("SUCCESS");
      case CTRLM_HAL_RESULT_PAIR_FAILURE: return("FAILURE");
      case CTRLM_HAL_RESULT_PAIR_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_hal_result_unpair_str(ctrlm_hal_result_unpair_t result) {
   switch(result) {
      case CTRLM_HAL_RESULT_UNPAIR_PENDING: return("PENDING");
      case CTRLM_HAL_RESULT_UNPAIR_SUCCESS: return("SUCCESS");
      case CTRLM_HAL_RESULT_UNPAIR_FAILURE: return("FAILURE");
      case CTRLM_HAL_RESULT_UNPAIR_MAX:     return("MAX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_hal_network_property_str(ctrlm_hal_network_property_t property) {
   switch(property) {
      case CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_LIST:     return("CONTROLLER_LIST");
      case CTRLM_HAL_NETWORK_PROPERTY_NETWORK_STATS:       return("NETWORK_STATS");
      case CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_STATS:    return("CONTROLLER_STATS");
      case CTRLM_HAL_NETWORK_PROPERTY_FREQUENCY_AGILITY:   return("FREQUENCY_AGILITY");
      case CTRLM_HAL_NETWORK_PROPERTY_PAIRING_TABLE_ENTRY: return("PAIRING_TABLE_ENTRY");
      case CTRLM_HAL_NETWORK_PROPERTY_FACTORY_RESET:       return("FACTORY_RESET");
      case CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_IMPORT:   return("CONTROLLER_IMPORT");
      case CTRLM_HAL_NETWORK_PROPERTY_THREAD_MONITOR:      return("THREAD_MONITOR");
      case CTRLM_HAL_NETWORK_PROPERTY_ENCRYPTION_KEY:      return("ENCRYPTION_KEY");
#if CTRLM_HAL_RF4CE_API_VERSION >= 10
      case CTRLM_HAL_NETWORK_PROPERTY_DPI_CONTROL:         return("DPI_CONTROL");
#if CTRLM_HAL_RF4CE_API_VERSION >= 14
      case CTRLM_HAL_NETWORK_PROPERTY_NVM_VERSION:         return("NVM_VERSION");
      case CTRLM_HAL_NETWORK_PROPERTY_INDIRECT_TX_TIMEOUT: return("INDIRECT_TX_TIMEOUT");
#if CTRLM_HAL_RF4CE_API_VERSION >= 16
      case CTRLM_HAL_NETWORK_PROPERTY_AUTO_ACK:            return("AUTO_ACK");
#endif
#endif
#endif
      case CTRLM_HAL_NETWORK_PROPERTY_MAX:                 return("MAX");
   }
   return(ctrlm_invalid_return(property));
}

const char *ctrlm_hal_rf4ce_result_str(ctrlm_hal_rf4ce_result_t result) {
   switch(result) {
      case CTRLM_HAL_RF4CE_RESULT_SUCCESS:                return("SUCCESS");
      case CTRLM_HAL_RF4CE_RESULT_NO_ORG_CAPACITY:        return("NO_ORG_CAPACITY");
      case CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY:        return("NO_REC_CAPACITY");
      case CTRLM_HAL_RF4CE_RESULT_NO_PAIRING:             return("NO_PAIRING");
      case CTRLM_HAL_RF4CE_RESULT_NO_RESPONSE:            return("NO_RESPONSE");
      case CTRLM_HAL_RF4CE_RESULT_NOT_PERMITTED:          return("NOT_PERMITTED");
      case CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING:      return("DUPLICATE_PAIRING");
      case CTRLM_HAL_RF4CE_RESULT_FRAME_COUNTER_EXPIRED:  return("FRAME_COUNTER_EXPIRED");
      case CTRLM_HAL_RF4CE_RESULT_DISCOVERY_ERROR:        return("DISCOVERY_ERROR");
      case CTRLM_HAL_RF4CE_RESULT_DISCOVERY_TIMEOUT:      return("DISCOVERY_TIMEOUT");
      case CTRLM_HAL_RF4CE_RESULT_SECURITY_TIMEOUT:       return("SECURITY_TIMEOUT");
      case CTRLM_HAL_RF4CE_RESULT_SECURITY_FAILURE:       return("SECURITY_FAILURE");
      case CTRLM_HAL_RF4CE_RESULT_BEACON_LOSS:            return("BEACON_LOSS");
      case CTRLM_HAL_RF4CE_RESULT_CHANNEL_ACCESS_FAILURE: return("CHANNEL_ACCESS_FAILURE");
      case CTRLM_HAL_RF4CE_RESULT_DENIED:                 return("DENIED");
      case CTRLM_HAL_RF4CE_RESULT_DISABLE_TRX_FAILURE:    return("DISABLE_TRX_FAILURE");
      case CTRLM_HAL_RF4CE_RESULT_FAILED_SECURITY_CHECK:  return("FAILED_SECURITY_CHECK");
      case CTRLM_HAL_RF4CE_RESULT_FRAME_TOO_LONG:         return("FRAME_TOO_LONG");
      case CTRLM_HAL_RF4CE_RESULT_INVALID_GTS:            return("INVALID_GTS");
      case CTRLM_HAL_RF4CE_RESULT_INVALID_HANDLE:         return("INVALID_HANDLE");
      case CTRLM_HAL_RF4CE_RESULT_INVALID_PARAMETER:      return("INVALID_PARAMETER");
      case CTRLM_HAL_RF4CE_RESULT_NO_ACK:                 return("NO_ACK");
      case CTRLM_HAL_RF4CE_RESULT_NO_BEACON:              return("NO_BEACON");
      case CTRLM_HAL_RF4CE_RESULT_NO_DATA:                return("NO_DATA");
      case CTRLM_HAL_RF4CE_RESULT_NO_SHORT_ADDRESS:       return("NO_SHORT_ADDRESS");
      case CTRLM_HAL_RF4CE_RESULT_OUT_OF_CAP:             return("OUT_OF_CAP");
      case CTRLM_HAL_RF4CE_RESULT_PAN_ID_CONFLICT:        return("PAN_ID_CONFLICT");
      case CTRLM_HAL_RF4CE_RESULT_REALIGNMENT:            return("REALIGNMENT");
      case CTRLM_HAL_RF4CE_RESULT_TRANSACTION_EXPIRED:    return("TRANSACTION_EXPIRED");
      case CTRLM_HAL_RF4CE_RESULT_TRANSACTION_OVERFLOW:   return("TRANSACTION_OVERFLOW");
      case CTRLM_HAL_RF4CE_RESULT_TX_ACTIVE:              return("TX_ACTIVE");
      case CTRLM_HAL_RF4CE_RESULT_UNAVAILABLE_KEY:        return("UNAVAILABLE_KEY");
      case CTRLM_HAL_RF4CE_RESULT_UNSUPPORTED_ATTRIBUTE:  return("UNSUPPORTED_ATTRIBUTE");
      case CTRLM_HAL_RF4CE_RESULT_INVALID_INDEX:          return("INVALID_INDEX");
   }
   return(ctrlm_invalid_return(result));
}

const char *ctrlm_key_status_str(ctrlm_key_status_t key_status) {
   switch(key_status) {
      case CTRLM_KEY_STATUS_DOWN:    return("DOWN");
      case CTRLM_KEY_STATUS_UP:      return("UP");
      case CTRLM_KEY_STATUS_REPEAT:  return("REPEAT");
      case CTRLM_KEY_STATUS_INVALID: return("INVALID");
   }
   return(ctrlm_invalid_return(key_status));
}

const char *ctrlm_bind_status_str(ctrlm_bind_status_t bind_status) {
   switch(bind_status) {
      case CTRLM_BIND_STATUS_SUCCESS:                   return("SUCCESS");
      case CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST:      return("NO_DISCOVERY_REQUEST");
      case CTRLM_BIND_STATUS_NO_PAIRING_REQUEST:        return("NO_PAIRING_REQUEST");
      case CTRLM_BIND_STATUS_HAL_FAILURE:               return("HAL_FAILURE");
      case CTRLM_BIND_STATUS_CTRLM_BLACKOUT:            return("CTRLM_BLACKOUT");
      case CTRLM_BIND_STATUS_ASB_FAILURE:               return("ASB_FAILURE");
      case CTRLM_BIND_STATUS_STD_KEY_EXCHANGE_FAILURE:  return("STD_KEY_EXCHANGE_FAILURE");
      case CTRLM_BIND_STATUS_PING_FAILURE:              return("PING_FAILURE");
      case CTRLM_BIND_STATUS_VALILDATION_FAILURE:       return("VALILDATION_FAILURE");
      case CTRLM_BIND_STATUS_RIB_UPDATE_FAILURE:        return("RIB_UPDATE_FAILURE");
      case CTRLM_BIND_STATUS_BIND_WINDOW_TIMEOUT:       return("BIND_WINDOW_TIMEOUT");
      case CTRLM_BIND_STATUS_UNKNOWN_FAILURE:           return("UNKNOWN_FAILURE");
   }
   return(ctrlm_invalid_return(bind_status));
}

const char *ctrlm_close_pairing_window_reason_str(ctrlm_close_pairing_window_reason reason) {
   switch(reason) {
      case CTRLM_CLOSE_PAIRING_WINDOW_REASON_PAIRING_SUCCESS: return("PAIRING_SUCCESS");
      case CTRLM_CLOSE_PAIRING_WINDOW_REASON_END:             return("END");
      case CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT:         return("TIMEOUT");
   }
   return(ctrlm_invalid_return(reason));
}

const char *ctrlm_battery_event_str(ctrlm_rcu_battery_event_t event) {
   switch(event) {
   case CTRLM_RCU_BATTERY_EVENT_NONE:          return("BATTERY_EVENT_NONE");
   case CTRLM_RCU_BATTERY_EVENT_REPLACED:      return("BATTERY_EVENT_REPLACED");
   case CTRLM_RCU_BATTERY_EVENT_CHARGING:      return("BATTERY_EVENT_CHARGING");
   case CTRLM_RCU_BATTERY_EVENT_PENDING_DOOM:  return("BATTERY_EVENT_PENDING_DOOM");
   case CTRLM_RCU_BATTERY_EVENT_INVALID:       return("BATTERY_EVENT_INVALID");
   case CTRLM_RCU_BATTERY_EVENT_75_PERCENT:    return("BATTERY_EVENT_75_PERCENT");
   case CTRLM_RCU_BATTERY_EVENT_50_PERCENT:    return("BATTERY_EVENT_50_PERCENT");
   case CTRLM_RCU_BATTERY_EVENT_25_PERCENT:    return("BATTERY_EVENT_25_PERCENT");
   case CTRLM_RCU_BATTERY_EVENT_0_PERCENT:     return("BATTERY_EVENT_0_PERCENT");
   default:                                    break;
   }
   return(ctrlm_invalid_return(event));
}

const char *ctrlm_rf4ce_reboot_reason_str(controller_reboot_reason_t reboot_reason) {
   switch(reboot_reason) {
   case CONTROLLER_REBOOT_POWER_ON:      return("POWER_ON");
   case CONTROLLER_REBOOT_EXTERNAL:      return("EXTERNAL");
   case CONTROLLER_REBOOT_WATCHDOG:      return("WATCHDOG");
   case CONTROLLER_REBOOT_CLOCK_LOSS:    return("CLOCK_LOSS");
   case CONTROLLER_REBOOT_BROWN_OUT:     return("BROWN_OUT");
   case CONTROLLER_REBOOT_OTHER:         return("OTHER");
   case CONTROLLER_REBOOT_ASSERT_NUMBER: return("ASSERT_NUMBER");
   }
   return(ctrlm_invalid_return(reboot_reason));
}

const char *ctrlm_dsp_event_str(ctrlm_rcu_dsp_event_t event) {
   switch(event) {
   case CTRLM_RCU_DSP_EVENT_MIC_FAILURE:          return("MIC_FAILURE");
   case CTRLM_RCU_DSP_EVENT_SPEAKER_FAILURE:      return("SPEAKER_FAILURE");
   case CTRLM_RCU_DSP_EVENT_INVALID:              return("INVALID");
   default:                                    break;
   }
   return(ctrlm_invalid_return(event));
}

const char *ctrlm_ir_state_str(ctrlm_ir_state_t state) {
   switch(state) {
   case CTRLM_IR_STATE_IDLE:          return("IDLE");
   case CTRLM_IR_STATE_WAITING:       return("WAITING");
   case CTRLM_IR_STATE_COMPLETE:      return("COMPLETE");
   case CTRLM_IR_STATE_FAILED:        return("FAILED");
   case CTRLM_IR_STATE_UNKNOWN:       return("UNKNOWN");
   default:                           break;
   }
   return(ctrlm_invalid_return(state));
}

const char *ctrlm_power_state_str(ctrlm_power_state_t state)
{
   switch(state) {
   case CTRLM_POWER_STATE_STANDBY:                return("STANDBY");
   case CTRLM_POWER_STATE_ON:                     return("ON");
   case CTRLM_POWER_STATE_DEEP_SLEEP:             return("DEEP_SLEEP");
   default:                                       break;
   }
   return(ctrlm_invalid_return(state));
}

const char *ctrlm_device_type_str(ctrlm_device_type_t device_type) {
   switch(device_type) {
   case CTRLM_DEVICE_TYPE_STB_IP:  return("STB_IP");
   case CTRLM_DEVICE_TYPE_STB_QAM: return("STB_QAM");
   case CTRLM_DEVICE_TYPE_TV:      return("TV");
   case CTRLM_DEVICE_TYPE_INVALID: return("INVALID");
   default:                        break;
   }
   return(ctrlm_invalid_return(device_type));
}

static const char *ctrlm_key_code_to_string[256] = {
      "OK",               // 0x00
      "UP_ARROW",         // 0x01
      "DOWN_ARROW",       // 0x02
      "LEFT_ARROW",       // 0x03
      "RIGHT_ARROW",      // 0x04
      "RESERVED",         // 0x05
      "RESERVED",         // 0x06
      "RESERVED",         // 0x07
      "RESERVED",         // 0x08
      "MENU",             // 0x09
      "RESERVED",         // 0x0A
      "DVR",              // 0x0B
      "FAV",              // 0x0C
      "EXIT",             // 0x0D
      "RESERVED",         // 0x0E
      "RESERVED",         // 0x0F
      "HOME",             // 0x10
      "RESERVED",         // 0x11
      "RESERVED",         // 0x12
      "RESERVED",         // 0x13
      "RESERVED",         // 0x14
      "RESERVED",         // 0x15
      "RESERVED",         // 0x16
      "RESERVED",         // 0x17
      "RESERVED",         // 0x18
      "RESERVED",         // 0x19
      "RESERVED",         // 0x1A
      "RESERVED",         // 0x1B
      "RESERVED",         // 0x1C
      "RESERVED",         // 0x1D
      "RESERVED",         // 0x1E
      "RESERVED",         // 0x1F
      "0",                // 0x20
      "1",                // 0x21
      "2",                // 0x22
      "3",                // 0x23
      "4",                // 0x24
      "5",                // 0x25
      "6",                // 0x26
      "7",                // 0x27
      "8",                // 0x28
      "9",                // 0x29
      ".",                // 0x2A
      "RETURN",           // 0x2B
      "RESERVED",         // 0x2C
      "RESERVED",         // 0x2D
      "RESERVED",         // 0x2E
      "RESERVED",         // 0x2F
      "CH_UP",            // 0x30
      "CH_DOWN",          // 0x31
      "LAST",             // 0x32
      "LANG",             // 0x33
      "INPUT_SELECT",     // 0x34
      "INFO",             // 0x35
      "HELP",             // 0x36
      "PAGE_UP",          // 0x37
      "PAGE_DOWN",        // 0x38
      "RESERVED",         // 0x39
      "RESERVED",         // 0x3A
      "MOTION",           // 0x3B
      "SEARCH",           // 0x3C
      "LIVE",             // 0x3D
      "HD_ZOOM",          // 0x3E
      "SHARE",            // 0x3F
      "TV_POWER",         // 0x40
      "VOL_UP",           // 0x41
      "VOL_DOWN",         // 0x42
      "MUTE",             // 0x43
      "PLAY",             // 0x44
      "STOP",             // 0x45
      "PAUSE",            // 0x46
      "RECORD",           // 0x47
      "REWIND",           // 0x48
      "FAST_FORWARD",     // 0x49
      "RESERVED",         // 0x4A
      "30_SEC_SKIP",      // 0x4B
      "REPLAY",           // 0x4C
      "TV_POWER_ON",      // 0x4D
      "TV_POWER_OFF",     // 0x4E
      "RESERVED",         // 0x4F
      "RESERVED",         // 0x50
      "SWAP",             // 0x51
      "ON_DEMAND",        // 0x52
      "GUIDE",            // 0x53
      "RESERVED",         // 0x54
      "RESERVED",         // 0x55
      "RESERVED",         // 0x56
      "PUSH_TO_TALK",     // 0x57
      "PIP_ON_OFF",       // 0x58
      "PIP_MOVE",         // 0x59
      "PIP_CH_UP",        // 0x5A
      "PIP_CH_DOWN",      // 0x5B
      "LOCK",             // 0x5C
      "DAY_PLUS",         // 0x5D
      "DAY_MINUS",        // 0x5E
      "RESERVED",         // 0x5F
      "RESERVED",         // 0x60
      "PLAY_PAUSE",       // 0x61
      "RESERVED",         // 0x62
      "RESERVED",         // 0x63
      "STOP_VIDEO",       // 0x64
      "MUTE_MIC",         // 0x65
      "RESERVED",         // 0x66
      "RESERVED",         // 0x67
      "AVR_POWER_TOGGLE", // 0x68
      "AVR_POWER_OFF",    // 0x69
      "AVR_POWER_ON",     // 0x6A
      "POWER_TOGGLE",     // 0x6B
      "POWER_OFF",        // 0x6C
      "POWER_ON",         // 0x6D
      "RESERVED",         // 0x6E
      "RESERVED",         // 0x6F
      "RESERVED",         // 0x70
      "OCAP_B",           // 0x71
      "OCAP_C",           // 0x72
      "OCAP_D",           // 0x73
      "OCAP_A",           // 0x74
      "RESERVED",         // 0x75
      "RESERVED",         // 0x76
      "RESERVED",         // 0x77
      "RESERVED",         // 0x78
      "RESERVED",         // 0x79
      "RESERVED",         // 0x7A
      "RESERVED",         // 0x7B
      "RESERVED",         // 0x7C
      "RESERVED",         // 0x7D
      "RESERVED",         // 0x7E
      "RESERVED",         // 0x7F
      "RESERVED",         // 0x80
      "RESERVED",         // 0x81
      "RESERVED",         // 0x82
      "RESERVED",         // 0x83
      "RESERVED",         // 0x84
      "RESERVED",         // 0x85
      "RESERVED",         // 0x86
      "RESERVED",         // 0x87
      "RESERVED",         // 0x88
      "RESERVED",         // 0x89
      "RESERVED",         // 0x8A
      "RESERVED",         // 0x8B
      "RESERVED",         // 0x8C
      "RESERVED",         // 0x8D
      "RESERVED",         // 0x8E
      "RESERVED",         // 0x8F
      "CC",               // 0x90
      "RESERVED",         // 0x91
      "RESERVED",         // 0x92
      "RESERVED",         // 0x93
      "RESERVED",         // 0x94
      "RESERVED",         // 0x95
      "RESERVED",         // 0x96
      "RESERVED",         // 0x97
      "RESERVED",         // 0x98
      "RESERVED",         // 0x99
      "RESERVED",         // 0x9A
      "RESERVED",         // 0x9B
      "RESERVED",         // 0x9C
      "RESERVED",         // 0x9D
      "RESERVED",         // 0x9E
      "RESERVED",         // 0x9F
      "PROFILE",          // 0xA0
      "CALL",             // 0xA1
      "HOLD",             // 0xA2
      "END",              // 0xA3
      "VIEWS",            // 0xA4
      "SELF_VIEW",        // 0xA5
      "ZOOM_IN",          // 0xA6
      "ZOOM_OUT",         // 0xA7
      "BACKSPACE",        // 0xA8
      "LOCK_UNLOCK",      // 0xA9
      "CAPS",             // 0xAA
      "ALT",              // 0xAB
      " ",                // 0xAC
      "WWW.",             // 0xAD
      ".COM",             // 0xAE
      "RESERVED",         // 0xAF
      "A",                // 0xB0
      "B",                // 0xB1
      "C",                // 0xB2
      "D",                // 0xB3
      "E",                // 0xB4
      "F",                // 0xB5
      "G",                // 0xB6
      "H",                // 0xB7
      "I",                // 0xB8
      "J",                // 0xB9
      "K",                // 0xBA
      "L",                // 0xBB
      "M",                // 0xBC
      "N",                // 0xBD
      "O",                // 0xBE
      "P",                // 0xBF
      "Q",                // 0xC0
      "R",                // 0xC1
      "S",                // 0xC2
      "T",                // 0xC3
      "U",                // 0xC4
      "V",                // 0xC5
      "W",                // 0xC6
      "X",                // 0xC7
      "Y",                // 0xC8
      "Z",                // 0xC9
      "a",                // 0xCA
      "b",                // 0xCB
      "c",                // 0xCC
      "d",                // 0xCD
      "e",                // 0xCE
      "f",                // 0xCF
      "g",                // 0xD0
      "h",                // 0xD1
      "i",                // 0xD2
      "j",                // 0xD3
      "k",                // 0xD4
      "l",                // 0xD5
      "m",                // 0xD6
      "n",                // 0xD7
      "o",                // 0xD8
      "p",                // 0xD9
      "q",                // 0xDA
      "r",                // 0xDB
      "s",                // 0xDC
      "t",                // 0xDD
      "u",                // 0xDE
      "v",                // 0xDF
      "w",                // 0xE0
      "x",                // 0xE1
      "y",                // 0xE2
      "z",                // 0xE3
      "?",                // 0xE4
      "!",                // 0xE5
      "POUND",            // 0xE6
      "$",                // 0xE7
      "%",                // 0xE8
      "&",                // 0xE9
      "*",                // 0xEA
      "(",                // 0xEB
      ")",                // 0xEC
      "+",                // 0xED
      "-",                // 0xEE
      "=",                // 0xEF
      "/",                // 0xF0
      "_",                // 0xF1
      "\"",               // 0xF2
      ":",                // 0xF3
      ";",                // 0xF4
      "@",                // 0xF5
      "'",                // 0xF6
      ",",                // 0xF7
      "RESERVED",         // 0xF8
      "RESERVED",         // 0xF9
      "RESERVED",         // 0xFA
      "RESERVED",         // 0xFB
      "RESERVED",         // 0xFC
      "RESERVED",         // 0xFD
      "RESERVED",         // 0xFE
      "RESERVED",         // 0xFF
};

const char *ctrlm_key_code_str(ctrlm_key_code_t key_code) {
   if(key_code < 256) {
      return(ctrlm_key_code_to_string[key_code]);
   }
   return(ctrlm_invalid_return(key_code));
}

// map to convert a key code to its identifiable name on the remote.
// map<key code, tuple<name, masked_name>>
static const map<uint16_t, tuple<const char*, const char*>> ctrlm_linux_key_names {
   {KEY_1,             {"1",                    "Numeric"}},
   {KEY_2,             {"2",                    "Numeric"}},
   {KEY_3,             {"3",                    "Numeric"}},
   {KEY_4,             {"4",                    "Numeric"}},
   {KEY_5,             {"5",                    "Numeric"}},
   {KEY_6,             {"6",                    "Numeric"}},
   {KEY_7,             {"7",                    "Numeric"}},
   {KEY_8,             {"8",                    "Numeric"}},
   {KEY_9,             {"9",                    "Numeric"}},
   {KEY_0,             {"0",                    "Numeric"}},
   {KEY_F1,            {"Standby",              "Standby"}},
   {KEY_F15,           {"Input",                "Input"}},
   {KEY_UP,            {"Up",                   "Directional"}},
   {KEY_LEFT,          {"Left",                 "Directional"}},
   {KEY_RIGHT,         {"Right",                "Directional"}},
   {KEY_DOWN,          {"Down",                 "Directional"}},
   {KEY_ENTER,         {"Select",               "Select"}},
   {KEY_KPPLUS,        {"Volume+",              "Volume+"}},
   {KEY_KPMINUS,       {"Volume-",              "Volume-"}},
   {KEY_KPASTERISK,    {"Mute",                 "Mute"}},
   {KEY_HOME,          {"Home",                 "Home"}},
   {KEY_F17,           {"Proximity Sensor",     "Proximity Sensor"}},
   {KEY_F18,           {"Xumo App",             "Xumo App"}},
   {KEY_F19,           {"YouTube App",          "YouTube App"}},
   {KEY_F20,           {"Info",                 "Info"}},
   {KEY_F21,           {"Guide",                "Guide"}},
   {KEY_F22,           {"Accessibility",        "Accessibility"}},
   {KEY_F8,            {"Voice",                "Voice"}},
   {KEY_ESC,           {"Dismiss",              "Dismiss"}},
   {KEY_F9,            {"Info",                 "Info"}},
   {KEY_BATTERY,       {"Battery Low",          "Battery Low"}},
   {KEY_F16,           {"Plus",                 "Plus"}},
   {KEY_F13,           {"Option",               "Option"}},
   {KEY_F4,            {"Netflix App",          "Netflix App"}},  // APP key for PLATCO
   {KEY_INSERT,        {"Peacock App",          "Peakcock App"}},  // APP key for PLATCO
   {KEY_END,           {"Disney+ App",          "Disney+ App"}},  // APP key for PLATCO
   {KEY_DELETE,        {"Prime Video App",      "Prime Video App"}},  // APP key for PLATCO
   {KEY_PAGEUP,        {"Channel+",             "Channel+"}},
   {KEY_PAGEDOWN,      {"Channel-",             "Channel-"}},
   {KEY_F2,            {"Help/Cog",             "Help/Cog"}},
   {KEY_F5,            {"Apps",                 "Apps"}},
   {KEY_F10,           {"Rewind",               "Rewind"}},
   {KEY_F11,           {"Play",                 "Play"}},
   {KEY_F12,           {"FFwd",                 "FFwd"}},
   {KEY_F7,            {"Record",               "Record"}},
   {KEY_F3,            {"Search",               "Search"}},
   {KEY_F6,            {"Provider",             "Provider"}},
   {KEY_F14,           {"Quick Access",         "Quick Access"}},
   {KEY_KPRIGHTPAREN,  {"App 1",                "App 1"}}, // APP key for LLAMA
   {KEY_KPLEFTPAREN,   {"App 2",                "App 2"}}, // APP key for LLAMA
   {KEY_KPCOMMA,       {"App 3",                "App 3"}}, // APP key for LLAMA
   {KEY_CONNECT,       {"US BLE Pairing",       "US BLE Pairing"}},
   {KEY_BLUETOOTH,     {"EU BLE Pairing",       "EU BLE Pairing"}},
   {KEY_VOLUMEDOWN,    {"Vol Down",             "Vol Down"}},
   {KEY_VOLUMEUP,      {"Vol Up",               "Vol Up"}},
   {KEY_RECORD,        {"Record",               "Record"}},
   {KEY_INFO,          {"Info",                 "Info"}},
   {KEY_CHANNELUP,     {"Chan Up",              "Chan Up"}},
   {KEY_CHANNELDOWN,   {"Chan Down",            "Chan Down"}},
   {KEY_SEARCH,        {"Voice",                "Voice"}},
   {KEY_SELECT,        {"Select",               "Select"}},
   {KEY_BACK,          {"Back",                 "Back"}},
   {KEY_PROGRAM,       {"EPG",                  "EPG"}},
   {KEY_HOMEPAGE,      {"Home",                 "Home"}},
   {KEY_PLAYPAUSE,     {"Play/Pause",           "Play/Pause"}},
   {KEY_CONFIG,        {"Config",               "Config"}},
   {KEY_POWER,         {"Power",                "Power"}},
   {KEY_MUTE,          {"Mute",                 "Mute"}}
};


const char *ctrlm_linux_key_code_str(uint16_t code, bool mask) {
   if (ctrlm_linux_key_names.end() != ctrlm_linux_key_names.find(code)) {
      return mask ? get<1>(ctrlm_linux_key_names.at(code)) : get<0>(ctrlm_linux_key_names.at(code));
   } else {
      return "Unknown";
   }
}

#ifdef ENABLE_DEEP_SLEEP
const char *ctrlm_wakeup_reason_str(DeepSleep_WakeupReason_t wakeup_reason) {
    switch(wakeup_reason) {
        case DEEPSLEEP_WAKEUPREASON_IR:               return("IR");
        case DEEPSLEEP_WAKEUPREASON_RCU_BT:           return("RCU_BT");
        case DEEPSLEEP_WAKEUPREASON_RCU_RF4CE:        return("RCU_RF4CE");
        case DEEPSLEEP_WAKEUPREASON_GPIO:             return("GPIO");
        case DEEPSLEEP_WAKEUPREASON_LAN:              return("LAN");
        case DEEPSLEEP_WAKEUPREASON_WLAN:             return("WLAN");
        case DEEPSLEEP_WAKEUPREASON_TIMER:            return("TIMER");
        case DEEPSLEEP_WAKEUPREASON_FRONT_PANEL:      return("FRONT_PANEL");
        case DEEPSLEEP_WAKEUPREASON_WATCHDOG:         return("WATCHDOG");
        case DEEPSLEEP_WAKEUPREASON_SOFTWARE_RESET:   return("SOFTWARE_RESET");
        case DEEPSLEEP_WAKEUPREASON_THERMAL_RESET:    return("THERMAL_RESET");
        case DEEPSLEEP_WAKEUPREASON_WARM_RESET:       return("WARM_RESET");
        case DEEPSLEEP_WAKEUPREASON_COLDBOOT:         return("COLDBOOT");
        case DEEPSLEEP_WAKEUPREASON_STR_AUTH_FAILURE: return("STR_AUTH_FAILURE");
        case DEEPSLEEP_WAKEUPREASON_CEC:              return("CEC");
        case DEEPSLEEP_WAKEUPREASON_PRESENCE:         return("PRESENCE");
        case DEEPSLEEP_WAKEUPREASON_VOICE:            return("VOICE");
        case DEEPSLEEP_WAKEUPREASON_UNKNOWN:          return("UNKNOWN");
        case DEEPSLEEP_WAKEUPREASON_MAX:              return("MAX");
    }
    return(ctrlm_invalid_return(wakeup_reason));
}
#endif


bool ctrlm_file_copy(const char* src, const char* dst, bool overwrite, bool follow_dst_symbolic_link) {
   bool    ret   = FALSE;
   GFile  *g_src = g_file_new_for_path(src);
   GFile  *g_dst = g_file_new_for_path(dst);
   GError *error = NULL;

   if (follow_dst_symbolic_link) {
      string link_target;
      if (ctrlm_file_get_symlink_target(dst, link_target)) {
         g_object_unref(g_dst);
         g_dst = g_file_new_for_path(link_target.c_str());
      } else {
         XLOGD_WARN("Failed to get link target for <%s>, not following symlink", dst);
      }
   }

   ret = g_file_copy(g_src, g_dst, (overwrite ? G_FILE_COPY_OVERWRITE : G_FILE_COPY_NONE), NULL, NULL, NULL, &error);
   if(FALSE == ret && error) {
      XLOGD_ERROR("Failed to copy file from <%s> to <%s> because %s", src, dst, error->message);
      g_error_free(error);
   }

   g_object_unref(g_src);
   g_object_unref(g_dst);

   return ret;
}

bool ctrlm_file_delete(const char* path, bool follow_symbolic_link) {
   bool    ret    = FALSE;
   GFile  *g_path = g_file_new_for_path(path);
   GError *error  = NULL;

   if (follow_symbolic_link) {
      string link_target;
      if (ctrlm_file_get_symlink_target(path, link_target)) {
         g_object_unref(g_path);
         g_path = g_file_new_for_path(link_target.c_str());
      } else {
         XLOGD_WARN("Failed to get link target for <%s>, not following symlink", path);
      }
   }

   ret = g_file_delete(g_path, NULL, &error);
   if(FALSE == ret && error) {
      XLOGD_ERROR("Failed to delete file <%s> because %s", path, error->message);
      g_error_free(error);
   }

   g_object_unref(g_path);

   return ret;
}

bool ctrlm_file_get_symlink_target(const char* path, string &link_target) {
   bool ret = false;

   char link_dst[PATH_MAX];
   struct stat link_stat;
   errno = 0;
   int rc = lstat(path, &link_stat);
   if (rc != 0) {
      int errsv = errno;
      XLOGD_WARN("lstat failed on path <%s>, error = <%s>", path, strerror(errsv));
   } else if (S_ISLNK(link_stat.st_mode)) {
      // get the target location of the link
      ssize_t len = readlink(path, link_dst, sizeof(link_dst)-1);
      if(len < 0) {
         int errsv = errno;
         XLOGD_ERROR("readlink failed on path <%s>, error = <%s>", path, strerror(errsv));
      } else {
         //readlink does not append a null byte
         link_dst[len] = '\0';
         XLOGD_DEBUG("path <%s> is a link pointing to <%s>", path, link_dst);
         link_target = link_dst;
         ret = true;
      }
   }
   return ret;
}

bool ctrlm_file_exists(const char* path) {
   bool ret;
   GFile *g_file = g_file_new_for_path(path);

   ret = g_file_query_exists(g_file, NULL);

   g_object_unref(g_file);

   return ret;
}

bool ctrlm_file_timestamp_get(const char *path, guint64 *ts) {
   GFile     *f           = NULL;
   GFileInfo *info        = NULL;
   GError    *error       = NULL;
   f    = g_file_new_for_path(path);
   info = g_file_query_info(f, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, &error);
   if(NULL == info) {
      XLOGD_ERROR("Failed to read timestamp for %s: %s", path, (error ? error->message : "unknown error"));
      if(error) {
         g_error_free(error);
      }
      g_object_unref(f);
      return(FALSE);
   }
   *ts = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
   g_object_unref(f);
   return(TRUE);
}

bool ctrlm_file_timestamp_set(const char *path, guint64  ts) {
   GFile   *f     = NULL;
   GError  *error = NULL;

   f = g_file_new_for_path(path);
   if(FALSE == g_file_set_attribute(f, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_ATTRIBUTE_TYPE_UINT64, &ts, G_FILE_QUERY_INFO_NONE, NULL, &error)) {
      XLOGD_ERROR("Failed to set time created attribute for %s: %s", path, (error ? error->message : "unknown error"));
      if(error) {
         g_error_free(error);
      }
      g_object_unref(f);
      return(FALSE);
   }
   g_object_unref(f);
   return(TRUE);
}

char *ctrlm_get_file_contents(const char *path) {
   gchar      *contents  = NULL;
   gsize       len       = 0;
   GError     *err       = NULL;

   if(FALSE == g_file_get_contents(path, &contents, &len, &err)) {
      if(err != NULL) {
         XLOGD_INFO("Failed to open RFC file <%s>", err->message);
         g_error_free(err);
      }  //CID:127825 - forward null
      return NULL;
   }
   return contents;
}

char *ctrlm_do_regex(char *re, char *str) {
   char       *result    = NULL;
   GMatchInfo *mInfo;
   GRegex     *regex     = NULL;

   regex = g_regex_new(re, (GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);
   if(NULL == regex) {
      XLOGD_ERROR("Fail creating regex");
      return NULL;
   }

   g_regex_match(regex, str, (GRegexMatchFlags)0, &mInfo);
   if(!g_match_info_matches(mInfo)) {
      XLOGD_WARN("No match for %s", re);
      g_free(regex);
      g_free(mInfo);
      return NULL;
   }

   result = g_match_info_fetch(mInfo, 0);
   g_free(regex);
   g_free(mInfo);

   return result;
}

bool ctrlm_dsmgr_init() {
   if(device::Manager::IsInitialized) {
      XLOGD_INFO("DSMgr already initialized");
      return true;
   }
   try {
      device::Manager::Initialize();
      XLOGD_INFO("DSMgr is initialized");
   }
   catch (...) {
      XLOGD_WARN("Failed to initialize DSMgr");
      return false;
   }
   return true;
}

bool ctrlm_dsmgr_deinit() {
   try {
      if(device::Manager::IsInitialized) {
         device::Manager::DeInitialize();
      }
   }
   catch(...) {
      XLOGD_WARN("Failed to deinitialize DSMgr");
      return false;
   }
   return true;
}

bool ctrlm_dsmgr_mute_audio(bool mute) {
  try {
     dsAudioDuckingAction_t action = mute ? dsAUDIO_DUCKINGACTION_START : dsAUDIO_DUCKINGACTION_STOP;
     device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, dsAUDIO_DUCKINGTYPE_ABSOLUTE, mute ? 0 : 100);
     XLOGD_INFO("Audio is %smuted", mute?"":"un-");
  }
  catch(std::exception& error) {
    XLOGD_WARN("Muting sound error : %s", error.what());
    return false;
  }
  return true;
}

bool ctrlm_dsmgr_duck_audio(bool enable, bool relative, double vol) {
  if(vol < 0 || vol > 1) {
      XLOGD_ERROR("Invalid volume");
      return false;
  }
  try {
     unsigned char level = (unsigned char)((vol * 100) + 0.5);

     dsAudioDuckingAction_t action = enable   ? dsAUDIO_DUCKINGACTION_START  : dsAUDIO_DUCKINGACTION_STOP;
     dsAudioDuckingType_t   type   = relative ? dsAUDIO_DUCKINGTYPE_RELATIVE : dsAUDIO_DUCKINGTYPE_ABSOLUTE;

     device::Host::getInstance().getAudioOutputPort("SPEAKER0").setAudioDucking(action, type, level);

     if(enable) {
        XLOGD_INFO("Audio ducking enabled - type <%s> level <%u%%>", relative ? "RELATIVE" : "ABSOLUTE", level);
     } else {
        XLOGD_INFO("Audio ducking disabled");
     }
  }
  catch(std::exception& error) {
    XLOGD_WARN("Ducking sound error : %s", error.what());
    return false;
  }
  return true;
}

bool ctrlm_dsmgr_LED(bool on) {
  try {
    device::FrontPanelIndicator &led =  device::FrontPanelIndicator::getInstance("Power");
    if (on) {
       led.setColor(0xFFFFFF);
       led.setBrightness(100);
    }
    led.setState(on);
  }
  catch(std::exception& error) {
    XLOGD_WARN("LED error : %s", error.what());
    return false;
  }
  return true;
}

bool ctrlm_is_voice_assistant(ctrlm_rcu_controller_type_t controller_type) {
   switch(controller_type) {
   case CTRLM_RCU_CONTROLLER_TYPE_XR19:
      return(true);
   case CTRLM_RCU_CONTROLLER_TYPE_XR2:
   case CTRLM_RCU_CONTROLLER_TYPE_XR5:
   case CTRLM_RCU_CONTROLLER_TYPE_XR11:
   case CTRLM_RCU_CONTROLLER_TYPE_XR15:
   case CTRLM_RCU_CONTROLLER_TYPE_XR15V2:
   case CTRLM_RCU_CONTROLLER_TYPE_XR16:
   case CTRLM_RCU_CONTROLLER_TYPE_XR18:
   case CTRLM_RCU_CONTROLLER_TYPE_XRA:
   case CTRLM_RCU_CONTROLLER_TYPE_UNKNOWN:
   case CTRLM_RCU_CONTROLLER_TYPE_INVALID:
      return(false);
   }
   return(false);
}

ctrlm_remote_keypad_config ctrlm_get_remote_keypad_config(const char *remote_type) {
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
   } else if (strncmp("XRA-", remote_type, 4) == 0) {
      remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NUMBER_KEYS;
   }
   return(remote_keypad_config);
}

unsigned long long ctrlm_convert_mac_string_to_long ( const char* apcDeviceMac) {
   unsigned long long   lBTRCoreDevId = 0;
   char                 lcDevHdlArr[13] = {'\0'};

   // MAC Address Format Supported: "AA:BB:CC:DD:EE:FF\0"
   if (apcDeviceMac && (strlen(apcDeviceMac) >= 17)) {
   lcDevHdlArr[0]  = apcDeviceMac[0];
   lcDevHdlArr[1]  = apcDeviceMac[1];
   lcDevHdlArr[2]  = apcDeviceMac[3];
   lcDevHdlArr[3]  = apcDeviceMac[4];
   lcDevHdlArr[4]  = apcDeviceMac[6];
   lcDevHdlArr[5]  = apcDeviceMac[7];
   lcDevHdlArr[6]  = apcDeviceMac[9];
   lcDevHdlArr[7]  = apcDeviceMac[10];
   lcDevHdlArr[8]  = apcDeviceMac[12];
   lcDevHdlArr[9]  = apcDeviceMac[13];
   lcDevHdlArr[10] = apcDeviceMac[15];
   lcDevHdlArr[11] = apcDeviceMac[16];

   lBTRCoreDevId = (unsigned long long) strtoll(lcDevHdlArr, NULL, 16);
   }

//  XLOGD_DEBUG ("converted to long long: <0x%012llX>", lBTRCoreDevId);
   return lBTRCoreDevId;
}

std::string ctrlm_convert_mac_long_to_string ( const unsigned long long ieee_address) {
   // MAC Address Format Supported: "AA:BB:CC:DD:EE:FF\0"
   char ascii_mac[18]={0};
   errno_t safec_rc = sprintf_s(ascii_mac, sizeof(ascii_mac), "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned int)((ieee_address & 0xFF0000000000)>>40),
                                                            (unsigned int)((ieee_address & 0x00FF00000000)>>32),
                                                            (unsigned int)((ieee_address & 0x0000FF000000)>>24),
                                                            (unsigned int)((ieee_address & 0x000000FF0000)>>16),
                                                            (unsigned int)((ieee_address & 0x00000000FF00)>>8),
                                                            (unsigned int)(ieee_address & 0x0000000000FF));
   if(safec_rc < EOK) {
       ERR_CHK(safec_rc);
   }
   // XLOGD_DEBUG ("ieee_address = <0x%llX>, converted to ASCII: <%s>", ieee_address, ascii_mac);
   return string(ascii_mac);
}

static bool ctrlm_rm_rf(const std::string &path, unsigned int recursive_depth = MAX_RECURSE_DEPTH) {
   /* This is the implementation for (rm -rf) command */
   XLOGD_INFO("Removing the directory <%s> ", path.c_str());
   bool status = false;

   if(0 == recursive_depth) {
     XLOGD_ERROR("Maximum recursion depth reached");
     return false;
   }

   /* DIR handler */
   errno = 0;
   DIR *handler = opendir(path.c_str());
   if(handler == NULL){
     int errsv = errno;
     XLOGD_ERROR("Failed opendir to %s, opendir call error (%s)", path.c_str(), strerror(errsv));
     return false;
   }

   /* handler to run-through the available directoreis and files */
   struct dirent *dir;

   /* store the length of the path */
   size_t path_len = strlen(path.c_str());

   errno = 0;
   status = true;
   while (status && (dir = readdir(handler))) {
     bool recurse_status = false;
     size_t len;

     /* skip recursive calls on "." and ".." */
     if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
        continue;
     }

     len = path_len + strlen(dir->d_name) + 2;
     static char ctrlm_rm_rf_buf[PATH_MAX];
     struct stat sbuf;
     snprintf(ctrlm_rm_rf_buf, len, "%s/%s", path.c_str(), dir->d_name);
     errno = 0;
     if (0 != stat(ctrlm_rm_rf_buf, &sbuf)) {
        int errsv = errno;
        XLOGD_ERROR("Failed to stat %s, stat call error (%s)", ctrlm_rm_rf_buf, strerror(errsv));
        errno = 0;
        if( 0 != closedir(handler)) {
          int errsv = errno;
          XLOGD_ERROR("Failed closedir to %s, closedir call error (%s)", path.c_str(), strerror(errsv));
        }
        return false;
     } else {
       if (S_ISDIR(sbuf.st_mode)){
         --recursive_depth;
         recurse_status = ctrlm_rm_rf(ctrlm_rm_rf_buf, recursive_depth);
       } else {
         errno = 0;
         if(0 != unlink(ctrlm_rm_rf_buf)){
            int errsv = errno;
            XLOGD_ERROR("Failed unlink %s, unlink call error (%s)", ctrlm_rm_rf_buf, strerror(errsv));
            recurse_status = false;
         } else {
            recurse_status = true;
         }
       }
     }
     status = recurse_status;
   }

   if(dir  == NULL && errno == 0){
     XLOGD_INFO("End of directory stream reached");
   } else if(dir  == NULL && errno != 0){
     int errsv = errno;
     XLOGD_ERROR("Failed readdir to %s, readdir call error (%s)", path.c_str(), strerror(errsv));
     status = false;
   }

   errno = 0;
   if( 0 != closedir(handler)) {
     int errsv = errno;
     XLOGD_ERROR("Failed closedir to %s, closedir call error (%s)", path.c_str(), strerror(errsv));
     return false;
   }

   if (status) {
      errno = 0;
      if(0 != rmdir(path.c_str())){
        int errsv = errno;
        XLOGD_ERROR("Failed rmdir %s, rmdir call error (%s)", path.c_str(), strerror(errsv));
        return false;
      }
   }
   return status;
}

bool ctrlm_utils_rm_rf(const std::string &path){
   bool status;
   ctrlm_utils_sem_wait();
   status = ctrlm_rm_rf(path);
   ctrlm_utils_sem_post();
   return status;
}

bool ctrlm_tar_archive_extract(const std::string &archive_path, const std::string &dest_path) {
   XLOGD_INFO("extracting <%s> to <%s>", archive_path.c_str(), dest_path.c_str());
   bool status = false;
   struct archive *arch = NULL;
   struct archive_entry *entry;
   int result=0;

   /* store the current working dir to getback to the same */
   char path[PATH_MAX];
   errno = 0;
   if(getcwd(path, PATH_MAX) == NULL){
     int errsv = errno;
     XLOGD_ERROR("Failed to getcwd and getcwd call error (%s)", strerror(errsv));
     return false;
   }

   /* switch to destination directory to extract the tar*/
   errno = 0;
   if(chdir (dest_path.c_str()) != 0) {
      int errsv = errno;
      XLOGD_ERROR("Failed chdir to %s and chdir call error (%s)", dest_path.c_str(), strerror(errsv));
      return false;
   }

   /* we can only read tar achives */
   arch = archive_read_new ();
   if(arch == NULL){
      XLOGD_ERROR("Unable to create an archive handlers");
      errno = 0;
      if(chdir (path) != 0) {
        int errsv = errno;
        XLOGD_ERROR("Failed chdir to %s and chdir call error (%s)", path, strerror(errsv));
      }
      return false;
   }

   if(ARCHIVE_OK != archive_read_support_format_all (arch) || ARCHIVE_OK != archive_read_support_filter_all (arch)) {
      XLOGD_WARN("Unable to support archive / decompression formats");
      /*free the archie*/
      archive_read_free (arch);
      errno = 0;
      if(chdir (path) != 0) {
        int errsv = errno;
        XLOGD_ERROR("Failed chdir to %s and chdir call error (%s)", path, strerror(errsv));
      }
      return false;
   }

   /* open the tar file */
   result = archive_read_open_filename (arch, archive_path.c_str(), BLOCK_SIZE);
   if(result != ARCHIVE_OK) {
      XLOGD_ERROR("Cannot open %s", archive_path.c_str());
       /* free the archive */
      archive_read_free (arch);
      errno = 0;
      if(chdir (path) != 0) {
        int errsv = errno;
        XLOGD_ERROR("Failed chdir to %s and chdir call error (%s)", path, strerror(errsv));
      }
      return false;
   }

   /* extract each file */
   do {
      result = archive_read_next_header (arch, &entry);
      if (result == ARCHIVE_EOF) {
         break;
      }
      if (result != ARCHIVE_OK) {
         XLOGD_ERROR("Cannot read header  %s", archive_error_string (arch));
         break;
      }

      result = archive_read_extract (arch, entry, 0);
      if (result == ARCHIVE_OK || result == ARCHIVE_WARN) {
         /* tar is extracted successfully*/
         status = true;
      } else {
         XLOGD_ERROR("Cannot extract %s", archive_error_string (arch));
         break;
      }
    } while(1);

   /* close the archive */
   archive_read_close (arch);
   archive_read_free (arch);

   errno = 0;
   if(chdir (path) != 0) {
      int errsv = errno;
      XLOGD_ERROR("Failed chdir to %s and chdir call error (%s)", path, strerror(errsv));
      return false;
   }

   return status;
}

void ctrlm_archive_extract_tmp_dir_make(const std::string &tmp_dir_path) {
   XLOGD_INFO("<%s>", tmp_dir_path.c_str());
   errno = 0;
   if(0 != mkdir(tmp_dir_path.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return;
   }
}

void ctrlm_archive_extract_ble_tmp_dir_make(const std::string &tmp_dir_path) {
   string dir = tmp_dir_path + "ctrlm/";
   XLOGD_DEBUG("<%s>", dir.c_str());
   errno = 0;
   if(0 != mkdir(dir.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return;
   }
}

bool ctrlm_archive_extract_ble_check_dir_exists(const std::string &path){
   struct stat st;
   XLOGD_DEBUG("test for dir <%s>", path.c_str());

   if(stat(path.c_str(),&st) == 0){
       if((st.st_mode & S_IFDIR) != 0){
           return true;
       }
   }
   XLOGD_DEBUG("dir not found <%s>", path.c_str());
   return false;
}

bool ctrlm_archive_extract(const std::string &file_path_archive, const std::string &tmp_dir_path, const std::string &archive_file_name) {
   string dest_path = tmp_dir_path + "ctrlm/" + archive_file_name + "/";

   if(ctrlm_archive_extract_ble_check_dir_exists(tmp_dir_path + "ctrlm/")==false){
      ctrlm_archive_extract_tmp_dir_make(tmp_dir_path);
      ctrlm_archive_extract_ble_tmp_dir_make(tmp_dir_path);
   }

   errno = 0;
   if(0 != mkdir(dest_path.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return false;
   }

   if( true != ctrlm_tar_archive_extract(file_path_archive, dest_path)) {
      XLOGD_WARN("Failed to extract the archive due to ctrlm_tar_archive_extract call error");
      return false;
   }
   return true;
}

void ctrlm_archive_remove(const std::string &dir) {
   XLOGD_INFO("deleting directory <%s>", dir.c_str());
   if( true != ctrlm_utils_rm_rf(dir)) {
      XLOGD_WARN("Failed to remove directory, ctrlm_rm_rf call error");
   }
}

std::string ctrlm_xml_tag_text_get(const std::string &xml, const std::string &tag) {
   // TODO currently this assume no spaces or tabs in the tag brackets. and no leading trailing spaces in text content
   size_t idx = xml.find("<" + tag);
   if(idx == std::string::npos) {
      XLOGD_INFO("tag <%s> not found in xml file", tag.c_str());
      return "";
   }
   // skip past the tag and its two brackets:
   idx += tag.length() + 2;

   // find end tag
   size_t idx2 = xml.find("</" + tag);

   //grab all content between start and end tag
   return xml.substr(idx, idx2 - idx);
}

ctrlm_power_state_t ctrlm_iarm_power_state_map(IARM_Bus_PowerState_t iarm_power_state) {
    ctrlm_power_state_t ctrlm_power_state = CTRLM_POWER_STATE_ON;

    switch(iarm_power_state) {
       case IARM_BUS_PWRMGR_POWERSTATE_ON:                  ctrlm_power_state = CTRLM_POWER_STATE_ON;          break;
       case IARM_BUS_PWRMGR_POWERSTATE_STANDBY:
       case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP: ctrlm_power_state = CTRLM_POWER_STATE_STANDBY;     break;
       case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP:
       case IARM_BUS_PWRMGR_POWERSTATE_OFF:                 ctrlm_power_state = CTRLM_POWER_STATE_DEEP_SLEEP;  break;
    }

    return ctrlm_power_state;
}

const char *ctrlm_rcu_wakeup_config_str(ctrlm_rcu_wakeup_config_t config) {
    switch(config) {
        case CTRLM_RCU_WAKEUP_CONFIG_ALL:       return("ALL");
        case CTRLM_RCU_WAKEUP_CONFIG_CUSTOM:    return("CUSTOM");
        case CTRLM_RCU_WAKEUP_CONFIG_NONE:      return("NONE");
        case CTRLM_RCU_WAKEUP_CONFIG_INVALID:   return("INVALID");
        default:                                return("INVALID__TYPE");
    }
}

bool ctrlm_utils_calc_crc32( const char *filename, uLong *crc_ret ) {
   uLong crc = 0;
   bool status = true;
   size_t read_size;
   Bytef crc_buff[CRC_BUFF_SIZE];
   FILE *fp = NULL;

   do {
      errno = 0;
      fp = fopen(filename, "r");
      if (NULL == fp) {
         int errsv = errno;
         XLOGD_ERROR("could not open %s, error = %d, <%s>", filename, errsv, strerror(errsv));
         status = false;
         break;
      }

      crc = crc32(0L, Z_NULL, 0);
      while (!feof(fp)) {
         read_size = fread((void *)&crc_buff[0], 1, CRC_BUFF_SIZE, fp);
         if (read_size != CRC_BUFF_SIZE) {
            if (ferror(fp)) {
               //fread does not set errno
               XLOGD_ERROR("error reading file <%s>", filename);
               status = false;
               break;
            }
         }
         crc = crc32(crc, (const Bytef *)&crc_buff[0], read_size);
      } // end of crc calculation loop

      if (status) {
         XLOGD_DEBUG("file <%s> successfully calculated CRC = 0x%lx", filename, crc);
      } else {
         XLOGD_ERROR("failed to calculate CRC of file <%s>", filename);
      }
   } while (0);

   if (NULL != fp) {
      fclose(fp);
   }
   *crc_ret = crc;
   return status;
}

bool ctrlm_utils_move_file_to_secure_nvm(const char *path) {
   int rc;
   int retry = 0, max_retries = 3;

   string path_str = path;
   size_t idx = path_str.rfind('/');
   string filename = path_str.substr(idx + 1);
   string secure_path = CTRLM_NVM_SECURE_PATH + filename;

   if (ctrlm_file_exists(path)) {
      struct stat link_stat;
      errno = 0;
      rc = lstat(path, &link_stat);
      if(rc != 0) {
         int errsv = errno;
         XLOGD_ERROR("lstat failed on path <%s>, error = <%s>", path, strerror(errsv));
         return false;
      }

      XLOGD_DEBUG("path <%s> is regular file? <%s>, is symbolic link? <%s>", path, S_ISREG(link_stat.st_mode) ? "TRUE":"FALSE", S_ISLNK(link_stat.st_mode) ? "TRUE":"FALSE");
      if (S_ISLNK(link_stat.st_mode)) {
         XLOGD_DEBUG("File <%s> already is a link to secure area, nothing to be done.", path);
         return true;
      } else {
         XLOGD_WARN("Moving file <%s> to <%s>", path, secure_path.c_str());
         uLong crc_src, crc_dest;
         // Calculate CRC of the file first
         for (retry = 0; retry < max_retries; retry++) {
            if (ctrlm_utils_calc_crc32(path, &crc_src)) { break; }
         }
         if (retry >= max_retries) { XLOGD_ERROR("Failed to calc CRC after <%d> retries, returning false.", max_retries); return false; }

         // Copy file to secure area and ensure CRCs match
         for (retry = 0; retry < max_retries; retry++) {
            if (false == ctrlm_file_copy(path, secure_path.c_str(), true, false)) {
               continue;
            }
            if (ctrlm_utils_calc_crc32(secure_path.c_str(), &crc_dest)) {
               if (crc_src == crc_dest) {
                  XLOGD_DEBUG("CRCs match");
                  break;
               } else {
                  XLOGD_ERROR("CRCs do NOT match, try copy again!!!!!!!!!!!!");
                  continue;
               }
            } else {
               continue;
            }
         }
         if (retry >= max_retries) {
            XLOGD_ERROR("Failed to copy file to secure area after <%d> retries, deleting it and returning false.", max_retries);
            rc = remove(secure_path.c_str());
            if (rc != 0) {
               int errsv = errno; XLOGD_ERROR("failed to delete <%s>, error = <%s>", secure_path.c_str(), strerror(errsv));
            }
            return false; 
         }

         XLOGD_INFO("Successfully copied file <%s> to <%s>", path, secure_path.c_str());

         // Delete the file in unsecure area
         for (retry = 0; retry < max_retries; retry++) {
            rc = remove(path);
            if (rc != 0) {
               int errsv = errno; XLOGD_ERROR("failed to delete <%s>, error = <%s>", path, strerror(errsv));
            } else {
               break;
            }
         }
         if (retry >= max_retries) { XLOGD_ERROR("Failed to remove <%s> after <%d> retries, returning false.", path, max_retries); return false; }

         XLOGD_INFO("Successfully deleted file <%s>, creating symlink.", path);
         for (retry = 0; retry < max_retries; retry++) {
            rc = symlink(secure_path.c_str(), path);
            if (rc != 0) {
               int errsv = errno; XLOGD_ERROR("failed to create symbolic link <%s> -> <%s>, error = <%s>", path, secure_path.c_str(), strerror(errsv));
            } else {
               break;
            }
         }
         if (retry >= max_retries) {
            XLOGD_ERROR("Failed to create symlink after <%d> retries, moving file back to <%s>", max_retries, path);
            ctrlm_file_copy(secure_path.c_str(), path, true, false);
            rc = remove(secure_path.c_str());
            if (rc != 0) {
               int errsv = errno; XLOGD_ERROR("failed to delete <%s>, error = <%s>", secure_path.c_str(), strerror(errsv));
            }
            return false;
         }
         XLOGD_INFO("Successfully moved file to secure area and created symbolic link <%s> -> <%s>", path, secure_path.c_str());
      }
   } else {
      XLOGD_INFO("file <%s> does not exist, creating link to secure area.", path);
      for (retry = 0; retry < max_retries; retry++) {
         rc = symlink(secure_path.c_str(), path);
         if (rc != 0) {
            int errsv = errno; XLOGD_ERROR("failed to create symbolic link <%s> -> <%s>, error = <%s>", path, secure_path.c_str(), strerror(errsv));
         } else {
            break;
         }
      }
      if (retry >= max_retries) { XLOGD_ERROR("Failed to create symlink after <%d> retries, returning false.", max_retries); return false; }
   }
   return true;
}

json_t *ctrlm_utils_json_from_path(json_t *root, const std::string &path, bool add_ref) {
    json_t *ret  = root;
    
    if(ret != NULL) {
        if(!path.empty()) {
            std::string path_copy = path;
            do {
                size_t delim_pos = path_copy.find(JSON_PATH_SEPERATOR);
                if(delim_pos != std::string::npos) {
                    std::string key = path_copy.substr(0, delim_pos);
                    path_copy = path_copy.substr(delim_pos+1);
                    ret  = json_object_get(ret, key.c_str());
                } else {
                    ret = json_object_get(ret, path_copy.c_str());
                    break;
                }
            } while(ret != NULL);
        }

        if(ret && add_ref) {
            json_incref(ret);
        }
    }
    return(ret);
}

std::string ctrlm_utils_json_string_from_path(json_t *root, const std::string &path) {
    std::string ret = "";
    json_t *obj     = root;
    
    if(obj != NULL) {
        if(!path.empty()) {
            std::string path_copy = path;
            do {
                size_t delim_pos = path_copy.find(JSON_PATH_SEPERATOR);
                if(delim_pos != std::string::npos) {
                    std::string key = path_copy.substr(0, delim_pos);
                    path_copy = path_copy.substr(delim_pos+1);
                    obj  = json_object_get(obj, key.c_str());
                } else {
                    obj = json_object_get(obj, path_copy.c_str());
                    break;
                }
            } while(obj != NULL);
        }

        if(obj) {
            char *obj_str = json_dumps(obj, JSON_ENCODE_ANY);
            if(obj_str) {
                ret = std::string(obj_str);
                free(obj_str);
                obj_str = NULL;
            }
        }
    } else {
        XLOGD_ERROR("config json object is NULL");
    }
    return(ret);
}


bool ctrlm_utils_thread_create(ctrlm_thread_t *thread, void *(*start_routine) (void *), void *arg, pthread_attr_t *attr) {
   thread->running = false;
   if(0 != pthread_create(&thread->id, attr, start_routine, arg)) {
      XLOGD_ERROR("unable to launch thread <%s>", thread->name==NULL ? "" : thread->name);
      return(false);
   }

   if(thread->name != NULL) {
      char name_max[16];
      snprintf(name_max, sizeof(name_max), "%s", thread->name);
      XLOGD_DEBUG("setting name of thread to <%s>", name_max);
      if(pthread_setname_np(thread->id, name_max) != 0) {
         XLOGD_WARN("pthread_setname_np() failed to set name of thread to <%s>", name_max);
      }
   }

   thread->running = true;
   return(true);
}

bool ctrlm_utils_thread_join(ctrlm_thread_t *thread, uint32_t timeout_secs) {
   if(!thread->running) {
      XLOGD_WARN("Thread <%s> not running.", thread->name);
      return(true);
   }

   XLOGD_INFO("Waiting for thread <%s> to exit", thread->name);

   struct timespec end_time;
   clock_gettime(CLOCK_REALTIME, &end_time);
   end_time.tv_sec += timeout_secs;

   if (0 != pthread_timedjoin_np(thread->id, NULL, &end_time)) {
      XLOGD_ERROR("Thread <%s> join timed-out, not waiting any longer for it to exit.", thread->name);
      return false;
   }

   XLOGD_INFO("Thread <%s> join successful.", thread->name);
   thread->running = false;
   return(true);
}

bool ctrlm_utils_message_queue_open(int *msgq, uint8_t max_msg, size_t msgsize) {
   xr_mq_attr_t attr = {.max_msg = max_msg, .max_msg_size = msgsize};

   *msgq = xr_mq_create(&attr);
   if(*msgq < 0) {
      return(false);
   }
   return(true);
}

void ctrlm_utils_message_queue_close(int *msgq) {
   if(*msgq >= 0) {
      xr_mq_destroy(*msgq);
   }
}

bool ctrlm_utils_queue_msg_push(int msgq, const char *msg, size_t msg_len) {
   XLOGD_DEBUG("msgq %d size %d msg %p", msgq, msg_len, msg);
   if(msgq < 0) {
      XLOGD_WARN("message queue invalid");
      return(false);
   }

   if(!xr_mq_push(msgq, msg, msg_len)) {
      XLOGD_ERROR("failed to push msg");
      return(false);
   }
   return(true);
}

const char *ctrlm_irdb_vendor_str(ctrlm_irdb_vendor_t vendor) {
   switch (vendor) {
      case CTRLM_IRDB_VENDOR_UEI:      return("UEI");
      case CTRLM_IRDB_VENDOR_RUWIDO:   return("RUWIDO");
      case CTRLM_IRDB_VENDOR_INVALID:  return("INVALID");
      default:                         return("UNKNOWN");
   }
}

std::string ctrlm_utils_time_as_string(time_t time) {
   char time_str[CTRLM_MAX_TIME_STR_LEN];
   errno_t safec_rc = -1;
   if(time == 0) {
      safec_rc = strcpy_s(time_str, sizeof(time_str), "NEVER");
      ERR_CHK(safec_rc);
   } else {
      time_str[0] = '\0';
      strftime(time_str, CTRLM_MAX_TIME_STR_LEN, "%F %T", localtime((time_t *)&time));
   }
   return string(time_str);
}

ctrlm_fmr_alarm_level_t ctrlm_utils_str_to_fmr_level(const std::string &level) {
    std::string level_copy = level;
    std::transform(level_copy.begin(), level_copy.end(), level_copy.begin(), static_cast<int(*)(int)>(std::tolower));

    if (level_copy == "off")
        return CTRLM_FMR_DISABLE;
    else if (level_copy == "mid")
        return CTRLM_FMR_LEVEL_MID;
    else if (level_copy == "high")
        return CTRLM_FMR_LEVEL_HIGH;
    else
        return CTRLM_FMR_LEVEL_INVALID;
}

ctrlm_rcu_wakeup_config_t ctrlm_utils_str_to_wakeup_config(const std::string &wakeup_config) {
    std::string wakeup_config_copy = wakeup_config;
    std::transform(wakeup_config_copy.begin(), wakeup_config_copy.end(), wakeup_config_copy.begin(), static_cast<int(*)(int)>(std::tolower));

    if (wakeup_config_copy == "all")
        return CTRLM_RCU_WAKEUP_CONFIG_ALL;
    else if (wakeup_config_copy == "none")
        return CTRLM_RCU_WAKEUP_CONFIG_NONE;
    else if (wakeup_config_copy == "custom")
        return CTRLM_RCU_WAKEUP_CONFIG_CUSTOM;
    else
        return CTRLM_RCU_WAKEUP_CONFIG_INVALID;
}

int ctrlm_utils_custom_key_str_to_array(const std::string &custom_keys, int *custom_list) {
    int size_of_custom_list = 0;
    std::stringstream ss(custom_keys);
    std::string token;

    while (std::getline(ss, token, ',')) {
        custom_list[size_of_custom_list] = std::stoi(token);
        size_of_custom_list++;
    }
    return size_of_custom_list;
}

const char *ctrlm_rf_pair_state_str(ctrlm_rf_pair_state_t state)
{
    switch(state) {
        case CTRLM_RF_PAIR_STATE_INITIALIZING: return("INITIALIZING");
        case CTRLM_RF_PAIR_STATE_IDLE:         return("IDLE");
        case CTRLM_RF_PAIR_STATE_SEARCHING:    return("SEARCHING");
        case CTRLM_RF_PAIR_STATE_PAIRING:      return("PAIRING");
        case CTRLM_RF_PAIR_STATE_COMPLETE:     return("COMPLETE");
        case CTRLM_RF_PAIR_STATE_FAILED:       return("FAILED");
        case CTRLM_RF_PAIR_STATE_UNKNOWN:      return("UNKNOWN");
        default:                               return("INVALID__TYPE");
    }
}

