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
#ifndef _CTRLM_DEVICE_UPDATE_H_
#define _CTRLM_DEVICE_UPDATE_H_

#include "jansson.h"
#include "ctrlm_ipc_device_update.h"
#include "rf4ce/ctrlm_rf4ce_network.h"

#define CTRLM_RF4CE_CONTROLLER_FIRMWARE_UPDATE_TIMEOUT 10 //controller formware writing firmware to flash timeout in seconds 

#define CTRLM_DEVICE_UPDATE_EXTENDED_TIMEOUT_VALUE (45)
#define CTRLM_DEVICE_UPDATE_USE_DEFAULT_TIMEOUT (0)

// HACK FOR XR15-704
#ifdef XR15_704
#define XR15_DEVICE_UPDATE_BUG_FIRMWARE_MAJOR    (2)
#define XR15_DEVICE_UPDATE_BUG_FIRMWARE_MINOR    (0)
#define XR15_DEVICE_UPDATE_BUG_FIRMWARE_REVISION (0)
#define XR15_DEVICE_UPDATE_BUG_FIRMWARE_PATCH    (0)
#endif
// HACK FOR XR15-704

typedef enum {
   // locations to check for firmware and audio updates
   DEVICE_UPDATE_CHECK_XCONF                  = 0,
   DEVICE_UPDATE_CHECK_FILESYSTEM             = 1,
   DEVICE_UPDATE_CHECK_BOTH                   = 2
} device_update_check_locations_t;

typedef enum {
   RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_TIME     = 0x00,
   RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_KEYPRESS = 0x01,
} rf4ce_device_update_image_check_when_t;

typedef enum {
   RF4CE_DEVICE_UPDATE_IMAGE_LOAD_NOW           = 0x00,
   RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE = 0x01,
   RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_KEYPRESS = 0x02,
   RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_AGAIN    = 0x03,
   RF4CE_DEVICE_UPDATE_IMAGE_LOAD_CANCEL        = 0x04
} rf4ce_device_update_image_load_when_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                         header;
   ctrlm_rf4ce_controller_type_t                         controller_type;
   ctrlm_controller_id_t                                 controller_id;
   version_software_t                                    update_version;
   ctrlm_rf4ce_device_update_image_type_t                update_type;
   char                                                  file_path_archive[CTRLM_DEVICE_UPDATE_PATH_LENGTH];
} ctrlm_main_queue_msg_update_file_check_t;

typedef struct {
   ctrlm_main_queue_msg_header_t                         header;
   ctrlm_rf4ce_device_update_image_type_t                image_type;
   ctrlm_rf4ce_controller_type_t                         controller_type;
   bool                                                  force_update;
   version_software_t                                    version_software;
   version_software_t                                    version_bootloader_min;
   version_hardware_t                                    version_hardware_min;
   bool                                                  type_z;
} ctrlm_main_queue_msg_notify_firmware_t;

typedef struct {
   guint16            id;
   version_software_t version;
   guint32            size;
   guint32            crc;
   gboolean           force_update;
   gboolean           do_not_load;
   char               file_name[CTRLM_DEVICE_UPDATE_PATH_LENGTH];
   gboolean           type_z;
} rf4ce_device_update_image_info_t;

typedef struct {
   rf4ce_device_update_image_check_when_t when;
   guint32                                time;
   gboolean                               background_download;
} rf4ce_device_update_begin_info_t;

typedef struct {
   rf4ce_device_update_image_load_when_t when;
   guint32                               time;
} rf4ce_device_update_load_info_t;

#ifdef __cplusplus
extern "C"
{
#endif

void     ctrlm_device_update_init(json_t *json_obj_device_update);
gboolean ctrlm_device_update_init_iarm(void);
void     ctrlm_device_update_terminate(void);
void     ctrlm_device_update_queue_msg_push(gpointer msg);
gboolean ctrlm_device_update_rf4ce_is_image_available(ctrlm_rf4ce_device_update_image_type_t image_type, ctrlm_rf4ce_controller_type_t type, version_hardware_t version_hardware, version_software_t version_bootloader, version_software_t version_software, ctrlm_rf4ce_device_update_audio_theme_t audio_theme, rf4ce_device_update_image_info_t *image_info, bool type_z);
guint16  ctrlm_device_update_rf4ce_image_data_read(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, guint32 offset, guint16 length, guchar *data);
gboolean ctrlm_device_update_rf4ce_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, version_hardware_t version_hardware, version_software_t version_bootloader, version_software_t version_software, guint16 image_id, guint32 timeout, gboolean manual_poll, rf4ce_device_update_begin_info_t *begin_info, ctrlm_device_update_session_id_t *session_id_in, ctrlm_device_update_session_id_t *session_id_out, gboolean session_resume);
void     ctrlm_device_update_rf4ce_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, ctrlm_device_update_iarm_load_result_t status);
void     ctrlm_device_update_rf4ce_load_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, rf4ce_device_update_load_info_t *load_info);
void     ctrlm_device_update_rf4ce_notify_reboot(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, gboolean session_resume);
gboolean ctrlm_device_update_is_image_id_valid(ctrlm_device_update_image_id_t image_id);
gboolean ctrlm_device_update_session_get_by_id(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_iarm_call_session_t *session);
gboolean ctrlm_device_update_image_device_get_by_id(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_image_id_t *image_id, ctrlm_device_update_device_t *device);
gboolean ctrlm_device_update_image_get_by_id(ctrlm_device_update_image_id_t image_id, ctrlm_device_update_image_t *image);
gboolean ctrlm_device_update_status_info_get(ctrlm_device_update_iarm_call_status_t *status);
gboolean ctrlm_device_update_interactive_download_start(ctrlm_device_update_session_id_t session_id, gboolean background, guchar percent_increment, gboolean load_image_immediately);
gboolean ctrlm_device_update_interactive_load_start(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_iarm_load_type_t load_type, guint32 time_to_load, guint32 time_after_inactive);
void     ctrlm_device_update_iarm_event_ready_to_download(ctrlm_device_update_session_id_t session_id);
void     ctrlm_device_update_iarm_event_download_status(ctrlm_device_update_session_id_t session_id, guchar percent_complete);
void     ctrlm_device_update_iarm_event_load_begin(ctrlm_device_update_session_id_t session_id);
void     ctrlm_device_update_iarm_event_load_end(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_iarm_load_result_t result);
gboolean ctrlm_device_update_is_controller_updating(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, bool ignore_load_waiting);
gboolean ctrlm_device_update_process_xconf_update(std::string fileLocation);
device_update_check_locations_t ctrlm_device_update_check_locations_get(void);
const char * ctrlm_device_update_get_xconf_path(void);
void ctrlm_device_update_check_for_update_file_delete(ctrlm_device_update_image_id_t image_id, ctrlm_controller_id_t controller_id, ctrlm_network_id_t network_it);
void     ctrlm_device_update_timeout_update_activity(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint timeout_value = CTRLM_DEVICE_UPDATE_USE_DEFAULT_TIMEOUT);
gboolean ctrlm_device_update_rf4ce_is_software_version_higher(version_software_t current, version_software_t proposed);
gboolean ctrlm_device_update_rf4ce_is_software_version_min_met(version_software_t current, version_software_t minimum);
gboolean ctrlm_device_update_rf4ce_is_hardware_version_min_met(version_hardware_t current, version_hardware_t minimum);
std::string ctrlm_device_update_get_software_version(guint16 image_id);
void     ctrlm_device_update_rf4ce_session_resume(std::vector<rf4ce_device_update_session_resume_info_t> *sessions);
guint32 ctrlm_device_update_request_timeout_get(void);

#ifdef XR15_704
gboolean ctrlm_device_update_xr15_crash_update_get();
#endif

#ifdef __cplusplus
}
#endif

#endif
