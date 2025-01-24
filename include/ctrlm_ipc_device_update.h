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
/// @cond DOXYGEN_IGNORE
#define _CTRLM_IPC_DEVICEUPDATE_H_
/// @endcond

/// @file ctrlm_ipc_device_update.h
///
/// @defgroup CTRLM_IPC_DEVICE_UPDATE IARM API - Device Update
/// @{
///
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_COMMS       Communication Interfaces
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_CALLS       IARM Remote Procedure Calls
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_EVENTS      IARM Events
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_DEFINITIONS Constants
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_ENUMS       Enumerations
/// @defgroup CTRLM_IPC_DEVICE_UPDATE_STRUCTS     Structures
///
/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_DEFINITIONS
/// @{
/// @brief Constant Defintions
/// @details The Control Manager provides definitions for constant values.

#define CTRLM_DEVICE_UPDATE_IARM_CALL_STATUS_GET        "DeviceUpdate_StatusGet"         ///< IARM Call to get Device Update Status
#define CTRLM_DEVICE_UPDATE_IARM_CALL_SESSION_GET       "DeviceUpdate_SessionGet"        ///< IARM Call to get Deivec Update Sessions
#define CTRLM_DEVICE_UPDATE_IARM_CALL_IMAGE_GET         "DeviceUpdate_ImageGet"          ///< IARM Call to get information about an update image
#define CTRLM_DEVICE_UPDATE_IARM_CALL_DOWNLOAD_INITIATE "DeviceUpdate_DownloadInitiate"  ///< IARM Call to initiate a download
#define CTRLM_DEVICE_UPDATE_IARM_CALL_LOAD_INITIATE     "DeviceUpdate_LoadInitiate"      ///< IARM Call to initiate a load
#define CTRLM_DEVICE_UPDATE_IARM_CALL_UPDATE_AVAILABLE  "DeviceUpdate_UpdateAvail"       ///< IARM Call to indictate update file available to process

#define CTRLM_DEVICE_UPDATE_IARM_BUS_API_REVISION       (1)                 ///< Revision of the Device Update IARM API
#define CTRLM_DEVICE_UPDATE_MAX_IMAGES                  (50)                ///< Maximum number of update images
#define CTRLM_DEVICE_UPDATE_MAX_SESSIONS                (50)                ///< Maximum number of update sessions
#define CTRLM_DEVICE_UPDATE_PATH_LENGTH                 (512)               ///< Maximum length of an update path string
#define CTRLM_DEVICE_UPDATE_ERROR_LENGTH                (256)               ///< Maximum length of an error string
#define CTRLM_DEVICE_UPDATE_VERSION_LENGTH              (18)                ///< Maximum length of the version string
#define CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH          (64)                ///< Maximum length of a device's name

/// @}

/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_ENUMS
/// @{
/// @brief Enumerated Types
/// @details The Control Manager provides enumerated types for logical groups of values.

/// @brief Device Update Load Types
/// @details An enumeration of the types of load that can be performed.
typedef enum {
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_DEFAULT = 0, ///< The default load type will be used.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_NORMAL  = 1, ///< Allows the caller to defer to a number of seconds in the future as well as a time after inactivity.  This setting will drive two other parameters: time_to_load and time_after_inactivity.
                                                   /// The device must wait time_to_load seconds where the load does not happen.  Immediately after the time_to_lLoad timer expires, if the time_after_inactivity is non-zero, the
                                                   /// device must wait timeAfterInactivity seconds after all activity on the remote and then attempt to load the image. If the caller wants the remote to load immediately, it will simply set both time parameters to 0.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_POLL    = 2, ///< Indicates that the device should poll frequently for new instructions on whether or not to load.  For example, a remote control would poll every key press.  Another device could poll once a second.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_ABORT   = 3, ///< Allows a caller to abort the download on the device if the device is currently waiting for some time or in a polling mode. If abort is called the session is considered over and the session ID is considered invalid.
   CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_MAX     = 4  ///< Load type maximum value
} ctrlm_device_update_iarm_load_type_t;

/// @brief Device Update Load Results
/// @details An enumeration of the result codes that can be returned from the image load operation.
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

/// @brief Device Update Image Types
/// @details An enumeration of the types of images that can be used in device updates.
typedef enum {
   CTRLM_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE   = 0, ///< A firmware image
   CTRLM_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA = 1, ///< An audio data image
   CTRLM_DEVICE_UPDATE_IMAGE_TYPE_OTHER      = 2, ///< Other type of image
   CTRLM_DEVICE_UPDATE_IMAGE_TYPE_MAX        = 3  ///< Image type maximum value
} ctrlm_device_update_image_type_t;

/// @brief Session Id Type
/// @details Type definition for the session identifier.
typedef unsigned char ctrlm_device_update_session_id_t;
/// @brief Image Id Type
/// @details Type definition for the image identifer.
typedef unsigned char ctrlm_device_update_image_id_t;

/// @}

/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_STRUCTS
/// @{
/// @brief Structure Definitions
/// @details The Control Manager provides structures that are used in IARM calls and events.

/// @brief Structure of Device Update's Status Get IARM call
/// @details This structure provides information about the status of device update.
typedef struct {
   unsigned char                    api_revision;                                  ///< Revision of this API
   ctrlm_iarm_call_result_t         result;                                        ///< Result of the IARM call
   unsigned char                    image_qty;                                     ///< The number of device update images available.
   ctrlm_device_update_image_id_t   image_ids[CTRLM_DEVICE_UPDATE_MAX_IMAGES];     ///< An array of image id's. Each image id corresponds to a specific download image.
   unsigned char                    session_qty;                                   ///< The number of sessions populated in the session id's array.
   ctrlm_device_update_session_id_t session_ids[CTRLM_DEVICE_UPDATE_MAX_SESSIONS]; ///< An array of session id's. Each session id corresponds to a specific download and a specific device.
   unsigned char                    interactive_download;                          ///< A flag indicating whether interactive downloads are enabled (1) or not (0).
   unsigned char                    interactive_load;                              ///< A flag indicating whether interactive loads are enabled (1) or not (0).
   unsigned char                    percent_increment;                             ///< A value from 0-100 indicating the increment at which download status updates events will be generated.
   unsigned char                    load_immediately;                              ///< A flag indicating whether to load the image immediately (1) or not (0).
   unsigned char                    running;                                       ///< A flag indicating that device update is running (1) or not (0).
} ctrlm_device_update_iarm_call_status_t;

/// @brief Structure of Device Update's Update Availible IARM call
/// @details This call is made to announce that the xconf CDL script has found and downloaded an update tar file for one or more of our remote controls.  The tar file needs to be
/// processed and the update procedure started in order to send the updated binary to the remote when the remote checks in
typedef struct {
   unsigned char                 api_revision;                                      ///< Revision of this API
   ctrlm_iarm_call_result_t      result;                                            ///< Result of the IARM call
   char                          firmwareLocation[CTRLM_DEVICE_UPDATE_PATH_LENGTH]; ///< location of update files
   char                          firmwareNames[CTRLM_DEVICE_UPDATE_PATH_LENGTH];    ///< file names of update files comma delimited
} ctrlm_device_update_iarm_call_update_available_t;

/// @brief Structure that describes an update image
/// @details This structure provides additional details about the image that is being used for an update.
typedef struct {
   ctrlm_device_update_image_id_t   image_id;                                             ///< Identifier of the image
   ctrlm_device_update_image_type_t image_type;                                           ///< The type of image
   unsigned char                    force_update;                                         ///< A flag indicating that the image must be updated regardless of software revision (1) or not (0).
   unsigned long                    image_size;                                           ///< The size of the image in bytes.
   char                             device_name[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH];  ///< The device's name string
   char                             device_class[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH]; ///< The class of the device ("remotes").
   char                             image_version[CTRLM_DEVICE_UPDATE_VERSION_LENGTH];    ///< The image's version string
   char                             image_file_path[CTRLM_DEVICE_UPDATE_PATH_LENGTH];     ///< The full path to the image file.
} ctrlm_device_update_image_t;

/// @brief Structure that describes an update image
/// @details This structure provides additional details about the image that is being used for an update.
typedef struct {
   unsigned char                    api_revision;                                         ///< Revision of this API
   ctrlm_iarm_call_result_t         result;                                               ///< Result of the IARM call
   ctrlm_device_update_image_id_t   image_id;                                             ///< Identifier of the image
   ctrlm_device_update_image_t      image;                                                ///< The image details
} ctrlm_device_update_iarm_call_image_t;

/// @brief Structure that describes an update device
/// @details This structure provides additional information about the hardware and software revisions of a device
/// that is being updated.
typedef struct {
   char name[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH];           ///< The name of the device
   char version_software[CTRLM_DEVICE_UPDATE_VERSION_LENGTH];   ///< The device's software version string
   char version_hardware[CTRLM_DEVICE_UPDATE_VERSION_LENGTH];   ///< The device's hardware version string
   char version_bootloader[CTRLM_DEVICE_UPDATE_VERSION_LENGTH]; ///< The device's bootloader version string
} ctrlm_device_update_device_t;

// Storage of state for a specific Update In Progress (session)
/// @brief Structure of Device Update's Session Get IARM call
/// @details This structure is used in the session get call. The session id field is an input parameter indicating
/// the session to get details for.  The rest of the parameters are output when the call is successful.
typedef struct {
   unsigned char                    api_revision;                                ///< Revision of this API
   ctrlm_iarm_call_result_t         result;                                      ///< Result of the IARM call
   ctrlm_device_update_session_id_t session_id;                                  ///< The session ID of the download. Corresponds to a specific download and a specific device.
   ctrlm_device_update_image_id_t   image_id;                                    ///< The identifier of the image that is being used for the update.
   ctrlm_device_update_device_t     device;                                      ///< Information about the device that is being updated.
   unsigned char                    interactive_download;                        ///< A flag indicating whether the download is interactive (1) or not (0).
   unsigned char                    interactive_load;                            ///< A flag indicating whether the load is interactive (1) or not (0).
   unsigned char                    download_percent;                            ///< The percentage of the download that has been completed in percent (0 - 100).
   unsigned char                    load_complete;                               ///< A flag indicating whether the load has completed (1) or not (0).
   int                              error_code;                                  ///< An error code associated with the update session.
} ctrlm_device_update_iarm_call_session_t;

/// @brief Structure of Device Update's Download Initiate IARM call
/// @details This call is made to initiate a download for a particular device. The download can be initiated as a background
/// download (meaning no impact to the device's normal operation) or an immediate download (move data to the device as fast as possible).
typedef struct {
   unsigned char                    api_revision;           ///< Revision of this API
   ctrlm_iarm_call_result_t         result;                 ///< Result of the IARM call
   ctrlm_device_update_session_id_t session_id;             ///< The session ID of the download. Corresponds to a specific download and a specific device.
   unsigned char                    background_download;    ///< Whether the background should be full speed or low speed in the background
   unsigned char                    percent_increment;      ///< Specifies the increment at which download status events are generated. If the value is 0x0, then an event will be sent at 0 and 100. If the value is 0xFF, then the default increment will be used.
   unsigned char                    load_image_immediately; ///< Indicates that the image should be loaded immediately after the download is complete.
} ctrlm_device_update_iarm_call_download_params_t;

/// @brief Structure of Device Update's Load Initiate IARM call
/// @details This call is made to initiate loading of an image for a particular device. The initiator can specify whether to load immediately (this would be expected if
/// the download was immediate), load after a given time and a given amount of inactivity, to poll repeatedly for updated instructions, or to abort an existing download session.
typedef struct {
   unsigned char                        api_revision;        ///< Revision of this API
   ctrlm_iarm_call_result_t             result;              ///< Result of the IARM call
   ctrlm_device_update_session_id_t     session_id;          ///< The session ID of the download. Corresponds to a specific download and a specific device.
   ctrlm_device_update_iarm_load_type_t load_type;           ///< The type of load to be used for the device
   unsigned int                         time_to_load;        ///< The amount of time (in seconds) to wait before loading the image.
   unsigned int                         time_after_inactive; ///< The amount of inactive time (in seconds) to wait before loading the image.
} ctrlm_device_update_iarm_call_load_params_t;

/// @brief Structure of Device Update's Ready to Download IARM event
/// @details This event is broadcast from the Device Update to inform listeners that a device has an available update.
typedef struct {
   unsigned char                    api_revision; ///< Revision of this API
   ctrlm_device_update_session_id_t session_id;   ///< The session ID of the download. Corresponds to a specific download and a specific device.
   ctrlm_device_update_image_id_t   image_id;     ///< The identifier of the image that is being used for the update.
   ctrlm_device_update_device_t     device;       ///< Information about the device that is being updated.
} ctrlm_device_update_iarm_event_ready_to_download_t;

/// @brief Structure of Device Update's Download Status IARM event
/// @details This event is broadcast from the Device Update to inform listeners of the status of the download.
/// The Device Update sends an initial status with percent complete (0) and a final status with percent complete (100).
/// This will notify listeners of the the beginning and end, respectively of the download.  The Device Update will send
/// send status updates at periods defined by the percent increment in the Download Initiate call.
typedef struct {
   unsigned char                    api_revision;     ///< Revision of this API
   ctrlm_device_update_session_id_t session_id;       ///< The session ID of the download. Corresponds to a specific download and a specific device.
   unsigned char                    percent_complete; ///< The percentage of the download that has completed in percent (0 - 100).
} ctrlm_device_update_iarm_event_download_status_t;

/// @brief Structure of Device Update's Load Begin IARM event
/// @details This event notifies listeners that the device has started to load the image.
typedef struct {
   unsigned char                     api_revision; ///< Revision of this API
   ctrlm_device_update_session_id_t  session_id;   ///< The session ID of the download. Corresponds to a specific download and a specific device.
} ctrlm_device_update_iarm_event_load_begin_t;

/// @brief Structure of Device Update's Load End IARM event
/// @details This event notifies listeners that the load has completed with the following result. Receiving
/// this event ends the update and invalidates the session id.
typedef struct {
   unsigned char                          api_revision; ///< Revision of this API
   ctrlm_device_update_session_id_t       session_id;    ///< The session ID of the download. Corresponds to a specific download and a specific device.
   ctrlm_device_update_iarm_load_result_t result;        ///< The result of the load operation.
} ctrlm_device_update_iarm_event_load_end_t;
/// @}

/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_CALLS
/// @{
/// @brief Remote Calls accessible via IARM bus
/// @details IARM calls are synchronous calls to functions provided by other IARM bus members. Each bus member can register
/// calls using the IARM_Bus_RegisterCall() function. Other members can invoke the call using the IARM_Bus_Call() function.
///
/// - - -
/// Call Registration
/// -----------------
///
/// The Device Update component of Control Manager registers the following calls.
///
/// | Bus Name                 | Call Name                                       | Argument                                     | Description |
/// | :-------                 | :--------                                       | :-------                                     | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_CALL_STATUS_GET        | ctrlm_device_update_iarm_status_t *          | Provides information about the status of device update |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_CALL_SESSION_GET       | ctrlm_device_update_iarm_session_t *         | Provides information about an update session |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_CALL_IMAGE_GET         | ctrlm_device_update_iarm_image_t *           | Provides information about an update image |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_CALL_DOWNLOAD_INITIATE | ctrlm_device_update_iarm_download_params_t * | Initiates an image download to a device |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_CALL_LOAD_INITIATE     | ctrlm_device_update_iarm_load_params_t *     | Initiates an image load on a device |
///
/// Examples:
///
/// Get Device Update status:
///
///     IARM_Result_t                     result;
///     ctrlm_device_update_iarm_status_t status;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_DEVICE_UPDATE_IARM_CALL_STATUS_GET, (void *)&status, sizeof(status));
///     if(IARM_RESULT_SUCCESS == result && CTRLM_IARM_CALL_RESULT_SUCCESS == status.result) {
///         // Status was successfully retrieved
///     }
///
/// Get Device Update session:
///
///     IARM_Result_t                      result;
///     ctrlm_device_update_iarm_session_t session;
///     session.session_id = 1; // The session id values are returned from status get call or ready to download event
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_DEVICE_UPDATE_IARM_CALL_SESSION_GET, (void *)&session, sizeof(session));
///     if(IARM_RESULT_SUCCESS == result && CTRLM_IARM_CALL_RESULT_SUCCESS == session.result) {
///         // Session was retrieved successfully
///     }
///     }
///
/// @}

/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_EVENTS
/// @{
/// @brief Broadcast Events accessible via IARM bus
/// @details The IARM bus uses events to broadcast information to interested clients. An event is sent separately to each client. There are no return values for an event and no
/// guarantee that a client will receive the event.  Each event has a different argument structure according to the information being transferred to the clients.  The events that the
/// Device Update component in Control Manager generates and subscribes to are detailed below.
///
/// - - -
/// Event Generation (Broadcast)
/// ----------------------------
///
/// The Device Update component generates events that can be received by other processes connected to the IARM bus. The following events
/// are registered during initialization:
///
/// | Bus Name                 | Event Name                                       | Argument                                             | Description |
/// | :-------                 | :---------                                       | :-------                                             | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_EVENT_READY_TO_DOWNLOAD | ctrlm_device_update_iarm_event_ready_to_download_t * | Generated when an update is detected for a device. |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_EVENT_DOWNLOAD_STATUS   | ctrlm_device_update_iarm_event_download_status_t *   | Generated at 0 and 100 percent and each time a download percent increment is reached. |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_EVENT_LOAD_BEGIN        | ctrlm_device_update_iarm_event_load_begin_t *        | Generated when a device begins to load an image |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_DEVICE_UPDATE_IARM_EVENT_LOAD_END          | ctrlm_device_update_iarm_event_load_end_t *          | Generated when a device completes loading an image |
///
/// IARM events are available on a subscription basis. In order to receive an event, a client must explicitly register to receive the event by calling
/// IARM_Bus_RegisterEventHandler() with the bus name, event name and a @link IARM_EventHandler_t handler function@endlink. Events may be generated at any time by the
/// Device Update component. All events are asynchronous.
///
/// Examples:
///
/// Register for a Device Update event:
///
///     IARM_Result_t result;
///
///     result = IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_DEVICE_UPDATE_IARM_EVENT_DOWNLOAD_STATUS, download_status_handler_func);
///     if(IARM_RESULT_SUCCESS == result) {
///         // Event registration was set successfully
///     }
///     }
///
/// @}
/// @addtogroup CTRLM_IPC_DEVICE_UPDATE_COMMS
/// @{
/// @brief Communication Interfaces
/// @details The following diagrams detail the main communication paths for the Device Update component.
/// @}
/// @}
#endif
