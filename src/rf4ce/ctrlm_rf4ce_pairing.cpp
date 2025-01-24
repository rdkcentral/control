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
#include "ctrlm_validation.h"
#include "ctrlm_utils.h"
#include "ctrlm_rf4ce_network.h"

static void ctrlm_network_queue_deliver_result_pair(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_rsp_pair_params_t params);
static void ctrlm_network_queue_deliver_result_unpair(ctrlm_main_queue_msg_rf4ce_ind_unpair_t *dqm, ctrlm_hal_rf4ce_rsp_unpair_params_t params);

void ctrlm_network_queue_deliver_result_pair(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_rsp_pair_params_t params) {
   g_assert(dqm);
   if(NULL == dqm->rsp_params) { // asynchronous
      g_assert(dqm->cb);
      if(NULL != dqm->cb) {
         dqm->cb(params, dqm->cb_data);
      }
   } else { // synchronous
      // Signal the semaphore to indicate that the result is present
      *dqm->rsp_params = params;
      sem_post(dqm->semaphore);
   }
}

void ctrlm_network_queue_deliver_result_unpair(ctrlm_main_queue_msg_rf4ce_ind_unpair_t *dqm, ctrlm_hal_rf4ce_rsp_unpair_params_t params) {
   g_assert(dqm);
   if(NULL == dqm->rsp_params) { // asynchronous
      g_assert(dqm->cb);
      if(NULL != dqm->cb) {
         dqm->cb(params, dqm->cb_data);
      }
   } else { // synchronous
      // Signal the semaphore to indicate that the result is present
      *dqm->rsp_params = params;
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_rf4ce_t::ind_process_pair(void *data, int size) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm = (ctrlm_main_queue_msg_rf4ce_ind_pair_t *)data;
   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_rf4ce_ind_pair_t));

   // Check if in blackout mode and if so, ignore the request and log MAC
   if(blackout_.is_blackout_enabled && blackout_.is_blackout) {
      if(ctrlm_pairing_window_active_get()) {
         ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_CTRLM_BLACKOUT);
      }
      XLOGD_INFO("BLACKOUT: Pair Request from MAC 0x%016llX...", dqm->params.src_ieee_addr);
      return;
   }
   if(ctrlm_pairing_window_active_get()) {
      //Now we're waiting for the rest of pairing to occur.  The bind status will be set to success when binding is finished
      ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_UNKNOWN_FAILURE);
   }

   ctrlm_hal_rf4ce_result_t status = dqm->params.status;

   ctrlm_hal_rf4ce_rsp_pair_params_t params;
   params.controller_id          = controller_id_get_by_ieee(dqm->params.src_ieee_addr); //Set controller id to prevent it being overwritten on a failed pairing
   params.dst_pan_id             = dqm->params.src_pan_id;
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = 0;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.user_data              = dqm->params.user_data;
   params.result                 = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   params.rec_user_string[0] = 0;
#endif
   if(params.controller_id == 0){ // Remote didn't exist in pairing table
      params.controller_id = CTRLM_HAL_CONTROLLER_ID_INVALID;
   }

   if(status != CTRLM_HAL_RF4CE_RESULT_SUCCESS && status != CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING && status != CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY) {
      XLOGD_FATAL("Status is not success.  Ignoring. <%s>", ctrlm_hal_rf4ce_result_str(dqm->params.status));
      params.result = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
      ctrlm_network_queue_deliver_result_pair(dqm, params);
      return;
   } else if(dqm->params.org_vendor_id != CTRLM_RF4CE_VENDOR_ID_COMCAST) {
      XLOGD_ERROR("Unsupported vendor id 0x%04X", dqm->params.org_vendor_id);
      params.result = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
      ctrlm_network_queue_deliver_result_pair(dqm, params);
      return;
   } else if(FALSE == g_atomic_int_compare_and_exchange(&binding_in_progress_, FALSE, TRUE)) {
      XLOGD_ERROR("Binding of another controller in progress");
      params.result = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
      ctrlm_network_queue_deliver_result_pair(dqm, params);
      return;
   }

   params.controller_id = CTRLM_HAL_CONTROLLER_ID_INVALID; //The appropiate binding process will set the controller ID

   // Set timeout for binding in progress
   binding_in_progress_tag_ = ctrlm_timeout_create(binding_in_progress_timeout_ * 1000, binding_in_progress_timeout, (gpointer)this); // Timeout in binding_in_progress_timeout_ seconds
   XLOGD_INFO("Setting timeout for binding state");

   if(is_autobind_active(dqm->params.src_ieee_addr)) { // autobind pairing
      if(status == CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY) {
         controller_id_to_remove_ = controller_id_get_last_recently_used();
         if(CTRLM_HAL_CONTROLLER_ID_INVALID != controller_id_to_remove_) {
            ind_process_pair_autobind(dqm, CTRLM_HAL_RF4CE_RESULT_SUCCESS);
         } else {
            ind_process_pair_autobind(dqm, status);
         }
      } else {
         ind_process_pair_autobind(dqm, status);
      }
   } else if(ctrlm_is_binding_button_pressed()) { // binding button pairing
      if(status == CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY) {
         controller_id_to_remove_ = controller_id_get_last_recently_used();
         if(CTRLM_HAL_CONTROLLER_ID_INVALID != controller_id_to_remove_) {
            ind_process_pair_binding_button(dqm, CTRLM_HAL_RF4CE_RESULT_SUCCESS);
         } else {
            ind_process_pair_binding_button(dqm, status);
         }
      } else {
         ind_process_pair_binding_button(dqm, status);
      }
   } else if(is_screen_bind_active(dqm->params.src_ieee_addr)) {
      if(status == CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY) {
         controller_id_to_remove_ = controller_id_get_last_recently_used();
         if(CTRLM_HAL_CONTROLLER_ID_INVALID != controller_id_to_remove_) {
            ind_process_pair_screen_bind(dqm, CTRLM_HAL_RF4CE_RESULT_SUCCESS);
         } else {
            ind_process_pair_screen_bind(dqm, status);
         }
      } else {
         ind_process_pair_screen_bind(dqm, status);
      }
   } else { // Normal pairing
      if(status == CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY) {
         controller_id_to_remove_ = controller_id_get_last_recently_used();
         if(CTRLM_HAL_CONTROLLER_ID_INVALID != controller_id_to_remove_) {
            ind_process_pair_stb(dqm, CTRLM_HAL_RF4CE_RESULT_SUCCESS);
         } else {
            ind_process_pair_stb(dqm, status);
         }
      } else {
         ind_process_pair_stb(dqm, status);
      }
   }
}


void ctrlm_obj_network_rf4ce_t::ind_process_pair_stb(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status) {
   THREAD_ID_VALIDATE();
   ctrlm_hal_rf4ce_rsp_pair_params_t params;
   ctrlm_controller_id_t controller_id;
   errno_t safec_rc = -1;
#ifdef ASB
   bool asb = false;
   asb_key_derivation_method_t key_derivation_method = ASB_KEY_DERIVATION_NONE;
   // Check to make sure IF this is a ASB session that there is a common Key Derivation method
   if(is_asb_enabled() && is_asb_active(dqm->params.src_ieee_addr)) {
      asb = true;
   }
   XLOGD_INFO("Normal Pair Request, status <%s>, MAC Address <0x%016llx>, ASB Enabled <%s>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr, (asb ? "YES" : "NO"));
#else
   XLOGD_INFO("Normal Pair Request, status <%s>, MAC Address <0x%016llx>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr);
#endif

   controller_id = controller_id_get_by_ieee(dqm->params.src_ieee_addr);
   if(0 != controller_id) {
      controller_bind_update(dqm, controller_id, status);
   }

   if( CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING == status ) {
      XLOGD_INFO("This remote was paired before, reply with SUCCESS");
      status = CTRLM_HAL_RF4CE_RESULT_SUCCESS;
   }

   // Create the pair response parameters
   params.status                 = status;
   params.controller_id          = (controller_id != 0) ? controller_id : controller_id_assign();
   params.dst_pan_id             = dqm->params.src_pan_id;
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   safec_rc = strncpy_s((char *)params.rec_user_string, sizeof(params.rec_user_string), user_string_.c_str(), 9);
   ERR_CHK(safec_rc);
#ifdef ASB
   if(asb) {
      params.rec_user_string[10] = key_derivation_method_get(dqm->params.org_user_string[10]);
      if(ASB_KEY_DERIVATION_NONE == params.rec_user_string[10]) {
         // No common key derivation methods, increase fail count
         XLOGD_ERROR("Controller and Target do not share common key derivation method! Ignore!");
         params.result           = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
         ctrlm_network_queue_deliver_result_pair(dqm, params);
         asb_fallback_count_        += 1;
         return;
      }
      key_derivation_method = params.rec_user_string[10];
   }
#endif
#endif
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.user_data              = dqm->params.user_data;
   params.result                 = CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND;

   if(!controller_exists(params.controller_id)) {
      // Create the controller object
      controller_insert(params.controller_id, dqm->params.src_ieee_addr, true);


      char user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE] = "XR2-";
      // get user string from discovery request
      discovered_user_strings_t::const_iterator it = discovered_user_strings_.find(dqm->params.src_ieee_addr);
      if(it != discovered_user_strings_.cend()) {
         safec_rc = strncpy_s(user_string, sizeof(user_string), it->second.user_string, sizeof(user_string)-1);
         ERR_CHK(safec_rc);
         discovered_user_strings_.erase(dqm->params.src_ieee_addr);
      } else if(dqm->params.org_user_string[0] != 0) {
         // if not available, grt user string from pairing request
         safec_rc = strncpy_s(user_string, sizeof(user_string), (const char*)dqm->params.org_user_string, sizeof(user_string)-1);
         ERR_CHK(safec_rc);
         user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE - 1] = '\0';
      }
      controller_user_string_set(params.controller_id, (guchar*)user_string);
      controller_init_uinput(params.controller_id);
   }
   controller_autobind_in_progress_set(params.controller_id, false);
   controller_binding_button_in_progress_set(params.controller_id, false);
   controller_screen_bind_in_progress_set(params.controller_id, false);

   // Set security type
#ifdef ASB
   controller_set_binding_security_type(params.controller_id, (asb ? CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED : CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL));
   controller_set_key_derivation_method(params.controller_id, (asb ? key_derivation_method : ASB_KEY_DERIVATION_NONE));
#else
   controller_set_binding_security_type(params.controller_id, CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL);
#endif
   // Send the parameters for the pair response
   ctrlm_network_queue_deliver_result_pair(dqm, params);
#ifdef ASB
   if(CTRLM_HAL_RF4CE_RESULT_SUCCESS == status && asb) {
      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::rf4ce_asb_init, (void *)NULL, 0, this);
   }
#endif
}

void ctrlm_obj_network_rf4ce_t::ind_process_pair_autobind(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status) {
   THREAD_ID_VALIDATE();
   ctrlm_hal_rf4ce_rsp_pair_params_t params;
   ctrlm_controller_id_t controller_id;
#ifdef ASB
   bool asb = false;
   asb_key_derivation_method_t key_derivation_method = ASB_KEY_DERIVATION_NONE;
   // Check to make sure IF this is a ASB session that there is a common Key Derivation method
   if(is_asb_enabled() && is_asb_active(dqm->params.src_ieee_addr)) {
      asb = true;
   }
   XLOGD_INFO("Autobind Pair Request, status <%s>, MAC Address <0x%016llx>, ASB Enabled <%s>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr, (asb ? "YES" : "NO"));
#else
   XLOGD_INFO("Autobind Pair Request, status <%s>, MAC Address <0x%016llx>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr);
#endif
   controller_id = controller_id_get_by_ieee(dqm->params.src_ieee_addr);
   if(0 != controller_id) {
      controller_bind_update(dqm, controller_id, status);
   }

   if( CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING == status ) {
      XLOGD_INFO("This remote was paired before, reply with SUCCESS");
      status = CTRLM_HAL_RF4CE_RESULT_SUCCESS;
   }

   // Create the pair response parameters
   params.status                 = status;
   params.controller_id          = (controller_id != 0) ? controller_id : controller_id_assign();
   params.dst_pan_id             = dqm->params.src_pan_id;
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   errno_t safec_rc = strncpy_s((char *)params.rec_user_string, sizeof(params.rec_user_string), user_string_.c_str(), 9);
   ERR_CHK(safec_rc);
#ifdef ASB
   if(asb) {
      params.rec_user_string[10] = key_derivation_method_get(dqm->params.org_user_string[10]);
      if(ASB_KEY_DERIVATION_NONE == params.rec_user_string[10]) {
         // No common key derivation methods, increase fail count
         XLOGD_ERROR("Controller and Target do not share common key derivation method! Ignore!");
         params.controller_id    = CTRLM_HAL_CONTROLLER_ID_INVALID;
         params.result           = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
         ctrlm_network_queue_deliver_result_pair(dqm, params);
         asb_fallback_count_        += 1;
         return;
      }
      key_derivation_method = params.rec_user_string[10];
   }
#endif
#endif
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.user_data              = dqm->params.user_data;
   params.result                 = CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND;

   if(!controller_exists(params.controller_id)) {
      // Create the controller object
      controller_insert(params.controller_id, dqm->params.src_ieee_addr, true);
      controller_user_string_set(params.controller_id, dqm->params.org_user_string);
      controller_init_uinput(params.controller_id);
   }
   controller_autobind_in_progress_set(params.controller_id, true);
   controller_binding_button_in_progress_set(params.controller_id, false);
   controller_screen_bind_in_progress_set(params.controller_id, false);

   // Set security type
#ifdef ASB
   controller_set_binding_security_type(params.controller_id, (asb ? CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED : CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL));
   controller_set_key_derivation_method(params.controller_id, (asb ? key_derivation_method : ASB_KEY_DERIVATION_NONE));
#else
   controller_set_binding_security_type(params.controller_id, CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL);
#endif

   // Send the parameters for the pair response
   ctrlm_network_queue_deliver_result_pair(dqm, params);
#ifdef ASB
   if(CTRLM_HAL_RF4CE_RESULT_SUCCESS == status && asb) {
      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::rf4ce_asb_init, (void *)NULL, 0, this);
   }
#endif
}

void ctrlm_obj_network_rf4ce_t::ind_process_pair_binding_button(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status) {
   THREAD_ID_VALIDATE();
   ctrlm_hal_rf4ce_rsp_pair_params_t params;
   ctrlm_controller_id_t controller_id;
   errno_t safec_rc = -1;
#ifdef ASB
   bool asb = false;
   asb_key_derivation_method_t key_derivation_method = ASB_KEY_DERIVATION_NONE;
   // Check to make sure IF this is a ASB session that there is a common Key Derivation method
   if(is_asb_enabled() && is_asb_active(dqm->params.src_ieee_addr)) {
      asb = true;
   }
   XLOGD_INFO("Button Binding Pair Request, status <%s>, MAC Address <0x%016llx>, ASB Enabled <%s>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr, (asb ? "YES" : "NO"));
#else
   XLOGD_INFO("Button Binding Pair Request, status <%s>, MAC Address <0x%016llx>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr);
#endif
   controller_id = controller_id_get_by_ieee(dqm->params.src_ieee_addr);
   if(0 != controller_id) {
      controller_bind_update(dqm, controller_id, status);
   }

   if( CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING == status ) {
      XLOGD_INFO("This remote was paired before, reply with SUCCESS");
      status = CTRLM_HAL_RF4CE_RESULT_SUCCESS;
   }

   // Create the pair response parameters
   params.status                 = status;
   params.controller_id          = (controller_id != 0) ? controller_id : controller_id_assign();
   params.dst_pan_id             = dqm->params.src_pan_id;
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   safec_rc = strncpy_s((char *)params.rec_user_string, sizeof(params.rec_user_string), user_string_.c_str(), 9);
   ERR_CHK(safec_rc);
#ifdef ASB
   if(asb) {
      params.rec_user_string[10] = key_derivation_method_get(dqm->params.org_user_string[10]);
      if(ASB_KEY_DERIVATION_NONE == params.rec_user_string[10]) {
         // No common key derivation methods, increase fail count
         XLOGD_ERROR("Controller and Target do not share common key derivation method! Ignore!");
         params.controller_id    = CTRLM_HAL_CONTROLLER_ID_INVALID;
         params.result           = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
         ctrlm_network_queue_deliver_result_pair(dqm, params);
         asb_fallback_count_        += 1;
         return;
      }
      key_derivation_method = params.rec_user_string[10];
   }
#endif
#endif
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.user_data              = dqm->params.user_data;
   params.result                 = CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND;

   if(!controller_exists(params.controller_id)) {
      // Create the controller object
      controller_insert(params.controller_id, dqm->params.src_ieee_addr, true);

      char user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE] = "XR2-";
      // get user string from discovery request
      discovered_user_strings_t::const_iterator it = discovered_user_strings_.find(dqm->params.src_ieee_addr);
      if(it != discovered_user_strings_.cend()) {
         safec_rc = strncpy_s(user_string, sizeof(user_string), it->second.user_string, sizeof(user_string)-1);
         ERR_CHK(safec_rc);
         discovered_user_strings_.erase(dqm->params.src_ieee_addr);
      } else if(dqm->params.org_user_string[0] != 0) {
         // if not available, grt user string from pairing request
         safec_rc = strncpy_s(user_string, sizeof(user_string), (const char*)dqm->params.org_user_string, sizeof(user_string)-1);
         ERR_CHK(safec_rc);
         user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE - 1] = '\0';
      }
      controller_user_string_set(params.controller_id, (guchar*)user_string);
      controller_init_uinput(params.controller_id);
   }
   controller_autobind_in_progress_set(params.controller_id, false);
   controller_binding_button_in_progress_set(params.controller_id, true);
   controller_screen_bind_in_progress_set(params.controller_id, false);

   // Set security type
#ifdef ASB
   controller_set_binding_security_type(params.controller_id, (asb ? CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED : CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL));
   controller_set_key_derivation_method(params.controller_id, (asb ? key_derivation_method : ASB_KEY_DERIVATION_NONE));
#else
   controller_set_binding_security_type(params.controller_id, CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL);
#endif

   // Send the parameters for the pair response
   ctrlm_network_queue_deliver_result_pair(dqm, params);
#ifdef ASB
   if(CTRLM_HAL_RF4CE_RESULT_SUCCESS == status && asb) {
      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::rf4ce_asb_init, (void *)NULL, 0, this);
   }
#endif
}

void ctrlm_obj_network_rf4ce_t::ind_process_pair_screen_bind(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_hal_rf4ce_result_t status) {
   THREAD_ID_VALIDATE();
   ctrlm_hal_rf4ce_rsp_pair_params_t params;
   ctrlm_controller_id_t controller_id;
#ifdef ASB
   bool asb = false;
   asb_key_derivation_method_t key_derivation_method = ASB_KEY_DERIVATION_NONE;
   // Check to make sure IF this is a ASB session that there is a common Key Derivation method
   if(is_asb_enabled() && is_asb_active(dqm->params.src_ieee_addr)) {
      asb = true;
   }
   XLOGD_INFO("Screen Bind Pair Request, status <%s>, MAC Address <0x%016llx>, ASB Enabled <%s>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr, (asb ? "YES" : "NO"));
#else
   XLOGD_INFO("Screen Bind Pair Request, status <%s>, MAC Address <0x%016llx>", ctrlm_hal_rf4ce_result_str(status), dqm->params.src_ieee_addr);
#endif
   controller_id = controller_id_get_by_ieee(dqm->params.src_ieee_addr);
   if(0 != controller_id) {
      controller_bind_update(dqm, controller_id, status);
   }

   if( CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING == status ) {
      XLOGD_INFO("This remote was paired before, reply with SUCCESS");
      status = CTRLM_HAL_RF4CE_RESULT_SUCCESS;
   }

   // Create the pair response parameters
   params.status                 = status;
   params.controller_id          = (controller_id != 0) ? controller_id : controller_id_assign();
   params.dst_pan_id             = dqm->params.src_pan_id;
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   errno_t safec_rc = strncpy_s((char *)params.rec_user_string, sizeof(params.rec_user_string), user_string_.c_str(), 9);
   ERR_CHK(safec_rc);
#ifdef ASB
   if(asb) {
      params.rec_user_string[10] = key_derivation_method_get(dqm->params.org_user_string[10]);
      if(ASB_KEY_DERIVATION_NONE == params.rec_user_string[10]) {
         // No common key derivation methods, increase fail count
         XLOGD_ERROR("Controller and Target do not share common key derivation method! Ignore!");
         params.controller_id    = CTRLM_HAL_CONTROLLER_ID_INVALID;
         params.result           = CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE;
         ctrlm_network_queue_deliver_result_pair(dqm, params);
         asb_fallback_count_        += 1;
         return;
      }
      key_derivation_method = params.rec_user_string[10];
   }
#endif
#endif
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.user_data              = dqm->params.user_data;
   params.result                 = CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND;

   if(!controller_exists(params.controller_id)) {
      // Create the controller object
      controller_insert(params.controller_id, dqm->params.src_ieee_addr, true);
      controller_user_string_set(params.controller_id, dqm->params.org_user_string);
      controller_init_uinput(params.controller_id);
   }
   controller_autobind_in_progress_set(params.controller_id, false);
   controller_binding_button_in_progress_set(params.controller_id, false);
   controller_screen_bind_in_progress_set(params.controller_id, true);

   // Set security type
#ifdef ASB
   controller_set_binding_security_type(params.controller_id, (asb ? CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED : CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL));
   controller_set_key_derivation_method(params.controller_id, (asb ? key_derivation_method : ASB_KEY_DERIVATION_NONE));
#else
   controller_set_binding_security_type(params.controller_id, CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL);
#endif

   // Send the parameters for the pair response
   ctrlm_network_queue_deliver_result_pair(dqm, params);
#ifdef ASB
   if(CTRLM_HAL_RF4CE_RESULT_SUCCESS == status && asb) {
      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_rf4ce_t::rf4ce_asb_init, (void *)NULL, 0, this);
   }
#endif
}

void ctrlm_obj_network_rf4ce_t::process_pair_result(ctrlm_controller_id_t controller_id, unsigned long long ieee_address, ctrlm_hal_result_pair_t result) {
   bool screen_bind = controller_screen_bind_in_progress_get(controller_id);
   if(controller_autobind_in_progress_get(controller_id) || screen_bind) { // autobinding or screen bind
      if(result != CTRLM_HAL_RESULT_PAIR_SUCCESS) {
         XLOGD_FATAL("Status is not success.  Ignoring. (0x%02X)", result);
         if(controller_exists(controller_id)) {
            if(controller_is_bound(controller_id)) {
               controller_restore(controller_id);
            } else {
               controller_remove(controller_id, true);
            }
         }
      } else { // Auto validate
         controller_stats_update(controller_id);
         ctrlm_main_queue_msg_bind_validation_begin_t begin;
         begin.controller_id = controller_id;
         begin.ieee_address = ieee_address;
         bind_validation_begin(&begin);
         ctrlm_main_queue_msg_bind_validation_end_t end;
         end.controller_id = controller_id;
         end.binding_type = (screen_bind ? CTRLM_RCU_BINDING_TYPE_SCREEN_BIND : CTRLM_RCU_BINDING_TYPE_AUTOMATIC);
         end.validation_type = (screen_bind ? CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND : CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC);
         end.result = CTRLM_RCU_VALIDATION_RESULT_SUCCESS;
         bind_validation_end(&end);
         ctrlm_inform_validation_end(network_id_get(), controller_id, (screen_bind ? CTRLM_RCU_BINDING_TYPE_SCREEN_BIND : CTRLM_RCU_BINDING_TYPE_AUTOMATIC), (screen_bind ? CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND : CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC), CTRLM_RCU_VALIDATION_RESULT_SUCCESS, NULL, NULL);
         
         if(ctrlm_pairing_window_active_get()) {
            ctrlm_close_pairing_window(CTRLM_CLOSE_PAIRING_WINDOW_REASON_PAIRING_SUCCESS);
         } else {
            if(screen_bind) {
               ctrlm_stop_binding_screen();
            }
         }
      }
   } else if(controller_binding_button_in_progress_get(controller_id)) { // binding button
      if(result != CTRLM_HAL_RESULT_PAIR_SUCCESS) {
         XLOGD_FATAL("Status is not success.  Ignoring. (0x%02X)", result);
         if(controller_exists(controller_id)) {
            if(controller_is_bound(controller_id)) {
               controller_restore(controller_id);
            } else {
               controller_remove(controller_id, true);
            }
         }
      } else { // Auto validate
         if(ctrlm_pairing_window_active_get()) {
            ctrlm_close_pairing_window(CTRLM_CLOSE_PAIRING_WINDOW_REASON_PAIRING_SUCCESS);
         } else {
            ctrlm_stop_binding_button();
         }
         controller_stats_update(controller_id);
         ctrlm_main_queue_msg_bind_validation_begin_t begin;
         begin.controller_id = controller_id;
         begin.ieee_address = ieee_address;
         bind_validation_begin(&begin);
         ctrlm_main_queue_msg_bind_validation_end_t end;
         end.controller_id = controller_id;
         end.binding_type = CTRLM_RCU_BINDING_TYPE_BUTTON;
         end.validation_type = CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC;
         end.result = CTRLM_RCU_VALIDATION_RESULT_SUCCESS;
         bind_validation_end(&end);
         ctrlm_inform_validation_end(network_id_get(), controller_id,  CTRLM_RCU_BINDING_TYPE_BUTTON, CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC, CTRLM_RCU_VALIDATION_RESULT_SUCCESS, NULL, NULL);
      }
   } else { // normal pairing
      if(result != CTRLM_HAL_RESULT_PAIR_SUCCESS) {
         XLOGD_FATAL("Status is not success.  Ignoring. (0x%02X)", result);
         if(controller_exists(controller_id)) { 
            if(controller_is_bound(controller_id)) {
               controller_restore(controller_id);
            } else {
               controller_remove(controller_id, true);
            }
         }
      } else { // Kick off validation
         if(controller_exists(controller_id)) {
            controller_stats_update(controller_id);
            ctrlm_inform_validation_begin(network_id_get(), controller_id, ieee_address);
         } else {
            XLOGD_ERROR("No kicking off validation because controller id is invalid");
         }
      }
   }

   // Even if we return success on pairing indication, it's possible the HAL could have an error causing
   // the overall pairing to fail. In this case, we need to make sure we reset our state.
   if(result != CTRLM_HAL_RESULT_PAIR_SUCCESS) {
      XLOGD_INFO("Pairing is not successful, reset Binding State");
      if(ctrlm_pairing_window_active_get()) {
         ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_HAL_FAILURE);
         ctrlm_close_pairing_window(CTRLM_CLOSE_PAIRING_WINDOW_REASON_END);
      }
      ctrlm_timeout_destroy(&binding_in_progress_tag_);
      g_atomic_int_set(&binding_in_progress_, FALSE);
   }
}

void ctrlm_obj_network_rf4ce_t::ind_process_pair_result(void *data, int size) { //ctrlm_main_queue_msg_rf4ce_ind_pair_result_t *dqm) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_rf4ce_ind_pair_result_t *dqm = (ctrlm_main_queue_msg_rf4ce_ind_pair_result_t *)data;
   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_rf4ce_ind_pair_result_t));

#ifdef ASB
   if(CTRLM_HAL_RESULT_PAIR_SUCCESS == dqm->result && controller_exists(dqm->controller_id)) {
      if(CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED == controllers_[dqm->controller_id]->binding_security_type_get()) {
         controller_stats_update(dqm->controller_id);
         controllers_[dqm->controller_id]->asb_key_derivation_start(network_id_get());
         return;
      }
   }
#endif
   process_pair_result(dqm->controller_id, dqm->ieee_address, dqm->result);
}

void ctrlm_obj_network_rf4ce_t::ind_process_unpair(void *data, int size) { //ctrlm_main_queue_msg_rf4ce_ind_unpair_t *dqm) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_rf4ce_ind_unpair_t *dqm = (ctrlm_main_queue_msg_rf4ce_ind_unpair_t *)data;
   ctrlm_hal_rf4ce_rsp_unpair_params_t params;
   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_rf4ce_ind_unpair_t));
   ctrlm_controller_id_t          controller_id = dqm->params.controller_id;
   ctrlm_hal_rf4ce_ieee_address_t ieee_address  = dqm->params.ieee_address;

   XLOGD_INFO("Controller Id %u", controller_id);

   params.controller_id = controller_id;

   if(!controller_exists(controller_id)) {
      XLOGD_ERROR("Controller doesn't exist!");
      params.result = CTRLM_HAL_RESULT_UNPAIR_FAILURE;
   } else {
      ctrlm_ieee_db_addr_t controller_ieee = controllers_[controller_id]->ieee_address_get();

      // Check to make sure the ieee address matches
      if(controller_ieee != ieee_address) {
         XLOGD_WARN("IEEE Address Mismatch! 0x%016llXX 0x%016llXX", ieee_address, controller_ieee);
      }

      ctrlm_rf4ce_result_validation_t validation_result = controllers_[controller_id]->validation_result_get();
      if(validation_result == CTRLM_RF4CE_RESULT_VALIDATION_PENDING || validation_result == CTRLM_RF4CE_RESULT_VALIDATION_IN_PROGRESS) { // Abort the validation attempt
         XLOGD_INFO("Abort validation in progress");
         ctrlm_inform_validation_end(network_id_get(), controller_id, CTRLM_RCU_BINDING_TYPE_INTERACTIVE, CTRLM_RCU_VALIDATION_TYPE_APPLICATION, CTRLM_RCU_VALIDATION_RESULT_FAILURE, NULL, NULL);
      } else if(controllers_[controller_id]->validation_result_get() == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS && controllers_[controller_id]->configuration_result_get() == CTRLM_RCU_CONFIGURATION_RESULT_PENDING) {
         XLOGD_INFO("Fail configuration in progress");
         // Inform control manager that the configuration has failed
         ctrlm_inform_configuration_complete(network_id_get(), controller_id, CTRLM_RCU_CONFIGURATION_RESULT_FAILURE);
         set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED);
         iarm_event_rcu_status();
      }

      // Unbind the controller
      controller_unbind(controller_id, CTRLM_UNBIND_REASON_CONTROLLER);

      params.result = CTRLM_HAL_RESULT_UNPAIR_SUCCESS;
   }

   ctrlm_network_queue_deliver_result_unpair(dqm, params);
}

void ctrlm_obj_network_rf4ce_t::controller_bind_update(ctrlm_main_queue_msg_rf4ce_ind_pair_t *dqm, ctrlm_controller_id_t controller_id, ctrlm_hal_rf4ce_result_t &status) {
   THREAD_ID_VALIDATE();

   ctrlm_rf4ce_result_validation_t validation_result;
   XLOGD_INFO("revalidating controller id %u", controller_id);

   if(status != CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING) {
      XLOGD_WARN("controller was not paired!");
   }
   // Store the pairing table entry in case it needs to be restored
   if(dqm->params.pairing_data) {
      controller_backup(controller_id, dqm->params.pairing_data);
   }

   validation_result = controllers_[controller_id]->validation_result_get();
   // if validated, set to unvalidated
   if(validation_result != CTRLM_RF4CE_RESULT_VALIDATION_IN_PROGRESS) { // Set controller to in progress
      controllers_[controller_id]->validation_result_set(CTRLM_RCU_BINDING_TYPE_INTERACTIVE, CTRLM_RCU_VALIDATION_TYPE_INVALID, CTRLM_RF4CE_RESULT_VALIDATION_PENDING);
      status = CTRLM_HAL_RF4CE_RESULT_SUCCESS;
   } else { // Ignore because we've already responded
      status = CTRLM_HAL_RF4CE_RESULT_NOT_PERMITTED;
      XLOGD_INFO("ignore multiple pair request");
   }
}

gboolean ctrlm_obj_network_rf4ce_t::binding_in_progress_timeout(gpointer user_data) {
   ctrlm_obj_network_rf4ce_t *rf4ce_net = (ctrlm_obj_network_rf4ce_t *)user_data;
   gint *binding_in_progress = rf4ce_net->controller_binding_in_progress_get_pointer();

   XLOGD_INFO("Binding State Timeout");
   if(NULL == rf4ce_net) {
      XLOGD_WARN("User data NULL");
      return(FALSE);
   }

   if(FALSE == g_atomic_int_compare_and_exchange(binding_in_progress, TRUE, FALSE)) {
      XLOGD_ERROR("Binding state is messed up, timeout occurred but binding is not in progress :(");
   }

   rf4ce_net->controller_binding_in_progress_tag_reset();

   return(FALSE);
}
