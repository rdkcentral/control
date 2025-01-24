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
#include "ctrlm_rf4ce_network.h"

// MSO Definitions for User String in Application Info field of discovery command response (RF4CE only)
#define CLASS_DESC_DCN_USE_NODE_DESC_AS_IS    (0x00)
#define CLASS_DESC_DCN_REMOVE_NODE_DESC       (0x10)
#define CLASS_DESC_DCN_RECLASSIFY_NODE_DESC   (0x20)
#define CLASS_DESC_DCN_ABORT_BINDING          (0x30)
#define CLASS_DESC_CLASS_NUMBER_MASK          (0x0F)
#define CLASS_DESC_APPLY_STRICT_LQI_THRESHOLD (0x40)
#define CLASS_DESC_ENABLE_REQUEST_AUTO_VAL    (0x80)
#define FULL_ROLLBACK_ENABLED                 (0x01)
#define STRICT_LQI_THRESHOLD_MASK             (0xFE)
#define LQI_THRESHOLD_BASIC                   (0xFF)
#define LQI_THRESHOLD_NONE                    (0x00)

static void ctrlm_network_queue_deliver_result(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t params);

void ctrlm_network_queue_deliver_result(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t params) {
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

void ctrlm_obj_network_rf4ce_t::ind_process_discovery(void *data, int size) {
   THREAD_ID_VALIDATE();
   char *user_string;
   ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm = (ctrlm_main_queue_msg_rf4ce_ind_discovery_t *)data;
   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_rf4ce_ind_discovery_t));

   ctrlm_hal_rf4ce_rsp_disc_params_t params;
   params.result = CTRLM_HAL_RESULT_DISCOVERY_IGNORE;
   errno_t safec_rc = -1;

   if(ctrlm_pairing_window_active_get()) {
      gboolean is_voice_remote    = ctrlm_rf4ce_is_voice_remote((char *)dqm->params.org_user_string);
      gboolean is_voice_assistant = ctrlm_rf4ce_is_voice_assistant((char *)dqm->params.org_user_string);
      ctrlm_pairing_restrict_by_remote_t restrict_pairing_by_remote = restrict_pairing_by_remote_get();
      if((!is_voice_remote && (restrict_pairing_by_remote==CTRLM_PAIRING_RESTRICT_TO_VOICE_REMOTES)) ||
         (!is_voice_assistant && (restrict_pairing_by_remote==CTRLM_PAIRING_RESTRICT_TO_VOICE_ASSISTANTS))) {
         XLOGD_WARN("Restricting <%s> for <%s>, ignoring discovery request ", ctrlm_rf4ce_pairing_restrict_by_remote_str(restrict_pairing_by_remote), (char *)dqm->params.org_user_string);
         return;
      }
   }

   if(TRUE == g_atomic_int_get(&binding_in_progress_)) {
      ctrlm_network_queue_deliver_result(dqm, params);
      XLOGD_ERROR("Binding of another controller in progress");
      return;
   }

   // Check if in blackout mode and if so, ignore the request and log MAC
   if(blackout_.is_blackout_enabled && blackout_.is_blackout) {
      ctrlm_network_queue_deliver_result(dqm, params);
      XLOGD_INFO("BLACKOUT: Discovery Request from MAC 0x%016llX... Discovery Type <%s>, MSO User String <%s>...", dqm->params.src_ieee_addr, ctrlm_rf4ce_discovery_type_str(dqm->params.search_dev_type), (char *)dqm->params.org_user_string);
      return;
   }

   // Ensure that we support the vendor id
   if(dqm->params.org_vendor_id != CTRLM_RF4CE_VENDOR_ID_COMCAST) {
      ctrlm_network_queue_deliver_result(dqm, params);
      XLOGD_ERROR("Unsupported vendor id 0x%04X", dqm->params.org_vendor_id);
      return;
   }

   if(CTRLM_RF4CE_DEVICE_TYPE_STB         != dqm->params.search_dev_type &&
      CTRLM_RF4CE_DEVICE_TYPE_ANY         != dqm->params.search_dev_type &&
      CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND    != dqm->params.search_dev_type &&
      CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND != dqm->params.search_dev_type) {
      // doesn't match the target device types that are supported
      ctrlm_network_queue_deliver_result(dqm, params);
      XLOGD_ERROR("Unsupported device type 0x%02X.", dqm->params.search_dev_type);
      return;
   }

   // Generic Discovery code
   // Create the discovery response parameters
   params.dst_ieee_addr          = dqm->params.src_ieee_addr;
   params.rec_app_capabilities   = 0x15;
   params.rec_dev_type_list[0]   = CTRLM_RF4CE_DEVICE_TYPE_STB;
   params.rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND;
   params.rec_dev_type_list[2]   = 0;
   params.rec_profile_id_list[0] = CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU;
   params.disc_req_lqi           = dqm->params.rx_link_quality;

   // Octets 0-8 MSO User String
   safec_rc = strncpy_s((char *)params.rec_user_string, sizeof(params.rec_user_string), user_string_.c_str(), 9);
   ERR_CHK(safec_rc);
   params.rec_user_string[9]  = '\0'; // Null
#ifdef ASB
   if(is_asb_enabled() && (dqm->params.org_user_string[10] & CTRLM_RF4CE_DISCOVERY_ASB_OCTET_ENABLED)) {
      if(asb_fallback_count_ >= asb_fallback_count_threshold_) {
         XLOGD_TELEMETRY("ASB Fail Count >= to threshold, falling back to normal pairing");
         params.rec_user_string[10] = 0x00; // ASB Octet
      } else {
         params.rec_user_string[10] = CTRLM_RF4CE_DISCOVERY_ASB_OCTET_ENABLED; // ASB Octet
         // store timestamp for ieee address for discovery deadline
         ctrlm_rf4ce_discovery_deadline_t discovery_deadline;
         ctrlm_timestamp_get(&discovery_deadline.timestamp);
         // replace older timestamp for the same ieee address
         asb_discovery_deadlines_.erase(dqm->params.src_ieee_addr);
         asb_discovery_deadlines_.insert(std::make_pair(dqm->params.src_ieee_addr, discovery_deadline));
         // clean up expired discovery deadlines
         for(auto it = asb_discovery_deadlines_.begin(), end_it = asb_discovery_deadlines_.end(); it != end_it;) {
            if(CTRLM_RF4CE_DISCOVERY_ASB_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
               asb_discovery_deadlines_.erase(it++);
            } else {
               ++it;
            }
         }
      }
   } else {
      params.rec_user_string[10] = 0x00;    // ASB Octet
   }
#else
   params.rec_user_string[10] = 0x00;    // ASB Octet
#endif
   // Primary / Secondary descriptors are handled in Discovery specific functions
   params.rec_user_string[13] = FULL_ROLLBACK_ENABLED; // Strict LQI Threshold and Full Rollback enabled
   params.rec_user_string[14] = LQI_THRESHOLD_NONE;    // LQI Threshold
   params.user_data           = dqm->params.user_data;

   
   // Discovery Type Specific Functions
   if(CTRLM_RF4CE_DEVICE_TYPE_AUTOBIND == dqm->params.search_dev_type) { // autobind discovery device
      process_discovery_autobind(dqm, &params);
   } else if(CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND == dqm->params.search_dev_type) {
      process_discovery_screen_bind(dqm, &params);
   } else {
      process_discovery_stb(dqm, &params);
   }

   ctrlm_network_queue_deliver_result(dqm, params);

   user_string = (char *)dqm->params.org_user_string;
#ifdef ASB
   XLOGD_INFO("Discovery Type <%s>, Respond? <%s>, MSO User String <%s>, Full Roll Back Enabled <%s>, Binding Initiation Indicator <%s>, MAC Address <0x%016llx>, ASB Enabled <%s>", ctrlm_rf4ce_discovery_type_str(dqm->params.search_dev_type), (CTRLM_HAL_RESULT_DISCOVERY_RESPOND == params.result ? "YES" : "NO"), user_string, (user_string[13] & RF4CE_BINDING_CONFIG_FULL_ROLLBACK_ENABLED) ? "YES" : "NO", ctrlm_rf4ce_binding_initiation_indicator_str((ctrlm_rf4ce_binding_initiation_indicator_t)(user_string[14])), dqm->params.src_ieee_addr, (user_string[10] & CTRLM_RF4CE_DISCOVERY_ASB_OCTET_ENABLED ? "YES" : "NO"));
#else
   XLOGD_INFO("Discovery Type <%s>, Respond? <%s>, MSO User String <%s>, Full Roll Back Enabled <%s>, Binding Initiation Indicator <%s>, MAC Address <0x%016llx>", ctrlm_rf4ce_discovery_type_str(dqm->params.search_dev_type), (CTRLM_HAL_RESULT_DISCOVERY_RESPOND == params.result ? "YES" : "NO"), user_string, (user_string[13] & RF4CE_BINDING_CONFIG_FULL_ROLLBACK_ENABLED) ? "YES" : "NO", ctrlm_rf4ce_binding_initiation_indicator_str((ctrlm_rf4ce_binding_initiation_indicator_t)(user_string[14])), dqm->params.src_ieee_addr);
#endif
   ctrlm_discovery_remote_type_set((const char *)user_string);
}

void ctrlm_obj_network_rf4ce_t::process_discovery_stb(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params) {
   unsigned char class_number = 0;
   // TODO Check vendor and user string?
   //dqm->vendor_str
   //dqm->user_str
   gboolean discovery_enabled, require_line_of_sight, is_line_of_sight;
   errno_t safec_rc = -1;

   if(NULL == params) {
       XLOGD_ERROR("Params is null");
       return;
   }

   if((ctrlm_is_binding_screen_active()) || (ctrlm_is_one_touch_autobind_active())) {
      discovery_enabled     = false;
   } else if(ctrlm_is_binding_screen_active()) {
      discovery_enabled     = discovery_config_menu_.enabled;
      require_line_of_sight = discovery_config_menu_.require_line_of_sight;

      class_number         += class_inc_bind_menu_active_;
   } else {
      discovery_enabled     = discovery_config_normal_.enabled;
      require_line_of_sight = discovery_config_normal_.require_line_of_sight;
   }

   if(!discovery_enabled) {
      params->result = CTRLM_HAL_RESULT_DISCOVERY_IGNORE;
      XLOGD_INFO("Normal discovery - DISABLED.  ignore.");
      return;
   }
   is_line_of_sight = ctrlm_is_line_of_sight();

   if(require_line_of_sight && !is_line_of_sight) {
      params->result = CTRLM_HAL_RESULT_DISCOVERY_IGNORE;
      XLOGD_INFO("Normal discovery - NOT line of sight.  ignore.");
      return;
   } else if(is_line_of_sight) {
      class_number += class_inc_line_of_sight_;
   }

   if(ctrlm_is_recently_booted())        { class_number += class_inc_recently_booted_;  }
   if(ctrlm_is_binding_button_pressed()) { class_number += class_inc_binding_button_;   }
   if(controllers_.size() == 0)          { class_number += class_inc_bind_table_empty_; }
   if(RF4CE_CONTROLLER_TYPE_UNKNOWN != controller_type_from_user_string(dqm->params.org_user_string)) {
      class_number += class_inc_xr_;
      // store user string and timestamp for ieee address
      ctrlm_rf4ce_user_string_t user_string_entry;
      safec_rc = strcpy_s(user_string_entry.user_string, sizeof(user_string_entry.user_string), (const char*)dqm->params.org_user_string);
      ERR_CHK(safec_rc);
      ctrlm_timestamp_get(&user_string_entry.timestamp);
      // replace user string with older timestamp for the same ieee address
      discovered_user_strings_.erase(dqm->params.src_ieee_addr);
      discovered_user_strings_.insert(std::make_pair(dqm->params.src_ieee_addr, user_string_entry));
      // clean up expired user strings
      for(auto it = discovered_user_strings_.begin(), end_it = discovered_user_strings_.end(); it != end_it;) {
         if(CTRLM_RF4CE_DISCOVERY_USER_STRING_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
            discovered_user_strings_.erase(it++);
         } else {
            ++it;
         }
      }
   }

   // Invert and adjust class number to the range 0-15 where 0 is highest priority
   class_number = (class_number > 15) ? 0 : (15 - class_number);

   #if 0
   #warning Class Number is hardcoded for debugging purposes
   class_number = 1;
   #endif

   // Create the discovery response parameters
   params->rec_user_string[11] = 0x3F;    // Secondary Class Descriptor
   params->rec_user_string[12] = CLASS_DESC_DCN_USE_NODE_DESC_AS_IS | class_number; // Primary Class Descriptor

   // Set the parameter result for the discovery response
   params->result = CTRLM_HAL_RESULT_DISCOVERY_RESPOND;

   // Only print after delivering the response
   XLOGD_INFO("Normal discovery - %s line of sight.  respond. Class Number (%u)", is_line_of_sight ? "IN" : "NOT", class_number);
}

void ctrlm_obj_network_rf4ce_t::process_discovery_autobind(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params) {
   gboolean autobind_enabled;

   if(NULL == params) {
       XLOGD_ERROR("Params is null");
       return;
   }

   if((ctrlm_is_binding_screen_active()) || (ctrlm_is_binding_button_pressed())) {
      autobind_enabled = false;
   } else {
      autobind_enabled = true;
   }
   if(!autobind_enabled) {
      params->result = CTRLM_HAL_RESULT_DISCOVERY_IGNORE;
      XLOGD_INFO("Autobind discovery - DISABLED.  ignore.");
   } else if(TRUE != ctrlm_is_autobind_active()) {
      autobind_status_.disc_ignore++;
      params->result = CTRLM_HAL_RESULT_DISCOVERY_IGNORE;
      if(ctrlm_pairing_window_active_get()) {
         //Now we're waiting for the pairing request
         ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_NO_PAIRING_REQUEST);
      }
      XLOGD_INFO("Autobind discovery - NOT line of sight.  ignore.");
   } else {
      if(ctrlm_is_binding_screen_active()) { // Send a different octet when in the remote control menu
         params->rec_user_string[9]  = autobind_config_menu_.octet; // Autobind octet
      } else {
         params->rec_user_string[9]  = autobind_config_normal_.octet; // Autobind octet
      }    
      params->rec_user_string[11] = 0; // Secondary Class Descriptor
      params->rec_user_string[12] = 0; // Primary Class Descriptor

      // Send the parameters for the discovery response
      params->result = CTRLM_HAL_RESULT_DISCOVERY_RESPOND;
      autobind_status_.disc_respond++;
      if(ctrlm_pairing_window_active_get()) {
         //Now we're waiting for the pairing request
         ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_NO_PAIRING_REQUEST);
      }
      XLOGD_INFO("Autobind discovery - IN line of sight.  respond.");

      // store timestamp for ieee address for discovery deadline
      ctrlm_rf4ce_discovery_deadline_t discovery_deadline;
      ctrlm_timestamp_get(&discovery_deadline.timestamp);
      // replace older timestamp for the same ieee address
      discovery_deadlines_autobind_.erase(dqm->params.src_ieee_addr);
      discovery_deadlines_autobind_.insert(std::make_pair(dqm->params.src_ieee_addr, discovery_deadline));
      // clean up expired discovery deadlines
      for(auto it = discovery_deadlines_autobind_.begin(), end_it = discovery_deadlines_autobind_.end(); it != end_it;) {
         if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
            discovery_deadlines_autobind_.erase(it++);
         } else {
            ++it;
         }
      }
   }
}

void ctrlm_obj_network_rf4ce_t::process_discovery_screen_bind(ctrlm_main_queue_msg_rf4ce_ind_discovery_t *dqm, ctrlm_hal_rf4ce_rsp_disc_params_t *params) {
   bool is_screen_bind_discovery_already_in_progress = false;

   if(NULL == params) {
       XLOGD_ERROR("Params is null");
       return;
   }

   if(!ctrlm_is_binding_screen_active()) {
      XLOGD_TELEMETRY("Binding Screen discovery - DISABLED.  ignore.");
   } else {
      for(auto it = discovery_deadlines_screen_bind_.begin(), end_it = discovery_deadlines_screen_bind_.end(); it != end_it;) {
         if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
            discovery_deadlines_screen_bind_.erase(it++);
         } else {
            // We need to make this check as if two remotes simultaneously send discovery requests, binding_in_progress_ may not be set yet
            if(it->first != dqm->params.src_ieee_addr) {
               is_screen_bind_discovery_already_in_progress = true;
            }
            ++it;
         }
      }
      if(is_screen_bind_discovery_already_in_progress == FALSE) {
         params->rec_dev_type_list[1]   = CTRLM_RF4CE_DEVICE_TYPE_SCREEN_BIND;
         params->rec_user_string[11] = 0; // Secondary Class Descriptor
         params->rec_user_string[12] = 0; // Primary Class Descriptor

         params->result = CTRLM_HAL_RESULT_DISCOVERY_RESPOND;
         if(ctrlm_pairing_window_active_get()) {
            //Now we're waiting for the pairing request
            ctrlm_pairing_window_bind_status_set(CTRLM_BIND_STATUS_NO_PAIRING_REQUEST);
         }
         XLOGD_INFO("Binding Screen discovery - respond.");

         // store timestamp for ieee address for discovery deadline
         ctrlm_rf4ce_discovery_deadline_t discovery_deadline;
         ctrlm_timestamp_get(&discovery_deadline.timestamp);
         // replace older timestamp for the same ieee address
         discovery_deadlines_screen_bind_.erase(dqm->params.src_ieee_addr);
         discovery_deadlines_screen_bind_.insert(std::make_pair(dqm->params.src_ieee_addr, discovery_deadline));
      } else {
         XLOGD_INFO("Binding Screen discovery - Already in progress for another controller.  ignore.");
      }
   }
}


gboolean ctrlm_obj_network_rf4ce_t::is_autobind_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address) {
   for(auto it = discovery_deadlines_autobind_.begin(), end_it = discovery_deadlines_autobind_.end(); it != end_it;) {
      if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
         discovery_deadlines_autobind_.erase(it++);
      } else {
         if(it->first == ieee_address) {
            return true;
         }
         ++it;
      }
   }
   return false;
}

gboolean ctrlm_obj_network_rf4ce_t::is_screen_bind_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address) {
   for(auto it = discovery_deadlines_screen_bind_.begin(), end_it = discovery_deadlines_screen_bind_.end(); it != end_it;) {
      if(CTRLM_RF4CE_DISCOVERY_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
         discovery_deadlines_screen_bind_.erase(it++);
      } else {
         if(it->first == ieee_address) {
            return true;
         }
         ++it;
      }
   }
   return false;
}

#ifdef ASB
gboolean ctrlm_obj_network_rf4ce_t::is_asb_active(ctrlm_hal_rf4ce_ieee_address_t ieee_address) {
   for(auto it = asb_discovery_deadlines_.begin(), end_it = asb_discovery_deadlines_.end(); it != end_it;) {
      if(CTRLM_RF4CE_DISCOVERY_ASB_EXPIRATION_TIME_MS < ctrlm_timestamp_since_ms(it->second.timestamp)) {
         asb_discovery_deadlines_.erase(it++);
      } else {
         if(it->first == ieee_address) {
            return true;
         }
         ++it;
      }
   }
   return false;
}
#endif
