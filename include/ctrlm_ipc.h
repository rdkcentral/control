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
#ifndef _CTRLM_IPC_H_
#define _CTRLM_IPC_H_

#include "libIARM.h"
#include "libIBusDaemon.h"
#include "ctrlm_ipc_key_codes.h"

/// @file ctrlm_ipc.h
///
/// @defgroup CTRLM_IPC_MAIN IARM API - Control Manager
/// @{
///
/// @defgroup CTRLM_IPC_MAIN_COMMS       Communication Interfaces
/// @defgroup CTRLM_IPC_MAIN_CALLS       IARM Remote Procedure Calls
/// @defgroup CTRLM_IPC_MAIN_EVENTS      IARM Events
/// @defgroup CTRLM_IPC_MAIN_DEFINITIONS Constants
/// @defgroup CTRLM_IPC_MAIN_ENUMS       Enumerations
/// @defgroup CTRLM_IPC_MAIN_STRUCTS     Structures
///
/// @mainpage Control Manager IARM Interface
/// This document describes the interfaces that Control Manager uses to communicate with other components in the system.  It exposes
/// functionality to other processes in the system via the IARM bus. The IARM interface provides inter-process communication via Calls
/// and Events.  Control Manager supports one or more network devices via a Hardware Abstraction Layer (HAL).  The HAL API specifies the
/// method by which a network device communicates with the Control Manager.
///
/// This manual is divided into the following sections:
/// - @subpage CTRLM_IPC_MAIN_INTRO
/// - @subpage CTRLM_IPC_MAIN_COMMS
/// - @subpage CTRL_MGR
/// - @subpage CTRL_MGR_HAL
/// - @subpage CTRL_MGR_HAL_RF4CE
/// - @subpage CTRL_MGR_HAL_IP
/// - @subpage CTRL_MGR_HAL_BLE
///
/// @page CTRLM_IPC_MAIN_INTRO Introduction
/// The state of the Control Manager is summarized in the following diagram.
///
/// @dot
/// digraph CTRLMGR_StateMachine {
///     "INIT"  [shape="ellipse", fontname=Helvetica, fontsize=10];
///     "READY" [shape="ellipse", fontname=Helvetica, fontsize=10];
///     "TERM"  [shape="ellipse", fontname=Helvetica, fontsize=10];
///
///     "INIT"  -> "INIT"  [fontname=Helvetica, fontsize=10,label="  IARM Connect Failed (delay)"];
///     "INIT"  -> "READY" [fontname=Helvetica, fontsize=10,label="    "headlabel="IARM\rConnect",labeldistance=4.5, labelangle=25];
///     "READY" -> "INIT"  [fontname=Helvetica, fontsize=10,label="  IARM\l  Disconnect"];
///     "READY" -> "TERM"  [fontname=Helvetica, fontsize=10,label="  Control Manager Exited"];
///     "READY" -> "READY" [fontname=Helvetica, fontsize=10,label="  Process events and service calls"];
/// }
/// \enddot
///
///
/// Initialization
/// --------------
///
/// During initialization, the Control Manager initializes the IARM bus by calling IARM_Bus_Init() using its well known bus name, CTRLM_MAIN_IARM_BUS_NAME.
/// Upon successful initialization, the Control Manager connects to the bus by calling IARM_Bus_Connect().\n
///  If initialization is not completed successfully, the process will sleep for X seconds and retry the initialization procedure.
///
/// For debug purposes, the Control Manager may register to receive log messages from the IARM bus calling the IARM_Bus_RegisterForLog() function.
///
/// Termination
/// -----------
///
/// Before the Control Manager exits, it unregisters for events, disconnects from the IARM bus and terminates the session by calling IARM_Bus_UnRegisterEventHandler(), IARM_Bus_Disconnect()
/// and IARM_Bus_Term() respectively.
///
/// Normal Operation
/// ----------------
///
/// After successful initialization, the Control Manager will begin processing and generating @link CTRLM_IPC_MAIN_EVENTS events @endlink and @link CTRLM_IPC_MAIN_CALLS remote process calls @endlink (RPC).  Periodically, the status of the IARM bus connection is
/// checked by calling IARM_Bus_IsConnected().  If the result is not IARM_RESULT_SUCCESS, the Control Manager will return to the pre-initialized state.
///
/// @addtogroup CTRLM_IPC_MAIN_DEFINITIONS
/// @{
/// @brief Macros for constant values used by Control Manager clients
/// @details The Control Manager API provides macros for some parameters which may change in the future.  Clients should use
/// these names to allow the client code to function correctly if the values change.

#define CTRLM_MAIN_IARM_BUS_NAME                                 "Ctrlm"                                ///< Control Manager's IARM Bus Name
#define CTRLM_MAIN_IARM_BUS_API_REVISION                         (16)                                   ///< Revision of the Control Manager Main IARM API

#define CTRLM_MAIN_IARM_CALL_STATUS_GET                          "Main_StatusGet"                       ///< Retrieves Control Manager's Status information
#define CTRLM_MAIN_IARM_CALL_NETWORK_STATUS_GET                  "Main_NetworkStatusGet"                ///< Retrieves the network's Status information
#define CTRLM_MAIN_IARM_CALL_PROPERTY_SET                        "Main_PropertySet"                     ///< Sets a property of the Control Manager
#define CTRLM_MAIN_IARM_CALL_PROPERTY_GET                        "Main_PropertyGet"                     ///< Gets a property of the Control Manager
#define CTRLM_MAIN_IARM_CALL_DISCOVERY_CONFIG_SET                "Main_DiscoveryConfigSet"              ///< Sets the discovery settings
#define CTRLM_MAIN_IARM_CALL_AUTOBIND_CONFIG_SET                 "Main_AutobindConfigSet"               ///< Sets the autobind settings
#define CTRLM_MAIN_IARM_CALL_PRECOMMISSION_CONFIG_SET            "Main_PrecommissionConfigSet"          ///< Sets the pre-commission settings
#define CTRLM_MAIN_IARM_CALL_FACTORY_RESET                       "Main_FactoryReset"                    ///< Sets the configuration back to factory default
#define CTRLM_MAIN_IARM_CALL_CONTROLLER_UNBIND                   "Main_ControllerUnbind"                ///< Removes a binding between the target and the specified controller
#define CTRLM_MAIN_IARM_CALL_IR_REMOTE_USAGE_GET                 "Main_IrRemoteUsageGet"                ///< Retrieves the ir remote usage info
#define CTRLM_MAIN_IARM_CALL_LAST_KEY_INFO_GET                   "Main_LastKeyInfoGet"                  ///< Retrieves the last key info
#define CTRLM_MAIN_IARM_CALL_LAST_KEYPRESS_GET                   "Main_LastKeyPressGet"                 ///< Retrieves the last key press (TODO: replace CTRLM_MAIN_IARM_CALL_LAST_KEY_INFO_GET with this)
#define CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_SET_VALUES          "Main_ControlService_SetValues"        ///< IARM Call to set control service values
#define CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_GET_VALUES          "Main_ControlService_GetValues"        ///< IARM Call to get control service values
#define CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_CAN_FIND_MY_REMOTE  "Main_ControlService_CanFindMyRemote"  ///< IARM Call to get control service find my remote
#define CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_START_PAIRING_MODE  "Main_ControlService_StartPairingMode" ///< IARM Call to set control service start pairing mode
#define CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_END_PAIRING_MODE    "Main_ControlService_EndPairingMode"   ///< IARM Call to set control service end pairing mode
#define CTRLM_MAIN_IARM_CALL_PAIRING_METRICS_GET                 "Main_PairingMetricsGet"               ///< Retrieves the stb's pairing metrics
#define CTRLM_MAIN_IARM_CALL_CHIP_STATUS_GET                     "Main_ChipStatusGet"                   ///< get Chip status
#define CTRLM_MAIN_IARM_CALL_AUDIO_CAPTURE_START                 "Main_AudioCaptureStart"               ///< Sends message to xraudio to capture mic data, in specified container
#define CTRLM_MAIN_IARM_CALL_AUDIO_CAPTURE_STOP                  "Main_AudioCaptureStop"                ///< Sends message to xraudio to stop capturing mic data
#define CTRLM_MAIN_IARM_CALL_POWER_STATE_CHANGE                  "Main_PowerStateChange"                ///< Sends message to xr-speech-router to set power state, download DSP firmware, etc
// IARM calls for the IR Database
#define CTRLM_MAIN_IARM_CALL_IR_CODES                            "Main_IRCodes"           ///< IARM Call to retrieve IR Codes based on type, manufacturer, and model
#define CTRLM_MAIN_IARM_CALL_IR_MANUFACTURERS                    "Main_IRManufacturers"   ///< IARM Call to retrieve list of manufacturers, based on (partial) name
#define CTRLM_MAIN_IARM_CALL_IR_MODELS                           "Main_IRModels"          ///< IARM Call to retrieve list of models, based on (partial) name
#define CTRLM_MAIN_IARM_CALL_IR_AUTO_LOOKUP                      "Main_IRAutoLookup"      ///< IARM Call to retrieve IR Codes based on EDID, Infoframe, and CEC
#define CTRLM_MAIN_IARM_CALL_IR_SET_CODE                         "Main_IRSetCode"         ///< IARM Call to set an IR Code into a specified BLE remote
#define CTRLM_MAIN_IARM_CALL_IR_CLEAR_CODE                       "Main_IRClear"           ///< IARM Call to clear all IR Codes from a specified BLE remote
#define CTRLM_MAIN_IARM_CALL_IR_INITIALIZE                       "Main_IRInitialize"      ///< IARM Call to initialize the IR database

// For Remote Plugin, only used for BLE currently, refactoring needed in other networks to use this interface
#define CTRLM_MAIN_IARM_CALL_GET_RCU_STATUS                      "Main_GetRcuStatus"            ///< IARM Call get the RCU status info (same as what's provided by CTRLM_RCU_IARM_EVENT_RCU_STATUS)
#define CTRLM_MAIN_IARM_CALL_START_PAIRING                       "Main_StartPairing"            ///< IARM Call to initiate searching for a remote to pair with
#define CTRLM_MAIN_IARM_CALL_START_PAIR_WITH_CODE                "Main_StartPairWithCode"       ///< IARM Call to initiate searching for a remote to pair with
#define CTRLM_MAIN_IARM_CALL_FIND_MY_REMOTE                      "Main_FindMyRemote"            ///< IARM Call to trigger the Find My Remote alarm on a specified remote
#define CTRLM_MAIN_IARM_CALL_WRITE_RCU_WAKEUP_CONFIG             "Main_WriteAdvertisingConfig"  ///< IARM Call to write the advertising configuration on all connected remotes
#define CTRLM_MAIN_IARM_CALL_START_FIRMWARE_UPDATE               "Main_StartFirmwareUpdate"     ///< IARM Call to start a firmware update session
#define CTRLM_MAIN_IARM_CALL_CANCEL_FIRMWARE_UPDATE              "Main_CancelFirmwareUpdate"    ///< IARM Call to cancel a firmware update session
#define CTRLM_MAIN_IARM_CALL_STATUS_FIRMWARE_UPDATE              "Main_StatusFirmwareUpdate"    ///< IARM Call to get the status of a firmware update session
#define CTRLM_MAIN_IARM_CALL_UNPAIR                              "Main_Unpair"                  ///< IARM Call to unpair all or particular remotes

#define CTRLM_MAIN_NETWORK_ID_INVALID                          (0xFF) ///< An invalid network identifier
#define CTRLM_MAIN_CONTROLLER_ID_INVALID                       (0xFF) ///< An invalid controller identifier
#define CTRLM_MAIN_CONTROLLER_ID_DSP                           (0)    ///< Default voice controller identifier
#define CTRLM_MAIN_NETWORK_ID_DSP                              (0)    ///< Controllers start at 1 so 0 is available for DSP

#define CTRLM_MAIN_NETWORK_ID_ALL                              (0xFE) ///< Indicates that the command applies to all networks
#define CTRLM_MAIN_CONTROLLER_ID_ALL                           (0xFE) ///< Indicates that the command applies to all networks

#define CTRLM_MAIN_CONTROLLER_ID_LAST_USED                     (0xFD) ///< A last used controller identifier
#define CTRLM_MAIN_CONTROLLER_ID_IR                            (0xFC) ///< Infrared controller identifier

#define CTRLM_MAIN_VERSION_LENGTH                                (20) ///< Maximum length of the version string
#define CTRLM_MAIN_MAX_NETWORKS                                   (4) ///< Maximum number of networks
#define CTRLM_MAIN_MAX_BOUND_CONTROLLERS                          (9) ///< Maximum number of bound controllers
#define CTRLM_MAIN_MAX_CHIPSET_LENGTH                            (16) ///< Maximum length of chipset name string (including null termination)
#define CTRLM_MAIN_COMMIT_ID_MAX_LENGTH                          (48) ///< Maximum length of commit ID string (including null termination)
#define CTRLM_MAIN_RECEIVER_ID_MAX_LENGTH                        (40) ///< Maximum length of receiver ID string (including null termination)
#define CTRLM_MAIN_DEVICE_ID_MAX_LENGTH                          (24) ///< Maximum length of device ID string (including null termination)

#define CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON_VALUE_MIN               (5000) ///< Minimum active period (in ms) for button binding.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON_VALUE_MAX             (600000) ///< Maximum active period (in ms) for button binding.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND_VALUE_MIN           (5000) ///< Minimum active period (in ms) for screen bind.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND_VALUE_MAX         (600000) ///< Maximum active period (in ms) for screen bind.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND_VALUE_MIN   (5000) ///< Minimum active period (in ms) for screen bind.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND_VALUE_MAX (600000) ///< Maximum active period (in ms) for screen bind.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT_VALUE_MIN        (5000) ///< Minimum active period (in ms) for line of sight.
#define CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT_VALUE_MAX       (60000) ///< Maximum active period (in ms) for line of sight.

#define CTRLM_PROPERTY_VALIDATION_TIMEOUT_MIN                  (1000) ///< Validation timeout value minimum (in ms)
#define CTRLM_PROPERTY_VALIDATION_TIMEOUT_MAX                 (45000) ///< Validation timeout value maximum (in ms)
#define CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS_MAX               (20) ///< Maximum number of validation attempts

#define CTRLM_PROPERTY_CONFIGURATION_TIMEOUT_MIN               (1000) ///< Configuration timeout value minimum (in ms)
#define CTRLM_PROPERTY_CONFIGURATION_TIMEOUT_MAX              (60000) ///< Configuration timeout value maximum (in ms)

#define CTRLM_AUTOBIND_THRESHOLD_MIN                              (1) ///< Autobind threshold minimum value
#define CTRLM_AUTOBIND_THRESHOLD_MAX                              (7) ///< Autobind threshold maximum value

#define CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH                        (20) ///< Maximum length of source name string (including null termination)

// Bitmask defines for setting the available value in ctrlm_main_iarm_call_control_service_settings_t
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_ASB_ENABLED                 (0x01) ///< Setting to enable/disable asb
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_OPEN_CHIME_ENABLED          (0x02) ///< Setting to enable/disable open chime
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_CLOSE_CHIME_ENABLED         (0x04) ///< Setting to enable/disable close chime
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_PRIVACY_CHIME_ENABLED       (0x08) ///< Setting to enable/disable privacy chime
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_CONVERSATIONAL_MODE         (0x10) ///< Setting for conversational mode (0-6)
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_SET_CHIME_VOLUME            (0x20) ///< Setting to set the chime volume
#define CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_SET_IR_COMMAND_REPEATS      (0x40) ///< Setting to set the ir command repeats

#define CTRLM_MIN_CONVERSATIONAL_MODE (0)
#define CTRLM_MAX_CONVERSATIONAL_MODE (6)
#define CTRLM_MIN_IR_COMMAND_REPEATS  (1)
#define CTRLM_MAX_IR_COMMAND_REPEATS  (10)

#ifdef ASB
#define CTRLM_ASB_ENABLED_DEFAULT                 (false)
#endif
#define CTRLM_OPEN_CHIME_ENABLED_DEFAULT          (false)
#define CTRLM_CLOSE_CHIME_ENABLED_DEFAULT         (true)
#define CTRLM_PRIVACY_CHIME_ENABLED_DEFAULT       (true)
#define CTRLM_CONVERSATIONAL_MODE_DEFAULT         (CTRLM_MAX_CONVERSATIONAL_MODE)
#define CTRLM_CHIME_VOLUME_DEFAULT                (CTRLM_CHIME_VOLUME_MEDIUM)
#define CTRLM_IR_COMMAND_REPEATS_DEFAULT          (3)

#ifdef ASB
#define CTRLM_ASB_ENABLED_LEN                     (1)
#endif
#define CTRLM_OPEN_CHIME_ENABLED_LEN              (1)
#define CTRLM_CLOSE_CHIME_ENABLED_LEN             (1)
#define CTRLM_PRIVACY_CHIME_ENABLED_LEN           (1)
#define CTRLM_CONVERSATIONAL_MODE_LEN             (1)
#define CTRLM_CHIME_VOLUME_LEN                    (1)
#define CTRLM_IR_COMMAND_REPEATS_LEN              (1)

#define CTRLM_MAX_NUM_REMOTES               (4)
#define CTRLM_IEEE_ADDR_LEN                 (18)
#define CTRLM_MAX_PARAM_STR_LEN             (64)
#define CTRLM_MAIN_IARM_CALL_RESULT_LEN_MAX (10240)
#define CTRLM_MAX_TIME_STR_LEN              (20)

#define CTRLM_WAKEUP_CONFIG_LIST_MAX_SIZE (256)
#define CTRLM_RCU_ASSERT_REPORT_MAX_SIZE  (128)

/// @}

/// @addtogroup CTRLM_IPC_MAIN_ENUMS
/// @{
/// @brief Enumerated Types
/// @details The Control Manager provides enumerated types for logical groups of values.

/// @brief Remote Procedure Call Results
/// @details The structure for each remote call has a result member which is populated with the result of the operation.  This field is only valid
/// when the IARM return code indicates a successful call.
typedef enum {
   CTRLM_IARM_CALL_RESULT_SUCCESS                 = 0, ///< The requested operation was completed successfully.
   CTRLM_IARM_CALL_RESULT_ERROR                   = 1, ///< An error occurred during the requested operation.
   CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY         = 2, ///< An error occurred trying to write to a read-only entity.
   CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER = 3, ///< An input parameter is invalid.
   CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION      = 4, ///< The API revision is invalid or no longer supported
   CTRLM_IARM_CALL_RESULT_ERROR_NOT_SUPPORTED     = 5, ///< The requested operation is not supported
   CTRLM_IARM_CALL_RESULT_INVALID                 = 6, ///< Invalid call result value
} ctrlm_iarm_call_result_t;

/// @brief Control Manager Properties
/// @details The properties enumeration is used in calls to @link CTRLM_IPC_MAIN_CALLS PropertyGet @endlink and @link CTRLM_IPC_MAIN_CALLS PropertySet@endlink.  They are used to get/set values in Control Manager.
typedef enum {
   CTRLM_PROPERTY_BINDING_BUTTON_ACTIVE            =  0, ///< (RO) Boolean value indicating whether a front panel button was recently pressed (1) or not (0).
   CTRLM_PROPERTY_BINDING_SCREEN_ACTIVE            =  1, ///< (RW) Boolean value indicating whether the 'Pairing Description Screen' is being displayed (1) or not (0).
   CTRLM_PROPERTY_BINDING_LINE_OF_SIGHT_ACTIVE     =  2, ///< (RO) Boolean value indicating whether the STB has received the Line of Sight remote command and is within the active period.
   CTRLM_PROPERTY_AUTOBIND_LINE_OF_SIGHT_ACTIVE    =  3, ///< (RO) Boolean value indicating that the STB has received the Autobind Line of Sight remote code and is within the active period.
   CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON             =  4, ///< (RW) Active period (in ms) for button binding.
   CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT      =  5, ///< (RW) Active period (in ms) for line of sight.
   CTRLM_PROPERTY_VALIDATION_TIMEOUT_INITIAL       =  6, ///< (RW) Timeout value (in ms) used for the start of the validation period.
   CTRLM_PROPERTY_VALIDATION_TIMEOUT_DURING        =  7, ///< (RW) Timeout value (in ms) used during the validation period.
   CTRLM_PROPERTY_CONFIGURATION_TIMEOUT            =  8, ///< (RW) Timeout value (in ms) used during the configuration period.
   CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS          =  9, ///< (RW) Maximum number of validation attempts.
   CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND         = 10, ///< (RW) Active period (in ms) for screenbind.
   CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND = 11, ///< (RW) Active period (in ms) for one-touch autobind.
   CTRLM_PROPERTY_REMOTE_REVERSE_CMD_ACTIVE        = 12, ///< (RW) Boolean value indicating whether the 'Remote Reverse Command' feature is enabled (1) or not (0).
   CTRLM_PROPERTY_MAC_POLLING_INTERVAL             = 13, ///< (RW) MAC polling polling interval, in milliseconds.
   CTRLM_PROPERTY_RCU_REVERSE_CMD_TIMEOUT          = 14, ///< (RW) Find My Remote RC response timeout, Factor of CTRLM_PROPERTY_MAC_POLLING_INTERVAL, min 2
   CTRLM_PROPERTY_AUTO_ACK                         = 15, ///< (RW) Boolean value indicating whether the 'Automatic Packet Acknowledgment' feature is enabled (1) or not (0).
   CTRLM_PROPERTY_MAX                              = 16, ///< (NA) Maximum property enumeration value.
} ctrlm_property_t;

/// @brief Control Manager Events
/// @details The events enumeration defines a value for each event that can be generated by Control Manager.
typedef enum {
   CTRLM_MAIN_IARM_EVENT_BINDING_BUTTON             =  0, ///< Generated when a state change of the binding button status occurs
   CTRLM_MAIN_IARM_EVENT_BINDING_LINE_OF_SIGHT      =  1, ///< Generated when a state change of the line of sight status occurs
   CTRLM_MAIN_IARM_EVENT_AUTOBIND_LINE_OF_SIGHT     =  2, ///< Generated when a state change of the autobind line of sight status occurs
   CTRLM_MAIN_IARM_EVENT_CONTROLLER_UNBIND          =  3, ///< Generated when a controller binding is removed
   CTRLM_RCU_IARM_EVENT_KEY_PRESS                   =  4, ///< Generated each time a key event occurs (down, repeat, up)
   CTRLM_RCU_IARM_EVENT_VALIDATION_BEGIN            =  5, ///< Generated at the beginning of a validation attempt
   CTRLM_RCU_IARM_EVENT_VALIDATION_KEY_PRESS        =  6, ///< Generated when the user enters a validation code digit/letter
   CTRLM_RCU_IARM_EVENT_VALIDATION_END              =  7, ///< Generated at the end of a validation attempt
   CTRLM_RCU_IARM_EVENT_CONFIGURATION_COMPLETE      =  8, ///< Generated upon completion of controller configuration
   CTRLM_RCU_IARM_EVENT_FUNCTION                    =  9, ///< Generated when a function is performed on a controller
   CTRLM_RCU_IARM_EVENT_KEY_GHOST                   = 10, ///< Generated when a ghost code is received from a controller
   CTRLM_RCU_IARM_EVENT_RIB_ACCESS_CONTROLLER       = 11, ///< Generated when a controller accesses a RIB entry
   CTRLM_VOICE_IARM_EVENT_SESSION_BEGIN             = 12, ///< Voice session began
   CTRLM_VOICE_IARM_EVENT_SESSION_END               = 13, ///< Voice session ended
   CTRLM_VOICE_IARM_EVENT_SESSION_RESULT            = 14, ///< Result of a voice session
   CTRLM_VOICE_IARM_EVENT_SESSION_STATS             = 15, ///< Statistics from a voice session
   CTRLM_VOICE_IARM_EVENT_SESSION_ABORT             = 16, ///< Voice session was aborted (denied)
   CTRLM_VOICE_IARM_EVENT_SESSION_SHORT             = 17, ///< Voice session did not meet minimum duration
   CTRLM_VOICE_IARM_EVENT_MEDIA_SERVICE             = 18, ///< Voice session results in media service event
   CTRLM_DEVICE_UPDATE_IARM_EVENT_READY_TO_DOWNLOAD = 19, ///< Indicates that a device has an update available
   CTRLM_DEVICE_UPDATE_IARM_EVENT_DOWNLOAD_STATUS   = 20, ///< Provides status of a download that is in progress
   CTRLM_DEVICE_UPDATE_IARM_EVENT_LOAD_BEGIN        = 21, ///< Indicates that a device has started to load an image
   CTRLM_DEVICE_UPDATE_IARM_EVENT_LOAD_END          = 22, ///< Indicates that a device has completed an image load
   CTRLM_RCU_IARM_EVENT_BATTERY_MILESTONE           = 23, ///< Indicates that a battery milestone event occured
   CTRLM_RCU_IARM_EVENT_REMOTE_REBOOT               = 24, ///< Indicates that a remote reboot event occured
   CTRLM_RCU_IARM_EVENT_RCU_REVERSE_CMD_BEGIN       = 25, ///< Indicates that a RCU Reverse Command started
   CTRLM_RCU_IARM_EVENT_RCU_REVERSE_CMD_END         = 26, ///< Indicates that a RCU Reverse Command ended 
   CTRLM_RCU_IARM_EVENT_CONTROL                     = 27, ///< Generated when a control event is received from a controller
   CTRLM_VOICE_IARM_EVENT_JSON_SESSION_BEGIN        = 28, ///< Generated on voice session begin, payload is JSON for consumption by Thunder Plugin
   CTRLM_VOICE_IARM_EVENT_JSON_STREAM_BEGIN         = 29, ///< Generated on voice stream begin, payload is JSON for consumption by Thunder Plugin
   CTRLM_VOICE_IARM_EVENT_JSON_KEYWORD_VERIFICATION = 30, ///< Generated on voice keyword verification, payload is JSON for consumption by Thunder Plugin
   CTRLM_VOICE_IARM_EVENT_JSON_SERVER_MESSAGE       = 31, ///< Generated on voice server message, payload is JSON for consumption by Thunder Plugin
   CTRLM_VOICE_IARM_EVENT_JSON_STREAM_END           = 32, ///< Generated on voice stream end, payload is JSON for consumption by Thunder Plugin
   CTRLM_VOICE_IARM_EVENT_JSON_SESSION_END          = 33, ///< Generated on voice session end, payload is JSON for consumption by Thunder Plugin
   CTRLM_RCU_IARM_EVENT_RCU_STATUS                  = 34, ///< Generated when something changes in the BLE remote
   CTRLM_RCU_IARM_EVENT_RF4CE_PAIRING_WINDOW_TIMEOUT = 35, ///< Indicates that a battery milestone event occured
   CTRLM_RCU_IARM_EVENT_FIRMWARE_UPDATE_PROGRESS    = 36, ///< Generated when an milestone is reached for remote firmware upgrade 
   CTRLM_MAIN_IARM_EVENT_MAX                        = 37  ///< Placeholder for the last event (used in registration)
} ctrlm_main_iarm_event_t;

/// @brief Remote Control Key Status
/// @details The status of the key.
typedef enum {
   CTRLM_KEY_STATUS_DOWN    = 0, ///< Key down
   CTRLM_KEY_STATUS_UP      = 1, ///< Key up
   CTRLM_KEY_STATUS_REPEAT  = 2, ///< Key repeat
   CTRLM_KEY_STATUS_INVALID = 3, ///< Invalid key status
} ctrlm_key_status_t;

/// @brief Remote Control Key Source
/// @details The source of the key press, e.g. infrared, rf, etc.
typedef enum {
	CTRLM_KEY_SOURCE_FP = 0,   //< Key Source Front Panel
	CTRLM_KEY_SOURCE_IR,       //< Key Source Infrared
	CTRLM_KEY_SOURCE_RF,	      //< Key Source RF
	CTRLM_KEY_SOURCE_INVALID   //< Invalid
} ctrlm_key_source_t;

/// @brief Data Access Type
/// @details The type of permission for access to data in Control Manager.
typedef enum {
   CTRLM_ACCESS_TYPE_READ    = 0, ///< Read access
   CTRLM_ACCESS_TYPE_WRITE   = 1, ///< Write access
   CTRLM_ACCESS_TYPE_INVALID = 2  ///< Invalid access type
} ctrlm_access_type_t;

/// @brief Network Type
/// @details The Control Manager network type.
typedef enum {
   CTRLM_NETWORK_TYPE_RF4CE        = 0,   ///< RF4CE Network
   CTRLM_NETWORK_TYPE_BLUETOOTH_LE = 1,   ///< Bluetooth Low Energy Network
   CTRLM_NETWORK_TYPE_IP           = 2,   ///< IP Network
   CTRLM_NETWORK_TYPE_DSP          = 3,   ///< DSP Network
   CTRLM_NETWORK_TYPE_INVALID      = 255  ///< Invalid Network
} ctrlm_network_type_t;

/// @brief Controller Unbind Reason
/// @details When a controller binding is removed, the reason is defined by the value in this enumeration.
typedef enum {
   CTRLM_UNBIND_REASON_CONTROLLER         = 0, ///< The controller initiated the unbind
   CTRLM_UNBIND_REASON_TARGET_USER        = 1, ///< The target initiated the unbind due to user request
   CTRLM_UNBIND_REASON_TARGET_NO_SPACE    = 2, ///< The target initiated the unbind due to lack of space in the pairing table
   CTRLM_UNBIND_REASON_FACTORY_RESET      = 3, ///< The target performed a factory reset
   CTRLM_UNBIND_REASON_CONTROLLER_RESET   = 4, ///< The controller performed a factory reset or RF reset
   CTRLM_UNBIND_REASON_INVALID_VALIDATION = 5, ///< A controller with an invalid validation was imported and needs to be unbinded
   CTRLM_UNBIND_REASON_MAX                = 6  ///< Maximum unbind reason value
} ctrlm_unbind_reason_t;

/// @brief Pairing Restrictions
/// @details The Control Manager network type.
typedef enum {
   CTRLM_PAIRING_RESTRICT_NONE                = 0,   ///< No restrictions on pairing
   CTRLM_PAIRING_RESTRICT_TO_VOICE_REMOTES    = 1,   ///< Only pair voice remotes
   CTRLM_PAIRING_RESTRICT_TO_VOICE_ASSISTANTS = 2,   ///< Only pair voice assistants
   CTRLM_PAIRING_RESTRICT_MAX                 = 3    ///< Maximum restriction value
} ctrlm_pairing_restrict_by_remote_t;

/// @brief Pairing Restrictions
/// @details The Control Manager network type.
typedef enum {
   CTRLM_PAIRING_MODE_BUTTON_BUTTON_BIND    = 0,   ///< Button Button binding
   CTRLM_PAIRING_MODE_SCREEN_BIND           = 1,   ///< Screen binding
   CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND   = 2,   ///< One Touch Auto binding
   CTRLM_PAIRING_MODE_MAX                   = 3,   ///< Maximum pairing mode value
} ctrlm_pairing_modes_t;

/// @brief Pairing Restrictions
/// @details The Control Manager network type.
typedef enum
{
   CTRLM_BIND_STATUS_SUCCESS,
   CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST,
   CTRLM_BIND_STATUS_NO_PAIRING_REQUEST,
   CTRLM_BIND_STATUS_HAL_FAILURE,
   CTRLM_BIND_STATUS_CTRLM_BLACKOUT,
   CTRLM_BIND_STATUS_ASB_FAILURE,
   CTRLM_BIND_STATUS_STD_KEY_EXCHANGE_FAILURE,
   CTRLM_BIND_STATUS_PING_FAILURE,
   CTRLM_BIND_STATUS_VALILDATION_FAILURE,
   CTRLM_BIND_STATUS_RIB_UPDATE_FAILURE,
   CTRLM_BIND_STATUS_BIND_WINDOW_TIMEOUT,
   CTRLM_BIND_STATUS_UNKNOWN_FAILURE,
} ctrlm_bind_status_t;

/// @brief chime volume settings
/// @details The Control Manager network type.
typedef enum
{
   CTRLM_CHIME_VOLUME_LOW,
   CTRLM_CHIME_VOLUME_MEDIUM,
   CTRLM_CHIME_VOLUME_HIGH,
   CTRLM_CHIME_VOLUME_INVALID,
} ctrlm_chime_volume_t;

/// @brief IR device types
/// @details Types of IR devices supported by Control Manager
typedef enum {
   CTRLM_IR_DEVICE_TV = 0,
   CTRLM_IR_DEVICE_AMP,
   CTRLM_IR_DEVICE_UNKNOWN
} ctrlm_ir_device_type_t;

typedef enum {
   CTRLM_RF_PAIR_STATE_INITIALIZING = 0,        // starting up, no paired remotes
   CTRLM_RF_PAIR_STATE_IDLE,                    // no activity
   CTRLM_RF_PAIR_STATE_SEARCHING,               // device is searching for RCUs
   CTRLM_RF_PAIR_STATE_PAIRING,                 // device is pairing to an RCU
   CTRLM_RF_PAIR_STATE_COMPLETE,                // device successfully paired to an RCU
   CTRLM_RF_PAIR_STATE_FAILED,                  // device failed to find or pair to an RCU
   CTRLM_RF_PAIR_STATE_UNKNOWN                  // unknown status
} ctrlm_rf_pair_state_t;

typedef enum {
   CTRLM_IR_STATE_IDLE = 0,                // no activity
   CTRLM_IR_STATE_WAITING,                 // IR programming in progress
   CTRLM_IR_STATE_COMPLETE,                // IR programming completed successfully
   CTRLM_IR_STATE_FAILED,                  // IR programming failed
   CTRLM_IR_STATE_UNKNOWN                  // unknown status
} ctrlm_ir_state_t;

typedef enum {
   CTRLM_FMR_DISABLE = 0,
   CTRLM_FMR_LEVEL_MID ,
   CTRLM_FMR_LEVEL_HIGH,
   CTRLM_FMR_LEVEL_INVALID
} ctrlm_fmr_alarm_level_t;

/// @brief audio capture container settings
/// @details Audio container types
typedef enum {
   CTRLM_AUDIO_CONTAINER_WAV     = 0,
   CTRLM_AUDIO_CONTAINER_NONE    = 1,
   CTRLM_AUDIO_CONTAINER_INVALID = 2
} ctrlm_audio_container_t;

/// @brief Power State Type
/// @details Power Manager sends the current power state and the new power state. This type is used to track the state information.
typedef enum {
   CTRLM_POWER_STATE_STANDBY                = 0,  //Indicates power state transition
   CTRLM_POWER_STATE_ON                     = 1,
   CTRLM_POWER_STATE_DEEP_SLEEP             = 2,
   CTRLM_POWER_STATE_INVALID                = 3
}ctrlm_power_state_t;

typedef enum {
   CTRLM_RCU_WAKEUP_CONFIG_ALL = 0,
   CTRLM_RCU_WAKEUP_CONFIG_CUSTOM,
   CTRLM_RCU_WAKEUP_CONFIG_NONE,
   CTRLM_RCU_WAKEUP_CONFIG_INVALID
} ctrlm_rcu_wakeup_config_t;

typedef enum {
   CTRLM_RCU_UPGRADE_STATE_SUCCESS = 0,
   CTRLM_RCU_UPGRADE_STATE_IDLE,
   CTRLM_RCU_UPGRADE_STATE_PENDING,
   CTRLM_RCU_UPGRADE_STATE_CANCELED,
   CTRLM_RCU_UPGRADE_STATE_ERROR,
   CTRLM_RCU_UPGRADE_STATE_INVALID
} ctrlm_rcu_upgrade_state_t;

/// @brief Network Id Type
/// @details During initialization, of the HAL network, Control Manager will assign a unique id to the network.  It must be used in all
/// subsequent communication with the Control Manager.
typedef unsigned char ctrlm_network_id_t;
/// @brief Controller Id Type
/// @details When a controller is paired, the Control Manager will assign an id (typically 48/64 bit MAC address) to the controller.
typedef unsigned char ctrlm_controller_id_t;

/// @}
/// @addtogroup CTRLM_IPC_MAIN_STRUCTS
/// @{
/// @brief Structure definitions
/// @details The Control Manager provides structures that are used in IARM calls and events.

/// @brief Network Structure
/// @details The information for a network.
typedef struct {
   ctrlm_network_id_t   id;   ///< identifier of the network
   ctrlm_network_type_t type; ///< Type of network
} ctrlm_network_t;

/// @brief Control Manager Status Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_STATUS_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                                       ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                             ///< OUT - The result of the operation.
   unsigned char            network_qty;                                        ///< OUT - Number of networks connected to Control Manager
   ctrlm_network_t          networks[CTRLM_MAIN_MAX_NETWORKS];                  ///< OUT - List of networks
   char                     ctrlm_version[CTRLM_MAIN_VERSION_LENGTH];           ///< OUT - Software version of Control Manager
   char                     ctrlm_commit_id[CTRLM_MAIN_COMMIT_ID_MAX_LENGTH];   ///< OUT - Last commit ID of Control Manager
   char                     stb_device_id[CTRLM_MAIN_DEVICE_ID_MAX_LENGTH];     ///< OUT - Device ID obtained from the Set-Top Box
   char                     stb_receiver_id[CTRLM_MAIN_RECEIVER_ID_MAX_LENGTH]; ///< OUT - Receiver ID obtained from the Set-Top Box
} ctrlm_main_iarm_call_status_t;

/// @brief RF Channel Structure
/// @details The diagnostics information for an RF channel.
typedef struct {
   unsigned char number;  ///< RF channel number (15, 20 or 25 for RF4CE)
   unsigned char quality; ///< Quality indicator for this channel
} ctrlm_rf_channel_t;

/// @brief RF4CE Network Status Structure
/// @details The RF4CE Network Status structure provided detailed information about the network.
typedef struct {
   char                  version_hal[CTRLM_MAIN_VERSION_LENGTH];        ///< Software version of the HAL driver
   unsigned char         controller_qty;                                ///< Number of controllers bound to the target device
   ctrlm_controller_id_t controllers[CTRLM_MAIN_MAX_BOUND_CONTROLLERS]; ///< List of controllers bound to the target device
   unsigned short        pan_id;                                        ///< PAN Identifier
   ctrlm_rf_channel_t    rf_channel_active;                             ///< Current RF channel on which the target is operating
   unsigned long long    ieee_address;                                  ///< The 64-bit IEEE Address of the target device
   unsigned short        short_address;                                 ///< Short address (if applicable)
   char                  chipset[CTRLM_MAIN_MAX_CHIPSET_LENGTH];        ///< Chipset of the target
} ctrlm_network_status_rf4ce_t;

/// @brief Bluetooth LE Network Status Structure
/// @details The Bluetooth LE Network Status structure provided detailed information about the network.
typedef struct {
   char                  version_hal[CTRLM_MAIN_VERSION_LENGTH];        ///< Software version of the HAL driver
   unsigned char         controller_qty;                                ///< Number of controllers bound to the target device.
   ctrlm_controller_id_t controllers[CTRLM_MAIN_MAX_BOUND_CONTROLLERS]; ///< List of controllers bound to the target device
} ctrlm_network_status_ble_t;

/// @brief IP Network Status Structure
/// @details The IP Network Status structure provided detailed information about the network.
typedef struct {
   char                  version_hal[CTRLM_MAIN_VERSION_LENGTH];        ///< Software version of the HAL driver
   unsigned char         controller_qty;                                ///< Number of controllers bound to the target device.
   ctrlm_controller_id_t controllers[CTRLM_MAIN_MAX_BOUND_CONTROLLERS]; ///< List of controllers bound to the target device
} ctrlm_network_status_ip_t;

/// @brief Network Status Structure
/// @details The Network Status structure is used in the CTRLM_MAIN_IARM_CALL_NETWORK_STATUS_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision; ///< Revision of this API
   ctrlm_iarm_call_result_t result;       ///< OUT - Result of the operation
   ctrlm_network_id_t       network_id;   ///< IN - identifier of network
   union {
      ctrlm_network_status_rf4ce_t rf4ce; ///< OUT - RF4CE network status
      ctrlm_network_status_ble_t   ble;   ///< OUT - BLE network status
      ctrlm_network_status_ip_t    ip;    ///< OUT - IP network status
   } status;                              ///< OUT - Union of network status types
} ctrlm_main_iarm_call_network_status_t;

/// @brief Property Structure
/// @details The property structure is used in PropertyGet and PropertySet calls. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision; ///< Revision of this API
   ctrlm_iarm_call_result_t result;       ///< Result of the operation
   ctrlm_network_id_t       network_id;   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   ctrlm_property_t         name;         ///< Property name on which this call will operate
   unsigned long            value;        ///< Value for this property
} ctrlm_main_iarm_call_property_t;

/// @brief Discovery Config Call Structure
/// @details The Discovery Config Call structure is used in the CTRLM_IARM_CALL_DISCOVERY_CONFIG_SET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                 ///< Revision of this API
   ctrlm_iarm_call_result_t result;                       ///< Result of the operation
   ctrlm_network_id_t       network_id;                   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char            enable;                       ///< Enable (1) or disable (0) open discovery
   unsigned char            require_line_of_sight;        ///< Require (1) or do not require (0) line of sight to respond to discovery requests
} ctrlm_main_iarm_call_discovery_config_t;

/// @brief Autobind Config Call Structure
/// @details The Autobind Config Call structure is used in the CTRLM_IARM_CALL_AUTOBIND_CONFIG_SET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                 ///< Revision of this API
   ctrlm_iarm_call_result_t result;                       ///< Result of the operation
   ctrlm_network_id_t       network_id;                   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char            enable;                       ///< Enable (1) or disable (0) autobinding.
   unsigned char            threshold_pass;               ///< Number of successful pairing attempts required to complete autobinding successfully
   unsigned char            threshold_fail;               ///< Number of unsuccessful pairing attempts required to complete autobinding unsuccessfully
} ctrlm_main_iarm_call_autobind_config_t;

/// @brief Precommision Config Call Structure
/// @details The Precommission Config Call structure is used in the CTRLM_IARM_CALL_PRECOMMISSION_CONFIG_SET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                                  ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                        ///< Result of the operation
   ctrlm_network_id_t       network_id;                                    ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned long            controller_qty;                                ///< Number of precommissioned controllers
   unsigned long long       controllers[CTRLM_MAIN_MAX_BOUND_CONTROLLERS]; ///< IEEE Address for precommissioned controllers
} ctrlm_main_iarm_call_precommision_config_t;

/// @brief Controller Unbind Call Structure
/// @details This structure provides a method to remove a binding between the target and a controller.  For RF4CE controllers, only the target will be unbound since
/// there is no method to wake up the controller to unbind it.
typedef struct {
   unsigned char            api_revision;                           ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                 ///< Result of the IARM call
   ctrlm_network_id_t       network_id;                             ///< IN - identifier of network on which the controller is bound
   ctrlm_controller_id_t    controller_id;                          ///< IN - identifier of the controller
} ctrlm_main_iarm_call_controller_unbind_t;

/// @brief Structure of Control Manager's Binding Button IARM event
/// @details This event notifies listeners that a state change has occurred in the binding button status.
typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the binding button status is active (1) or not active (0)
} ctrlm_main_iarm_event_binding_button_t;

/// @brief Structure of Control Manager's Binding Line of Sight IARM event
/// @details This event notifies listeners that a state change has occurred in the binding line of sight status.
typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the binding line of sight status is active (1) or not active (0)
} ctrlm_main_iarm_event_binding_line_of_sight_t;

/// @brief Structure of Control Manager's Autobind Line of Sight IARM event
/// @details This event notifies listeners that a state change has occurred in the autobind line of sight status.
typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the autobind line of sight status is active (1) or not active (0)
} ctrlm_main_iarm_event_autobind_line_of_sight_t;

/// @brief Structure of Control Manager's Unbind IARM event
/// @details The unbind Event is generated whenever a controller is removed from the binding table.
typedef struct {
   unsigned char         api_revision;  ///< Revision of this API
   ctrlm_network_id_t    network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t  network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t controller_id; ///< Identifier of the controller
   ctrlm_unbind_reason_t reason;        ///< Reason that the controller binding was removed
} ctrlm_main_iarm_event_controller_unbind_t;

/// @brief Control Manager IR Remote Usage Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_IR_REMOTE_USAGE_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;             ///< Revision of this API
   ctrlm_iarm_call_result_t result;                   ///< OUT - The result of the operation.
   unsigned long            today;                    ///< OUT - The current day
   unsigned char            has_ir_xr2_yesterday;     ///< OUT - Boolean value indicating XR2 in IR mode was used the previous day
   unsigned char            has_ir_xr5_yesterday;     ///< OUT - Boolean value indicating XR5 in IR mode was used the previous day
   unsigned char            has_ir_xr11_yesterday;    ///< OUT - Boolean value indicating XR11 in IR mode was used the previous day
   unsigned char            has_ir_xr15_yesterday;    ///< OUT - Boolean value indicating XR15 in IR mode was used the previous day
   unsigned char            has_ir_xr2_today;         ///< OUT - Boolean value indicating XR2 in IR mode was used the current day
   unsigned char            has_ir_xr5_today;         ///< OUT - Boolean value indicating XR5 in IR mode was used the current day
   unsigned char            has_ir_xr11_today;        ///< OUT - Boolean value indicating XR11 in IR mode was used the current day
   unsigned char            has_ir_xr15_today;        ///< OUT - Boolean value indicating XR15 in IR mode was used the current day
   unsigned char            has_ir_remote_yesterday;  ///< OUT - Boolean value indicating remote in IR mode was used the previous day
   unsigned char            has_ir_remote_today;      ///< OUT - Boolean value indicating remote in IR mode was used the current day
} ctrlm_main_iarm_call_ir_remote_usage_t;

/// @brief Control Manager Last Key Info Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_LAST_KEY_INFO_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                                       ///< Revision of this API
   ctrlm_network_id_t       network_id;                                         ///< IN - identifier of network on which the controller is bound
   ctrlm_iarm_call_result_t result;                                             ///< OUT - The result of the operation.
   int                      controller_id;                                      ///< OUT - The controller id of the last key press.
   ctrlm_key_source_t       source_type;                                        ///< OUT - The source type of the last key press.
   unsigned long            source_key_code;                                    ///< OUT - The keycode of the last key press.
   long long                timestamp;                                          ///< OUT - The timestamp of the last key press.
   unsigned char            is_screen_bind_mode;                                ///< OUT - Indicates if the last key press is from a remote is in screen bind mode.
   unsigned char            remote_keypad_config;                               ///< OUT - The remote keypad configuration (Has Setup/NumberKeys).
   char                     source_name[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH];     ///< OUT - The source name of the last key press.
} ctrlm_main_iarm_call_last_key_info_t;

/// @brief Control Manager Control Service Settings Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_SET_VALUES call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char                api_revision;                                   ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                         ///< Result of the IARM call
   unsigned long                available;                                      ///< Bitmask indicating the settings that are available in this event
   unsigned char                asb_supported;                                  ///< Read only Boolean value to indicate asb supported enable (non-zero) or not supported (zero) asb
   unsigned char                asb_enabled;                                    ///< Boolean value to enable (non-zero) or disable (zero) asb
   unsigned char                open_chime_enabled;                             ///< Boolean value to enable (non-zero) or disable (zero) open chime
   unsigned char                close_chime_enabled;                            ///< Boolean value to enable (non-zero) or disable (zero) close chime
   unsigned char                privacy_chime_enabled;                          ///< Boolean value to enable (non-zero) or disable (zero) privacy chime
   unsigned char                conversational_mode;                            ///< Boolean value to set conversational mode (0-6)
   ctrlm_chime_volume_t         chime_volume;                                   ///< The chime volume
   unsigned char                ir_command_repeats;                             ///< The ir command repeats (1 - 10)
} ctrlm_main_iarm_call_control_service_settings_t;

/// @brief Control Manager Control Service Can Find My Remote Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_CAN_FIND_MY_REMOTE call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char                api_revision;                                   ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                         ///< Result of the IARM call
   unsigned char                is_supported;                                   ///< Read only Boolean value to indicate if findMyRemote is supported enable (non-zero) or not supported (zero)
   ctrlm_network_type_t         network_type;                                   ///< [in]  Type of network on which the controller is bound
} ctrlm_main_iarm_call_control_service_can_find_my_remote_t;

/// @brief Control Manager Control Service Pairing Mode Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_START_PAIRING_MODE call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.

typedef struct {
   unsigned char                api_revision;                                   ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                         ///< Result of the IARM call
   ctrlm_network_id_t           network_id;                                     ///< Identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char                pairing_mode;                                   ///< Indicates the pairing mode
   unsigned char                restrict_by_remote;                             ///< Indicates the remote bucket (no restrictions, only voice remotes, only voice assistants)
   unsigned int                 bind_status;                                    ///< OUT - The bind status of the pairing session
} ctrlm_main_iarm_call_control_service_pairing_mode_t;

/// @brief Control Manager Pairing Metrics Structure
/// @details The Control Manager Status structure is used in the CTRLM_MAIN_IARM_CALL_PAIRING_METRICS_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;                                                       ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                                             ///< OUT - The result of the operation.
   unsigned long            num_screenbind_failures;                                            ///< OUT - The total number of screenbind failures on this stb
   unsigned long            last_screenbind_error_timestamp;                                    ///< OUT - Timestamp of the last screenbind error
   ctrlm_bind_status_t      last_screenbind_error_code;                                         ///< OUT - The last screenbind error code
   char                     last_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH];     ///< OUT - The last screenbind error remote type
   unsigned long            num_non_screenbind_failures;                                        ///< OUT - The total number of screenbind failures on this stb
   unsigned long            last_non_screenbind_error_timestamp;                                ///< OUT - Timestamp of the last screenbind error
   ctrlm_bind_status_t      last_non_screenbind_error_code;                                     ///< OUT - The last screenbind error code
   unsigned char            last_non_screenbind_error_binding_type;                             ///< OUT - The last screenbind error binding type
   char                     last_non_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH]; ///< OUT - The last screenbind error remote type
} ctrlm_main_iarm_call_pairing_metrics_t;


typedef struct {
   ctrlm_controller_id_t      controller_id;                               ///< identifier of the controller, used for calls to a specific RCU
   char                       ieee_address_str[CTRLM_MAX_PARAM_STR_LEN];
   char                       serialno[CTRLM_MAX_PARAM_STR_LEN];
   int                        deviceid;
   char                       make[CTRLM_MAX_PARAM_STR_LEN];
   char                       model[CTRLM_MAX_PARAM_STR_LEN];
   char                       name[CTRLM_MAX_PARAM_STR_LEN];
   char                       btlswver[CTRLM_MAX_PARAM_STR_LEN];
   char                       hwrev[CTRLM_MAX_PARAM_STR_LEN];
   char                       rcuswver[CTRLM_MAX_PARAM_STR_LEN];
   char                       tv_code[CTRLM_MAX_PARAM_STR_LEN];
   char                       avr_code[CTRLM_MAX_PARAM_STR_LEN];
   unsigned char              connected;
   int                        batterylevel;
   int                        wakeup_key_code;
   ctrlm_rcu_wakeup_config_t  wakeup_config;
   int                        wakeup_custom_list[CTRLM_WAKEUP_CONFIG_LIST_MAX_SIZE];
   int                        wakeup_custom_list_size;
} ctrlm_rcu_data_t;

// This struct is used for the event (CTRLM_RCU_IARM_EVENT_RCU_STATUS) and get (CTRLM_MAIN_IARM_CALL_GET_RCU_STATUS)
typedef struct {
   unsigned char            api_revision;                    ///< Revision of this API
   ctrlm_network_id_t       network_id;                      ///< IN - Identifier of network
   ctrlm_rf_pair_state_t    status;
   ctrlm_ir_state_t         ir_state;
   int                      num_remotes;
   ctrlm_rcu_data_t         remotes[CTRLM_MAX_NUM_REMOTES];
   ctrlm_iarm_call_result_t result;
} ctrlm_iarm_RcuStatus_params_t;

typedef struct {
   unsigned char            api_revision;       ///< Revision of this API
   ctrlm_network_id_t       network_id;         ///< IN - Identifier of network
   unsigned int             key_code;           ///< IN - Key code from device
   unsigned int             pair_code;          ///< IN - Pairing code from device
   ctrlm_iarm_call_result_t result;             ///< OUT - return code of the operation
} ctrlm_iarm_call_StartPairWithCode_params_t;

typedef struct {
   unsigned char                 api_revision;
   ctrlm_network_id_t            network_id;
   ctrlm_controller_id_t         controller_id;
   ctrlm_rcu_wakeup_config_t     config;
   int                           customList[CTRLM_WAKEUP_CONFIG_LIST_MAX_SIZE];
   int                           customListSize;
   ctrlm_iarm_call_result_t      result;
} ctrlm_iarm_call_WriteRcuWakeupConfig_params_t;

/// @brief Chip Status Structure
/// @details The Chip Status structure is used in the CTRLM_MAIN_IARM_CALL_CHIP_STATUS_GET call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;   ///< Revision of this API
   ctrlm_iarm_call_result_t result;         ///< OUT - Result of the operation
   ctrlm_network_id_t       network_id;     ///< IN - identifier of network
   unsigned char            chip_connected; ///< OUT - 1 - chip connected, 0 - chip disconnected
} ctrlm_main_iarm_call_chip_status_t;

/// @brief Control Manager Audio Capture Structure
/// @details The Control Manager Audio Capture structure is used in the CTRLM_MAIN_IARM_CALL_AUDIO_CAPTURE_START call. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char            api_revision;
   ctrlm_iarm_call_result_t result;
   ctrlm_audio_container_t  container;
   char                     file_path[128];
   unsigned char            raw_mic_enable;
} ctrlm_main_iarm_call_audio_capture_t;

typedef struct {
   unsigned char api_revision;
   ctrlm_iarm_call_result_t result;
   ctrlm_power_state_t new_state;
} ctrlm_main_iarm_call_power_state_change_t;

/// @brief Control Manager General Thunder IARM IPC Call Structure
/// @details The Control Manager General Thunder IARM IPC Call structure is used by multiple CTRLM_MAIN_IARM calls. See the @link CTRLM_IPC_MAIN_CALLS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char api_revision;                                ///< Revision of this API
   char          result[CTRLM_MAIN_IARM_CALL_RESULT_LEN_MAX]; ///< OUT - Result of the operation formatted in JSON
   char          payload[];                                   ///< IN - Input params specific to RPC formatted in JSON (e.g. Network ID)
} ctrlm_main_iarm_call_json_t;

/// @brief Control Manager General Thunder IARM IPC Event Structure
/// @details The Control Manager General Thunder IARM IPC Event structure is used by multiple CTRLM_MAIN_IARM events. See the @link CTRLM_IPC_MAIN_EVENTS Calls@endlink section for more details on invoking this call.
typedef struct {
   unsigned char api_revision; ///< Revision of this API
   char          payload[];    ///< OUT - Result of the operation formmated in JSON
} ctrlm_main_iarm_event_json_t;

/// @}

/// @addtogroup CTRLM_IPC_MAIN_EVENTS Events
/// @{
/// @brief Broadcast Events accessible via IARM bus
/// @details The iARM bus uses events to broadcast information to interested clients. An event is sent separately to each client. There are no return values for an event and no
/// guarantee that a client will receive the event.  Each event has a different argument structure according to the information being transferred to the clients.  The
/// Control Manager generates and subscribes to are detailed below.
///
/// - - -
/// Event Generation (Broadcast)
/// ----------------------------
///
/// The Control Manager generates events that can be received by other processes connected to the IARM bus. The following events
/// are registered during initialization:
///
/// | Bus Name                 | Event Name                                   | Argument                                       | Description |
/// | :-------                 | :---------                                   | :-------                                       | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_MAIN_IARM_EVENT_BINDING_BUTTON         | ctrlm_main_iarm_event_binding_button_t         | Generated when a state change of the binding button status occurs |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_MAIN_IARM_EVENT_BINDING_LINE_OF_SIGHT  | ctrlm_main_iarm_event_binding_line_of_sight_t  | Generated when a state change of the line of sight status occurs |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_MAIN_IARM_EVENT_AUTOBIND_LINE_OF_SIGHT | ctrlm_main_iarm_event_autobind_line_of_sight_t | Generated when a state change of the autobind line of sight status occurs |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_MAIN_IARM_EVENT_CONTROLLER_UNBIND      | ctrlm_main_iarm_event_controller_unbind_t      | Generated when a controller binding is removed |
///
/// IARM events are available on a subscription basis. In order to receive an event, a client must explicitly register to receive the event by calling
/// IARM_Bus_RegisterEventHandler() with the Control Manager bus name, event name and a @link IARM_EventHandler_t handler function @endlink. Events may be generated at any time by the
/// Control Manager. All events are asynchronous.
///
/// Examples:
///
/// Register for a Control Manager event:
///
///     IARM_Result_t result;
///
///     result = IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_MAIN_IARM_EVENT_KEY_PRESS, key_handler_func);
///     if(IARM_RESULT_SUCCESS == result) {
///         // Event registration was set successful
///     }
///
/// @}
///
/// @addtogroup CTRLM_IPC_MAIN_CALLS Remote Procedure Calls
/// @{
/// @brief Remote Calls accessible via IARM bus
/// @details IARM calls are synchronous calls to functions provided by other IARM bus members. Each bus member can register
/// calls using the IARM_Bus_RegisterCall() function. Other members can invoke the call using the IARM_Bus_Call() function.
///
/// - - -
/// Call Registration
/// -----------------
///
/// The Control Manager registers the following calls.
///
/// | Bus Name                 | Call Name                                | Argument                                   | Description |
/// | :-------                 | :--------                                | :-------                                   | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_STATUS_GET               | ctrlm_main_iarm_call_status_t              | Retrieves Control Manager's Status information |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_NETWORK_STATUS_GET       | ctrlm_main_iarm_call_network_status_t      | Retrieves the specified network's Status information |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_PROPERTY_SET             | ctrlm_main_iarm_call_property_t            | Sets a property of the Control Manager |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_PROPERTY_GET             | ctrlm_main_iarm_call_property_t            | Gets a property of the Control Manager |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_UNIQUE_REFERENCE_ID_SET  | ctrlm_main_iarm_call_unique_reference_id_t | Sets the unique reference id for the STB |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_AUTOBIND_CONFIG_SET      | ctrlm_main_iarm_call_autobind_config_t     | Sets the configuration settings for autobind |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_PRECOMMISSION_CONFIG_SET | ctrlm_main_iarm_call_precommision_config_t | Sets the pre-commission settings |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_FACTORY_RESET            | ctrlm_main_iarm_call_json_t                | Resets settings to factory default for the specified network |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_CONTROLLER_UNBIND        | ctrlm_main_iarm_call_controller_unbind_t   | Removes a controller from the specified network |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_LAST_KEYPRESS_GET        | ctrlm_main_iarm_call_json_t                | Returns last key pressed from a controller on a specified network |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_START_PAIRING            | ctrlm_main_iarm_call_json_t                | Initiates the pairing process for a specified network |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_IARM_CALL_FIND_MY_REMOTE           | ctrlm_main_iarm_call_json_t                | Initiates find my remote for controllers on a specified network |
///
/// Examples:
///
/// Set a Control Manager property:
///
///     IARM_Result_t    result;
///     ctrlm_property_t property;
///     property.name  = CTRLM_PROPERTY_CLASS_DESC_LINE_OF_SIGHT;
///     property.value = 1;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_IARM_CALL_PROPERTY_SET, (void *)&property, sizeof(property));
///     if(IARM_RESULT_SUCCESS == result && CTRLMGR_RESULT_SUCCESS == property.result) {
///         // Property was set successfully
///     }
///
/// Get a Control Manager property:
///
///     IARM_Result_t    result;
///     ctrlm_property_t property;
///     property.name  = CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_IARM_CALL_PROPERTY_GET, (void *)&property, sizeof(property));
///     if(IARM_RESULT_SUCCESS == result && CTRLMGR_RESULT_SUCCESS == property.result) {
///         // Property was retrieved successfully
///     }
///
/// Get Network Status information:
///
///     IARM_Result_t     result;
///     ctrlm_rf_status_t status;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_IARM_CALL_NETWORK_STATUS_GET, (void *)&status, sizeof(status));
///     if(IARM_RESULT_SUCCESS == result && CTRLMGR_RESULT_SUCCESS == status.result) {
///         // Process Status
///     }
///
/// @}


/// @}

#endif
