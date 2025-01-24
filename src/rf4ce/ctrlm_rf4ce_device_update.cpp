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
#include "ctrlm_rcu.h"
#include "ctrlm_rf4ce_network.h"
#include "ctrlm_device_update.h"
#include "ctrlm_database.h"

#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_CHECK_REQUEST     (2)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_CHECK_RESPONSE    (7)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_AVAILABLE         (39)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DATA_REQUEST      (7)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DATA_RESPONSE     (88) // 95 minus 6 byte header rounded down to even number
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_REQUEST      (3)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_RESPONSE     (8)
#define RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DOWNLOAD_COMPLETE (4)

#define IMAGE_CHECK_RESPONSE_FLAG_NO_IMAGE           (0x01)
#define IMAGE_CHECK_RESPONSE_FLAG_IMAGE_PENDING      (0x02)
#define IMAGE_CHECK_RESPONSE_FLAG_POLL_TIME          (0x04)
#define IMAGE_CHECK_RESPONSE_FLAG_POLL_NEXT_KEYPRESS (0x08)

#define IMAGE_AVAILABLE_FLAG_FORCE_UPDATE            (0x01)
#define IMAGE_AVAILABLE_FLAG_DO_NOT_LOAD             (0x04)

void ctrlm_obj_network_rf4ce_t::ind_process_data_device_update(ctrlm_main_queue_msg_rf4ce_ind_data_t *dqm) {
   THREAD_ID_VALIDATE();
   
   XLOGD_DEBUG("enter");
   if(dqm == NULL) {
      XLOGD_ERROR("Invalid parameters");
      return;
   }

   if(!controller_exists(dqm->controller_id)) {
      XLOGD_ERROR("Invalid controller id %u", dqm->controller_id);
      return;
   }

   if(dqm->length < 2) {
      XLOGD_ERROR("Invalid size %u", dqm->length);
      return;
   }

   ctrlm_controller_id_t           controller_id     = dqm->controller_id;
   ctrlm_timestamp_t               timestamp         = dqm->timestamp;
   unsigned char                   cmd_length        = dqm->length;
   guchar *                        cmd_data          = dqm->data;
   ctrlm_rf4ce_device_update_cmd_t device_update_cmd = (ctrlm_rf4ce_device_update_cmd_t)(cmd_data[0]);
   
   switch(device_update_cmd) {
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_CHECK_REQUEST: {
         if(cmd_length != RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_CHECK_REQUEST) {
            XLOGD_ERROR("Image check request. Invalid Length %u, %u", cmd_length, RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_CHECK_REQUEST);
            // TODO need to send error response back in this case?
         } else if(!controller_exists(controller_id)) {
            XLOGD_ERROR("Image check request. Invalid controller %u", controller_id);
         } else {
            XLOGD_INFO("Image check request: Controller id %u.", controller_id);
            ctrlm_rf4ce_device_update_image_type_t image_type = (ctrlm_rf4ce_device_update_image_type_t)cmd_data[1];
            controllers_[controller_id]->device_update_image_check_request(timestamp, image_type);
         }
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_CHECK_RESPONSE: {
         XLOGD_WARN("Image check response - NOT IMPLEMENTED");
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_AVAILABLE: {
         XLOGD_WARN("Image available - NOT IMPLEMENTED");
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_DATA_REQUEST: {
         if(cmd_length != RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DATA_REQUEST) {
            XLOGD_ERROR("Image data request. Invalid Length %u, %u", cmd_length, RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DATA_REQUEST);
            // TODO need to send error response back in this case?
         } else if(!controller_exists(controller_id)) {
            XLOGD_ERROR("Image data request. Invalid controller %u", controller_id);
         } else {
            //XLOGD_INFO("Image data request");
            guint16 image_id = (cmd_data[2] << 8)  | cmd_data[1];
            guint32 offset   = (cmd_data[5] << 16) | (cmd_data[4] << 8) | cmd_data[3];
            guchar  length   = cmd_data[6];
            controllers_[controller_id]->device_update_image_data_request(timestamp, image_id, offset, length);
         }
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_DATA_RESPONSE: {
         XLOGD_WARN("Image data response - NOT IMPLEMENTED");
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_LOAD_REQUEST: {
         if(cmd_length != RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_REQUEST) {
            XLOGD_ERROR("Image load request. Invalid Length %u, %u", cmd_length, RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_REQUEST);
            // TODO need to send error response back in this case?
         } else if(!controller_exists(controller_id)) {
            XLOGD_ERROR("Image load request. Invalid controller %u", controller_id);
         } else {
            XLOGD_INFO("Image load request: Controller id %u.", controller_id);
            guint16 image_id = (cmd_data[2] << 8) | cmd_data[1];
            
            controllers_[controller_id]->device_update_image_load_request(timestamp, image_id);
         }
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_LOAD_RESPONSE: {
         XLOGD_WARN("Image load response - NOT IMPLEMENTED");
         break;
      }
      case RF4CE_DEVICE_UPDATE_CMD_IMAGE_DOWNLOAD_COMPLETE: {
         if(cmd_length != RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DOWNLOAD_COMPLETE) {
            XLOGD_ERROR("Image download complete. Invalid Length %u, %u", cmd_length, RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DOWNLOAD_COMPLETE);
            // TODO need to send error response back in this case?
         } else if(!controller_exists(controller_id)) {
            XLOGD_ERROR("Image download complete. Invalid controller %u", controller_id);
         } else {
            guint16 image_id = (cmd_data[2] << 8) | cmd_data[1];
            ctrlm_rf4ce_device_update_result_t result = (ctrlm_rf4ce_device_update_result_t)cmd_data[3];
            controllers_[controller_id]->device_update_image_download_complete(timestamp, image_id, result);
         }
         break;
      }
      default: {
         XLOGD_ERROR("Unhandled frame control (0x%02X) Length %u.", device_update_cmd, cmd_length);
         guint32 index = 0;
         for(index = 0; index < cmd_length; index += 8) {
            if(index >= 64) {
               break;
            }
            XLOGD_ERROR("%02X %02X %02X %02X %02X %02X %02X %02X", cmd_data[index], cmd_data[index+1], cmd_data[index+2], cmd_data[index+3], cmd_data[index+4], cmd_data[index+5], cmd_data[index+6], cmd_data[index+7]);
         }
         break;
      }
   }
}

void ctrlm_obj_controller_rf4ce_t::device_update_image_check_request(ctrlm_timestamp_t timestamp, ctrlm_rf4ce_device_update_image_type_t image_type) {
   rf4ce_device_update_image_info_t image_info;
   rf4ce_device_update_begin_info_t begin_info;
   gboolean image_available   = false;
   gboolean ready_to_download = false;
   guchar   response[RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_AVAILABLE];
   guchar   response_length;
   gboolean manual_poll = false;
   errno_t safec_rc = -1;

#ifdef XR15_704
   if(did_reset_) {
      XLOGD_INFO("CHECK IMAGE REQUEST due to XR15 reset code!");
      did_reset_ = false;
   }
#endif

   // is image type supported?
   switch(image_type) {
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE: {
         XLOGD_INFO("<%s> (%u) %s", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), ctrlm_rf4ce_device_update_image_type_str(image_type));
         print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_UPGRADE_CHECK);
         time_last_checkin_for_device_update_set();
         // Is firmware available for this remote?
         if(manual_poll_firmware_) { // Manual poll from the remote
            manual_poll_firmware_ = false;
            manual_poll           = true;
         }
         image_available = ctrlm_device_update_rf4ce_is_image_available(image_type, controller_type_, version_hardware_->to_versiont(), version_bootloader_->to_versiont(), version_software_->to_versiont(), RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID, &image_info, is_controller_type_z());
         break;
      }
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1: {
         XLOGD_INFO("(%u) Audio Data 1", controller_id_get());
         if(manual_poll_audio_data_) { // Manual poll from the remote, use the audio theme if set
            image_available = ctrlm_device_update_rf4ce_is_image_available(image_type, controller_type_, version_hardware_->to_versiont(), version_bootloader_->to_versiont(), version_audio_data_->to_versiont(), audio_theme_, &image_info, false);
            manual_poll_audio_data_ = false;
            manual_poll             = true;
         } else {
            image_available = ctrlm_device_update_rf4ce_is_image_available(image_type, controller_type_, version_hardware_->to_versiont(), version_bootloader_->to_versiont(), version_audio_data_->to_versiont(), RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID, &image_info, false);
         }
         break;
      }
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_2:
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_DSP:
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_KEYWORD_MODEL: {
         if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
            version_software_t version = {0};
            XLOGD_INFO("<%s> (%u) %s", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), ctrlm_rf4ce_device_update_image_type_str(image_type));
            time_last_checkin_for_device_update_set();
            // Is firmware available for this remote?
            if(manual_poll_firmware_) { // Manual poll from the remote
               manual_poll_firmware_ = false;
               manual_poll           = true;
            }
            switch(image_type) {
               case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_DSP:           version = version_dsp_->to_versiont();           break;
               case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_KEYWORD_MODEL: version = version_keyword_model_->to_versiont(); break;
               case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_2:  version = version_audio_data_->to_versiont();    break;
               default: break;
            }
            image_available = ctrlm_device_update_rf4ce_is_image_available(image_type, controller_type_, version_hardware_->to_versiont(), version_bootloader_->to_versiont(), version, RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID, &image_info, false);
            break;
         }
      }
      default: {
         XLOGD_TELEMETRY("(%u) Unsupported image type - 0x%08X", controller_id_get(), image_type);
         return;
      }
   }

   ctrlm_device_update_session_id_t session_id = 0;
   if(image_available) { // Attempt to start update session
      ready_to_download = ctrlm_device_update_rf4ce_begin(network_id_get(), controller_id_get(), version_hardware_->to_versiont(), version_bootloader_->to_versiont(), version_software_->to_versiont(), image_info.id, ctrlm_device_update_request_timeout_get(), manual_poll, &begin_info, NULL, &session_id, device_update_session_resume_support());
      print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_STARTED, ctrlm_device_update_get_software_version(image_info.id));
      download_in_progress_ = true;
   }

#ifdef XR15_704
   // HACK: We need to make XR15s running < 2.0.0.0 do not get an image pending flag to avoid bug on device.
   ctrlm_sw_version_t version_bug(XR15_DEVICE_UPDATE_BUG_FIRMWARE_MAJOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_MINOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_REVISION, XR15_DEVICE_UPDATE_BUG_FIRMWARE_PATCH);
#endif
 
   if(!image_available || !ready_to_download) {
      guchar  flags_check     = IMAGE_CHECK_RESPONSE_FLAG_NO_IMAGE;
      guint32 next_check_time = 0;

      if(!image_available) {
         next_check_time = update_polling_period_get() * 60 * 60; // Use update polling rib entry - convert to seconds
         XLOGD_INFO("Image Check response - No image is available.  Check again in %u seconds.", next_check_time);

         if (is_controller_type_z()) {
            ota_failure_type_z_cnt_set(ota_failure_type_z_cnt_get() + 1);
         }
      }
#ifdef XR15_704
      // HACK: We need to make XR15s running < 2.0.0.0 do not get an image pending flag to avoid bug on device.
      else if(RF4CE_CONTROLLER_TYPE_XR15 == controller_type_ && *version_software_ < version_bug) {
         XLOGD_INFO("Image Check response - Image is pending - XR15v1 running < 2.0.0.0, sending No image available");
         print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_PENDING, ctrlm_device_update_get_software_version(image_info.id));
         if(begin_info.when == RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_TIME) {
            next_check_time = begin_info.time;
         } else {
            next_check_time = update_polling_period_get() * 60 * 60; // Use update polling rib entry - convert to seconds
         }
      }
#endif 
      else {
         flags_check = IMAGE_CHECK_RESPONSE_FLAG_IMAGE_PENDING;
         XLOGD_INFO("Image Check response - Image is pending");
         print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_PENDING, ctrlm_device_update_get_software_version(image_info.id));

         if(begin_info.when == RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_TIME) {
            flags_check |= IMAGE_CHECK_RESPONSE_FLAG_POLL_TIME;
            next_check_time = begin_info.time;
            XLOGD_INFO("Poll in %u seconds", next_check_time);
         } else if(begin_info.when == RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_KEYPRESS) {
            flags_check |= IMAGE_CHECK_RESPONSE_FLAG_POLL_NEXT_KEYPRESS;
            XLOGD_INFO("Poll keypress");
         }
      }
      response[0] = RF4CE_DEVICE_UPDATE_CMD_IMAGE_CHECK_RESPONSE;
      response[1] = image_type;                      // Image Type
      response[2] = flags_check;                     // Flags
      response[3] = (guchar)(next_check_time);       // Time in seconds for next image check request
      response[4] = (guchar)(next_check_time >> 8);
      response[5] = (guchar)(next_check_time >> 16);
      response[6] = (guchar)(next_check_time >> 24);
      response_length = RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_CHECK_RESPONSE;
   } else {
      guchar flags_available = 0;

      if(begin_info.background_download) {
         download_rate_ = (guchar)DOWNLOAD_RATE_BACKGROUND;
         XLOGD_INFO("Image Available response - Background");
      } else {
         download_rate_ = (guchar)DOWNLOAD_RATE_IMMEDIATE;
         image_info.do_not_load = false;
         XLOGD_INFO("Image Available response - Foreground");
      }
      if(image_info.force_update) {
         flags_available = IMAGE_AVAILABLE_FLAG_FORCE_UPDATE;
         XLOGD_INFO("Force update");
      }
      if(image_info.do_not_load) {
         flags_available |= IMAGE_AVAILABLE_FLAG_DO_NOT_LOAD;
         XLOGD_INFO("Do not load the image: flags <0x%02X>", flags_available);
      } else {
         XLOGD_INFO("Load the image: flags <0x%02X>", flags_available);
      }
      response[0]  = RF4CE_DEVICE_UPDATE_CMD_IMAGE_AVAILABLE;
      response[1]  = (guchar)(image_info.id);         // Image Id
      response[2]  = (guchar)(image_info.id >> 8);
      response[3]  = image_type;                      // Image Type
      response[4]  = flags_available;                 // Flags
      response[5]  = image_info.version.major;        // Version
      response[6]  = image_info.version.minor;
      response[7]  = image_info.version.revision;
      response[8]  = image_info.version.patch;
      response[9]  = (guchar)(image_info.size);       // Image Size
      response[10] = (guchar)(image_info.size >> 8);
      response[11] = (guchar)(image_info.size >> 16);
      response[12] = (guchar)(image_info.size >> 24);
      response[13] = (guchar)(image_info.crc);        // Image CRC
      response[14] = (guchar)(image_info.crc >> 8);
      response[15] = (guchar)(image_info.crc >> 16);
      response[16] = (guchar)(image_info.crc >> 24);
      response[17] = (guchar)((version_hardware_->get_manufacturer() << 4) | (version_hardware_->get_model() & 0xF)); // Hardware Version
      response[18] = version_hardware_->get_revision();
      safec_rc = strcpy_s((char *)&response[19], sizeof(response)-19, product_name_->to_string().c_str());
      ERR_CHK(safec_rc);
      response_length = RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_AVAILABLE;

      if(device_update_session_resume_support()) { // Store download session info in persistent memory
         rf4ce_device_update_session_resume_state_t state;
         state.session_id          = session_id;
         state.image_id            = image_info.id;
         safec_rc = memset_s(&state.expiration, sizeof(state.expiration), 0, sizeof(state.expiration));
         ERR_CHK(safec_rc);
         state.image_type          = image_type;
         state.manual_poll         = manual_poll;
         state.when                = begin_info.when;
         state.time                = begin_info.time;
         state.background_download = begin_info.background_download;
         safec_rc = strcpy_s(state.file_name, sizeof(state.file_name), image_info.file_name);
         ERR_CHK(safec_rc);

         ctrlm_timestamp_get(&state.expiration);
         ctrlm_timestamp_add_secs(&state.expiration, obj_network_rf4ce_->device_update_session_timeout_get());

         device_update_session_resume_store(&state);
      }
   }

   // Determine when to send the response (50 ms after receipt)
   unsigned long long nsecs = timestamp.tv_nsec + (CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME * 1000000);
   timestamp.tv_nsec  = nsecs % 1000000000;
   timestamp.tv_sec  += nsecs / 1000000000;

   // Send the response back to the controller. Controller will respond with either first data packet request or update complete command
   req_data(CTRLM_RF4CE_PROFILE_ID_DEVICE_UPDATE, timestamp, response_length, response, NULL, NULL);

   unsigned long delay = ctrlm_timestamp_until_us(timestamp);

   if(delay == 0) {
      ctrlm_timestamp_t now;
      ctrlm_timestamp_get(&now);

      long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
      if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
         XLOGD_WARN("<%s> (%u) LATE response packet - diff %ld ms", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), diff);
      }
   }

}

void ctrlm_obj_controller_rf4ce_t::device_update_image_data_request(ctrlm_timestamp_t timestamp, guint16 image_id, guint32 offset, guchar length) {
   if(length > RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_DATA_RESPONSE) {
      XLOGD_WARN("(%u, 0x%04X) requested size is too big (%u bytes)", controller_id_get(), image_id, length);
      return;
   }
   guchar response_length = length + 6;
   guchar response[response_length];

   response[0] = RF4CE_DEVICE_UPDATE_CMD_IMAGE_DATA_RESPONSE;
   response[1] = (guchar)(image_id);      // Image Id
   response[2] = (guchar)(image_id >> 8);
   response[3] = (guchar)(offset);        // Offset
   response[4] = (guchar)(offset >> 8);
   response[5] = (guchar)(offset >> 16);
   
   if(length != ctrlm_device_update_rf4ce_image_data_read(network_id_get(), controller_id_get(), image_id, offset, length, &response[6])) {
      XLOGD_WARN("(%u, 0x%04X) unable to get image data (offset 0x%08x, %u bytes)", controller_id_get(), image_id, offset, length);
      return;
   }
   
   // Determine when to send the response (50 ms after receipt)
   ctrlm_timestamp_add_ms(&timestamp, CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME);
   unsigned long delay = ctrlm_timestamp_until_us(timestamp);

   if(delay == 0) {
      ctrlm_timestamp_t now;
      ctrlm_timestamp_get(&now);

      long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
      if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
         XLOGD_WARN("(%u, 0x%04X) LATE response packet - diff %ld ms", controller_id_get(), image_id, diff);
      }
   }

   // Send the response back to the controller
   req_data(CTRLM_RF4CE_PROFILE_ID_DEVICE_UPDATE, timestamp, response_length, response, NULL, NULL);
}

void ctrlm_obj_controller_rf4ce_t::device_update_image_load_request(ctrlm_timestamp_t timestamp, guint16 image_id) {
   ctrlm_rf4ce_device_update_image_load_rsp_t load_response = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_RSP_CANCEL;
   rf4ce_device_update_load_info_t            load_info;
   guint32 time = 0;
   guchar  response[RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_RESPONSE];
   ctrlm_network_id_t    network_id    = network_id_get();
   ctrlm_controller_id_t controller_id = controller_id_get();
   XLOGD_INFO("(%u, %u)", network_id, controller_id);

   // Determine when to load the image
   ctrlm_device_update_rf4ce_load_info(network_id, controller_id, image_id, &load_info);

   std::string log_string = ctrlm_device_update_get_software_version(image_id) + ". ";

#ifdef XR15_704
   // HACK: We need to make XR15s running < 2.0.0.0 load ASAP to avoid bug on device.
   ctrlm_sw_version_t version_bug(XR15_DEVICE_UPDATE_BUG_FIRMWARE_MAJOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_MINOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_REVISION, XR15_DEVICE_UPDATE_BUG_FIRMWARE_PATCH);

   if(RF4CE_CONTROLLER_TYPE_XR15 == controller_type_ && *version_software_ < version_bug) {
       XLOGD_INFO("XR15v1 running < 2.0.0.0, LOAD IMMEDIATELY");
       load_info.when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_NOW;
       log_string += "Load scheduled: Immediately.";
   }
#endif

   if(load_info.when == RF4CE_DEVICE_UPDATE_IMAGE_LOAD_NOW) {
      load_response = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_RSP_NOW;
      XLOGD_INFO("Image Load response - Now");
      log_string += "Load scheduled: Now.";
   } else if(load_info.when == RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE) {
      load_response = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_RSP_WHEN_INACTIVE;
      time          = load_info.time;
      XLOGD_INFO("Image Load response - When inactive for %u seconds", time);
      log_string += "Load scheduled: When inactive for "  + std::to_string(time) + " seconds.";
   } else if(load_info.when == RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_KEYPRESS) {
      load_response = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_RSP_POLL_KEYPRESS;
      XLOGD_INFO("Image Load response - Poll next keypress");
      log_string += "Load scheduled: Next Keypress.";
   } else if(load_info.when == RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_AGAIN) {
      load_response = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_RSP_POLL_AGAIN;
      time          = load_info.time;
      XLOGD_INFO("Image Load response - Poll in %u seconds", time);
      log_string += "Poll for load in " + std::to_string(time) + " seconds.";
   } else {
      XLOGD_INFO("Image Load response - Cancel");
      log_string += "LOAD CANCELLED.";
   }

   print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_REBOOT_SCHEDULE, log_string);
   response[0] = RF4CE_DEVICE_UPDATE_CMD_IMAGE_LOAD_RESPONSE;
   response[1] = (guchar)(image_id);      // Image Id
   response[2] = (guchar)(image_id >> 8);
   response[3] = load_response;         // Response
   response[4] = (guchar)(time);          // Time
   response[5] = (guchar)(time >> 8);
   response[6] = (guchar)(time >> 16);
   response[7] = (guchar)(time >> 24);

   // Determine when to send the response (50 ms after receipt)
   ctrlm_timestamp_add_ms(&timestamp, CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME);
   unsigned long delay = ctrlm_timestamp_until_us(timestamp);

   if(delay == 0) {
      ctrlm_timestamp_t now;
      ctrlm_timestamp_get(&now);

      long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
      if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
         XLOGD_WARN("(%u, 0x%04X) LATE response packet - diff %ld ms", controller_id_get(), image_id, diff);
      }
   }

   // Send the response back to the controller
   req_data(CTRLM_RF4CE_PROFILE_ID_DEVICE_UPDATE, timestamp, RF4CE_DEVICE_UPDATE_CMD_LEN_IMAGE_LOAD_RESPONSE, response, NULL, NULL);

   //if(download_in_progress_) { // End the download session since the controller has finished loading the image
   //   ctrlm_device_update_rf4ce_end(network_id_get(), controller_id_get(), image_id);
   //   download_in_progress_ = false;
   //}
}

void ctrlm_obj_controller_rf4ce_t::device_update_image_download_complete(ctrlm_timestamp_t timestamp, guint16 image_id, ctrlm_rf4ce_device_update_result_t result) {
   ctrlm_device_update_iarm_load_result_t load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_OTHER;
   std::string log_string = ctrlm_device_update_get_software_version(image_id);

   //Explicitly set the download rate to background in case the remote downloaded in the foreground
   guchar data[CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE];
   data[0] = DOWNLOAD_RATE_BACKGROUND;
   rf4ce_rib_set_target(CTRLM_RF4CE_RIB_ATTR_ID_DOWNLOAD_RATE, 0x00, CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE, data);

   switch(result) {
      case RF4CE_DEVICE_UPDATE_RESULT_SUCCESS: {
         XLOGD_INFO("<%s> (%u, 0x%04X) Image download complete - SUCCESS", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_SUCCESS;
         set_firmware_updated();
         ctrlm_device_update_check_for_update_file_delete(image_id,controller_id_get(),network_id_get());
         log_string += " status: SUCCESS";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_REQUEST: {
         XLOGD_INFO("<%s> (%u, 0x%04X) Image download complete - REQUEST ERROR", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REQUEST;
         log_string += " status: REQUEST ERROR";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_CRC: {
         XLOGD_TELEMETRY("<%s> (%u, 0x%04X) Image download complete - CRC ERROR", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_CRC;
         log_string += " status: CRC ERROR";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_ABORTED: {
         XLOGD_INFO("<%s> (%u, 0x%04X) Image download complete - ABORTED", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_ABORT;
         log_string += " status: ABORTED";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_REJECTED: {
         XLOGD_INFO("<%s> (%u, 0x%04X) Image download complete - REJECTED", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REJECT;
         log_string += " status: REJECTED";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_TIMEOUT: {
         XLOGD_INFO("<%s> (%u, 0x%04X) Image download complete - TIMEOUT", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_TIMEOUT;
         log_string += " status: TIMEOUT";
         break;
      }
      case RF4CE_DEVICE_UPDATE_RESULT_ERROR_BAD_HASH: {
         XLOGD_TELEMETRY("<%s> (%u, 0x%04X) Image download complete - BAD HASH", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         load_result = CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_BAD_HASH;
         log_string += " status: BAD HASH";
         break;
      }
      default: {
         XLOGD_TELEMETRY("<%s> (%u, 0x%04X) Image download complete - UNKNOWN", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), image_id);
         log_string += " status: UNKNOWN";
      }
   }

   if (result == RF4CE_DEVICE_UPDATE_RESULT_SUCCESS) {
      // Reset controller ota counter to zero
      ota_failure_type_z_cnt_set(0);
   } else {
      if (is_controller_type_z() || result == RF4CE_DEVICE_UPDATE_RESULT_ERROR_CRC || result == RF4CE_DEVICE_UPDATE_RESULT_ERROR_BAD_HASH) {
         // Increment ota counter
         ota_failure_type_z_cnt_set(ota_failure_type_z_cnt_get() + 1);
      }
   }

   print_remote_firmware_debug_info(RF4CE_PRINT_FIRMWARE_LOG_IMAGE_DOWNLOAD_COMPLETE, log_string);

   if(download_in_progress_) { // End the download session since the controller has finished loading the image
      ctrlm_device_update_rf4ce_end(network_id_get(), controller_id_get(), image_id, load_result);
      download_in_progress_ = false;

      if(device_update_session_resume_support()) { // Remove download session info from persistent memory
         device_update_session_resume_remove();
      }
   }
}

bool ctrlm_obj_controller_rf4ce_t::device_update_session_resume_support(void) {
   if(controller_type_ == RF4CE_CONTROLLER_TYPE_XR19) {
      version_software_t xr19_fw4_w_ota_resume = { .major    = 2,
                                                   .minor    = 6,
                                                   .revision = 0,
                                                   .patch    = 2 };
      return(ctrlm_device_update_rf4ce_is_software_version_min_met(version_software_get().to_versiont(), xr19_fw4_w_ota_resume));
   }
   return(false);
}

bool ctrlm_obj_controller_rf4ce_t::device_update_session_resume_available(void) {
   if(!this->device_update_session_resume_support()) {
      return(false);
   }
   return(ctrlm_db_rf4ce_exists_device_update_session_state(network_id_get(), controller_id_get()));
}

void ctrlm_obj_controller_rf4ce_t::device_update_session_resume_store(rf4ce_device_update_session_resume_state_t *state) {
   if(state == NULL) {
      XLOGD_ERROR("<%s> (%u) NULL param", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return;  //CID:110370 - Forward null
   }
   if(!this->device_update_session_resume_support()) {
      XLOGD_ERROR("<%s> (%u) device does not support session resume", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return;
   }
   XLOGD_INFO("session id <%u> image id <%u> type <%s> expires <%llu> seconds", state->session_id, state->image_id, ctrlm_rf4ce_device_update_image_type_str(state->image_type), ctrlm_timestamp_until_ms(state->expiration) / 1000);

   download_image_id_ = state->image_id;

   ctrlm_db_rf4ce_write_device_update_session_state(network_id_get(), controller_id_get(), (guchar *)state, sizeof(*state));
}

bool ctrlm_obj_controller_rf4ce_t::device_update_session_resume_load(rf4ce_device_update_session_resume_info_t *info) {
   if(info == NULL) {
      XLOGD_ERROR("<%s> (%u) NULL param", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return(false);
   }
   if(!this->device_update_session_resume_available()) {
      return(false);
   }

   rf4ce_device_update_session_resume_state_t *state = NULL;
   guint32 length = sizeof(*state);
   ctrlm_db_rf4ce_read_device_update_session_state(network_id_get(), controller_id_get(), (guchar **)&state, &length);

   if(state == NULL) {
      XLOGD_ERROR("<%s> (%u) NULL state", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return(false);
   } else if(length != sizeof(*state)) {
      XLOGD_ERROR("<%s> (%u) invalid length <%u> expected <%u>", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get(), length, sizeof(*state));
      free(state);
      return(false);
   }

   ctrlm_timestamp_t timestamp;
   ctrlm_timestamp_get(&timestamp);

   if(ctrlm_timestamp_cmp(timestamp, state->expiration) > 0) { // Session expired
      XLOGD_INFO("session id <%u> image id <%u> type <%s> expired", state->session_id, state->image_id, ctrlm_rf4ce_device_update_image_type_str(state->image_type));
      this->device_update_session_resume_remove();
      free(state);
      return(false);
   }

   XLOGD_INFO("session id <%u> image id <%u> type <%s> expires <%llu> secs", state->session_id, state->image_id, ctrlm_rf4ce_device_update_image_type_str(state->image_type), ctrlm_timestamp_until_ms(state->expiration) / 1000);

   download_image_id_       = state->image_id;

   info->state              = *state;
   info->network_id         = network_id_get();
   info->controller_id      = controller_id_get();
   info->type               = controller_type_;
   info->version_hardware   = version_hardware_->to_versiont();
   info->version_bootloader = version_bootloader_->to_versiont();
   info->version_software   = version_software_->to_versiont();
   info->timeout            = obj_network_rf4ce_->device_update_session_timeout_get();
   info->type_z             = is_controller_type_z();

   free(state);

   return(true);
}

void ctrlm_obj_controller_rf4ce_t::device_update_session_resume_unload(void) {
   if(!device_update_session_resume_support()) {
      XLOGD_INFO("<%s> (%u) session resume is not supported", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return;
   }
   if(!this->device_update_session_resume_available()) {
      XLOGD_INFO("<%s> (%u) session resume is not available", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return;
   }

   if(download_image_id_ == RF4CE_DOWNLOAD_IMAGE_ID_INVALID) {
      XLOGD_ERROR("<%s> (%u) session resume invalid image id", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());
      return;
   }

   XLOGD_INFO("<%s> (%u) session resume will be unloaded", ctrlm_rf4ce_controller_type_str(controller_type_), controller_id_get());

   // End the download session since the controller will not resume it
   ctrlm_device_update_rf4ce_end(network_id_get(), controller_id_get(), download_image_id_, CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_ABORT);

   device_update_session_resume_remove();
}

void ctrlm_obj_controller_rf4ce_t::device_update_session_resume_remove(void) {
   ctrlm_db_rf4ce_destroy_device_update_session_state(network_id_get(), controller_id_get());
   download_image_id_ = RF4CE_DOWNLOAD_IMAGE_ID_INVALID;
}

