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

#ifndef _CTRLM_IPC_RCU_H_
#define _CTRLM_IPC_RCU_H_

#define CTRLM_RCU_IARM_CALL_CONTROLLER_STATUS            "Rcu_ControllerStatus"     ///< IARM Call to get controller information
#define CTRLM_RCU_IARM_CALL_CONTROLLER_LINK_KEY          "Rcu_ControllerLinkKey"    ///< IARM Call to get controller link key
#define CTRLM_RCU_IARM_CALL_RF4CE_POLLING_ACTION         "Rcu_Rf4cePollingAction"   ///< IARM Call to Send Remote Heartbeat Response Polling Action

#define CTRLM_RCU_IARM_BUS_API_REVISION                  (13)    ///< Revision of the RCU IARM API
#define CTRLM_RCU_VALIDATION_KEY_QTY                      (3)    ///< Number of validation keys used in internal validation
#define CTRLM_RCU_MAX_SETUP_COMMAND_SIZE                  (5)    ///< Maximum setup command size (numbers entered after setup key)
#define CTRLM_RCU_VERSION_LENGTH                         (18)    ///< Maximum length of the version string
#define CTRLM_RCU_BUILD_ID_LENGTH                        (93)    ///< Maximum length of the build id string
#define CTRLM_RCU_DSP_BUILD_ID_LENGTH                    (93)    ///< Maximum length of the dsp build id string
#define CTRLM_RCU_MAX_USER_STRING_LENGTH                  (9)    ///< Maximum length of remote user string (including null termination)
#define CTRLM_RCU_MAX_IR_DB_CODE_LENGTH                   (7)    ///< Maximum length of an IR DB code string (including null termination)
#define CTRLM_RCU_MAX_MANUFACTURER_LENGTH                (16)    ///< Maximum length of manufacturer name string (including null termination)
#define CTRLM_RCU_MAX_CHIPSET_LENGTH                     (16)    ///< Maximum length of chipset name string (including null termination)
#define CTRLM_RCU_CALL_RCU_REVERSE_CMD_PARAMS_MAX        (10)    ///< Maximum number of parameters for CTRLM_RCU_IARM_CALL_REVERSE_CMD call
#define CTRLM_RCU_MAX_EVENT_SOURCE_LENGTH                (10)    ///< Maximum length of the event source (including null termination)
#define CTRLM_RCU_MAX_EVENT_TYPE_LENGTH                  (20)    ///< Maximum length of the event type (including null termination)
#define CTRLM_RCU_MAX_EVENT_DATA_LENGTH                  (50)    ///< Maximum length of the event data (including null termination)

#define CTRLM_RCU_RIB_ATTR_LEN_PERIPHERAL_ID              (4)    ///< RIB Attribute Length - Peripheral Id
#define CTRLM_RCU_RIB_ATTR_LEN_RF_STATISTICS             (16)    ///< RIB Attribute Length - RF Statistics
#define CTRLM_RCU_RIB_ATTR_LEN_VERSIONING                 (4)    ///< RIB Attribute Length - Versioning
#define CTRLM_RCU_RIB_ATTR_LEN_VERSIONING_BUILD_ID       (92)    ///< RIB Attribute Length - Versioning (Build ID)
#define CTRLM_RCU_RIB_ATTR_LEN_BATTERY_STATUS            (11)    ///< RIB Attribute Length - Battery Status
#define CTRLM_RCU_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD      (4)    ///< RIB Attribute Length - Short RF Retry Period
#define CTRLM_RCU_RIB_ATTR_LEN_POLLING_METHODS            (1)    ///< RIB Attribute Length - Polling Methods
#define CTRLM_RCU_RIB_ATTR_LEN_POLLING_CONFIGURATION      (8)    ///< RIB Attribute Length - Polling Configuration
#define CTRLM_RCU_RIB_ATTR_LEN_PRIVACY                    (1)    ///< RIB Attribute Length - Privacy
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_COMMAND_STATUS       (1)    ///< RIB Attribute Length - Voice Command Status
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_COMMAND_LENGTH       (1)    ///< RIB Attribute Length - Voice Command Length
#define CTRLM_RCU_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH   (2)    ///< RIB Attribute Length - Maximum Utterance Length
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION   (1)    ///< RIB Attribute Length - Voice Command Encryption
#define CTRLM_RCU_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY       (1)    ///< RIB Attribute Length - Voice Data Retry
#define CTRLM_RCU_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF     (1)    ///< RIB Attribute Length - Maximum Voice CSMA Backoff
#define CTRLM_RCU_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF     (1)    ///< RIB Attribute Length - Minimum Voice Data Backoff
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_CTRL_AUDIO_PROFILES  (2)    ///< RIB Attribute Length - Voice Controller Audio Profiles
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_TARG_AUDIO_PROFILES  (2)    ///< RIB Attribute Length - Voice Target Audio Profiles
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_STATISTICS           (8)    ///< RIB Attribute Length - Voice Statistics
#define CTRLM_RCU_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS       (5)    ///< RIB Attribute Length - OPUS Encoding Params
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_SESSION_QOS          (7)    ///< RIB Attribute Length - Voice Session QOS
#define CTRLM_RCU_RIB_ATTR_LEN_RIB_ENTRIES_UPDATED        (1)    ///< RIB Attribute Length - Entries Updated
#define CTRLM_RCU_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL  (2)    ///< RIB Attribute Length - Update Check Interval
#define CTRLM_RCU_RIB_ATTR_LEN_VOICE_SESSION_STATISTICS  (16)    ///< RIB Attribute Length - Voice Session Statistics
#define CTRLM_RCU_RIB_ATTR_LEN_UPDATE_VERSIONING          (4)    ///< RIB Attribute Length - Update Versioning
#define CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME              (20)    ///< RIB Attribute Length - Product Name
#define CTRLM_RCU_RIB_ATTR_LEN_DOWNLOAD_RATE              (1)    ///< RIB Attribute Length - Download Rate
#define CTRLM_RCU_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD      (2)    ///< RIB Attribute Length - Update Polling Period
#define CTRLM_RCU_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME     (2)    ///< RIB Attribute Length - Data Request Wait Time
#define CTRLM_RCU_RIB_ATTR_LEN_IR_RF_DATABASE_STATUS      (1)    ///< RIB Attribute Length - IR RF Database Status
#define CTRLM_RCU_RIB_ATTR_LEN_IR_RF_DATABASE            (92)    ///< RIB Attribute Length - IR RF Database
#define CTRLM_RCU_RIB_ATTR_LEN_VALIDATION_CONFIGURATION   (4)    ///< RIB Attribute Length - Validation Configuration
#define CTRLM_RCU_RIB_ATTR_LEN_CONTROLLER_IRDB_STATUS    (15)    ///< RIB Attribute Length - Controller IRDB Status
#define CTRLM_RCU_RIB_ATTR_LEN_TARGET_IRDB_STATUS        (13)    ///< RIB Attribute Length - Target IRDB Status
#define CTRLM_RCU_RIB_ATTR_LEN_MFG_TEST                   (8)    ///< RIB Attribute Length - MFG Test
#define CTRLM_RCU_RIB_ATTR_LEN_MFG_TEST_HAPTICS          (12)    ///< RIB Attribute Length - MFG Test with haptics
#define CTRLM_RCU_RIB_ATTR_LEN_MFG_TEST_RESULT            (1)    ///< RIB Attribute Length - MFG Security Key Test Rib Result

#define CTRLM_RCU_POLLING_RESPONSE_DATA_LEN               (5)

typedef enum {
   CTRLM_RCU_VALIDATION_RESULT_SUCCESS          =  0, ///< The validation completed successfully.
   CTRLM_RCU_VALIDATION_RESULT_PENDING          =  1, ///< The validation is still pending.
   CTRLM_RCU_VALIDATION_RESULT_TIMEOUT          =  2, ///< The validation has exceeded the timeout period.
   CTRLM_RCU_VALIDATION_RESULT_COLLISION        =  3, ///< The validation resulted in a collision (key was received from a different remote than the one being validated).
   CTRLM_RCU_VALIDATION_RESULT_FAILURE          =  4, ///< The validation did not complete successfully (communication failures).
   CTRLM_RCU_VALIDATION_RESULT_ABORT            =  5, ///< The validation was aborted (infinity key from remote being validated).
   CTRLM_RCU_VALIDATION_RESULT_FULL_ABORT       =  6, ///< The validation was fully aborted (exit key from remote being validated).
   CTRLM_RCU_VALIDATION_RESULT_FAILED           =  7, ///< The validation has failed (ie. did not put in correct code, etc).
   CTRLM_RCU_VALIDATION_RESULT_BIND_TABLE_FULL  =  8, ///< The validation has failed due to lack of space in the binding table.
   CTRLM_RCU_VALIDATION_RESULT_IN_PROGRESS      =  9, ///< The validation has failed because another validation is in progress.
   CTRLM_RCU_VALIDATION_RESULT_CTRLM_RESTART    = 10, ///< The validation has failed because of restarting controlMgr.
   CTRLM_RCU_VALIDATION_RESULT_MAX              = 11  ///< Maximum validation result value
} ctrlm_rcu_validation_result_t;

typedef enum {
   CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS = 0, ///< The configuration completed successfully.
   CTRLM_RCU_CONFIGURATION_RESULT_PENDING = 1, ///< The configuration is still pending.
   CTRLM_RCU_CONFIGURATION_RESULT_TIMEOUT = 2, ///< The configuration has exceeded the timeout period.
   CTRLM_RCU_CONFIGURATION_RESULT_FAILURE = 4, ///< The configuration did not complete successfully.
   CTRLM_RCU_CONFIGURATION_RESULT_MAX     = 5  ///< Maximum validation result value
} ctrlm_rcu_configuration_result_t;

typedef enum {
   CTRLM_RCU_RIB_ATTR_ID_PERIPHERAL_ID             = 0x00, ///< RIB Attribute - Peripheral Id
   CTRLM_RCU_RIB_ATTR_ID_RF_STATISTICS             = 0x01, ///< RIB Attribute - RF Statistics
   CTRLM_RCU_RIB_ATTR_ID_VERSIONING                = 0x02, ///< RIB Attribute - Versioning
   CTRLM_RCU_RIB_ATTR_ID_BATTERY_STATUS            = 0x03, ///< RIB Attribute - Battery Status
   CTRLM_RCU_RIB_ATTR_ID_SHORT_RF_RETRY_PERIOD     = 0x04, ///< RIB Attribute - Short RF Retry Period
   CTRLM_RCU_RIB_ATTR_ID_TARGET_ID_DATA            = 0x05, ///< RIB Attribute - Target ID Data
   CTRLM_RCU_RIB_ATTR_ID_POLLING_METHODS           = 0x08, ///< RIB Attribute - Polling Methods
   CTRLM_RCU_RIB_ATTR_ID_POLLING_CONFIGURATION     = 0x09, ///< RIB Attribute - Polling Configuration
   CTRLM_RCU_RIB_ATTR_ID_PRIVACY                   = 0x0B, ///< RIB Attribute - Privacy
   CTRLM_RCU_RIB_ATTR_ID_CONTROLLER_CAPABILITIES   = 0x0C, ///< RIB Attribute - Controller Capabilities
   CTRLM_RCU_RIB_ATTR_ID_RESPONSE_TIME             = 0x0D, ///< RIB Attribute - Response Time
   CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_STATUS      = 0x10, ///< RIB Attribute - Voice Command Status
   CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_LENGTH      = 0x11, ///< RIB Attribute - Voice Command Length
   CTRLM_RCU_RIB_ATTR_ID_MAXIMUM_UTTERANCE_LENGTH  = 0x12, ///< RIB Attribute - Maximum Utterance Length
   CTRLM_RCU_RIB_ATTR_ID_VOICE_COMMAND_ENCRYPTION  = 0x13, ///< RIB Attribute - Voice Command Encryption
   CTRLM_RCU_RIB_ATTR_ID_MAX_VOICE_DATA_RETRY      = 0x14, ///< RIB Attribute - Voice Data Retry
   CTRLM_RCU_RIB_ATTR_ID_MAX_VOICE_CSMA_BACKOFF    = 0x15, ///< RIB Attribute - Maximum Voice CSMA Backoff
   CTRLM_RCU_RIB_ATTR_ID_MIN_VOICE_DATA_BACKOFF    = 0x16, ///< RIB Attribute - Minimum Voice Data Backoff
   CTRLM_RCU_RIB_ATTR_ID_VOICE_CTRL_AUDIO_PROFILES = 0x17, ///< RIB Attribute - Voice Controller Audio Profiles
   CTRLM_RCU_RIB_ATTR_ID_VOICE_TARG_AUDIO_PROFILES = 0x18, ///< RIB Attribute - Voice Target Audio Profiles
   CTRLM_RCU_RIB_ATTR_ID_VOICE_STATISTICS          = 0x19, ///< RIB Attribute - Voice Statistics
   CTRLM_RCU_RIB_ATTR_ID_RIB_ENTRIES_UPDATED       = 0x1A, ///< RIB Attribute - Entries Updated
   CTRLM_RCU_RIB_ATTR_ID_RIB_UPDATE_CHECK_INTERVAL = 0x1B, ///< RIB Attribute - Update Check Interval
   CTRLM_RCU_RIB_ATTR_ID_VOICE_SESSION_STATISTICS  = 0x1C, ///< RIB Attribute - Voice Session Statistics
   CTRLM_RCU_RIB_ATTR_ID_OPUS_ENCODING_PARAMS      = 0x1D, ///< RIB Attribute - OPUS Encoding Params
   CTRLM_RCU_RIB_ATTR_ID_VOICE_SESSION_QOS         = 0x1E, ///< RIB Attribute - Voice Session QOS
   CTRLM_RCU_RIB_ATTR_ID_UPDATE_VERSIONING         = 0x31, ///< RIB Attribute - Update Versioning
   CTRLM_RCU_RIB_ATTR_ID_PRODUCT_NAME              = 0x32, ///< RIB Attribute - Product Name
   CTRLM_RCU_RIB_ATTR_ID_DOWNLOAD_RATE             = 0x33, ///< RIB Attribute - Download Rate
   CTRLM_RCU_RIB_ATTR_ID_UPDATE_POLLING_PERIOD     = 0x34, ///< RIB Attribute - Update Polling Period
   CTRLM_RCU_RIB_ATTR_ID_DATA_REQUEST_WAIT_TIME    = 0x35, ///< RIB Attribute - Data Request Wait Time
   CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE_STATUS     = 0xDA, ///< RIB Attribute - IR RF Database Status
   CTRLM_RCU_RIB_ATTR_ID_IR_RF_DATABASE            = 0xDB, ///< RIB Attribute - IR RF Database
   CTRLM_RCU_RIB_ATTR_ID_VALIDATION_CONFIGURATION  = 0xDC, ///< RIB Attribute - Validation Configuration
   CTRLM_RCU_RIB_ATTR_ID_CONTROLLER_IRDB_STATUS    = 0xDD, ///< RIB Attribute - Controller IRDB Status
   CTRLM_RCU_RIB_ATTR_ID_TARGET_IRDB_STATUS        = 0xDE, ///< RIB Attribute - Target IRDB Status
   CTRLM_RCU_RIB_ATTR_ID_FAR_FIELD_CONFIGURATION   = 0xE0, ///< RIB Attribute - Far Field Configuration
   CTRLM_RCU_RIB_ATTR_ID_FAR_FIELD_METRICS         = 0xE1, ///< RIB Attribute - Far Field Metrics
   CTRLM_RCU_RIB_ATTR_ID_DSP_CONFIGURATION         = 0xE2, ///< RIB Attribute - DSP Configuration
   CTRLM_RCU_RIB_ATTR_ID_DSP_METRICS               = 0xE3, ///< RIB Attribute - DSP Metrics
   CTRLM_RCU_RIB_ATTR_ID_MFG_TEST                  = 0xFB, ///< RIB Attribute - MFG Test
   CTRLM_RCU_RIB_ATTR_ID_MEMORY_DUMP               = 0xFE, ///< RIB Attribute - Memory Dump
   CTRLM_RCU_RIB_ATTR_ID_GENERAL_PURPOSE           = 0xFF, ///< RIB Attribute - General Purpose
} ctrlm_rcu_rib_attr_id_t;

typedef enum {
   CTRLM_RCU_BINDING_TYPE_INTERACTIVE = 0, ///< User initiated binding method
   CTRLM_RCU_BINDING_TYPE_AUTOMATIC   = 1, ///< Automatic binding method
   CTRLM_RCU_BINDING_TYPE_BUTTON      = 2, ///< Button binding method
   CTRLM_RCU_BINDING_TYPE_SCREEN_BIND = 3, ///< Screen bind method
   CTRLM_RCU_BINDING_TYPE_INVALID     = 4  ///< Invalid binding type
} ctrlm_rcu_binding_type_t;

typedef enum {
   CTRLM_RCU_VALIDATION_TYPE_APPLICATION   = 0, ///< Application based validation
   CTRLM_RCU_VALIDATION_TYPE_INTERNAL      = 1, ///< Control Manager based validation
   CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC     = 2, ///< Autobinding validation
   CTRLM_RCU_VALIDATION_TYPE_BUTTON        = 3, ///< Button based validation
   CTRLM_RCU_VALIDATION_TYPE_PRECOMMISSION = 4, ///< Precommissioned controller
   CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND   = 5, ///< Screen bind based validation
   CTRLM_RCU_VALIDATION_TYPE_INVALID       = 6  ///< Invalid validation type
} ctrlm_rcu_validation_type_t;

typedef enum {
   CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL   = 0, ///< Normal Security
   CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED = 1  ///< Advanced Security
} ctrlm_rcu_binding_security_type_t;

typedef enum {
   CTRLM_RCU_GHOST_CODE_VOLUME_UNITY_GAIN = 0, ///< Volume unity gain (vol +/- or mute pressed)
   CTRLM_RCU_GHOST_CODE_POWER_OFF         = 1, ///< Power off pressed
   CTRLM_RCU_GHOST_CODE_POWER_ON          = 2, ///< Power on pressed
   CTRLM_RCU_GHOST_CODE_IR_POWER_TOGGLE   = 3, ///< Power toggled
   CTRLM_RCU_GHOST_CODE_IR_POWER_OFF      = 4, ///< Power off pressed
   CTRLM_RCU_GHOST_CODE_IR_POWER_ON       = 5, ///< Power on pressed
   CTRLM_RCU_GHOST_CODE_VOLUME_UP         = 6, ///< Volume up pressed
   CTRLM_RCU_GHOST_CODE_VOLUME_DOWN       = 7, ///< Volume down pressed
   CTRLM_RCU_GHOST_CODE_MUTE              = 8, ///< Mute pressed
   CTRLM_RCU_GHOST_CODE_INPUT             = 9, ///< TV input pressed
   CTRLM_RCU_GHOST_CODE_FIND_MY_REMOTE    = 10,///< User pressed any button in Find My Remote mode
   CTRLM_RCU_GHOST_CODE_INVALID           = 11 ///< Invalid ghost code
} ctrlm_rcu_ghost_code_t;

typedef enum {
   CTRLM_RCU_FUNCTION_SETUP                  =  0, ///< <setup> Setup key held for 3 seconds
   CTRLM_RCU_FUNCTION_BACKLIGHT              =  1, ///< <setup><92X> Backlight time where X is on time in seconds
   CTRLM_RCU_FUNCTION_POLL_FIRMWARE          =  2, ///< <setup><964> Poll for a firmware update
   CTRLM_RCU_FUNCTION_POLL_AUDIO_DATA        =  3, ///< <setup><965> Poll for an audio data update
   CTRLM_RCU_FUNCTION_RESET_SOFT             =  4, ///< <setup><980> Soft Reset on the controller
   CTRLM_RCU_FUNCTION_RESET_FACTORY          =  5, ///< <setup><981> Factory Reset on the controller (mode is changed to clip discovery)
   CTRLM_RCU_FUNCTION_BLINK_SOFTWARE_VERSION =  6, ///< <setup><983> Software version
   CTRLM_RCU_FUNCTION_BLINK_AVR_CODE         =  7, ///< <setup><985> AVR Code
   CTRLM_RCU_FUNCTION_RESET_IR               =  8, ///< <setup><986> Reset IR only
   CTRLM_RCU_FUNCTION_RESET_RF               =  9, ///< <setup><987> RF Reset on the controller  (mode is changed to clip discovery)
   CTRLM_RCU_FUNCTION_BLINK_TV_CODE          = 10, ///< <setup><990> Blink the TV code on the LED's
   CTRLM_RCU_FUNCTION_IR_DB_TV_SEARCH        = 11, ///< <setup><991> Library Search for TV's
   CTRLM_RCU_FUNCTION_IR_DB_AVR_SEARCH       = 12, ///< <setup><992> Library Search for AVR's
   CTRLM_RCU_FUNCTION_KEY_REMAPPING          = 13, ///< <setup><994> Key remapping
   CTRLM_RCU_FUNCTION_BLINK_IR_DB_VERSION    = 14, ///< <setup><995> IR DB version
   CTRLM_RCU_FUNCTION_BLINK_BATTERY_LEVEL    = 15, ///< <setup><999> Blink the battery level
   CTRLM_RCU_FUNCTION_DISCOVERY              = 16, ///< <setup><XFINITY> Discovery Request
   CTRLM_RCU_FUNCTION_MODE_IR_CLIP           = 17, ///< <setup><A> Clip mode
   CTRLM_RCU_FUNCTION_MODE_IR_MOT            = 18, ///< <setup><B> Mot
   CTRLM_RCU_FUNCTION_MODE_IR_CIS            = 19, ///< <setup><C> Cis Mode
   CTRLM_RCU_FUNCTION_MODE_CLIP_DISCOVERY    = 20, ///< <setup><D> Clip Discovery mode
   CTRLM_RCU_FUNCTION_IR_DB_TV_SELECT        = 21, ///< <setup><1####> TV code select
   CTRLM_RCU_FUNCTION_IR_DB_AVR_SELECT       = 22, ///< <setup><3####> AVR code select
   CTRLM_RCU_FUNCTION_INVALID_KEY_COMBO      = 23, ///< Invalid key combo (key combo for XR16, not XR11 or XR15)
   CTRLM_RCU_FUNCTION_INVALID                = 24  ///< Invalid function
} ctrlm_rcu_function_t;

typedef enum {
   CTRLM_RCU_CONTROLLER_TYPE_XR2     = 0,
   CTRLM_RCU_CONTROLLER_TYPE_XR5     = 1,
   CTRLM_RCU_CONTROLLER_TYPE_XR11    = 2,
   CTRLM_RCU_CONTROLLER_TYPE_XR15    = 3,
   CTRLM_RCU_CONTROLLER_TYPE_XR15V2  = 4,
   CTRLM_RCU_CONTROLLER_TYPE_XR16    = 5,
   CTRLM_RCU_CONTROLLER_TYPE_XR18    = 6,
   CTRLM_RCU_CONTROLLER_TYPE_XR19    = 7,
   CTRLM_RCU_CONTROLLER_TYPE_XRA     = 8,
   CTRLM_RCU_CONTROLLER_TYPE_UNKNOWN = 9,
   CTRLM_RCU_CONTROLLER_TYPE_INVALID = 10
} ctrlm_rcu_controller_type_t;

typedef enum {
   CTRLM_RCU_IR_DB_TYPE_UEI     = 0,
   CTRLM_RCU_IR_DB_TYPE_REMOTEC = 1,
   CTRLM_RCU_IR_DB_TYPE_INVALID = 2
} ctrlm_rcu_ir_db_type_t;

typedef enum {
   CTRLM_RCU_IR_DB_STATE_NO_CODES       = 0,
   CTRLM_RCU_IR_DB_STATE_TV_CODE        = 1,
   CTRLM_RCU_IR_DB_STATE_AVR_CODE       = 2,
   CTRLM_RCU_IR_DB_STATE_TV_AVR_CODES   = 3,
   CTRLM_RCU_IR_DB_STATE_IR_RF_DB_CODES = 4,
   CTRLM_RCU_IR_DB_STATE_INVALID        = 5
} ctrlm_rcu_ir_db_state_t;

typedef enum {
   CTRLM_RCU_BATTERY_EVENT_NONE         = 0,
   CTRLM_RCU_BATTERY_EVENT_REPLACED     = 1,
   CTRLM_RCU_BATTERY_EVENT_CHARGING     = 2,
   CTRLM_RCU_BATTERY_EVENT_PENDING_DOOM = 3,
   CTRLM_RCU_BATTERY_EVENT_75_PERCENT   = 4,
   CTRLM_RCU_BATTERY_EVENT_50_PERCENT   = 5,
   CTRLM_RCU_BATTERY_EVENT_25_PERCENT   = 6,
   CTRLM_RCU_BATTERY_EVENT_0_PERCENT    = 7, 
   CTRLM_RCU_BATTERY_EVENT_INVALID      = 8,
} ctrlm_rcu_battery_event_t;

typedef enum {
   CTRLM_RCU_REVERSE_CMD_FIND_MY_REMOTE = 0,
   CTRLM_RCU_REVERSE_CMD_REBOOT         = 1,
} ctrlm_rcu_reverse_cmd_t;

typedef enum {
   CTRLM_RCU_REVERSE_CMD_SUCCESS                = 0,
   CTRLM_RCU_REVERSE_CMD_FAILURE                = 1,
   CTRLM_RCU_REVERSE_CMD_CONTROLLER_NOT_CAPABLE = 2,
   CTRLM_RCU_REVERSE_CMD_CONTROLLER_NOT_FOUND   = 3,
   CTRLM_RCU_REVERSE_CMD_CONTROLLER_FOUND       = 4,
   CTRLM_RCU_REVERSE_CMD_USER_INTERACTION       = 5,
   CTRLM_RCU_REVERSE_CMD_DISABLED               = 6,
   CTRLM_RCU_REVERSE_CMD_INVALID                = 7,
} ctrlm_rcu_reverse_cmd_result_t;

typedef enum {
   CTRLM_RCU_FMR_ALERT_FLAGS_ID = 1,           // combination of flags defined in ctrlm_rcu_find_my_remote_alert_flag_t
   CTRLM_FIND_RCU_FMR_ALERT_DURATION_ID = 2,   // unsigned integer alert duration in msec
} ctrlm_rcu_find_my_remote_parameter_id_t;

typedef enum {
  CTRLM_RCU_ALERT_AUDIBLE = 0x01,
  CTRLM_RCU_ALERT_VISUAL  = 0x02
} ctrlm_rcu_alert_flags_t;

typedef enum {
   CTRLM_RCU_DSP_EVENT_MIC_FAILURE      = 1,
   CTRLM_RCU_DSP_EVENT_SPEAKER_FAILURE  = 2,
   CTRLM_RCU_DSP_EVENT_INVALID          = 3,
} ctrlm_rcu_dsp_event_t;

typedef enum {
   CONTROLLER_REBOOT_POWER_ON      = 0,
   CONTROLLER_REBOOT_EXTERNAL      = 1,
   CONTROLLER_REBOOT_WATCHDOG      = 2,
   CONTROLLER_REBOOT_CLOCK_LOSS    = 3,
   CONTROLLER_REBOOT_BROWN_OUT     = 4,
   CONTROLLER_REBOOT_OTHER         = 5,
   CONTROLLER_REBOOT_ASSERT_NUMBER = 6
} controller_reboot_reason_t;

typedef enum {
   RCU_POLLING_ACTION_NONE                  = 0x00,
   RCU_POLLING_ACTION_REBOOT                = 0x01,
   RCU_POLLING_ACTION_REPAIR                = 0x02,
   RCU_POLLING_ACTION_CONFIGURATION         = 0x03,
   RCU_POLLING_ACTION_OTA                   = 0x04,
   RCU_POLLING_ACTION_ALERT                 = 0x05,
   RCU_POLLING_ACTION_IRDB_STATUS           = 0x06,
   RCU_POLLING_ACTION_POLL_CONFIGURATION    = 0x07,
   RCU_POLLING_ACTION_VOICE_CONFIGURATION   = 0x08,
   RCU_POLLING_ACTION_DSP_CONFIGURATION     = 0x09,
   RCU_POLLING_ACTION_METRICS               = 0x0A,
   RCU_POLLING_ACTION_EOS                   = 0x0B,
   RCU_POLLING_ACTION_SETUP_COMPLETE        = 0x0C,
   RCU_POLLING_ACTION_BATTERY_STATUS        = 0x0D,
   RCU_POLLING_ACTION_PROFILE_CONFIGURATION = 0x0E,
   RCU_POLLING_ACTION_IRRF_STATUS           = 0x10
} ctrlm_rcu_polling_action_t;

typedef struct {
   unsigned long long                ieee_address;                                          ///< The 64-bit IEEE Address
   unsigned short                    short_address;                                         ///< Short address (if applicable)
   unsigned long                     time_binding;                                          ///< Time that the controller was bound (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   ctrlm_rcu_binding_type_t          binding_type;                                          ///< Type of binding that was performed
   ctrlm_rcu_validation_type_t       validation_type;                                       ///< Type of validation that was performed
   ctrlm_rcu_binding_security_type_t security_type;                                         ///< Security type for the binding
   unsigned long                     command_count;                                         ///< Amount of commands received from this controller
   ctrlm_key_code_t                  last_key_code;                                         ///< Last key code received
   ctrlm_key_status_t                last_key_status;                                       ///< Key status of last key code
   unsigned char                     link_quality_percent;                                  ///< Link quality percentage (0-100)
   unsigned char                     link_quality;                                          ///< Link quality indicator
   char                              manufacturer[CTRLM_RCU_MAX_MANUFACTURER_LENGTH];       ///< Manufacturer of the controller
   char                              chipset[CTRLM_RCU_MAX_CHIPSET_LENGTH];                 ///< Chipset of the controller
   char                              version_software[CTRLM_RCU_VERSION_LENGTH];            ///< Software version of controller
   char                              version_dsp[CTRLM_RCU_VERSION_LENGTH];                 ///< DSP version of controller
   char                              version_keyword_model[CTRLM_RCU_VERSION_LENGTH];       ///< Keyword model version of controller
   char                              version_arm[CTRLM_RCU_VERSION_LENGTH];                 ///< ARM version of controller
   char                              version_hardware[CTRLM_RCU_VERSION_LENGTH];            ///< Hardware version of controller
   char                              version_irdb[CTRLM_RCU_VERSION_LENGTH];                ///< IR database version (if available)
   char                              version_build_id[CTRLM_RCU_BUILD_ID_LENGTH];           ///< Build ID of software on controller
   char                              version_dsp_build_id[CTRLM_RCU_DSP_BUILD_ID_LENGTH];   ///< DSP Build ID of software on controller
   char                              version_bootloader[CTRLM_RCU_VERSION_LENGTH];          ///< Bootloader
   char                              version_golden[CTRLM_RCU_VERSION_LENGTH];              ///< Golden Software version
   char                              version_audio_data[CTRLM_RCU_VERSION_LENGTH];          ///< Audio Data version (if available)
   unsigned char                     firmware_updated;                                      ///< Boolean value indicating that the controller's firmware has been updated (1) or not (0)
   unsigned char                     has_battery;                                           ///< Boolean value indicating that the controller has a battery (1) or not (0)
   unsigned char                     battery_level_percent;                                 ///< Battery Level percentage (0-100)
   float                             battery_voltage_loaded;                                ///< Battery Voltage under load
   float                             battery_voltage_unloaded;                              ///< Battery Voltage not under load
   unsigned long                     time_last_key;                                         ///< Time of last key code received (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned long                     time_battery_update;                                   ///< Time of battery update (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned long                     time_battery_changed;                                  ///< Time of battery changed (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_changed_actual_percentage;                     ///< Actual percentage of the batteries when the were "changed"
   float                             battery_changed_unloaded_voltage;                      ///< Actual voltage of the batteries when the were "changed"
   unsigned long                     time_battery_75_percent;                               ///< Time of battery at 75% (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_75_percent_actual_percentage;                  ///< Actual percentage of the batteries when they hit 75% or below
   float                             battery_75_percent_unloaded_voltage;                   ///< Actual voltage of the batteries when they hit 75% or below
   unsigned long                     time_battery_50_percent;                               ///< Time of battery at 50% (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_50_percent_actual_percentage;                  ///< Actual percentage of the batteries when they hit 50% or below
   float                             battery_50_percent_unloaded_voltage;                   ///< Actual voltage of the batteries when they hit 50% or below
   unsigned long                     time_battery_25_percent;                               ///< Time of battery at 25% (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_25_percent_actual_percentage;                  ///< Actual percentage of the batteries when they hit 25% or below
   float                             battery_25_percent_unloaded_voltage;                   ///< Actual voltage of the batteries when they hit 25% or below
   unsigned long                     time_battery_5_percent;                                ///< Time of battery at 5% (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_5_percent_actual_percentage;                   ///< Actual percentage of the batteries when they hit 5% or below
   float                             battery_5_percent_unloaded_voltage;                    ///< Actual voltage of the batteries when they hit 5% or below
   unsigned long                     time_battery_0_percent;                                ///< Time of battery at 0% (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   unsigned char                     battery_0_percent_actual_percentage;                   ///< Actual percentage of the batteries when they hit 0% or below
   float                             battery_0_percent_unloaded_voltage;                    ///< Actual voltage of the batteries when they hit 0% or below
   ctrlm_rcu_battery_event_t         battery_event;                                         ///< Last battery event
   unsigned long                     time_battery_event;                                    ///< Time of the last battery event (number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC))
   char                              type[CTRLM_RCU_MAX_USER_STRING_LENGTH];                ///< Remote control's type string
   ctrlm_rcu_ir_db_type_t            ir_db_type;                                            ///< Type of IR Database in the controller
   ctrlm_rcu_ir_db_state_t           ir_db_state;                                           ///< State of IR Database in the controller
   char                              ir_db_code_tv[CTRLM_RCU_MAX_IR_DB_CODE_LENGTH];        ///< Current TV Code programmed in the controller
   char                              ir_db_code_avr[CTRLM_RCU_MAX_IR_DB_CODE_LENGTH];       ///< Current AVR Code programmed in the controller
   unsigned long                     voice_cmd_count_today;                                 ///< Number of normal voice commands received today
   unsigned long                     voice_cmd_count_yesterday;                             ///< Number of normal voice commands received yesterday
   unsigned long                     voice_cmd_short_today;                                 ///< Number of short voice commands received today
   unsigned long                     voice_cmd_short_yesterday;                             ///< Number of short voice commands received yesterday
   unsigned long                     voice_packets_sent_today;                              ///< Number of voice packets sent today
   unsigned long                     voice_packets_sent_yesterday;                          ///< Number of voice packets sent yesterday
   unsigned long                     voice_packets_lost_today;                              ///< Number of voice packets lost today
   unsigned long                     voice_packets_lost_yesterday;                          ///< Number of voice packets lost yesterday
   float                             voice_packet_loss_average_today;                       ///< The average packet loss for today (sent-received)/sent
   float                             voice_packet_loss_average_yesterday;                   ///< The average packet loss for yesterday (sent-received)/sent
   unsigned long                     utterances_exceeding_packet_loss_threshold_today;      ///< Number utterances exceeding the packet loss threshold today
   unsigned long                     utterances_exceeding_packet_loss_threshold_yesterday;  ///< Number utterances exceeding the packet loss threshold yesterday
   unsigned char                     checkin_for_device_update;                             ///< Boolean value indicating that the controller has checked in in the last x hours for device update
   unsigned char                     ir_db_code_download_supported;                         ///< Boolean value indicating that the controller supports irdb code download
   unsigned char                     has_dsp;                                               ///< Boolean value indicating that the controller has a dsp chip (1) or not (0)
   unsigned long                     average_time_in_privacy_mode;                          ///< Average time in privacy mode
   unsigned char                     in_privacy_mode;                                       ///< Boolean value indicating whether the controller is currently in privacy mode
   unsigned char                     average_snr;                                           ///< Average signal to noise ratio
   unsigned char                     average_keyword_confidence;                            ///< Average keyword confidence
   unsigned char                     total_number_of_mics_working;                          ///< Total number of mics that are working
   unsigned char                     total_number_of_speakers_working;                      ///< Total number of speakers that are working
   unsigned int                      end_of_speech_initial_timeout_count;                   ///< End of speech initial timeout count
   unsigned int                      end_of_speech_timeout_count;                           ///< End of speech timeout count
   unsigned long                     time_uptime_start;                                     ///< The time uptime started counting
   unsigned long                     uptime_seconds;                                        ///< The uptime of the remote in seconds
   unsigned long                     privacy_time_seconds;                                  ///< Total amount of time remote is in privacy mode
   unsigned char                     reboot_reason;                                         ///< The last remote reboot reason
   unsigned char                     reboot_voltage;                                        ///< The last remote reboot voltage
   unsigned int                      reboot_assert_number;                                  ///< The last remote assert_number 
   unsigned long                     reboot_timestamp;                                      ///< Time of the last remote reboot
   unsigned long                     time_last_heartbeat;                                   ///< The time of the last heartbeat
   char                              irdb_entry_id_name_tv[CTRLM_MAX_PARAM_STR_LEN];        ///< The TV irdb code name
   char                              irdb_entry_id_name_avr[CTRLM_MAX_PARAM_STR_LEN];       ///< The AVR irdb code name
   unsigned char                     battery_voltage_large_jump_counter;                    ///< The large jump counter for battery voltage
   unsigned char                     battery_voltage_large_decline_detected;                ///< The large decline detected flag for battery voltage
} ctrlm_controller_status_t;

typedef struct {
   unsigned char             api_revision;  ///< Revision of this API
   ctrlm_iarm_call_result_t  result;        ///< Result of the IARM call
   ctrlm_network_id_t        network_id;    ///< IN The identifier of network on which the controller is bound
   ctrlm_controller_id_t     controller_id; ///< IN
   ctrlm_controller_status_t status;        ///< Status of the controller
} ctrlm_rcu_iarm_call_controller_status_t;

typedef struct {
   unsigned char             api_revision;  ///< Revision of this API
   ctrlm_iarm_call_result_t  result;        ///< Result of the IARM call
   ctrlm_network_id_t        network_id;    ///< IN The identifier of network on which the controller is bound
   ctrlm_controller_id_t     controller_id; ///< IN
   unsigned char             link_key[16];  ///< OUT The link key for the controller
} ctrlm_rcu_iarm_call_controller_link_key_t;

typedef struct {
   unsigned char            api_revision;                                      ///< Revision of this API
   ctrlm_network_id_t       network_id;                                        ///< identifier of network on which the controller is bound
   ctrlm_network_type_t     network_type;                                      ///< type of network on which the controller is bound
   ctrlm_controller_id_t    controller_id;                                     ///< identifier of the controller on which the key was pressed
   ctrlm_key_status_t       key_status;                                        ///< status of the key press (down, repeat, up)
   ctrlm_key_code_t         key_code;                                          ///< received key code
   ctrlm_rcu_binding_type_t binding_type;                                      ///< Type of binding that was performed
   char                     controller_type[CTRLM_RCU_MAX_USER_STRING_LENGTH]; ///< Remote control's type string
} ctrlm_rcu_iarm_event_key_press_t;

typedef struct {
   unsigned char               api_revision;                                      ///< Revision of this API
   ctrlm_network_id_t          network_id;                                        ///< identifier of network on which the controller is bound
   ctrlm_network_type_t        network_type;                                      ///< type of network on which the controller is bound
   ctrlm_controller_id_t       controller_id;                                     ///< identifier of the controller on which the validation is being performed
   ctrlm_rcu_binding_type_t    binding_type;                                      ///< Type of binding that is being performed
   ctrlm_rcu_validation_type_t validation_type;                                   ///< Type of validation that is being performed
   ctrlm_key_code_t            validation_keys[CTRLM_RCU_VALIDATION_KEY_QTY];     ///< Validation keys to be displayed for internal validation
   char                        controller_type[CTRLM_RCU_MAX_USER_STRING_LENGTH]; ///< Remote control's type string
} ctrlm_rcu_iarm_event_validation_begin_t;

typedef struct {
   unsigned char                 api_revision;                                      ///< Revision of this API
   ctrlm_network_id_t            network_id;                                        ///< identifier of network on which the controller is bound
   ctrlm_network_type_t          network_type;                                      ///< type of network on which the controller is bound
   ctrlm_controller_id_t         controller_id;                                     ///< identifier of the controller on which the validation was performed
   ctrlm_rcu_binding_type_t      binding_type;                                      ///< Type of binding that was performed
   ctrlm_rcu_validation_type_t   validation_type;                                   ///< Type of validation that was performed
   ctrlm_rcu_validation_result_t result;                                            ///< Result of the validation attempt
   char                          controller_type[CTRLM_RCU_MAX_USER_STRING_LENGTH]; ///< Remote control's type string
} ctrlm_rcu_iarm_event_validation_end_t;

typedef struct {
   unsigned char                    api_revision;                                      ///< Revision of this API
   ctrlm_network_id_t               network_id;                                        ///< identifier of network on which the controller is bound
   ctrlm_network_type_t             network_type;                                      ///< type of network on which the controller is bound
   ctrlm_controller_id_t            controller_id;                                     ///< identifier of the controller on which the validation was performed
   ctrlm_rcu_configuration_result_t result;                                            ///< Result of the configuration attempt
   ctrlm_rcu_binding_type_t         binding_type;                                      ///< Type of binding that was performed
   char                             controller_type[CTRLM_RCU_MAX_USER_STRING_LENGTH]; ///< Remote control's type string
   ctrlm_controller_status_t        status;                                            ///< Remote control's status
} ctrlm_rcu_iarm_event_configuration_complete_t;

typedef struct {
   unsigned char         api_revision;                                  ///< Revision of this API
   ctrlm_network_id_t    network_id;                                    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t  network_type;                                  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t controller_id;                                 ///< Identifier of the controller on which the key was pressed
   ctrlm_rcu_function_t  function;                                      ///< Function that was performed on the controller
   unsigned long         value;                                         ///< Value associated with the function (if applicable)
} ctrlm_rcu_iarm_event_function_t;

typedef struct {
   unsigned char          api_revision;          ///< Revision of this API
   ctrlm_network_id_t     network_id;            ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t   network_type;          ///< Type of network on which the controller is bound
   ctrlm_controller_id_t  controller_id;         ///< Identifier of the controller on which the key was pressed
   ctrlm_rcu_ghost_code_t ghost_code;            ///< Ghost code
   unsigned char          remote_keypad_config;  /// The remote keypad configuration (Has Setup/NumberKeys).
} ctrlm_rcu_iarm_event_key_ghost_t;

typedef struct {
   unsigned char          api_revision;                                    ///< Revision of this API
   int                    controller_id;                                   ///< Identifier of the controller on which the key was pressed
   char                   event_source[CTRLM_RCU_MAX_EVENT_SOURCE_LENGTH]; ///< The key source
   char                   event_type[CTRLM_RCU_MAX_EVENT_TYPE_LENGTH];     ///< The control type
   char                   event_data[CTRLM_RCU_MAX_EVENT_DATA_LENGTH];     ///< The data
   int                    event_value;                                     ///< The value
   int                    spare_value;                                     ///< A spare value (sfm needs this extra one)
} ctrlm_rcu_iarm_event_control_t;

typedef struct {
   unsigned char           api_revision;  ///< Revision of this API
   ctrlm_network_id_t      network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t    network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t   controller_id; ///< Identifier of the controller
   ctrlm_rcu_rib_attr_id_t identifier;    ///< RIB attribute identifier
   unsigned char           index;         ///< RIB attribute index
   ctrlm_access_type_t     access_type;   ///< RIB access type (read/write)
} ctrlm_rcu_iarm_event_rib_entry_access_t;

typedef struct {
   unsigned char              api_revision;  ///< Revision of this API
   ctrlm_network_id_t         network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t       network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t      controller_id; ///< Identifier of the controller
   unsigned char              voltage;       ///< Voltage when reboot reason is CONTROLLER_REBOOT_ASSERT_NUMBER
   controller_reboot_reason_t reason;        ///< Remote reboot reason
   unsigned long              timestamp;     ///< Reboot timestamp
   unsigned int               assert_number; ///< Assert Number when reboot reason is CONTROLLER_REBOOT_ASSERT_NUMBER
} ctrlm_rcu_iarm_event_remote_reboot_t;

typedef struct {
   unsigned char              api_revision;  ///< Revision of this API
   ctrlm_network_id_t         network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t       network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t      controller_id; ///< Identifier of the controller
   ctrlm_rcu_battery_event_t  battery_event; ///< Battery event
   unsigned char              percent;       ///< Battery percentage
} ctrlm_rcu_iarm_event_battery_t;

typedef struct {
   unsigned char                 api_revision;       ///< Revision of this API
   ctrlm_rcu_validation_result_t validation_result;  ///< Result of the validation
} ctrlm_rcu_iarm_event_rf4ce_pairing_window_timeout_t;

typedef struct {
   time_t        time_uptime_start;
   unsigned long uptime_seconds;
   unsigned long privacy_time_seconds;
} uptime_privacy_info_t;

typedef struct {
   unsigned char  param_id; ///< parameter id
   unsigned long  size;     ///< parameter size, in bytes
} ctrlm_rcu_reverse_cmd_param_descriptor_t;

typedef struct {
   unsigned char            api_revision;     ///< [in]  Revision of this API
   ctrlm_iarm_call_result_t result;           ///< [out] Result of the IARM call
   ctrlm_network_type_t     network_type;     ///< [in]  Type of network on which the controller is bound
   ctrlm_controller_id_t    controller_id;    ///< [in]  Identifier of the controller. controller ID, CTRLM_MAIN_CONTROLLER_ID_ALL or CTRLM_MAIN_CONTROLLER_ID_LAST_USED
   ctrlm_rcu_reverse_cmd_t  cmd;              ///< [in]  command ID
   ctrlm_rcu_reverse_cmd_result_t cmd_result; ///< [out] Reverse Command result
   unsigned long            total_size;       ///< [in]  ctrlm_main_iarm_call_rcu_reverse_cmd_t + data size
   unsigned char            num_params;       ///< [in]  number of parameters
   ctrlm_rcu_reverse_cmd_param_descriptor_t params_desc[CTRLM_RCU_CALL_RCU_REVERSE_CMD_PARAMS_MAX];   ///< command parameter descriptor
   unsigned char            param_data[1];    ///< parameters data
} ctrlm_main_iarm_call_rcu_reverse_cmd_t;

typedef struct {
   unsigned char                  api_revision;     ///< Revision of this API
   ctrlm_network_id_t             network_id;       ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t           network_type;     ///< Type of network on which the controller is bound
   ctrlm_controller_id_t          controller_id;    ///< Identifier of the controller on which the key was pressed
   ctrlm_rcu_reverse_cmd_t        action;           ///< Reverse Command that was performed on the controller
   ctrlm_rcu_reverse_cmd_result_t result;           ///< Reverse Command result
   int                            result_data_size; ///< Result Data Size
   unsigned char                  result_data[1];   ///< Result Data buffer
} ctrlm_rcu_iarm_event_reverse_cmd_t;

typedef struct {
   unsigned char              api_revision;                               ///< Revision of this API
   ctrlm_iarm_call_result_t   result;                                     ///< Result of the IARM call
   ctrlm_network_id_t         network_id;                                 ///< IN - identifier of network on which the controller is bound
   ctrlm_controller_id_t      controller_id;                              ///< IN - identifier of the controller
   unsigned char              action;                                     ///< IN - Polling action performed on the controller
   char                       data[CTRLM_RCU_POLLING_RESPONSE_DATA_LEN];  ///< IN - Polling data
} ctrlm_rcu_iarm_call_rf4ce_polling_action_t;

#endif
