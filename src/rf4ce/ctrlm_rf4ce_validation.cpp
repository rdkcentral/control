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
#include "libIBus.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "ctrlm_rf4ce_network.h"

void ctrlm_obj_network_rf4ce_t::bind_validation_begin(ctrlm_main_queue_msg_bind_validation_begin_t *dqm) {
   THREAD_ID_VALIDATE();
   if(NULL == dqm) {
      XLOGD_ERROR("dqm is null");
      return;
   }
   set_rf_pair_state(CTRLM_RF_PAIR_STATE_PAIRING);
   XLOGD_INFO("Controller Id %u 0x%016llX", dqm->controller_id, dqm->ieee_address);
   // Destroy the timeout because we switch the state at validation end
   ctrlm_timeout_destroy(&binding_in_progress_tag_);
}

// Validation finish from control manager
void ctrlm_obj_network_rf4ce_t::bind_validation_end(ctrlm_main_queue_msg_bind_validation_end_t *dqm) {
   THREAD_ID_VALIDATE();
   if(NULL == dqm) {
      XLOGD_ERROR("dqm is null");
      return;
   }

   XLOGD_INFO("Controller Id %u result <%s>", dqm->controller_id, ctrlm_rcu_validation_result_str(dqm->result));

   ctrlm_timeout_destroy(&binding_in_progress_tag_);
   g_atomic_int_set(&binding_in_progress_, FALSE);

   // Convert the control manager result into an RF4CE result with values that conform to the MSO spec and a HAL result for the HAL network
   ctrlm_rf4ce_result_validation_t result_rf4ce;

   switch(dqm->result) {
      case CTRLM_RCU_VALIDATION_RESULT_SUCCESS:         result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS;         set_rf_pair_state(CTRLM_RF_PAIR_STATE_PAIRING); break;
      case CTRLM_RCU_VALIDATION_RESULT_PENDING:         result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_PENDING;         set_rf_pair_state(CTRLM_RF_PAIR_STATE_PAIRING); break;
      case CTRLM_RCU_VALIDATION_RESULT_TIMEOUT:         result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_TIMEOUT;         set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_COLLISION:       result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_COLLISION;       set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_ABORT:           result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_ABORT;           set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_FULL_ABORT:      result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_FULL_ABORT;      set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_FAILED:          result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_FAILED;          set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_BIND_TABLE_FULL: result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_BIND_TABLE_FULL; set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_IN_PROGRESS:     result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_IN_PROGRESS;     set_rf_pair_state(CTRLM_RF_PAIR_STATE_PAIRING); break;
      case CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART:   result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_FULL_ABORT;      set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
      case CTRLM_RCU_VALIDATION_RESULT_FAILURE:
      default:                                          result_rf4ce = CTRLM_RF4CE_RESULT_VALIDATION_FAILURE;         set_rf_pair_state(CTRLM_RF_PAIR_STATE_FAILED); break;
   }

   if(!controller_exists(dqm->controller_id)) { 
      XLOGD_INFO("Controller Id %u does not exist!", dqm->controller_id);
      return;
   }

   // Set the validation status
   controllers_[dqm->controller_id]->validation_result_set(dqm->binding_type, dqm->validation_type, result_rf4ce);

   // set a timer here in case the controller never reads the result.  Otherwise controller object will remain resident until the process is restarted.
   if(dqm->result != CTRLM_RCU_VALIDATION_RESULT_SUCCESS) {
      if(!controller_is_bound(dqm->controller_id)) {
         ctrlm_bind_validation_timeout_t* timeout  = (ctrlm_bind_validation_timeout_t*)g_malloc(sizeof(ctrlm_bind_validation_timeout_t));
         timeout->controller_id = dqm->controller_id;
         timeout->network_id = network_id_get();
         // convert interval to seconds from milliseconds, rounding to next integer
         guint interval = (3*auto_check_validation_period_)/1000 + (auto_check_validation_period_%1000 ? 1 : 0);
         timeout->timer_id = g_timeout_add_seconds(interval, bind_validation_failed, timeout);
         bind_validation_failed_timeout_.push_back(timeout);
         XLOGD_DEBUG("timer id=%u (%u,%u)", timeout->timer_id, timeout->network_id, timeout->controller_id);
      } else if((dqm->result == CTRLM_RCU_VALIDATION_RESULT_TIMEOUT) || (dqm->result == CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART)) { // If the controller is bound and the validation timesout or was interrupted by a restart of ctrlm, it is duplicate pairing and needs to be restored
         XLOGD_INFO("Controller %u validation timed out or was interrupted by a restart of ctrlm and is already bound.. Restore previous pairing..", dqm->controller_id);
         controller_restore(dqm->controller_id);
      }
      // Indicate failed bind
      blackout_bind_fail();
      XLOGD_INFO("Failed pairing, MAC address: 0x%016llX", controllers_[dqm->controller_id]->ieee_address_get());
   } else {
      if(controller_id_to_remove_ != CTRLM_HAL_CONTROLLER_ID_INVALID) {
         // no space in the pairing table
         XLOGD_WARN("Out of space in the pairing table.  Will kick out Controller Id <%u>", controller_id_to_remove_);

         // Unbind the last recently used controller
         controller_unbind(controller_id_to_remove_, CTRLM_UNBIND_REASON_TARGET_NO_SPACE);
      }
      // Indicate successful pairing
      blackout_bind_success();

      // Remove persistent download session (if present)
      controllers_[dqm->controller_id]->device_update_session_resume_unload();

      controllers_[dqm->controller_id]->print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_PAIRED_REMOTE);
   }

   if(dqm->result != CTRLM_RCU_VALIDATION_RESULT_COLLISION) {
      controller_id_to_remove_ = CTRLM_HAL_CONTROLLER_ID_INVALID;
   }

   // clean up expired discovery deadlines
   for(auto it = discovery_deadlines_autobind_.begin(), end_it = discovery_deadlines_autobind_.end(); it != end_it;) {
      //Erase the deadline of the new remote
      if(it->first == controllers_[dqm->controller_id]->ieee_address_get()) {
         discovery_deadlines_autobind_.erase(it++);
      //Erase expired deadline
      } else if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
         discovery_deadlines_autobind_.erase(it++);
      } else {
         ++it;
      }
   }

   // clean up expired discovery deadlines screen bind
      for(auto it = discovery_deadlines_screen_bind_.begin(), end_it = discovery_deadlines_screen_bind_.end(); it != end_it;) {
      //Erase the deadline of the new remote
      if(it->first == controllers_[dqm->controller_id]->ieee_address_get()) {
         discovery_deadlines_screen_bind_.erase(it++);
      //Erase expired deadline
      } else if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
         discovery_deadlines_screen_bind_.erase(it++);
      } else {
         ++it;
      }
   }
}

// Validation result processed timeout callback
gboolean ctrlm_obj_network_rf4ce_t::bind_validation_failed(gpointer user_data){
    ctrlm_bind_validation_timeout_t* timeout = reinterpret_cast<ctrlm_bind_validation_timeout_t*>(user_data);

    ctrlm_main_queue_msg_bind_validation_failed_timeout_t *msg = (ctrlm_main_queue_msg_bind_validation_failed_timeout_t *)g_malloc(sizeof(ctrlm_main_queue_msg_bind_validation_failed_timeout_t));

    if(NULL == msg) {
       XLOGD_FATAL("Out of memory");
       g_assert(0);
       return(FALSE);
    }

    msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_FAILED_TIMEOUT;
    msg->header.network_id = timeout->network_id;
    msg->controller_id = timeout->controller_id;
    ctrlm_main_queue_msg_push(msg);

    // Since returning false destroys the timeout, set the tag to 0 so controlMgr doesn't also destroy it.
    timeout->timer_id = 0;
    return(FALSE);
}

bool ctrlm_obj_network_rf4ce_t::bind_validation_timeout(ctrlm_controller_id_t controller_id){
    THREAD_ID_VALIDATE();

    if (!controller_exists(controller_id)){
        return false;
    }

    auto timeout_end = bind_validation_failed_timeout_.end();
    for (auto timeout = bind_validation_failed_timeout_.begin(); timeout != timeout_end; ++timeout ){
        if ((*timeout)->controller_id == controller_id){
            ctrlm_timeout_destroy(&(*timeout)->timer_id);
            g_free((*timeout));
            bind_validation_failed_timeout_.erase(timeout);
            break;
        }
    }
    controller_unpair(controller_id);
    controller_remove(controller_id, true);
    return true;
    }

