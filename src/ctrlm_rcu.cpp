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
#include <semaphore.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "ctrlm_validation.h"
#include "rf4ce/ctrlm_rf4ce_network.h"

void ctrlm_rcu_init(void) {
   XLOGD_INFO("");
   if(!ctrlm_rcu_init_iarm()) {
      XLOGD_ERROR("IARM init failed.");
   }
}

void ctrlm_rcu_terminate(void) {
   XLOGD_INFO("clean up");
   ctrlm_rcu_terminate_iarm();
}

gboolean ctrlm_rcu_validation_finish(ctrlm_rcu_iarm_call_validation_finish_t *params) {

#if 1 // ASYNCHRONOUS
   if(!ctrlm_controller_id_is_valid(params->network_id, params->controller_id)) {
      XLOGD_ERROR("invalid controller id (%u, %u)", params->network_id, params->controller_id);
      return(false);
   }
   if(params->validation_result >= CTRLM_RCU_VALIDATION_RESULT_MAX) {
      XLOGD_ERROR("invalid validation result (%d)", params->validation_result);
      return(false);
   }

   ctrlm_inform_validation_end(params->network_id, params->controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, CTRLM_RCU_VALIDATION_TYPE_APPLICATION, params->validation_result, NULL, NULL);
   return(true);
#else // synchronous
   sem_t semaphore;
   ctrlm_validation_end_cmd_result_t cmd_result = CTRLM_VALIDATION_END_CMD_RESULT_PENDING;

   sem_init(&semaphore, 0, 0);

   if(!ctrlm_inform_validation_end(params->network_id, params->controller_id, params->validation_result, &semaphore, &cmd_result)) {
      sem_destroy(&semaphore);
      return(false);
   }

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_VALIDATION_END_CMD_RESULT_SUCCESS) {
      return(true);
   }
   return(false);
#endif
}

gboolean ctrlm_rcu_controller_status(ctrlm_rcu_iarm_call_controller_status_t *params) {
   XLOGD_INFO("(%u, %u)", params->network_id, params->controller_id);

   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL || params->controller_id == CTRLM_MAIN_CONTROLLER_ID_ALL) {
      XLOGD_ERROR("Cannot get status for multiple controllers");
      return(false);
   }
   sem_t semaphore;
   ctrlm_controller_status_cmd_result_t cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_controller_status_t msg = {0};

   sem_init(&semaphore, 0, 0);

   msg.controller_id     = params->controller_id;
   msg.status            = &params->status;
   msg.semaphore         = &semaphore;
   msg.cmd_result        = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_controller_status, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_rcu_rib_request_get(ctrlm_rcu_iarm_call_rib_request_t *params) {
   if(params->attribute_id == CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE) {
      XLOGD_INFO("(%u, %u) Attribute <%s> Index <%s> Length %u", params->network_id, params->controller_id, ctrlm_rcu_rib_attr_id_str(params->attribute_id), ctrlm_key_code_str((ctrlm_key_code_t)params->attribute_index), params->length);
   } else {
      XLOGD_INFO("(%u, %u) Attribute <%s> Index %u Length %u", params->network_id, params->controller_id, ctrlm_rcu_rib_attr_id_str(params->attribute_id), params->attribute_index, params->length);
   }

   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL || params->controller_id == CTRLM_MAIN_CONTROLLER_ID_ALL) {
      XLOGD_ERROR("Cannot get multiple RIB entries");
      return(false);
   }
   sem_t  semaphore;
   ctrlm_rib_request_cmd_result_t cmd_result = CTRLM_RIB_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_rib_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.controller_id     = params->controller_id;
   msg.attribute_id      = params->attribute_id;
   msg.attribute_index   = params->attribute_index;
   msg.length            = params->length;
   msg.length_out        = &params->length;
   msg.data              = (guchar *)params->data;
   msg.semaphore         = &semaphore;
   msg.cmd_result        = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_rib_get, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_RIB_REQUEST_SUCCESS) {
      return(true);
   }

   params->length = 0;
   XLOGD_ERROR("Failed to get RIB entry");

   return(false);
}

gboolean ctrlm_rcu_rib_request_set(ctrlm_rcu_iarm_call_rib_request_t *params) {

   if(params->attribute_id == CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE) {
      XLOGD_INFO("(%u, %u) Attribute <%s> Index <%s> Length %u", params->network_id, params->controller_id, ctrlm_rcu_rib_attr_id_str(params->attribute_id), ctrlm_key_code_str((ctrlm_key_code_t)params->attribute_index), params->length);
   } else {
      XLOGD_INFO("(%u, %u) Attribute <%s> Index %u Length %u", params->network_id, params->controller_id, ctrlm_rcu_rib_attr_id_str(params->attribute_id), params->attribute_index, params->length);
   }

   if(params->length > CTRLM_RCU_MAX_RIB_ATTRIBUTE_SIZE) {
      XLOGD_ERROR("Invalid length %u", params->length);
      return(false);
   }
   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
      XLOGD_ERROR("Cannot set rib for multiple networks");
      return(false);
   }

   sem_t semaphore;
   ctrlm_rib_request_cmd_result_t cmd_result = CTRLM_RIB_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_rib_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.controller_id     = params->controller_id;
   msg.attribute_id      = params->attribute_id;
   msg.attribute_index   = params->attribute_index;
   msg.length            = params->length;
   msg.length_out        = 0;
   msg.data              = (guchar *)params->data;
   msg.semaphore         = &semaphore;
   msg.cmd_result        = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_rib_set, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_RIB_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_rcu_controller_link_key(ctrlm_rcu_iarm_call_controller_link_key_t *params) {
   XLOGD_INFO("(%u, %u)", params->network_id, params->controller_id);

   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL || params->controller_id == CTRLM_MAIN_CONTROLLER_ID_ALL) {
      XLOGD_ERROR("Cannot get status for multiple controllers");
      return(false);
   }
   sem_t semaphore;
   ctrlm_controller_status_cmd_result_t cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_controller_link_key_t msg = {0};

   sem_init(&semaphore, 0, 0);

   msg.controller_id     = params->controller_id;
   msg.link_key          = params->link_key;
   msg.semaphore         = &semaphore;
   msg.cmd_result        = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_controller_link_key, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_rcu_reverse_cmd(ctrlm_main_iarm_call_rcu_reverse_cmd_t *params) {
   XLOGD_INFO("(%s, %s)", ctrlm_network_type_str(params->network_type).c_str(), ctrlm_controller_name_str(params->controller_id).c_str());
#if (CTRLM_HAL_RF4CE_API_VERSION < 14)
   XLOGD_ERROR("Reverse command is supported with Ctrlm HAL API >= 14");
   return(false);
#endif

   sem_t semaphore;
   ctrlm_controller_status_cmd_result_t cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   unsigned int extra_data = params->total_size - sizeof(ctrlm_main_iarm_call_rcu_reverse_cmd_t);
   unsigned int msg_size = sizeof(ctrlm_main_queue_msg_rcu_reverse_cmd_t) + extra_data;
   ctrlm_main_queue_msg_rcu_reverse_cmd_t *msg = (ctrlm_main_queue_msg_rcu_reverse_cmd_t *)g_malloc(msg_size);

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_REVERSE_CMD;
   msg->header.network_id = ctrlm_network_id_get(params->network_type);
   msg->controller_id     = params->controller_id;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   errno_t safec_rc = memcpy_s(&msg->reverse_command, sizeof(msg->reverse_command) + extra_data, params, params->total_size);
   ERR_CHK(safec_rc);

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   // Return reverse command result
   params->cmd_result = msg->reverse_command.cmd_result;

   if(cmd_result == CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}


gboolean ctrlm_rcu_controller_type_get(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t *type) {
   XLOGD_INFO("(%u, %u)", network_id, controller_id);

   if(network_id == CTRLM_MAIN_NETWORK_ID_ALL || controller_id == CTRLM_MAIN_CONTROLLER_ID_ALL) {
      XLOGD_ERROR("Cannot get type for multiple controllers");
      return(false);
   }

   sem_t semaphore;
   ctrlm_controller_status_cmd_result_t cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_controller_type_get_t *msg = (ctrlm_main_queue_msg_controller_type_get_t *)g_malloc(sizeof(ctrlm_main_queue_msg_controller_type_get_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE_GET;
   msg->header.network_id = network_id;
   msg->controller_id     = controller_id;
   msg->controller_type   = type;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_rcu_rf4ce_polling_action(ctrlm_rcu_iarm_call_rf4ce_polling_action_t *params) {
   XLOGD_INFO("(%u, %u)", params->network_id, params->controller_id);

   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
      XLOGD_ERROR("Cannot set polling action for multiple networks");
      return(false);
   }

   errno_t safec_rc = -1;
   sem_t semaphore;
   ctrlm_controller_status_cmd_result_t cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_rcu_polling_action_t msg;
   safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.header.type        = CTRLM_MAIN_QUEUE_MSG_TYPE_RCU_POLLING_ACTION;
   msg.header.network_id  = params->network_id;
   msg.controller_id      = params->controller_id;
   msg.action             = params->action;
   msg.semaphore          = &semaphore;
   msg.cmd_result         = &cmd_result;
   safec_rc = memcpy_s(msg.data, sizeof(msg.data), params->data, sizeof(msg.data));
   ERR_CHK(safec_rc);

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_polling_action_push, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}
