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
#include <glib.h>
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "ctrlm_ble_iarm.h"
#include "ctrlm_ble_network.h"
#include "ctrlm_log.h"

static IARM_Result_t ctrlm_main_iarm_call_get_rcu_unpair_reason(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_get_rcu_reboot_reason(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_get_rcu_last_wakeup_key(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_send_rcu_action(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_get_ir_protocol_support(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_set_ir_protocol_control(void *arg);

typedef struct {
   const char *   name;
   IARM_BusCall_t handler;
} ctrlm_ble_iarm_call_t;

// Array to hold the IARM Calls that will be registered by Control Manager
ctrlm_ble_iarm_call_t ctrlm_ble_iarm_calls[] = {
   {CTRLM_BLE_IARM_CALL_GET_RCU_UNPAIR_REASON,               ctrlm_main_iarm_call_get_rcu_unpair_reason},
   {CTRLM_BLE_IARM_CALL_GET_RCU_REBOOT_REASON,               ctrlm_main_iarm_call_get_rcu_reboot_reason},
   {CTRLM_BLE_IARM_CALL_GET_LAST_WAKEUP_KEY,                 ctrlm_main_iarm_call_get_rcu_last_wakeup_key},
   {CTRLM_BLE_IARM_CALL_SEND_RCU_ACTION,                     ctrlm_main_iarm_call_send_rcu_action},
   {CTRLM_BLE_IARM_CALL_GET_IR_PROTOCOL_SUPPORT,             ctrlm_main_iarm_call_get_ir_protocol_support},
   {CTRLM_BLE_IARM_CALL_SET_IR_PROTOCOL_CONTROL,             ctrlm_main_iarm_call_set_ir_protocol_control},
};

// Keep state since we do not want to service calls on termination
static volatile int running = 0;

bool ctrlm_ble_iarm_init(void) {
   IARM_Result_t result;
   guint index;

   // Register calls that can be invoked by IARM bus clients
   for(index = 0; index < sizeof(ctrlm_ble_iarm_calls)/sizeof(ctrlm_ble_iarm_call_t); index++) {
      result = IARM_Bus_RegisterCall(ctrlm_ble_iarm_calls[index].name, ctrlm_ble_iarm_calls[index].handler);
      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_FATAL("Unable to register method %s!", ctrlm_ble_iarm_calls[index].name);
         return(false);
      }
   }

   // Change to running state so we can accept calls
   g_atomic_int_set(&running, 1);

   return(true);
}

void ctrlm_ble_iarm_terminate(void) {
   // Change to stopped or terminated state, so we do not accept new calls
   g_atomic_int_set(&running, 0);
}


IARM_Result_t ctrlm_main_iarm_call_get_rcu_unpair_reason(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_GetRcuUnpairReason_params_t *params = (ctrlm_iarm_call_GetRcuUnpairReason_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_get_rcu_unpair_reason_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);
   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_get_rcu_unpair_reason, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_get_rcu_reboot_reason(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_GetRcuRebootReason_params_t *params = (ctrlm_iarm_call_GetRcuRebootReason_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_get_rcu_reboot_reason_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);
   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_get_rcu_reboot_reason, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_get_rcu_last_wakeup_key(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_GetRcuLastWakeupKey_params_t *params = (ctrlm_iarm_call_GetRcuLastWakeupKey_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_get_last_wakeup_key_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);
   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_get_rcu_last_wakeup_key, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_send_rcu_action(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_SendRcuAction_params_t *params = (ctrlm_iarm_call_SendRcuAction_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_send_rcu_action_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);
   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_send_rcu_action, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_get_ir_protocol_support(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_IrProtocol_params_t *params = (ctrlm_iarm_call_IrProtocol_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_ir_protocol_characteristic_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_get_ir_protocol_support, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_set_ir_protocol_control(void *arg) {
   XLOGD_INFO("Enter...");
   ctrlm_iarm_call_IrProtocol_params_t *params = (ctrlm_iarm_call_IrProtocol_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_DEBUG("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;
   sem_init(&semaphore, 0, 0);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_ir_protocol_characteristic_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);
   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::req_process_set_ir_protocol_control, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);
   return(IARM_RESULT_SUCCESS);
}
