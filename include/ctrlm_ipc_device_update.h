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
#include "ctrlm_ipc.h"

#ifndef _CTRLM_IPC_DEVICEUPDATE_H_
#define _CTRLM_IPC_DEVICEUPDATE_H_

#define CTRLM_DEVICE_UPDATE_IARM_CALL_UPDATE_AVAILABLE  "DeviceUpdate_UpdateAvail"       ///< IARM Call to indictate update file available to process

#define CTRLM_DEVICE_UPDATE_IARM_BUS_API_REVISION       (1)                 ///< Revision of the Device Update IARM API
#define CTRLM_DEVICE_UPDATE_PATH_LENGTH                 (512)               ///< Maximum length of an update path string
#define CTRLM_DEVICE_UPDATE_VERSION_LENGTH              (18)                ///< Maximum length of the version string
#define CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH          (64)                ///< Maximum length of a device's name

typedef enum {
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_DEFAULT = 0, ///< The default load type will be used.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_NORMAL  = 1, ///< Allows the caller to defer to a number of seconds in the future as well as a time after inactivity.  This setting will drive two other parameters: time_to_load and time_after_inactivity.
                                                   /// The device must wait time_to_load seconds where the load does not happen.  Immediately after the time_to_lLoad timer expires, if the time_after_inactivity is non-zero, the
                                                   /// device must wait timeAfterInactivity seconds after all activity on the remote and then attempt to load the image. If the caller wants the remote to load immediately, it will simply set both time parameters to 0.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_POLL    = 2, ///< Indicates that the device should poll frequently for new instructions on whether or not to load.  For example, a remote control would poll every key press.  Another device could poll once a second.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_ABORT   = 3, ///< Allows a caller to abort the download on the device if the device is currently waiting for some time or in a polling mode. If abort is called the session is considered over and the session ID is considered invalid.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_MAX     = 4  ///< Load type maximum value
} ctrlm_device_update_iarm_load_type_t;

typedef enum {
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_SUCCESS        = 0, ///< The image load completed successfully.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REQUEST  = 1, ///< The image load failed due to a request error.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_CRC      = 2, ///< The image load failed due to a CRC mismatch.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_ABORT    = 3, ///< The image load failed due to an abort.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_REJECT   = 4, ///< The image load failed due to rejection by the device.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_TIMEOUT  = 5, ///< The image load failed due to a timeout.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_BAD_HASH = 6, ///< The image load failed due to bad hash.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_OTHER    = 7, ///< The image load failed due to another error.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_MAX            = 8  ///< Load result maximum value
} ctrlm_device_update_iarm_load_result_t;

typedef unsigned char ctrlm_device_update_session_id_t;
typedef unsigned char ctrlm_device_update_image_id_t;

typedef struct {
   unsigned char                 api_revision;                                      ///< Revision of this API
   ctrlm_iarm_call_result_t      result;                                            ///< Result of the IARM call
   char                          firmwareLocation[CTRLM_DEVICE_UPDATE_PATH_LENGTH]; ///< location of update files
   char                          firmwareNames[CTRLM_DEVICE_UPDATE_PATH_LENGTH];    ///< file names of update files comma delimited
} ctrlm_device_update_iarm_call_update_available_t;

typedef struct {
   char name[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH];           ///< The name of the device
   char version_software[CTRLM_DEVICE_UPDATE_VERSION_LENGTH];   ///< The device's software version string
   char version_hardware[CTRLM_DEVICE_UPDATE_VERSION_LENGTH];   ///< The device's hardware version string
   char version_bootloader[CTRLM_DEVICE_UPDATE_VERSION_LENGTH]; ///< The device's bootloader version string
} ctrlm_device_update_device_t;

#endif
