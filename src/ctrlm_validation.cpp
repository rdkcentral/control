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
#include <glib.h>
#include "libIBus.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "ctrlm_validation.h"
#include "ctrlm_config_default.h"
#include <algorithm>

#define CTRLM_VALIDATION_NUMBER_QTY (10)
#define CTRLM_VALIDATION_LETTER_QTY ( 4)

#define CTRLM_EXCLUDED_CODE_911 ((CTRLM_KEY_CODE_DIGIT_9 << 16) | (CTRLM_KEY_CODE_DIGIT_1 << 8) | CTRLM_KEY_CODE_DIGIT_1)
#define CTRLM_EXCLUDED_CODE_666 ((CTRLM_KEY_CODE_DIGIT_6 << 16) | (CTRLM_KEY_CODE_DIGIT_6 << 8) | CTRLM_KEY_CODE_DIGIT_6)

#define CTRLM_MAX_NUM_WRONG_KEYPRESSES (5)

static ctrlm_key_code_t ctrlm_validation_numbers[CTRLM_VALIDATION_NUMBER_QTY] = {
   CTRLM_KEY_CODE_DIGIT_0,
   CTRLM_KEY_CODE_DIGIT_1,
   CTRLM_KEY_CODE_DIGIT_2,
   CTRLM_KEY_CODE_DIGIT_3,
   CTRLM_KEY_CODE_DIGIT_4,
   CTRLM_KEY_CODE_DIGIT_5,
   CTRLM_KEY_CODE_DIGIT_6,
   CTRLM_KEY_CODE_DIGIT_7,
   CTRLM_KEY_CODE_DIGIT_8,
   CTRLM_KEY_CODE_DIGIT_9
};

static ctrlm_key_code_t ctrlm_validation_letters[CTRLM_VALIDATION_LETTER_QTY] = {
   CTRLM_KEY_CODE_OCAP_A,
   CTRLM_KEY_CODE_OCAP_B,
   CTRLM_KEY_CODE_OCAP_C,
   CTRLM_KEY_CODE_OCAP_D
};

typedef struct {
   gboolean                       validation_in_progress;
   gboolean                       configuration_in_progress;
   ctrlm_network_id_t             network_id;
   ctrlm_controller_id_t          controller_id;
   guint                          timeout_source_id;
   guint                          timeout_ignore_id;
   ctrlm_key_code_t               golden_code[CTRLM_RCU_VALIDATION_KEY_QTY];
   guint                          golden_index;
   guint                          timeout_initial;
   guint                          timeout_subsequent;
   guint                          timeout_configuration;
   guint                          timeout_ignore;
   guint                          max_attempts;
   gboolean                       app_based_validation;
   gboolean                       ignore_abort;
   unsigned char                  num_wrong_keypresses;
} ctrlm_validation_global_t;

static ctrlm_validation_global_t g_ctrlm_validation;

static gboolean         ctrlm_validation_is_in_progress(void);
static gboolean         ctrlm_validation_on_this_network(ctrlm_network_id_t network_id);
static gboolean         ctrlm_validation_on_this_controller(ctrlm_controller_id_t controller_id);
static gboolean         ctrlm_validation_timeout(gpointer user_data);
static gboolean         ctrlm_validation_timeout_ignore(gpointer user_data);
static void             ctrlm_validation_timeout_update(gint timeout);
static void             ctrlm_validation_random_code(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type);
static ctrlm_key_code_t ctrlm_validation_random_number(void);
static ctrlm_key_code_t ctrlm_validation_random_letter(void);
static gboolean         ctrlm_validation_load_config(json_t *json_obj_validation);
static gboolean         ctrlm_validation_ignore_abort(void);

void ctrlm_validation_init(json_t *json_obj_validation) {
   struct timespec tp;
   clock_gettime(CLOCK_MONOTONIC, &tp);
   srandom(tp.tv_nsec);

   g_ctrlm_validation.validation_in_progress    = FALSE;
   g_ctrlm_validation.configuration_in_progress = FALSE;
   g_ctrlm_validation.network_id                = 0;
   g_ctrlm_validation.controller_id             = 0;
   g_ctrlm_validation.timeout_source_id         = 0;
   for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
      g_ctrlm_validation.golden_code[index]     = CTRLM_KEY_CODE_OK;
   }
   g_ctrlm_validation.golden_index              = 0;
   g_ctrlm_validation.num_wrong_keypresses      = 0;
   g_ctrlm_validation.timeout_initial           = JSON_INT_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_INITIAL;
   g_ctrlm_validation.timeout_subsequent        = JSON_INT_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_SUBSEQUENT;
   g_ctrlm_validation.timeout_configuration     = JSON_INT_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_TIMEOUT_CONFIG_COMPLETE;
   g_ctrlm_validation.timeout_ignore            = JSON_INT_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_IGNORE_ABORT;
   g_ctrlm_validation.max_attempts              = JSON_INT_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_MAX_ATTEMPTS;
   g_ctrlm_validation.app_based_validation      = JSON_BOOL_VALUE_CTRLM_GLOBAL_VALIDATION_CONFIG_APP_BASED_VALIDATION;

   ctrlm_validation_load_config(json_obj_validation);
}

void ctrlm_validation_terminate(void) {
   XLOGD_INFO("clean up");
   if(g_ctrlm_validation.validation_in_progress) {
      XLOGD_INFO("Validation in progress, ending validation");
      ctrlm_rcu_validation_type_t validation_type = g_ctrlm_validation.app_based_validation ? CTRLM_RCU_VALIDATION_TYPE_APPLICATION : CTRLM_RCU_VALIDATION_TYPE_INTERNAL;
      ctrlm_inform_validation_end(g_ctrlm_validation.network_id, g_ctrlm_validation.controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART, NULL, NULL);
   }
}

gboolean ctrlm_validation_load_config(json_t *json_obj_validation) {
   if(json_obj_validation == NULL || !json_is_object(json_obj_validation)) {
      XLOGD_INFO("use default configuration");
   } else {
      json_t *json_obj = json_object_get(json_obj_validation, JSON_INT_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_TIMEOUT_CONFIG_COMPLETE);
      const char *text = "Timeout Config Complete";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm_validation.timeout_configuration = value;
         }
      }
      json_obj = json_object_get(json_obj_validation, JSON_BOOL_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_APP_BASED_VALIDATION);
      text     = "App Based Validation";
      if(json_obj == NULL || !json_is_boolean(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         if(json_is_true(json_obj)) {
            g_ctrlm_validation.app_based_validation = true;
         } else {
            g_ctrlm_validation.app_based_validation = false;
         }
         XLOGD_INFO("%-24s - PRESENT <%s>", text, g_ctrlm_validation.app_based_validation ? "true" : "false");
      }

      json_t *json_obj_ctrlm = json_object_get(json_obj_validation, JSON_OBJ_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM);
      if(json_obj_ctrlm == NULL || !json_is_object(json_obj_ctrlm)) {
         XLOGD_INFO("%s object not found", JSON_OBJ_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM);
      } else {
         json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_MAX_ATTEMPTS);
         text     = "Max Attempts";
         if(json_obj == NULL || !json_is_integer(json_obj)) {
            XLOGD_INFO("%-24s - ABSENT", text);
         } else {
            json_int_t value = json_integer_value(json_obj);
            XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
            if(value < 0) {
               XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
            } else {
               g_ctrlm_validation.max_attempts = value;
            }
         }
         json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_INITIAL);
         text     = "Timeout Initial";
         if(json_obj == NULL || !json_is_integer(json_obj)) {
            XLOGD_INFO("%-24s - ABSENT", text);
         } else {
            json_int_t value = json_integer_value(json_obj);
            XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
            if(value < 0) {
               XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
            } else {
               g_ctrlm_validation.timeout_initial = value;
            }
         }
         json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_SUBSEQUENT);
         text     = "Timeout Subsequent";
         if(json_obj == NULL || !json_is_integer(json_obj)) {
            XLOGD_INFO("%-24s - ABSENT", text);
         } else {
            json_int_t value = json_integer_value(json_obj);
            XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
            if(value < 0) {
               XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
            } else {
               g_ctrlm_validation.timeout_subsequent = value;
            }
         }
         json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG_CTRLM_TIMEOUT_IGNORE_ABORT);
         text     = "Timeout Ignore Abort";
         if(json_obj == NULL || !json_is_integer(json_obj)) {
            XLOGD_INFO("%-24s - ABSENT", text);
         } else {
            json_int_t value = json_integer_value(json_obj);
            XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
            if(value < 0) {
               XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
            } else {
               g_ctrlm_validation.timeout_ignore = value;
            }
         }
      }
   }

   XLOGD_INFO("App Based Validation    <%s>",  g_ctrlm_validation.app_based_validation ? "YES" : "NO");
   XLOGD_INFO("Max Attempts            %u",    g_ctrlm_validation.max_attempts);
   XLOGD_INFO("Timeout Initial         %u ms", g_ctrlm_validation.timeout_initial);
   XLOGD_INFO("Timeout Subsequent      %u ms", g_ctrlm_validation.timeout_subsequent);
   XLOGD_INFO("Timeout Config Complete %u ms", g_ctrlm_validation.timeout_configuration);
   XLOGD_INFO("Timeout Ignore Abort    %u ms", g_ctrlm_validation.timeout_ignore);

   return(true);
}

gboolean ctrlm_validation_is_in_progress(void) {
   return(g_ctrlm_validation.validation_in_progress | g_ctrlm_validation.configuration_in_progress);
}

gboolean ctrlm_validation_on_this_network(ctrlm_network_id_t network_id) {
   XLOGD_DEBUG("0x%X 0x%X", g_ctrlm_validation.network_id, network_id);
   return(g_ctrlm_validation.network_id == network_id);
}

gboolean ctrlm_validation_on_this_controller(ctrlm_controller_id_t controller_id) {
   XLOGD_DEBUG("0x%X 0x%X", g_ctrlm_validation.controller_id, controller_id);
   return(g_ctrlm_validation.controller_id == controller_id);
}

guint ctrlm_validation_timeout_initial_get() {
   return(g_ctrlm_validation.timeout_initial);
}

void ctrlm_validation_timeout_initial_set(guint value) {
   g_ctrlm_validation.timeout_initial = value;
}

guint ctrlm_validation_timeout_subsequent_get() {
   return(g_ctrlm_validation.timeout_subsequent);
}

void ctrlm_validation_timeout_subsequent_set(guint value) {
   g_ctrlm_validation.timeout_subsequent = value;
}

guint ctrlm_validation_timeout_configuration_get() {
   return(g_ctrlm_validation.timeout_configuration);
}

void ctrlm_validation_timeout_configuration_set(guint value) {
   g_ctrlm_validation.timeout_configuration = value;
}

guint ctrlm_validation_max_attempts_get(void) {
   return(g_ctrlm_validation.max_attempts);
}

void ctrlm_validation_max_attempts_set(guint value) {
   g_ctrlm_validation.max_attempts = value;
}

void ctrlm_validation_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type) {
   ctrlm_rcu_validation_type_t validation_type = g_ctrlm_validation.app_based_validation ? CTRLM_RCU_VALIDATION_TYPE_APPLICATION : CTRLM_RCU_VALIDATION_TYPE_INTERNAL;

   if(ctrlm_validation_is_in_progress()) { // Check to make sure a validation is not already in progress (this is global not per network)
      if(ctrlm_validation_on_this_network(network_id) && ctrlm_validation_on_this_controller(controller_id)) {
         XLOGD_INFO("(%u, %u) ALREADY IN PROGRESS (SAME CONTROLLER)", network_id, controller_id);
         return;
      } else {
         XLOGD_INFO("(%u, %u) ALREADY IN PROGRESS", network_id, controller_id);
         ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_IN_PROGRESS, NULL, NULL);
         return;
      }
   } else if(ctrlm_is_binding_table_full()) { // Binding table is full
      XLOGD_INFO("(%u, %u) BINDING TABLE FULL", network_id, controller_id);
      ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_BIND_TABLE_FULL, NULL, NULL);
      return;
   }

   if(ctrlm_precomission_lookup(network_id, controller_id)) { // validation due to successful lookup in precommission table
      XLOGD_INFO("(%u, %u) PRECOMMISSIONED", network_id, controller_id);
      ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, CTRLM_RCU_VALIDATION_TYPE_PRECOMMISSION, CTRLM_RCU_VALIDATION_RESULT_SUCCESS, NULL, NULL);
   } else {
      XLOGD_INFO("(%u, %u) BEGIN", network_id, controller_id);

      if(!g_ctrlm_validation.app_based_validation) {
         // Choose a new golden code
         ctrlm_validation_random_code(network_id, controller_id, controller_type);
         for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
            XLOGD_INFO("Golden Code %lu <%s>", index, ctrlm_key_code_str(g_ctrlm_validation.golden_code[index]));
         }
      }
      XLOGD_INFO("(%u, %u) Binding type <%s> Validation Type <%s> Controller Type <%s>", network_id, controller_id, ctrlm_rcu_binding_type_str(CTRLM_RCU_BINDING_TYPE_INTERACTIVE), ctrlm_rcu_validation_type_str(validation_type), ctrlm_rcu_controller_type_str(controller_type));

      // Start a timeout for the validation response
      g_ctrlm_validation.timeout_source_id      = ctrlm_timeout_create(g_ctrlm_validation.timeout_initial, ctrlm_validation_timeout, NULL);
      g_ctrlm_validation.timeout_ignore_id      = ctrlm_timeout_create(g_ctrlm_validation.timeout_ignore, ctrlm_validation_timeout_ignore, NULL);
      g_ctrlm_validation.validation_in_progress = TRUE;
      g_ctrlm_validation.network_id             = network_id;
      g_ctrlm_validation.controller_id          = controller_id;
      g_ctrlm_validation.ignore_abort           = true;
      g_ctrlm_validation.num_wrong_keypresses   = 0;
   }
}

gboolean ctrlm_validation_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_rcu_validation_type_t validation_type, ctrlm_rcu_validation_result_t validation_result, sem_t *semaphore, ctrlm_validation_end_cmd_result_t *cmd_result) {
   XLOGD_INFO("(%u, %u) Type <%s> result <%s>", network_id, controller_id, ctrlm_rcu_validation_type_str(validation_type), ctrlm_rcu_validation_result_str(validation_result));

   for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
      g_ctrlm_validation.golden_code[index]     = CTRLM_KEY_CODE_OK;
   }

   if(validation_type == CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC || validation_type == CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND) {
      XLOGD_INFO("(%u, %u) Binding Type <%s> Validation Type <%s> Controller Type <%s> Result <%s>", network_id, controller_id, ctrlm_rcu_binding_type_str(binding_type), ctrlm_rcu_validation_type_str(validation_type), ctrlm_rcu_controller_type_str(controller_type), ctrlm_rcu_validation_result_str(validation_result));

      if(validation_result == CTRLM_RCU_VALIDATION_RESULT_SUCCESS) {
         // Move to configuration in progress state
         g_ctrlm_validation.configuration_in_progress = TRUE;
         g_ctrlm_validation.ignore_abort              = TRUE;
         g_ctrlm_validation.network_id                = network_id;
         g_ctrlm_validation.controller_id             = controller_id;
         g_ctrlm_validation.timeout_source_id         = ctrlm_timeout_create(g_ctrlm_validation.timeout_configuration, ctrlm_validation_timeout, NULL);
      }

      if(semaphore != NULL && cmd_result != NULL) {
         // Signal the semaphore to indicate that the result is present
         *cmd_result = CTRLM_VALIDATION_END_CMD_RESULT_SUCCESS;
         sem_post(semaphore);
      }
      return(true);
   } else if(ctrlm_validation_is_in_progress() && ctrlm_validation_on_this_network(network_id) && ctrlm_validation_on_this_controller(controller_id)) {
      if(!g_ctrlm_validation.validation_in_progress) {
         XLOGD_ERROR("Validation NOT IN PROGRESS (%u, %u)", network_id, controller_id);
      } else {
         // Cancel validation response timer
         ctrlm_timeout_destroy(&g_ctrlm_validation.timeout_source_id);

         //If validation ended due to ctrlm restarting, treat as full abort for MSO pairing
         if(validation_result == CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART) {
            validation_result = CTRLM_RCU_VALIDATION_RESULT_FULL_ABORT;
         }

         XLOGD_INFO("(%u, %u) Binding Type <%s> Validation Type <%s> Controller Type <%s> Result <%s>", network_id, controller_id, ctrlm_rcu_binding_type_str(binding_type), ctrlm_rcu_validation_type_str(validation_type), ctrlm_rcu_controller_type_str(controller_type), ctrlm_rcu_validation_result_str(validation_result));

         if(validation_result == CTRLM_RCU_VALIDATION_RESULT_SUCCESS) {
            // Move to configuration in progress state
            g_ctrlm_validation.configuration_in_progress = TRUE;
            ctrlm_validation_timeout_update(g_ctrlm_validation.timeout_configuration);
         }
         g_ctrlm_validation.validation_in_progress    = FALSE;
      }
      if(semaphore != NULL && cmd_result != NULL) {
         // Signal the semaphore to indicate that the result is present
         *cmd_result = CTRLM_VALIDATION_END_CMD_RESULT_SUCCESS;
         sem_post(semaphore);
      }
      return(true);
   }
   XLOGD_ERROR("NOT IN PROGRESS (%u, %u)", network_id, controller_id);
   if(semaphore != NULL && cmd_result != NULL) {
      // Signal the semaphore to indicate that the result is present
      *cmd_result = CTRLM_VALIDATION_END_CMD_RESULT_ERROR;
      sem_post(semaphore);
   }
   return(false);
}

gboolean ctrlm_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_rcu_binding_type_t binding_type, ctrlm_controller_status_t *status, ctrlm_rcu_configuration_result_t configuration_result) {
   XLOGD_INFO("(%u, %u) result <%s>", network_id, controller_id, ctrlm_rcu_configuration_result_str(configuration_result));
   if(ctrlm_validation_is_in_progress() && ctrlm_validation_on_this_network(network_id) && ctrlm_validation_on_this_controller(controller_id)) {
      if(!g_ctrlm_validation.configuration_in_progress) {
         XLOGD_ERROR("Configuration NOT IN PROGRESS (%u, %u)", network_id, controller_id);
         return(false);
      }
      // Cancel validation response timer
      ctrlm_timeout_destroy(&g_ctrlm_validation.timeout_source_id);
      XLOGD_INFO("(%u, %u) Controller Type <%s> Binding Type <%s> Result <%s>", network_id, controller_id, ctrlm_rcu_controller_type_str(controller_type), ctrlm_rcu_binding_type_str(binding_type), ctrlm_rcu_configuration_result_str(configuration_result));
      g_ctrlm_validation.configuration_in_progress = FALSE;
      return(true);
   }
   XLOGD_TELEMETRY("Validation NOT IN PROGRESS (%u, %u)", network_id, controller_id);
   return(false);
}

gboolean ctrlm_validation_timeout(gpointer user_data) {
   ctrlm_rcu_validation_type_t validation_type = g_ctrlm_validation.app_based_validation ? CTRLM_RCU_VALIDATION_TYPE_APPLICATION : CTRLM_RCU_VALIDATION_TYPE_INTERNAL;

   XLOGD_INFO("Validation timed out.");
   if(ctrlm_validation_is_in_progress()) { // Fail the new bind validation request
      if(g_ctrlm_validation.configuration_in_progress) {
         ctrlm_inform_configuration_complete(g_ctrlm_validation.network_id, g_ctrlm_validation.controller_id, CTRLM_RCU_CONFIGURATION_RESULT_TIMEOUT);
      } else {
         ctrlm_inform_validation_end(g_ctrlm_validation.network_id, g_ctrlm_validation.controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_TIMEOUT, NULL, NULL);
      }
   }
   g_ctrlm_validation.timeout_source_id = 0;
   return(FALSE);
}

gboolean ctrlm_validation_timeout_ignore(gpointer user_data) {
   XLOGD_INFO("Validation ignore timed out. Abort key will be processed.");
   if(ctrlm_validation_is_in_progress()) {
      g_ctrlm_validation.ignore_abort = false;
   }
   g_ctrlm_validation.timeout_ignore_id = 0;
   return(FALSE);
};


void ctrlm_validation_timeout_update(gint timeout) {
   ctrlm_timeout_destroy(&g_ctrlm_validation.timeout_source_id);
   g_ctrlm_validation.timeout_source_id = ctrlm_timeout_create(timeout, ctrlm_validation_timeout, NULL);
}

void ctrlm_inform_validation_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long long ieee_address) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_bind_validation_begin_t *msg = (ctrlm_main_queue_msg_bind_validation_begin_t *)g_malloc(sizeof(ctrlm_main_queue_msg_bind_validation_begin_t));
   
   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }
   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_BEGIN;
   msg->header.network_id = network_id;
   msg->controller_id     = controller_id;
   msg->ieee_address      = ieee_address;
   
   ctrlm_main_queue_msg_push(msg);
}

gboolean ctrlm_inform_validation_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t binding_type, ctrlm_rcu_validation_type_t validation_type, ctrlm_rcu_validation_result_t validation_result, sem_t *semaphore, ctrlm_validation_end_cmd_result_t *cmd_result) {
   g_ctrlm_validation.golden_index         = 0;
   g_ctrlm_validation.num_wrong_keypresses = 0;
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_bind_validation_end_t *msg = (ctrlm_main_queue_msg_bind_validation_end_t *)g_malloc(sizeof(ctrlm_main_queue_msg_bind_validation_end_t));
   
   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }
   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_END;
   msg->header.network_id = network_id;
   msg->controller_id     = controller_id;
   msg->binding_type      = binding_type;
   msg->validation_type   = validation_type;
   msg->result            = validation_result;
   msg->semaphore         = semaphore;
   msg->cmd_result        = cmd_result;
   
   ctrlm_main_queue_msg_push(msg);
   return(true);
}

gboolean ctrlm_validation_key_sniff(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type, ctrlm_key_status_t key_status, ctrlm_key_code_t key_code, gboolean auto_bind_in_progress) {
   ctrlm_rcu_validation_type_t validation_type = g_ctrlm_validation.app_based_validation ? CTRLM_RCU_VALIDATION_TYPE_APPLICATION : CTRLM_RCU_VALIDATION_TYPE_INTERNAL;

   // Need to ensure the input is coming from the controller that is being validated
   if(!ctrlm_validation_is_in_progress()) {
      XLOGD_DEBUG("Validation not in progress");
      return(TRUE);
   }
   // Ignore keypresses if this is autobinding
   if(auto_bind_in_progress) {
      XLOGD_DEBUG("Autobind validation in progress.  Ignore keypresses.");
      return(TRUE);
   }
   if(!ctrlm_validation_on_this_network(network_id) || !ctrlm_validation_on_this_controller(controller_id)) { // Abort the validation due to collision
      XLOGD_INFO("Validation ABORT (collision)");
      ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_COLLISION, NULL, NULL);
      return(TRUE); // Process the key press
   }
   
   if(key_status != CTRLM_KEY_STATUS_DOWN) { // No further processing
      // Update validation response timer
      ctrlm_validation_timeout_update(g_ctrlm_validation.timeout_subsequent);
      return(FALSE);
   }

   if(CTRLM_KEY_CODE_MENU == key_code) { // xfinity key was pressed.  Abort.
      if(ctrlm_validation_ignore_abort()) {
         XLOGD_INFO("Validation IGNORE ABORT (xfinity key)");
      } else {
         XLOGD_INFO("Validation ABORT (xfinity key)");
         ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_ABORT, NULL, NULL);
      }
   } else if(CTRLM_KEY_CODE_EXIT == key_code) { // Exit key was pressed.  Full Abort.
      XLOGD_INFO("Validation FULL ABORT");
      ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_FULL_ABORT, NULL, NULL);
   } else { // Any other key was pressed
      // Update validation response timer
      ctrlm_validation_timeout_update(g_ctrlm_validation.timeout_subsequent);

      // Send the validation key as a broadcast event
      ctrlm_rcu_iarm_event_key_press_validation(network_id, controller_id, controller_type, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, key_status, key_code);
      
      if(!g_ctrlm_validation.app_based_validation) {
         if(g_ctrlm_validation.golden_index < 3) {// Check the input
            if(g_ctrlm_validation.golden_code[g_ctrlm_validation.golden_index] == key_code) {
               g_ctrlm_validation.golden_index++;
               if(g_ctrlm_validation.golden_index == 3) {
                  XLOGD_INFO("Validation SUCCESS");
                  ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_SUCCESS, NULL, NULL);
               }
            } else {
               g_ctrlm_validation.golden_index = 0;
               g_ctrlm_validation.num_wrong_keypresses++;

               if(g_ctrlm_validation.num_wrong_keypresses>=CTRLM_MAX_NUM_WRONG_KEYPRESSES) {
                  XLOGD_INFO("Validation FAILURE (too many wrong keys %d)", g_ctrlm_validation.num_wrong_keypresses);
                  ctrlm_inform_validation_end(network_id, controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, validation_type, CTRLM_RCU_VALIDATION_RESULT_FAILED, NULL, NULL);
               }
            }
         }
      }
   }
   return(FALSE); // Don't process the key press
}

gboolean ctrlm_inform_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_configuration_result_t configuration_result) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_bind_configuration_complete_t *msg = (ctrlm_main_queue_msg_bind_configuration_complete_t *)g_malloc(sizeof(ctrlm_main_queue_msg_bind_configuration_complete_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }
   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_CONFIGURATION_COMPLETE;
   msg->header.network_id = network_id;
   msg->controller_id     = controller_id;
   msg->result            = configuration_result;

   ctrlm_main_queue_msg_push(msg);
   return(true);
}

void ctrlm_validation_random_code(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_controller_type_t controller_type) {

   g_ctrlm_validation.golden_index = 0;

   XLOGD_INFO("Controller Type <%s>", ctrlm_rcu_controller_type_str(controller_type));
   switch(controller_type) {
      case CTRLM_RCU_CONTROLLER_TYPE_UNKNOWN:
      case CTRLM_RCU_CONTROLLER_TYPE_XR2:
      case CTRLM_RCU_CONTROLLER_TYPE_XR5:
      case CTRLM_RCU_CONTROLLER_TYPE_XR11:
      case CTRLM_RCU_CONTROLLER_TYPE_XR15:
      case CTRLM_RCU_CONTROLLER_TYPE_XR15V2:
      case CTRLM_RCU_CONTROLLER_TYPE_XR18:
      case CTRLM_RCU_CONTROLLER_TYPE_XRA:
      case CTRLM_RCU_CONTROLLER_TYPE_XR19: {
         for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
            g_ctrlm_validation.golden_code[index] = ctrlm_validation_random_number();
         }
         break;
      }
      case CTRLM_RCU_CONTROLLER_TYPE_XR16: {
         for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
            g_ctrlm_validation.golden_code[index] = ctrlm_validation_random_letter();
         }
         break;
      }
      case CTRLM_RCU_CONTROLLER_TYPE_INVALID: {
         for(unsigned long index = 0; index < CTRLM_RCU_VALIDATION_KEY_QTY; index++) {
            g_ctrlm_validation.golden_code[index] = CTRLM_KEY_CODE_OK;
         }
      }
   }

   // Handle excluded codes
   #if (CTRLM_RCU_VALIDATION_KEY_QTY == 3)
   guint32 code = (g_ctrlm_validation.golden_code[0] << 16) | (g_ctrlm_validation.golden_code[1] << 8) | g_ctrlm_validation.golden_code[2];
   switch(code) {
      case CTRLM_EXCLUDED_CODE_911:
      case CTRLM_EXCLUDED_CODE_666:
         g_ctrlm_validation.golden_code[0] = CTRLM_KEY_CODE_DIGIT_1;
         g_ctrlm_validation.golden_code[1] = CTRLM_KEY_CODE_DIGIT_3;
         g_ctrlm_validation.golden_code[2] = CTRLM_KEY_CODE_DIGIT_5;
         break;
   }
   #endif
}

ctrlm_key_code_t ctrlm_validation_random_number(void) {
   return(ctrlm_validation_numbers[random() % CTRLM_VALIDATION_NUMBER_QTY]);
}

ctrlm_key_code_t ctrlm_validation_random_letter(void) {
   return(ctrlm_validation_letters[random() % CTRLM_VALIDATION_LETTER_QTY]);
}

gboolean ctrlm_validation_ignore_abort(void) {
   return(g_ctrlm_validation.ignore_abort);
}

std::vector<ctrlm_key_code_t> ctrlm_validation_golden_code_get(void) {
    std::vector<ctrlm_key_code_t> golden_code;
    golden_code.insert(golden_code.end(), &g_ctrlm_validation.golden_code[0], &g_ctrlm_validation.golden_code[CTRLM_RCU_VALIDATION_KEY_QTY]);
    return (std::all_of(golden_code.begin(), golden_code.end(), [](int i) { return i == 0; })) ? std::vector<ctrlm_key_code_t>() : golden_code;
}
