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

#define CTRLM_MAIN_IARM_BUS_NAME                                 "Ctrlm"                                ///< Control Manager's IARM Bus Name
#define CTRLM_MAIN_IARM_BUS_API_REVISION                         (16)                                   ///< Revision of the Control Manager Main IARM API

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
#define CTRLM_MAIN_IARM_CALL_STOP_PAIRING                        "Main_StopPairing"             ///< IARM Call to cancel an active search for a remote to pair with
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

#define CTRLM_ASB_ENABLED_DEFAULT                 (false)
#define CTRLM_OPEN_CHIME_ENABLED_DEFAULT          (false)
#define CTRLM_CLOSE_CHIME_ENABLED_DEFAULT         (true)
#define CTRLM_PRIVACY_CHIME_ENABLED_DEFAULT       (true)
#define CTRLM_CONVERSATIONAL_MODE_DEFAULT         (CTRLM_MAX_CONVERSATIONAL_MODE)
#define CTRLM_CHIME_VOLUME_DEFAULT                (CTRLM_CHIME_VOLUME_MEDIUM)
#define CTRLM_IR_COMMAND_REPEATS_DEFAULT          (3)

#define CTRLM_ASB_ENABLED_LEN                     (1)
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

typedef enum {
   CTRLM_IARM_CALL_RESULT_SUCCESS                 = 0, ///< The requested operation was completed successfully.
   CTRLM_IARM_CALL_RESULT_ERROR                   = 1, ///< An error occurred during the requested operation.
   CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY         = 2, ///< An error occurred trying to write to a read-only entity.
   CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER = 3, ///< An input parameter is invalid.
   CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION      = 4, ///< The API revision is invalid or no longer supported
   CTRLM_IARM_CALL_RESULT_ERROR_NOT_SUPPORTED     = 5, ///< The requested operation is not supported
   CTRLM_IARM_CALL_RESULT_INVALID                 = 6, ///< Invalid call result value
} ctrlm_iarm_call_result_t;

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
   CTRLM_RCU_IARM_EVENT_VALIDATION_STATUS           = 37, ///< Generated when the validation status changes
   CTRLM_MAIN_IARM_EVENT_MAX                        = 38  ///< Placeholder for the last event (used in registration)
} ctrlm_main_iarm_event_t;

typedef enum {
   CTRLM_KEY_STATUS_DOWN    = 0, ///< Key down
   CTRLM_KEY_STATUS_UP      = 1, ///< Key up
   CTRLM_KEY_STATUS_REPEAT  = 2, ///< Key repeat
   CTRLM_KEY_STATUS_INVALID = 3, ///< Invalid key status
} ctrlm_key_status_t;

typedef enum {
   CTRLM_KEY_SOURCE_FP = 0,   //< Key Source Front Panel
   CTRLM_KEY_SOURCE_IR,       //< Key Source Infrared
   CTRLM_KEY_SOURCE_RF,       //< Key Source RF
   CTRLM_KEY_SOURCE_INVALID   //< Invalid
} ctrlm_key_source_t;

typedef enum {
   CTRLM_ACCESS_TYPE_READ    = 0, ///< Read access
   CTRLM_ACCESS_TYPE_WRITE   = 1, ///< Write access
   CTRLM_ACCESS_TYPE_INVALID = 2  ///< Invalid access type
} ctrlm_access_type_t;

typedef enum {
   CTRLM_NETWORK_TYPE_RF4CE        = 0,   ///< RF4CE Network
   CTRLM_NETWORK_TYPE_BLUETOOTH_LE = 1,   ///< Bluetooth Low Energy Network
   CTRLM_NETWORK_TYPE_IP           = 2,   ///< IP Network
   CTRLM_NETWORK_TYPE_DSP          = 3,   ///< DSP Network
   CTRLM_NETWORK_TYPE_INVALID      = 255  ///< Invalid Network
} ctrlm_network_type_t;

typedef enum {
   CTRLM_UNBIND_REASON_CONTROLLER         = 0, ///< The controller initiated the unbind
   CTRLM_UNBIND_REASON_TARGET_USER        = 1, ///< The target initiated the unbind due to user request
   CTRLM_UNBIND_REASON_TARGET_NO_SPACE    = 2, ///< The target initiated the unbind due to lack of space in the pairing table
   CTRLM_UNBIND_REASON_FACTORY_RESET      = 3, ///< The target performed a factory reset
   CTRLM_UNBIND_REASON_CONTROLLER_RESET   = 4, ///< The controller performed a factory reset or RF reset
   CTRLM_UNBIND_REASON_INVALID_VALIDATION = 5, ///< A controller with an invalid validation was imported and needs to be unbinded
   CTRLM_UNBIND_REASON_MAX                = 6  ///< Maximum unbind reason value
} ctrlm_unbind_reason_t;

typedef enum {
   CTRLM_PAIRING_RESTRICT_NONE                = 0,   ///< No restrictions on pairing
   CTRLM_PAIRING_RESTRICT_TO_VOICE_REMOTES    = 1,   ///< Only pair voice remotes
   CTRLM_PAIRING_RESTRICT_TO_VOICE_ASSISTANTS = 2,   ///< Only pair voice assistants
   CTRLM_PAIRING_RESTRICT_MAX                 = 3    ///< Maximum restriction value
} ctrlm_pairing_restrict_by_remote_t;

typedef enum {
   CTRLM_PAIRING_MODE_BUTTON_BUTTON_BIND    = 0,   ///< Button Button binding
   CTRLM_PAIRING_MODE_SCREEN_BIND           = 1,   ///< Screen binding
   CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND   = 2,   ///< One Touch Auto binding
   CTRLM_PAIRING_MODE_MAX                   = 3,   ///< Maximum pairing mode value
} ctrlm_pairing_modes_t;

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

typedef enum
{
   CTRLM_CHIME_VOLUME_LOW,
   CTRLM_CHIME_VOLUME_MEDIUM,
   CTRLM_CHIME_VOLUME_HIGH,
   CTRLM_CHIME_VOLUME_INVALID,
} ctrlm_chime_volume_t;

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

typedef enum {
   CTRLM_AUDIO_CONTAINER_WAV     = 0,
   CTRLM_AUDIO_CONTAINER_NONE    = 1,
   CTRLM_AUDIO_CONTAINER_INVALID = 2
} ctrlm_audio_container_t;

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
   CTRLM_RCU_UPGRADE_STATE_RETRYING,
   CTRLM_RCU_UPGRADE_STATE_ERROR,
   CTRLM_RCU_UPGRADE_STATE_INVALID
} ctrlm_rcu_upgrade_state_t;

typedef unsigned char ctrlm_network_id_t;
typedef unsigned char ctrlm_controller_id_t;

typedef struct {
   ctrlm_network_id_t   id;   ///< identifier of the network
   ctrlm_network_type_t type; ///< Type of network
} ctrlm_network_t;

typedef struct {
   unsigned char            api_revision; ///< Revision of this API
   ctrlm_iarm_call_result_t result;       ///< Result of the operation
   ctrlm_network_id_t       network_id;   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   ctrlm_property_t         name;         ///< Property name on which this call will operate
   unsigned long            value;        ///< Value for this property
} ctrlm_main_iarm_call_property_t;

typedef struct {
   unsigned char            api_revision;                 ///< Revision of this API
   ctrlm_iarm_call_result_t result;                       ///< Result of the operation
   ctrlm_network_id_t       network_id;                   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char            enable;                       ///< Enable (1) or disable (0) open discovery
   unsigned char            require_line_of_sight;        ///< Require (1) or do not require (0) line of sight to respond to discovery requests
} ctrlm_main_iarm_call_discovery_config_t;

typedef struct {
   unsigned char            api_revision;                 ///< Revision of this API
   ctrlm_iarm_call_result_t result;                       ///< Result of the operation
   ctrlm_network_id_t       network_id;                   ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char            enable;                       ///< Enable (1) or disable (0) autobinding.
   unsigned char            threshold_pass;               ///< Number of successful pairing attempts required to complete autobinding successfully
   unsigned char            threshold_fail;               ///< Number of unsuccessful pairing attempts required to complete autobinding unsuccessfully
} ctrlm_main_iarm_call_autobind_config_t;

typedef struct {
   unsigned char            api_revision;                                  ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                        ///< Result of the operation
   ctrlm_network_id_t       network_id;                                    ///< IN - identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned long            controller_qty;                                ///< Number of precommissioned controllers
   unsigned long long       controllers[CTRLM_MAIN_MAX_BOUND_CONTROLLERS]; ///< IEEE Address for precommissioned controllers
} ctrlm_main_iarm_call_precommision_config_t;

typedef struct {
   unsigned char            api_revision;                           ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                 ///< Result of the IARM call
   ctrlm_network_id_t       network_id;                             ///< IN - identifier of network on which the controller is bound
   ctrlm_controller_id_t    controller_id;                          ///< IN - identifier of the controller
} ctrlm_main_iarm_call_controller_unbind_t;

typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the binding button status is active (1) or not active (0)
} ctrlm_main_iarm_event_binding_button_t;

typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the binding line of sight status is active (1) or not active (0)
} ctrlm_main_iarm_event_binding_line_of_sight_t;

typedef struct {
   unsigned char api_revision; ///< Revision of this API
   unsigned char active;       ///< Indicates that the autobind line of sight status is active (1) or not active (0)
} ctrlm_main_iarm_event_autobind_line_of_sight_t;

typedef struct {
   unsigned char         api_revision;  ///< Revision of this API
   ctrlm_network_id_t    network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t  network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t controller_id; ///< Identifier of the controller
   ctrlm_unbind_reason_t reason;        ///< Reason that the controller binding was removed
} ctrlm_main_iarm_event_controller_unbind_t;

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

typedef struct {
   unsigned char            api_revision;                                       ///< Revision of this API
   ctrlm_network_id_t       network_id;                                         ///< IN - identifier of network on which the controller is bound
   ctrlm_iarm_call_result_t result;                                             ///< OUT - The result of the operation.
   int                      controller_id;                                      ///< OUT - The controller id of the last key press.
   unsigned char            source_type;                                        ///< OUT - The source type of the last key press.
   unsigned long            source_key_code;                                    ///< OUT - The keycode of the last key press.
   long long                timestamp;                                          ///< OUT - The timestamp of the last key press.
   unsigned char            is_screen_bind_mode;                                ///< OUT - Indicates if the last key press is from a remote is in screen bind mode.
   unsigned char            remote_keypad_config;                               ///< OUT - The remote keypad configuration (Has Setup/NumberKeys).
   char                     source_name[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH];     ///< OUT - The source name of the last key press.
} ctrlm_main_iarm_call_last_key_info_t;

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

typedef struct {
   unsigned char                api_revision;                                   ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                         ///< Result of the IARM call
   unsigned char                is_supported;                                   ///< Read only Boolean value to indicate if findMyRemote is supported enable (non-zero) or not supported (zero)
   ctrlm_network_type_t         network_type;                                   ///< [in]  Type of network on which the controller is bound
} ctrlm_main_iarm_call_control_service_can_find_my_remote_t;

typedef struct {
   unsigned char                api_revision;                                   ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                         ///< Result of the IARM call
   ctrlm_network_id_t           network_id;                                     ///< Identifier of network or CTRLM_MAIN_NETWORK_ID_ALL for all networks
   unsigned char                pairing_mode;                                   ///< Indicates the pairing mode
   unsigned char                restrict_by_remote;                             ///< Indicates the remote bucket (no restrictions, only voice remotes, only voice assistants)
   unsigned char                use_timeout;                                    ///< Indicates whether to use a timeout for the pairing mode (0 - do not use timeout, otherwise use timeout)
   unsigned int                 bind_status;                                    ///< OUT - The bind status of the pairing session
} ctrlm_main_iarm_call_control_service_pairing_mode_t;

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

typedef struct {
   unsigned char            api_revision;   ///< Revision of this API
   ctrlm_iarm_call_result_t result;         ///< OUT - Result of the operation
   ctrlm_network_id_t       network_id;     ///< IN - identifier of network
   unsigned char            chip_connected; ///< OUT - 1 - chip connected, 0 - chip disconnected
} ctrlm_main_iarm_call_chip_status_t;

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

typedef struct {
   unsigned char api_revision;                                ///< Revision of this API
   char          result[CTRLM_MAIN_IARM_CALL_RESULT_LEN_MAX]; ///< OUT - Result of the operation formatted in JSON
   char          payload[];                                   ///< IN - Input params specific to RPC formatted in JSON (e.g. Network ID)
} ctrlm_main_iarm_call_json_t;

typedef struct {
   unsigned char api_revision; ///< Revision of this API
   char          payload[];    ///< OUT - Result of the operation formmated in JSON
} ctrlm_main_iarm_event_json_t;

#endif
