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

/// @file ctrlm_ipc_rcu.h
///
/// @defgroup CTRLM_IPC_RCU IARM API - Remote Control
/// @{
///
/// @defgroup CTRLM_IPC_RCU_COMMS       Communication Interfaces
/// @defgroup CTRLM_IPC_RCU_CALLS       IARM Remote Procedure Calls
/// @defgroup CTRLM_IPC_RCU_EVENTS      IARM Events
/// @defgroup CTRLM_IPC_RCU_DEFINITIONS Constants
/// @defgroup CTRLM_IPC_RCU_ENUMS       Enumerations
/// @defgroup CTRLM_IPC_RCU_STRUCTS     Structures
///
/// @addtogroup CTRLM_IPC_RCU_DEFINITIONS
/// @{
/// @brief Constant Defintions
/// @details The Control Manager provides definitions for constant values.

#define CTRLM_RCU_IARM_CALL_VALIDATION_FINISH            "Rcu_ValidationFinish"     ///< IARM Call to complete controller validation
#define CTRLM_RCU_IARM_CALL_CONTROLLER_STATUS            "Rcu_ControllerStatus"     ///< IARM Call to get controller information
#define CTRLM_RCU_IARM_CALL_CONTROLLER_LINK_KEY          "Rcu_ControllerLinkKey"    ///< IARM Call to get controller link key
#define CTRLM_RCU_IARM_CALL_RIB_REQUEST_GET              "Rcu_RibRequestGet"        ///< IARM Call to retrieves an attribute from the controller's RIB
#define CTRLM_RCU_IARM_CALL_RIB_REQUEST_SET              "Rcu_RibRequestSet"        ///< IARM Call to set an attribute in the controller's RIB
#define CTRLM_RCU_IARM_CALL_REVERSE_CMD                  "Rcu_ReverseCmd"           ///< IARM Call to Trigger Remote Controller Action
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

#define CTRLM_RCU_MAX_RIB_ATTRIBUTE_SIZE                 (92)    ///< Maximum size of a RIB attribute (in bytes)
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
/// @}

/// @addtogroup CTRLM_IPC_RCU_ENUMS
/// @{
/// @brief Enumerated Types
/// @details The Control Manager provides enumerated types for logical groups of values.

/// @brief RCU Binding Validation Results
/// @details The process of binding a remote control will terminate with one of the results in this enumeration.
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

/// @brief RCU Configuration Results
/// @details The process of configuring a remote control will terminate with one of the results in this enumeration.
typedef enum {
   CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS = 0, ///< The configuration completed successfully.
   CTRLM_RCU_CONFIGURATION_RESULT_PENDING = 1, ///< The configuration is still pending.
   CTRLM_RCU_CONFIGURATION_RESULT_TIMEOUT = 2, ///< The configuration has exceeded the timeout period.
   CTRLM_RCU_CONFIGURATION_RESULT_FAILURE = 4, ///< The configuration did not complete successfully.
   CTRLM_RCU_CONFIGURATION_RESULT_MAX     = 5  ///< Maximum validation result value
} ctrlm_rcu_configuration_result_t;

/// @brief RCU RIB Attribute Id's
/// @details The attribute identifier of all supported RIB entries.
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

/// @brief RCU Binding Type
/// @details The type of binding that can be performed between a controller and target.
typedef enum {
   CTRLM_RCU_BINDING_TYPE_INTERACTIVE = 0, ///< User initiated binding method
   CTRLM_RCU_BINDING_TYPE_AUTOMATIC   = 1, ///< Automatic binding method
   CTRLM_RCU_BINDING_TYPE_BUTTON      = 2, ///< Button binding method
   CTRLM_RCU_BINDING_TYPE_SCREEN_BIND = 3, ///< Screen bind method
   CTRLM_RCU_BINDING_TYPE_INVALID     = 4  ///< Invalid binding type
} ctrlm_rcu_binding_type_t;

/// @brief RCU Validation Type
/// @details The type of validation that can be performed between a controller and target.
typedef enum {
   CTRLM_RCU_VALIDATION_TYPE_APPLICATION   = 0, ///< Application based validation
   CTRLM_RCU_VALIDATION_TYPE_INTERNAL      = 1, ///< Control Manager based validation
   CTRLM_RCU_VALIDATION_TYPE_AUTOMATIC     = 2, ///< Autobinding validation
   CTRLM_RCU_VALIDATION_TYPE_BUTTON        = 3, ///< Button based validation
   CTRLM_RCU_VALIDATION_TYPE_PRECOMMISSION = 4, ///< Precommissioned controller
   CTRLM_RCU_VALIDATION_TYPE_SCREEN_BIND   = 5, ///< Screen bind based validation
   CTRLM_RCU_VALIDATION_TYPE_INVALID       = 6  ///< Invalid validation type
} ctrlm_rcu_validation_type_t;

/// @brief RCU Binding Security Type
/// @details The type of binding security used between the controller and target.
typedef enum {
   CTRLM_RCU_BINDING_SECURITY_TYPE_NORMAL   = 0, ///< Normal Security
   CTRLM_RCU_BINDING_SECURITY_TYPE_ADVANCED = 1  ///< Advanced Security
} ctrlm_rcu_binding_security_type_t;

/// @brief RCU Ghost Codes
/// @details The enumeration of ghost codes that can be produced by the controller. XRC spec 11.2.10.1
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

/// @brief RCU Functions
/// @details The enumeration of functions that can be produced by the controller.
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

/// @brief RCU Controller Type
/// @details The type of controller.
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

/// @brief RCU IR DB Type
/// @details The type of IR DB in the controller.
typedef enum {
   CTRLM_RCU_IR_DB_TYPE_UEI     = 0,
   CTRLM_RCU_IR_DB_TYPE_REMOTEC = 1,
   CTRLM_RCU_IR_DB_TYPE_INVALID = 2
} ctrlm_rcu_ir_db_type_t;

/// @brief RCU IR DB State
/// @details The state of IR DB in the controller.
typedef enum {
   CTRLM_RCU_IR_DB_STATE_NO_CODES       = 0,
   CTRLM_RCU_IR_DB_STATE_TV_CODE        = 1,
   CTRLM_RCU_IR_DB_STATE_AVR_CODE       = 2,
   CTRLM_RCU_IR_DB_STATE_TV_AVR_CODES   = 3,
   CTRLM_RCU_IR_DB_STATE_IR_RF_DB_CODES = 4,
   CTRLM_RCU_IR_DB_STATE_INVALID        = 5
} ctrlm_rcu_ir_db_state_t;


/// @brief RCU Battery Event
/// @details The events associate with the battery in the controller.
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

/// @brief RCU Reverse Command Type
/// @details An enumeration of the reverse command types.
typedef enum {
   CTRLM_RCU_REVERSE_CMD_FIND_MY_REMOTE = 0,
   CTRLM_RCU_REVERSE_CMD_REBOOT         = 1,
} ctrlm_rcu_reverse_cmd_t;

/// @brief RCU Reverse Command Event
/// @details The events associate with the Action performed on the controller.
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

/// @brief RCU Alert flags
typedef enum {
  CTRLM_RCU_ALERT_AUDIBLE = 0x01,
  CTRLM_RCU_ALERT_VISUAL  = 0x02
} ctrlm_rcu_alert_flags_t;

/// @brief RCU DSP Event
/// @details The events associate with the dsp in the controller.
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

/// @}

/// @addtogroup CTRLM_IPC_RCU_STRUCTS
/// @{
/// @brief Structure Definitions
/// @details The Control Manager provides structures that are used in IARM calls and events.

/// @brief Structure of Remote Controls's Validation Finish IARM call
/// @details This structure provides information about the completion of a controller validation.
typedef struct {
   unsigned char                 api_revision;      ///< Revision of this API
   ctrlm_iarm_call_result_t      result;            ///< Result of the IARM call
   ctrlm_network_id_t            network_id;        ///< The identifier of network on which the controller is bound
   ctrlm_controller_id_t         controller_id;     ///< The identifier of the controller
   ctrlm_rcu_validation_result_t validation_result; ///< Result of the validation attempt
} ctrlm_rcu_iarm_call_validation_finish_t;

/// @brief Controller Status Structure
/// @details This structure contains a controller's diagnostic information.
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

/// @brief Structure of Remote Controls's Controller Status IARM call
/// @details This structure provides information about a controller.
typedef struct {
   unsigned char             api_revision;  ///< Revision of this API
   ctrlm_iarm_call_result_t  result;        ///< Result of the IARM call
   ctrlm_network_id_t        network_id;    ///< IN The identifier of network on which the controller is bound
   ctrlm_controller_id_t     controller_id; ///< IN
   ctrlm_controller_status_t status;        ///< Status of the controller
} ctrlm_rcu_iarm_call_controller_status_t;

/// @brief Structure of Remote Controls's RIB Request get and set IARM calls
/// @details This structure provides information about a controller's RIB entry.
typedef struct {
   unsigned char            api_revision;                           ///< Revision of this API
   ctrlm_iarm_call_result_t result;                                 ///< Result of the IARM call
   ctrlm_network_id_t       network_id;                             ///< IN - identifier of network on which the controller is bound
   ctrlm_controller_id_t    controller_id;                          ///< IN - identifier of the controller
   ctrlm_rcu_rib_attr_id_t  attribute_id;                           ///< RIB attribute identifier
   unsigned char            attribute_index;                        ///< RIB attribute index
   unsigned char            length;                                 ///< RIB data length
   char                     data[CTRLM_RCU_MAX_RIB_ATTRIBUTE_SIZE]; ///< RIB entry's data
} ctrlm_rcu_iarm_call_rib_request_t;

/// @brief Structure of Remote Controls's Controller Link Key IARM call
/// @details This stucture provides the controllers link key
typedef struct {
   unsigned char             api_revision;  ///< Revision of this API
   ctrlm_iarm_call_result_t  result;        ///< Result of the IARM call
   ctrlm_network_id_t        network_id;    ///< IN The identifier of network on which the controller is bound
   ctrlm_controller_id_t     controller_id; ///< IN
   unsigned char             link_key[16];  ///< OUT The link key for the controller
} ctrlm_rcu_iarm_call_controller_link_key_t;

/// @brief Structure of Remote Control's Key Press IARM event
/// @details This event notifies listeners that a key event has occurred.
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

/// @brief Structure of Remote Control's Validation Begin IARM event
/// @details This event notifies listeners that a validation attempt has begun.
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

/// @brief Structure of Remote Control's Validation End IARM event
/// @details This event notifies listeners that a validation attempt has completed.
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

/// @brief Structure of Remote Control's Configuration Complete IARM event
/// @details This event notifies listeners that a validation attempt has completed.
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

/// @brief Structure of Remote Control's Setup Key IARM event
/// @details This event notifies listeners that a remote control function event has occurred.
typedef struct {
   unsigned char         api_revision;                                  ///< Revision of this API
   ctrlm_network_id_t    network_id;                                    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t  network_type;                                  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t controller_id;                                 ///< Identifier of the controller on which the key was pressed
   ctrlm_rcu_function_t  function;                                      ///< Function that was performed on the controller
   unsigned long         value;                                         ///< Value associated with the function (if applicable)
} ctrlm_rcu_iarm_event_function_t;

/// @brief Structure of Remote Control's Ghost Key IARM event
/// @details This event notifies listeners that a ghost code event has occurred.
typedef struct {
   unsigned char          api_revision;          ///< Revision of this API
   ctrlm_network_id_t     network_id;            ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t   network_type;          ///< Type of network on which the controller is bound
   ctrlm_controller_id_t  controller_id;         ///< Identifier of the controller on which the key was pressed
   ctrlm_rcu_ghost_code_t ghost_code;            ///< Ghost code
   unsigned char          remote_keypad_config;  /// The remote keypad configuration (Has Setup/NumberKeys).
} ctrlm_rcu_iarm_event_key_ghost_t;

/// @brief Structure of Remote Control's Ghost Key IARM event
/// @details This event notifies listeners that a ghost code event has occurred.
typedef struct {
   unsigned char          api_revision;                                    ///< Revision of this API
   int                    controller_id;                                   ///< Identifier of the controller on which the key was pressed
   char                   event_source[CTRLM_RCU_MAX_EVENT_SOURCE_LENGTH]; ///< The key source
   char                   event_type[CTRLM_RCU_MAX_EVENT_TYPE_LENGTH];     ///< The control type
   char                   event_data[CTRLM_RCU_MAX_EVENT_DATA_LENGTH];     ///< The data
   int                    event_value;                                     ///< The value
   int                    spare_value;                                     ///< A spare value (sfm needs this extra one)
} ctrlm_rcu_iarm_event_control_t;

/// @brief Structure of Remote Control's RIB Entry Access IARM event
/// @details The RIB Entry Access Event uses this structure. See the @link CTRLM_IPC_RCU_EVENTS Events@endlink section for more details on registering to receive this event.
typedef struct {
   unsigned char           api_revision;  ///< Revision of this API
   ctrlm_network_id_t      network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t    network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t   controller_id; ///< Identifier of the controller
   ctrlm_rcu_rib_attr_id_t identifier;    ///< RIB attribute identifier
   unsigned char           index;         ///< RIB attribute index
   ctrlm_access_type_t     access_type;   ///< RIB access type (read/write)
} ctrlm_rcu_iarm_event_rib_entry_access_t;

/// @brief Structure of Remote Control's remote reboot IARM event
/// @details This event notifies listeners that a remote rebooot event has occurred.
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

/// @brief Structure of Remote Control's Battery IARM event
/// @details This event notifies listeners that a DSP event has occurred.
typedef struct {
   unsigned char              api_revision;  ///< Revision of this API
   ctrlm_network_id_t         network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t       network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t      controller_id; ///< Identifier of the controller
   ctrlm_rcu_battery_event_t  battery_event; ///< Battery event
   unsigned char              percent;       ///< Battery percentage
} ctrlm_rcu_iarm_event_battery_t;

/// @brief Structure of Remote Control's remote reboot IARM event
/// @details This event notifies listeners that a remote rebooot event has occurred.
typedef struct {
   unsigned char                 api_revision;       ///< Revision of this API
   ctrlm_rcu_validation_result_t validation_result;  ///< Result of the validation
} ctrlm_rcu_iarm_event_rf4ce_pairing_window_timeout_t;

typedef struct {
   time_t        time_uptime_start;
   unsigned long uptime_seconds;
   unsigned long privacy_time_seconds;
} uptime_privacy_info_t;

/// @brief Remote Controller Reverse Command Structure
/// @details Remote Controller Reverse Command structure is used in the CTRLM_MAIN_IARM_CALL_RCU_REVERSE_CMD call.
/// ctrlm_main_iarm_call_rcu_reverse_cmd_t memory structure:
///
/// size of structure: sizeof (struct ctrlm_main_iarm_call_rcu_reverse_cmd_t) + (param data size) - 1
///
/// Find My Remote
/// cmd = CTRLM_RCU_REVERSE_CMD_FIND_MY_REMOTE
/// num_params = 2
///
/// parameter id = CTRLM_RCU_FMR_ALERT_FLAGS_ID
/// parameter length 1 byte
/// parameter values CTRLM_RCU_ALERT_AUDIBLE | CTRLM_RCU_ALERT_VISUAL
///
/// parameter id = CTRLM_FIND_RCU_FMR_ALERT_DURATION_ID
/// parameter length 2 bytes unsigned short
/// parameter values chime diration in milliseconds
/// memory needed: sizeof (ctrlm_main_iarm_call_rcu_reverse_cmd_t) + 2


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

/// @brief Structure of Remote HeartBeat response polling action IARM call
/// @details This structure provides information about a controller's HeartBeat response polling action.
typedef struct {
   unsigned char              api_revision;                               ///< Revision of this API
   ctrlm_iarm_call_result_t   result;                                     ///< Result of the IARM call
   ctrlm_network_id_t         network_id;                                 ///< IN - identifier of network on which the controller is bound
   ctrlm_controller_id_t      controller_id;                              ///< IN - identifier of the controller
   unsigned char              action;                                     ///< IN - Polling action performed on the controller
   char                       data[CTRLM_RCU_POLLING_RESPONSE_DATA_LEN];  ///< IN - Polling data
} ctrlm_rcu_iarm_call_rf4ce_polling_action_t;

/// @addtogroup CTRLM_IPC_RCU_CALLS
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
/// | Bus Name                 | Call Name                             | Argument                             | Description |
/// | :-------                 | :--------                             | :-------                             | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_CALL_VALIDATION_FINISH | ctrlm_rcu_iarm_validation_finish_t * | Completes the controller validation |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_CALL_CONTROLLER_STATUS | ctrlm_rcu_iarm_controller_status_t * | Provides information about a controller |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_CALL_IR_CODE_SET       | ctrlm_rcu_iarm_ir_code_t *           | Sets an IR code in the controller |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_CALL_RIB_REQUEST_GET   | ctrlm_rcu_iarm_rib_request_t *       | Retrieves an attribute from the controller's RIB |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_CALL_RIB_REQUEST_SET   | ctrlm_rcu_iarm_rib_request_t *       | Sets an attribute in the controller's RIB  |
///
/// Examples:
///
/// Get a controller's status:
///
///     IARM_Result_t                      result;
///     ctrlm_rcu_iarm_controller_status_t status;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_CALL_CONTROLLER_STATUS, (void *)&status, sizeof(status));
///     if(IARM_RESULT_SUCCESS == result && CTRLM_IARM_CALL_RESULT_SUCCESS == status.result) {
///         // Status was successfully retrieved
///     }
///
/// Set an attribute in the controller's RIB:
///
///     IARM_Result_t                result;
///     ctrlm_rcu_iarm_rib_request_t request;
///     request.network_id    = 0x00;                                 // The network ID
///     request.controller_id = 0x01;                                 // The controller ID
///     request.attribute     = CTRLM_RCU_RIB_ATTR_ID_DOWNLOAD_RATE;  // The RIB attribute
///     request.index         = 0;                                    // The index into the attribute
///     request.length        = CTRLM_RCU_RIB_ATTR_LEN_DOWNLOAD_RATE; // The attribute data length
///     request.data[0]       = 0;                                    // The attribute data
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_CALL_RIB_REQUEST_SET, (void *)&request, sizeof(request));
///     if(IARM_RESULT_SUCCESS == result && CTRLM_IARM_CALL_RESULT_SUCCESS == request.result) {
///         // RIB attribute was set successfully
///     }
///     }
///
/// @}

/// @addtogroup CTRLM_IPC_RCU_EVENTS
/// @{
/// @brief Broadcast Events accessible via IARM bus
/// @details The IARM bus uses events to broadcast information to interested clients. An event is sent separately to each client. There are no return values for an event and no
/// guarantee that a client will receive the event.  Each event has a different argument structure according to the information being transferred to the clients.  The events that the
/// Remote Control component in Control Manager generates and subscribes to are detailed below.
///
/// - - -
/// Event Generation (Broadcast)
/// ----------------------------
///
/// The Remote Control component generates events that can be received by other processes connected to the IARM bus. The following events
/// are registered during initialization:
///
/// | Bus Name                 | Event Name                                 | Argument                                  | Description |
/// | :-------                 | :---------                                 | :-------                                  | :---------- |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_KEY_PRESS             | ctrlm_rcu_iarm_event_key_press_t *        | Generated each time a key event occurs (down, repeat, up) |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_VALIDATION_BEGIN      | ctrlm_rcu_iarm_event_validation_begin_t * | Generated at the beginning of a validation attempt |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_VALIDATION_KEY_UPDATE | ctrlm_rcu_iarm_event_key_press_t *        | Generated when the user enters a validation code digit/letter |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_VALIDATION_END        | ctrlm_rcu_iarm_event_validation_end_t *   | Generated at the end of a validation attempt |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_KEY_SETUP             | ctrlm_rcu_iarm_event_key_setup_t *        | Generated when the setup key combo is entered on a controller |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_KEY_GHOST             | ctrlm_rcu_iarm_event_key_ghost_t *        | Generated when a ghost code is received from a controller |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_RIB_ACCESS_CONTROLLER | ctrlm_rcu_iarm_event_rib_entry_access_t * | Generated when a controller accesses the RIB |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_RCU_IARM_EVENT_RCU_STATUS            | ctrlm_main_iarm_event_json_t &            | Generated when a controller status has been updated by a network |
///
/// IARM events are available on a subscription basis. In order to receive an event, a client must explicitly register to receive the event by calling
/// IARM_Bus_RegisterEventHandler() with the bus name, event name and a @link IARM_EventHandler_t handler function@endlink. Events may be generated at any time by the
/// Remote Control component. All events are asynchronous.
///
/// Examples:
///
/// Register for a Remote Control event:
///
///     IARM_Result_t result;
///
///     result = IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_KEY_PRESS, key_press_handler_func);
///     if(IARM_RESULT_SUCCESS == result) {
///         // Event registration was set successfully
///     }
///     }
///
/// @}
///
/// @addtogroup CTRLM_IPC_RCU_COMMS
/// @{
/// @brief Communication Interfaces
/// @details The following diagrams detail the main communication paths for the RCU component.
///
/// ---------------------------------
/// RF4CE Controller Binding - Normal
/// ---------------------------------
///
/// The binding procedure between the controller and target (STB) is initiated on the controller.  The controller will send out a discovery request
/// and collect the responses.  The responses are prioritized (ordered) according to the likelihood that it is the intended target.  The controller
/// will attempt to pair and bind with each target once until it has successfully bound to a target, the operation is aborted, or it has exhausted
/// the candidate list.  The flow of the binding procedure is shown below.
///
/// Controller View
///
/// @dot
/// digraph BINDING_COMMS_CONTROLLER {
///     "UNBOUND"         [shape="ellipse", fontname=Helvetica, fontsize=10, label="UNBOUND"];
///     "IS_DISC_OK"      [shape="diamond", fontname=Helvetica, fontsize=10, label="Successful\ndiscovery?"];
///     "IS_LIST_EMPTY"   [shape="diamond", fontname=Helvetica, fontsize=10, label="Is another\ntarget available?"];
///     "IS_PAIR_OK"      [shape="diamond", fontname=Helvetica, fontsize=10, label="Pairing\nsuccess?"];
///     "IS_VAL_OK"       [shape="diamond", fontname=Helvetica, fontsize=10, label="Validation\nsuccess?"];
///     "BOUND"           [shape="ellipse", fontname=Helvetica, fontsize=10, label="BOUND"];
///
///     node[shape=none, width=0, height=0, label=""];
///     {rank=same;  a3; a4; a5;}
///
///     "UNBOUND"       -> "IS_DISC_OK"    [dir="forward", fontname=Helvetica, fontsize=10,label="  Discovery initiated\non controller"];
///     "IS_DISC_OK"    -> "IS_LIST_EMPTY" [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, sort pairing\ntarget list."];
///     "IS_LIST_EMPTY" -> "IS_PAIR_OK"    [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, perform\npairing procedure."];
///     "IS_PAIR_OK"    -> "IS_VAL_OK"     [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, perform\nvalidation procedure."];
///     "IS_VAL_OK"     -> "BOUND"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes"];
///
///     {rank=same; UNBOUND; IS_DISC_OK; IS_LIST_EMPTY; IS_PAIR_OK; IS_VAL_OK; BOUND;}
///     {rank=same; b1; b2; b3; b4; b5;}
///
///     edge[dir=none];
///     a3 -> a4 -> a5
///     b1 -> b2 -> b3
///     a4 -> "IS_PAIR_OK"    [fontname=Helvetica, fontsize=10,label="  No, go to next target."];
///     a5 -> "IS_VAL_OK"     [fontname=Helvetica, fontsize=10,label="  No, unpair and go\n  to next target."];
///     b2 -> "IS_DISC_OK"    [fontname=Helvetica, fontsize=10,label="  No"];
///     "IS_LIST_EMPTY" -> b3 [fontname=Helvetica, fontsize=10,label="  No"];
///
///     edge[dir=forward];
///     b1 -> "UNBOUND";
///     a3 -> "IS_LIST_EMPTY";
/// }
/// \enddot
///
/// Target View (Control Manager)
///
/// @dot
/// digraph BINDING_COMMS_TARGET {
///     node[fontname=Helvetica, fontsize=10];
///     b1 [shape="ellipse", label="IDLE"];
///     b2 [shape="diamond", label="Is bind table\lfull or binding\lin progress?"];
///     b3 [shape="ellipse", label="VALIDATION\lIN PROGRESS"];
///     b4 [shape="ellipse", label="VALIDATION\lCOMPLETE"];
///
///     node[shape=none, width=0, height=0, label=""];
///     {rank=same; a1, a2, a3; a4;}
///     {rank=same; b1; b2; b3; b4;}
///     {rank=same; c1; c2; c3; c4;}
///
///     edge[dir=none];
///     c1 -> c2 -> c3 -> c4
///
///     edge[dir=forward, fontname=Helvetica, fontsize=10];
///     b1 -> b2 [label="  HAL bind validation\l  start called"];
///     b3 -> b3 [label="  Other key pressed\lon this remote"];
///     b2 -> b3 [label="  No, send IARM validation\l  start event"];
///     a3 -> b3 [dir=none, label="  IARM validation\l  complete called"]
///     a3 -> a4 [dir=none]
///     a4 -> b4
///
///     edge[dir=none, fontname=Helvetica, fontsize=10];
///     b1 -> c1 [dir=back];
///     b2 -> c2 [dir=none, label="  Yes, call HAL bind validation finish (full, pending)"];
///     b3 -> c3 [dir=none, label="  Infinity or exit key pressed on this remote or collision occurred.\l  Broadcast IARM validation complete event\lCall HAL bind validation finish (abort, collision)"];
///     b4 -> c4 [dir=none, label="  Broadcast IARM validation complete event\l  Call HAL validation finish (success, failed, timeout, error)"];
///
/// }
/// \enddot
///
/// ------------------------------------
/// RF4CE Controller Binding - Automatic
/// ------------------------------------
///
/// When the remote control is set to CLIP Discovery mode (setup + D), the controller will attempt to automatically bind without any user interaction required.  Several keys on the remote control
/// are designated as autobinding keys (replay, exit, mic, menu, guide, last, info, A, B, C, D).  When one of these keys are released, the controller will send the IR code for that key followed by the
/// autobind ghost code.  After a brief delay, it will issue a discovery request using a vendor specific device id.  If the controller receives more than the required number of responses from a single
/// target, it will attempt to bind with this target.
///
/// @dot
/// digraph AUTOBIND_COMMS {
///     "UNBOUND"         [shape="ellipse", fontname=Helvetica, fontsize=10, label="UNBOUND"];
///     "DISC_RSP_RXD"    [shape="diamond", fontname=Helvetica, fontsize=10, label="Exactly one discovery\nresponse received?"];
///     "FAIL_LIMIT"      [shape="diamond", fontname=Helvetica, fontsize=10, label="Failure count greater\nthan fail threshold?"];
///     "SUCCESS_LIMIT"   [shape="diamond", fontname=Helvetica, fontsize=10, label="Success count greater\nthan pair threshold?"];
///     "BOUND"       [shape="ellipse", fontname=Helvetica, fontsize=10, label="BOUND"];
///
///     node[shape=none, width=0, height=0, label=""];
///     {rank=same;  a1; a2; a3;}
///
///     "UNBOUND"          -> "DISC_RSP_RXD"  [dir="forward", fontname=Helvetica, fontsize=10,label="  Autobinding key is released.\nSend IR ghost code.\nDelay. Start discovery."];
///     "DISC_RSP_RXD"     -> "SUCCESS_LIMIT" [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, increment\nsuccess count"];
///     "SUCCESS_LIMIT"    -> "BOUND"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, complete\nbinding procedure."];
///
///     {rank=same; UNBOUND; DISC_RSP_RXD; SUCCESS_LIMIT;  BOUND;}
///     {rank=same; b1; FAIL_LIMIT; }
///     {rank=same; c1; c2; }
///
///     edge[dir=none];
///     a1 -> a2 -> a3
///     c1 -> c2
///     a3 -> SUCCESS_LIMIT [fontname=Helvetica, fontsize=10,label="  No"];
///     b1 -> FAIL_LIMIT    [fontname=Helvetica, fontsize=10,label="  No."];
///     FAIL_LIMIT -> c2    [fontname=Helvetica, fontsize=10,label="  Yes, set success and\n  failure counters to zero."];
///     c1 -> b1
///
///     edge[dir=forward];
///     DISC_RSP_RXD -> FAIL_LIMIT [fontname=Helvetica, fontsize=10,label="  No, increment\nfailure count."];
///     b1 -> UNBOUND;
///     a1 -> UNBOUND;
/// }
/// \enddot
///
/// -------------------------
/// IP Remote Control Binding
/// -------------------------
///
/// The binding procedure between the IP based controller and target (STB) is initiated on the target.  The user must navigate to the IP Remote Control
/// configuration screen to listen for incoming pairing requests.  While on this screen, ...
/// The flow of the binding procedure is shown below.
///
/// @dot
/// digraph BINDING_COMMS {
///     "UNBOUND"         [shape="ellipse", fontname=Helvetica, fontsize=10, label="UNBOUND"];
///     "IS_DISC_OK"      [shape="diamond", fontname=Helvetica, fontsize=10, label="Devices\nfound?"];
///     "IS_USER_DONE"    [shape="diamond", fontname=Helvetica, fontsize=10, label="Pair\nanother\ncontroller?"];
///     "IS_PAIR_OK"      [shape="diamond", fontname=Helvetica, fontsize=10, label="Pairing\nsuccess?"];
///     "IS_VAL_OK"       [shape="diamond", fontname=Helvetica, fontsize=10, label="Validation\nsuccess?"];
///     "BOUND"           [shape="ellipse", fontname=Helvetica, fontsize=10, label="BOUND"];
///
///     node[shape=none, width=0, height=0, label=""];
///     {rank=same;  a3; a4; a5;}
///
///     "UNBOUND"       -> "IS_DISC_OK"    [dir="forward", fontname=Helvetica, fontsize=10,label="  User navigates to IP RCU\nconfig screen. Discovery\nrequest sent by controller."];
///     "IS_DISC_OK"    -> "IS_USER_DONE"  [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, user is presented\nwith controller list."];
///     "IS_USER_DONE"  -> "IS_PAIR_OK"    [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, perform\npairing procedure\non selected controller."];
///     "IS_PAIR_OK"    -> "IS_VAL_OK"     [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, perform\nvalidation procedure."];
///     "IS_VAL_OK"     -> "BOUND"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes"];
///
///     {rank=same; UNBOUND; IS_DISC_OK; IS_USER_DONE; IS_PAIR_OK; IS_VAL_OK; BOUND;}
///     {rank=same; b1; b2; b3; b4; b5;}
///
///     edge[dir=none];
///     a3 -> a4 -> a5
//     b1 -> b2 -> b3
///     a4 -> "IS_PAIR_OK"    [fontname=Helvetica, fontsize=10,label="  No"];
///     a5 -> "IS_VAL_OK"     [fontname=Helvetica, fontsize=10,label="  No, unpair\ncontroller."];
///
///     edge[dir=forward];
///     "IS_DISC_OK" -> b2    [fontname=Helvetica, fontsize=10,label="  No, user exited."];
///     "IS_USER_DONE" -> b3  [fontname=Helvetica, fontsize=10,label="  No, user exited."];
///     a3 -> "IS_USER_DONE";
/// }
/// \enddot
///
/// -------------------------
/// Controller Key Press Flow
/// -------------------------
///
/// Controller key press events are generated when the user presses, holds and releases a key on a bound controller.
///
///
/// @dot
/// digraph CTRL_StateMachine {
///     "RCU"   [shape="ellipse", fontname=Helvetica, fontsize=10, label="Remote Control"];
///     "CTRLD" [shape="ellipse", fontname=Helvetica, fontsize=10, label="Control Driver"];
///     "CTRLM" [shape="ellipse", fontname=Helvetica, fontsize=10, label="Control Manager"];
///     "IRM"   [shape="ellipse", fontname=Helvetica, fontsize=10, label="IR Manager"];
///     "SVCM"  [shape="ellipse", fontname=Helvetica, fontsize=10, label="Service Manager"];
///
///     "RCU"   -> "CTRLD" [fontname=Helvetica, fontsize=10,label="  User presses, holds or\n releases a key"];
///     "CTRLD" -> "CTRLM" [fontname=Helvetica, fontsize=10,label="  Key received in driver"];
///     "CTRLM" -> "IRM"   [fontname=Helvetica, fontsize=10,label="  Key Broadcast  "];
///     "CTRLM" -> "SVCM"  [fontname=Helvetica, fontsize=10,label=""];
/// }
/// \enddot
/// @}
/// @}
#endif
