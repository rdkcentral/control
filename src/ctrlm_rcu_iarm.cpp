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
#include <typeinfo>
#include <memory>
#include "libIBus.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_rcu.h"
#include "ctrlm_utils.h"

static IARM_Result_t ctrlm_rcu_iarm_call_validation_finish(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_controller_status(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_rib_request_get(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_rib_request_set(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_controller_link_key(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_reverse_cmd(void *arg);
static IARM_Result_t ctrlm_rcu_iarm_call_rf4ce_polling_action(void *arg);

typedef struct {
      const char* iarm_call_name;
      IARM_Result_t (*iarm_call_handler)(void*);
} iarm_call_handler_t;

static iarm_call_handler_t handlers[] = {
      { CTRLM_RCU_IARM_CALL_VALIDATION_FINISH,    &ctrlm_rcu_iarm_call_validation_finish },
      { CTRLM_RCU_IARM_CALL_CONTROLLER_STATUS,    &ctrlm_rcu_iarm_call_controller_status },
      { CTRLM_RCU_IARM_CALL_RIB_REQUEST_GET,      &ctrlm_rcu_iarm_call_rib_request_get },
      { CTRLM_RCU_IARM_CALL_RIB_REQUEST_SET,      &ctrlm_rcu_iarm_call_rib_request_set },
      { CTRLM_RCU_IARM_CALL_CONTROLLER_LINK_KEY,  &ctrlm_rcu_iarm_call_controller_link_key },
      { CTRLM_RCU_IARM_CALL_REVERSE_CMD,          &ctrlm_rcu_iarm_call_reverse_cmd },
      { CTRLM_RCU_IARM_CALL_RF4CE_POLLING_ACTION, &ctrlm_rcu_iarm_call_rf4ce_polling_action }
};

// Keep state since we do not want to service calls on termination
static volatile int running = 0;

gboolean ctrlm_rcu_init_iarm(void) {
   IARM_Result_t rc;
   XLOGD_INFO("");

   for (const iarm_call_handler_t& handler :  handlers) {
      rc = IARM_Bus_RegisterCall(handler.iarm_call_name, handler.iarm_call_handler);
   if(rc != IARM_RESULT_SUCCESS) {
         XLOGD_ERROR("%s %d", handler.iarm_call_name, rc);
      return(false);
   }
   }

   // Change to running state so we can accept calls
   g_atomic_int_set(&running, 1);

   return(true);
}

template <typename iarm_event_struct>
static void init_iarm_event_struct(iarm_event_struct& msg,ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   msg.api_revision  = CTRLM_RCU_IARM_BUS_API_REVISION;
   msg.network_id    = network_id;
   msg.network_type  = ctrlm_network_type_get(network_id);
   msg.controller_id = controller_id;
}

void ctrlm_rcu_iarm_event_key_press(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code) {

   // Convert discrete power codes to toggle UNLESS the key is from IP Remote, CEDIA community requires discrete for home automation
   if(((key_code == CTRLM_KEY_CODE_POWER_OFF) || (key_code == CTRLM_KEY_CODE_POWER_ON)) && CTRLM_NETWORK_TYPE_IP != ctrlm_network_type_get(network_id)) {
      key_code = CTRLM_KEY_CODE_POWER_TOGGLE;
   }

   // Handle atomic keypresses as key down and key up until atomic is supported in API
   if(CTRLM_KEY_STATUS_ATOMIC == key_status) {
      XLOGD_INFO("Atomic Keypress");
      ctrlm_rcu_iarm_event_key_press(network_id, controller_id, CTRLM_KEY_STATUS_DOWN, key_code);
      ctrlm_rcu_iarm_event_key_press(network_id, controller_id, CTRLM_KEY_STATUS_UP, key_code);
      return;
   }

   ctrlm_rcu_iarm_event_key_press_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.key_status    = key_status;
   msg.key_code      = key_code;
   if(ctrlm_is_pii_mask_enabled()) {
      XLOGD_INFO("(%u, %u) key %6s <*>", network_id, controller_id, ctrlm_key_status_str(key_status));
   } else {
      XLOGD_INFO("(%u, %u) key %6s (0x%02X) %s", network_id, controller_id, ctrlm_key_status_str(key_status), (guchar)key_code, ctrlm_key_code_str(key_code));
   }
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_KEY_PRESS, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_key_press_validation(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code) {
   ctrlm_rcu_iarm_event_key_press_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.binding_type  = binding_type;
   msg.key_status    = key_status;
   msg.key_code      = key_code;
   errno_t safec_rc = strncpy_s(msg.controller_type, sizeof(msg.controller_type), ctrlm_rcu_controller_type_str(controller_type), CTRLM_RCU_MAX_USER_STRING_LENGTH - 1);
   ERR_CHK(safec_rc);
   msg.controller_type[CTRLM_RCU_MAX_USER_STRING_LENGTH - 1] = '\0';
   if(ctrlm_is_pii_mask_enabled()) {
      XLOGD_INFO("(%u, %u) Controller Type <%s> key %s *", network_id, controller_id, msg.controller_type, ctrlm_key_status_str(key_status));
   } else {
      XLOGD_INFO("(%u, %u) Controller Type <%s> key %s (0x%02X) %s", network_id, controller_id, msg.controller_type, ctrlm_key_status_str(key_status), (guchar)key_code, ctrlm_key_code_str(key_code));
   }
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_VALIDATION_KEY_PRESS, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_function(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_function_t function, unsigned long value) {
   ctrlm_rcu_iarm_event_function_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.function      = function;
   msg.value         = value;

   XLOGD_INFO("(%u, %u) Function <%s> Value %lu", network_id, controller_id, ctrlm_rcu_function_str(function), value);
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_FUNCTION, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_key_ghost(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_remote_keypad_config remote_keypad_config, ctrlm_rcu_ghost_code_t ghost_code) {
   ctrlm_rcu_iarm_event_key_ghost_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.ghost_code           = ghost_code;
   msg.remote_keypad_config = remote_keypad_config;

   XLOGD_INFO("(%u, %u) Ghost Code <%s> Remote Keypad Config <%u>", network_id, controller_id, ctrlm_rcu_ghost_code_str(ghost_code), remote_keypad_config);
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_KEY_GHOST, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_control(int controller_id, const char *event_source, const char *event_type, const char *event_data, int event_value, int spare_value) {
   ctrlm_rcu_iarm_event_control_t msg;
   errno_t safec_rc = -1;
   msg.api_revision  = CTRLM_RCU_IARM_BUS_API_REVISION;
   msg.controller_id = controller_id;
   msg.event_value   = event_value;
   msg.spare_value   = spare_value;
   safec_rc = strncpy_s(msg.event_source, sizeof(msg.event_source), event_source, CTRLM_RCU_MAX_EVENT_SOURCE_LENGTH - 1);
   ERR_CHK(safec_rc);
   msg.event_source[CTRLM_RCU_MAX_EVENT_SOURCE_LENGTH - 1] = '\0';
   safec_rc = strncpy_s(msg.event_type, sizeof(msg.event_type), event_type, CTRLM_RCU_MAX_EVENT_TYPE_LENGTH - 1);
   ERR_CHK(safec_rc);
   msg.event_type[CTRLM_RCU_MAX_EVENT_TYPE_LENGTH - 1] = '\0';
   safec_rc = strncpy_s(msg.event_data, sizeof(msg.event_data), event_data, CTRLM_RCU_MAX_EVENT_DATA_LENGTH - 1);
   ERR_CHK(safec_rc);
   msg.event_data[CTRLM_RCU_MAX_EVENT_DATA_LENGTH - 1] = '\0';

   XLOGD_INFO("Controller Id <%d> Source <%s> Type <%s> Data <%s> value <%u>", controller_id, event_source, event_type, event_data, event_value);
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_CONTROL, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_rib_access_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_rib_attr_id_t identifier, guchar index, ctrlm_access_type_t access_type) {
   ctrlm_rcu_iarm_event_rib_entry_access_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.identifier    = identifier;
   msg.index         = index;
   msg.access_type   = access_type;

   XLOGD_INFO("(%u, %u) Attr <%s> Index %u Access Type <%s>", network_id, controller_id, ctrlm_rcu_rib_attr_id_str(identifier), index, ctrlm_access_type_str(access_type));
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_RIB_ACCESS_CONTROLLER, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_reverse_cmd(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_main_iarm_event_t event, ctrlm_rcu_reverse_cmd_result_t cmd_result, int result_data_size, const unsigned char* result_data) {
   if (event != CTRLM_RCU_IARM_EVENT_RCU_REVERSE_CMD_BEGIN && event != CTRLM_RCU_IARM_EVENT_RCU_REVERSE_CMD_END ) {
      XLOGD_ERROR("Unexpected IARM Event ID %d!", event);
      return;
   }
   int msg_size = sizeof (ctrlm_rcu_iarm_event_reverse_cmd_t) + result_data_size;
   if (result_data_size > 1) {
      --msg_size;
   }
   std::unique_ptr<unsigned char[]> msg_buf(new unsigned char[msg_size]);
   ctrlm_rcu_iarm_event_reverse_cmd_t& msg = *(ctrlm_rcu_iarm_event_reverse_cmd_t*)msg_buf.get();
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.action = CTRLM_RCU_REVERSE_CMD_FIND_MY_REMOTE;
   msg.result = cmd_result;
   msg.result_data_size = result_data_size;
   if (result_data_size > 0) {
      errno_t safec_rc = memcpy_s(&msg.result_data[0], msg.result_data_size, result_data, result_data_size);
      ERR_CHK(safec_rc);
   }
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, event, &msg, msg_size);
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

template <typename iarm_call_struct>
static IARM_Result_t ctrlm_rcu_iarm_call_dispatch(iarm_call_struct* params, gboolean (*iarm_call_handler)(iarm_call_struct*)) {
   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("(%s) IARM Call received when IARM component in stopped/terminated state, reply with ERROR", typeid(iarm_call_struct).name());
      return(IARM_RESULT_INVALID_STATE);
    }

   if(params == NULL) {
      XLOGD_ERROR("(%s) NULL parameters", typeid(iarm_call_struct).name());
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(params->api_revision != CTRLM_RCU_IARM_BUS_API_REVISION) {
      XLOGD_INFO("(%s) Unsupported API Revision (%u, %u)", typeid(iarm_call_struct).name(), params->api_revision, CTRLM_RCU_IARM_BUS_API_REVISION);
      params->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }

   if(!iarm_call_handler(params)) {
      XLOGD_ERROR("(%s) Error ", typeid(iarm_call_struct).name());
      params->result = CTRLM_IARM_CALL_RESULT_ERROR;
   } else {
      params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
   }

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcu_iarm_call_validation_finish(void *arg) {
   ctrlm_rcu_iarm_call_validation_finish_t *params = (ctrlm_rcu_iarm_call_validation_finish_t *) arg;
   XLOGD_INFO("(%u, %u) Validation Result <%s>", params->network_id, params->controller_id, ctrlm_rcu_validation_result_str(params->validation_result));
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_validation_finish);
}

IARM_Result_t ctrlm_rcu_iarm_call_controller_status(void *arg) {
   ctrlm_rcu_iarm_call_controller_status_t *params = (ctrlm_rcu_iarm_call_controller_status_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_controller_status);
}

IARM_Result_t ctrlm_rcu_iarm_call_rib_request_get(void *arg) {
   ctrlm_rcu_iarm_call_rib_request_t *params = (ctrlm_rcu_iarm_call_rib_request_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_rib_request_get);
}

IARM_Result_t ctrlm_rcu_iarm_call_rib_request_set(void *arg) {
   ctrlm_rcu_iarm_call_rib_request_t *params = (ctrlm_rcu_iarm_call_rib_request_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_rib_request_set);
}

IARM_Result_t ctrlm_rcu_iarm_call_controller_link_key(void *arg) {
   ctrlm_rcu_iarm_call_controller_link_key_t *params = (ctrlm_rcu_iarm_call_controller_link_key_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_controller_link_key);
    }

IARM_Result_t ctrlm_rcu_iarm_call_reverse_cmd(void *arg) {
   ctrlm_main_iarm_call_rcu_reverse_cmd_t *params = (ctrlm_main_iarm_call_rcu_reverse_cmd_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_reverse_cmd);
}

IARM_Result_t ctrlm_rcu_iarm_call_rf4ce_polling_action(void *arg) {
   ctrlm_rcu_iarm_call_rf4ce_polling_action_t *params = (ctrlm_rcu_iarm_call_rf4ce_polling_action_t *) arg;
   return ctrlm_rcu_iarm_call_dispatch(params, &ctrlm_rcu_rf4ce_polling_action);
}

void ctrlm_rcu_iarm_event_battery_milestone(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_battery_event_t battery_event, guchar percent) {
   ctrlm_rcu_iarm_event_battery_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.battery_event = battery_event;
   msg.percent       = percent;
   XLOGD_INFO("(%u, %u) Battery Event <%s> Percent <%d>", network_id, controller_id, ctrlm_battery_event_str(battery_event), percent);
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_BATTERY_MILESTONE, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_iarm_event_remote_reboot(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar voltage, controller_reboot_reason_t reason, guint32 assert_number) {
   ctrlm_rcu_iarm_event_remote_reboot_t msg;
   init_iarm_event_struct(msg, network_id, controller_id);
   msg.voltage       = voltage;
   msg.reason        = reason;
   msg.assert_number = assert_number;
   if(reason == CONTROLLER_REBOOT_ASSERT_NUMBER) {
      XLOGD_INFO("(%u, %u) Voltage <%d> Reboot Reason <%s> Assert Number <%u>", network_id, controller_id, voltage, ctrlm_rf4ce_reboot_reason_str(reason), assert_number);
   } else {
      XLOGD_INFO("(%u, %u) Voltage <%d> Reboot Reason <%s>", network_id, controller_id, voltage, ctrlm_rf4ce_reboot_reason_str(reason));
   }
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_REMOTE_REBOOT, &msg, sizeof(msg));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_rcu_terminate_iarm() {
   // Change state to terminated so we do not accept calls
   g_atomic_int_set(&running, 0);
}
