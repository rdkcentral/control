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
#ifndef _CTRLM_VALIDATION_H_
#define _CTRLM_VALIDATION_H_

#include "jansson.h"
#include <vector>

typedef enum {
   CTRLM_VALIDATION_END_CMD_RESULT_PENDING = 0,
   CTRLM_VALIDATION_END_CMD_RESULT_SUCCESS = 1,
   CTRLM_VALIDATION_END_CMD_RESULT_ERROR   = 2
} ctrlm_validation_end_cmd_result_t;

typedef struct {
   ctrlm_main_queue_msg_header_t       header;
   ctrlm_controller_id_t               controller_id;
   unsigned long long                  ieee_address;
} ctrlm_main_queue_msg_bind_validation_begin_t;

typedef struct {
   ctrlm_main_queue_msg_header_t      header;
   ctrlm_controller_id_t              controller_id;
   ctrlm_rcu_binding_type_t           binding_type;
   ctrlm_rcu_validation_type_t        validation_type;
   ctrlm_rcu_validation_result_t      result;
   sem_t *                            semaphore;
   ctrlm_validation_end_cmd_result_t *cmd_result;
} ctrlm_main_queue_msg_bind_validation_end_t;

typedef struct {
   ctrlm_main_queue_msg_header_t      header;
   ctrlm_controller_id_t              controller_id;
   ctrlm_rcu_binding_type_t           binding_type;
   ctrlm_rcu_configuration_result_t   result;
} ctrlm_main_queue_msg_bind_configuration_complete_t;

#ifdef __cplusplus
extern "C"
{
#endif

void     ctrlm_validation_init(json_t *json_obj_validation);
void     ctrlm_validation_terminate(void);
guint    ctrlm_validation_timeout_initial_get(void);
void     ctrlm_validation_timeout_initial_set(guint value);
guint    ctrlm_validation_timeout_subsequent_get(void);
void     ctrlm_validation_timeout_subsequent_set(guint value);
guint    ctrlm_validation_timeout_configuration_get(void);
void     ctrlm_validation_timeout_configuration_set(guint value);
guint    ctrlm_validation_max_attempts_get(void);
void     ctrlm_validation_max_attempts_set(guint value);
std::vector<ctrlm_key_code_t> ctrlm_validation_golden_code_get(void);

void     ctrlm_inform_validation_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long long ieee_address);
gboolean ctrlm_inform_validation_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t binding_type, ctrlm_rcu_validation_type_t validation_type, ctrlm_rcu_validation_result_t validation_result, sem_t *semaphore, ctrlm_validation_end_cmd_result_t *cmd_result);
gboolean ctrlm_inform_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_configuration_result_t configuration_result);

void     ctrlm_validation_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type);
gboolean ctrlm_validation_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_rcu_validation_type_t validation_type, ctrlm_rcu_validation_result_t validation_result, sem_t *semaphore, ctrlm_validation_end_cmd_result_t *cmd_result);
gboolean ctrlm_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_controller_status_t *status, ctrlm_rcu_configuration_result_t configuration_result);
gboolean ctrlm_validation_key_sniff(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code, gboolean auto_bind_in_progress);

#ifdef __cplusplus
}
#endif

#endif
