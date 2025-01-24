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
#ifndef _CTRLM_HAL_BLE_H_
#define _CTRLM_HAL_BLE_H_

#include <stdint.h>
#include <linux/input.h>
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_ble.h"
#include "ctrlm_irdb.h"


#define CTRLM_HAL_BLE_MAX_IRDBS_SUPPORTED (8)

/// @file ctrlm_hal_ble.h
///
/// @defgroup CTRL_MGR_HAL_BLE Control Manager BLE Network HAL API
/// @{

/// @addtogroup HAL_BLE_Enums   Enumerations
/// @{
/// @brief Enumerated Types
/// @details The Control Manager HAL provides enumerated types for logical groups of values.


/// @brief Supported voice data encoding types.
typedef enum {
   CTRLM_HAL_BLE_ENCODING_PCM = 0,
   CTRLM_HAL_BLE_ENCODING_ADPCM,
   CTRLM_HAL_BLE_ENCODING_UNKNOWN
} ctrlm_hal_ble_VoiceEncoding_t;

/// @brief Supported voice stream end types.
typedef enum {
   CTRLM_HAL_BLE_VOICE_STREAM_END_ON_KEY_UP = 0,
   CTRLM_HAL_BLE_VOICE_STREAM_END_ON_AUDIO_DURATION,
   CTRLM_HAL_BLE_VOICE_STREAM_END_ON_EOS,
   CTRLM_HAL_BLE_VOICE_STREAM_END_UNKNOWN
} ctrlm_hal_ble_VoiceStreamEnd_t;

/// @brief Supported voice data encoding types.
typedef enum {
   CTRLM_HAL_BLE_PROPERTY_IEEE_ADDDRESS = 0,
   CTRLM_HAL_BLE_PROPERTY_DEVICE_ID,
   CTRLM_HAL_BLE_PROPERTY_NAME,
   CTRLM_HAL_BLE_PROPERTY_MANUFACTURER,
   CTRLM_HAL_BLE_PROPERTY_MODEL,
   CTRLM_HAL_BLE_PROPERTY_SERIAL_NUMBER,
   CTRLM_HAL_BLE_PROPERTY_HW_REVISION,
   CTRLM_HAL_BLE_PROPERTY_FW_REVISION,
   CTRLM_HAL_BLE_PROPERTY_SW_REVISION,
   CTRLM_HAL_BLE_PROPERTY_TOUCH_MODE,
   CTRLM_HAL_BLE_PROPERTY_TOUCH_MODE_SETTABLE,
   CTRLM_HAL_BLE_PROPERTY_BATTERY_LEVEL,
   CTRLM_HAL_BLE_PROPERTY_IR_CODE,
   CTRLM_HAL_BLE_PROPERTY_CONNECTED,
   CTRLM_HAL_BLE_PROPERTY_IS_PAIRING,
   CTRLM_HAL_BLE_PROPERTY_PAIRING_CODE,
   CTRLM_HAL_BLE_PROPERTY_AUDIO_STREAMING,
   CTRLM_HAL_BLE_PROPERTY_AUDIO_GAIN_LEVEL,
   CTRLM_HAL_BLE_PROPERTY_AUDIO_CODECS,
   CTRLM_HAL_BLE_PROPERTY_IR_STATE,
   CTRLM_HAL_BLE_PROPERTY_STATE,
   CTRLM_HAL_BLE_PROPERTY_IS_UPGRADING,
   CTRLM_HAL_BLE_PROPERTY_UPGRADE_PROGRESS,
   CTRLM_HAL_BLE_PROPERTY_UPGRADE_ERROR,
   CTRLM_HAL_BLE_PROPERTY_UNPAIR_REASON,
   CTRLM_HAL_BLE_PROPERTY_REBOOT_REASON,
   CTRLM_HAL_BLE_PROPERTY_LAST_WAKEUP_KEY,
   CTRLM_HAL_BLE_PROPERTY_WAKEUP_CONFIG,
   CTRLM_HAL_BLE_PROPERTY_WAKEUP_CUSTOM_LIST,
   CTRLM_HAL_BLE_PROPERTY_IRDBS_SUPPORTED,
   CTRLM_HAL_BLE_PROPERTY_UNKNOWN
} ctrlm_hal_ble_RcuProperty_t;
/// @}

/// @addtogroup HAL_BLE_Structs Structures
/// @{
/// @brief Structure Definitions
/// @details The Control Manager HAL provides structures that are used in calls to the HAL networks.

typedef struct {
   int                           device_minor_id;
   uint8_t                       battery_level;
   bool                          connected;
   unsigned long long            ieee_address;
   char                          fw_revision[CTRLM_MAX_PARAM_STR_LEN];
   char                          hw_revision[CTRLM_MAX_PARAM_STR_LEN];
   char                          sw_revision[CTRLM_MAX_PARAM_STR_LEN];
   char                          manufacturer[CTRLM_MAX_PARAM_STR_LEN];
   char                          model[CTRLM_MAX_PARAM_STR_LEN];
   char                          name[CTRLM_MAX_PARAM_STR_LEN];
   char                          serial_number[CTRLM_MAX_PARAM_STR_LEN];
   int                           ir_code;
   uint8_t                       audio_gain_level;
   uint32_t                      audio_codecs;
   bool                          audio_streaming;
   unsigned int                  touch_mode;
   bool                          touch_mode_settable;
   bool                          is_upgrading;
   int                           upgrade_progress;
   char                          upgrade_error[CTRLM_MAX_PARAM_STR_LEN];
   ctrlm_ble_RcuUnpairReason_t   unpair_reason;
   ctrlm_ble_RcuRebootReason_t   reboot_reason;
   char                          assert_report[CTRLM_RCU_ASSERT_REPORT_MAX_SIZE+1];
   uint16_t                      last_wakeup_key;
   uint8_t                       wakeup_config;
   int                           wakeup_custom_list[CTRLM_WAKEUP_CONFIG_LIST_MAX_SIZE];
   int                           wakeup_custom_list_size;
   ctrlm_irdb_vendor_t           irdbs_supported[CTRLM_HAL_BLE_MAX_IRDBS_SUPPORTED];
   int                           num_irdbs_supported;
} ctrlm_hal_ble_rcu_data_t;

typedef struct {
   ctrlm_rf_pair_state_t         state;
   ctrlm_ir_state_t              ir_state;
   bool                          is_pairing;
   int                           pairing_code;
   ctrlm_hal_ble_RcuProperty_t   property_updated;
   ctrlm_hal_ble_rcu_data_t      rcu_data;
} ctrlm_hal_ble_RcuStatusData_t;

typedef struct {
   unsigned long long         ieee_address;
   struct input_event         event;
} ctrlm_hal_ble_IndKeypress_params_t;

typedef struct {
   ctrlm_hal_ble_rcu_data_t   rcu_data;
} ctrlm_hal_ble_IndPaired_params_t;

typedef struct {
   unsigned long long            ieee_address;
} ctrlm_hal_ble_IndUnPaired_params_t;

typedef struct {
   double   minInterval;
   double   maxInterval;
   int      latency;
   int      supvTimeout;
} ctrlm_hal_ble_connection_params_t;

typedef struct {
   unsigned long long                ieee_address;
   ctrlm_hal_ble_connection_params_t connParams;
} ctrlm_hal_ble_SetBLEConnParams_params_t;


/// @}

/// @}

/// @addtogroup HAL_BLE_Callback_Functions - Callback Function Prototypes
/// @{
/// @brief Functions that are implemented by the BLE Network device that the HAL layer can call
/// @details ControlMgr provides a set of functions that the BLE HAL layer can call.

/// @brief BLE Confirm Init Parameters Structure
/// @details The structure which is passed from the HAL Network device in the BLE initialization confirmation.
typedef struct {
   ctrlm_hal_result_t                           result;
   char                                         version[CTRLM_HAL_NETWORK_VERSION_STRING_SIZE];
} ctrlm_hal_ble_cfm_init_params_t;

/// @}
#endif
