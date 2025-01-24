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

#ifndef _CTRLM_IPC_BTREMOTE_H_
#define _CTRLM_IPC_BTREMOTE_H_

#include "ctrlm_ipc.h"

// Something similar to this is needed to check and ensure all code using the API gets compiled together - both calls and events
// It is this value that gets passed in the "api_revision" member (by the caller for bus calls, or by ControlMgr for events sent)
#define CTRLM_BLE_IARM_BUS_API_REVISION                  (1)    ///< Revision of the BLE IARM API
#define CTRLM_BLE_MAX_IRDBS_SUPPORTED                    (8)    ///< IR Protocol characteristic is a single octet containing a bitmask of IRDB values


union daemon_logging_t {
  uint8_t log_levels;
  struct bit {
   uint8_t fatal : 1;
   uint8_t error : 1;
   uint8_t warning : 1;
   uint8_t milestone : 1;
   uint8_t info : 1;
   uint8_t debug : 1;
   } level;
};

typedef enum {
   CTRLM_BLE_RCU_UNPAIR_REASON_NONE = 0,
   CTRLM_BLE_RCU_UNPAIR_REASON_SFM,
   CTRLM_BLE_RCU_UNPAIR_REASON_FACTORY_RESET,
   CTRLM_BLE_RCU_UNPAIR_REASON_RCU_ACTION,
   CTRLM_BLE_RCU_UNPAIR_REASON_INVALID
} ctrlm_ble_RcuUnpairReason_t;

typedef enum {
   CTRLM_BLE_RCU_REBOOT_REASON_FIRST_BOOT = 0,
   CTRLM_BLE_RCU_REBOOT_REASON_FACTORY_RESET,
   CTRLM_BLE_RCU_REBOOT_REASON_NEW_BATTERIES,
   CTRLM_BLE_RCU_REBOOT_REASON_ASSERT,
   CTRLM_BLE_RCU_REBOOT_REASON_BATTERY_INSERTION,
   CTRLM_BLE_RCU_REBOOT_REASON_RCU_ACTION,
   CTRLM_BLE_RCU_REBOOT_REASON_FW_UPDATE,
   CTRLM_BLE_RCU_REBOOT_REASON_INVALID
} ctrlm_ble_RcuRebootReason_t;

typedef enum {
   CTRLM_BLE_RCU_ACTION_REBOOT = 0,
   CTRLM_BLE_RCU_ACTION_FACTORY_RESET,
   CTRLM_BLE_RCU_ACTION_UNPAIR,
   CTRLM_BLE_RCU_ACTION_INVALID
} ctrlm_ble_RcuAction_t;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IARM Calls into ControlMgr
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
   unsigned char            api_revision;                      ///< Revision of this API
   ctrlm_network_id_t       network_id;                        ///< IN - Identifier of network
   daemon_logging_t         logging;                           ///< IN/OUT - 0x020 Debug, 0x010 Info, 0x008 Milestone, 0x004 Warning, 0x002 Error, 0x001 Fatal
   ctrlm_iarm_call_result_t result;                            ///< OUT - return code of the operation
} ctrlm_iarm_call_BleLogLevels_params_t;

typedef struct {
   unsigned char                 api_revision;
   ctrlm_network_id_t            network_id;
   ctrlm_controller_id_t         controller_id;
   ctrlm_ble_RcuUnpairReason_t   reason;
   ctrlm_iarm_call_result_t      result;
} ctrlm_iarm_call_GetRcuUnpairReason_params_t;

typedef struct {
   unsigned char                 api_revision;
   ctrlm_network_id_t            network_id;
   ctrlm_controller_id_t         controller_id;
   ctrlm_ble_RcuRebootReason_t   reason;
   ctrlm_iarm_call_result_t      result;
} ctrlm_iarm_call_GetRcuRebootReason_params_t;

typedef struct {
   unsigned char                 api_revision;
   ctrlm_network_id_t            network_id;
   ctrlm_controller_id_t         controller_id;
   uint8_t                       key;
   ctrlm_iarm_call_result_t      result;
} ctrlm_iarm_call_GetRcuLastWakeupKey_params_t;

typedef struct {
   unsigned char              api_revision;
   ctrlm_network_id_t         network_id;
   ctrlm_controller_id_t      controller_id;
   ctrlm_ble_RcuAction_t      action;
   bool                       wait_for_reply;
   ctrlm_iarm_call_result_t   result;
} ctrlm_iarm_call_SendRcuAction_params_t;

typedef struct {
   unsigned char                 api_revision;
   ctrlm_network_id_t            network_id;
   ctrlm_controller_id_t         controller_id;
   uint8_t                       irdbs_supported[CTRLM_BLE_MAX_IRDBS_SUPPORTED];
   int                           num_irdbs_supported;
   ctrlm_iarm_call_result_t      result;
} ctrlm_iarm_call_IrProtocol_params_t;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// All IARM bus calls made to ControlMgr MUST include a string that specifies which call is being made
// (and ControlMgr must "register" these calls by name, with IARM)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CTRLM_BLE_IARM_CALL_GET_DAEMON_LOG_LEVELS   "Ble_GetLogLevels"   ///< IARM Call to get the logging level of BLE Daemon
#define CTRLM_BLE_IARM_CALL_SET_DAEMON_LOG_LEVELS   "Ble_SetLogLevels"   ///< IARM Call to set the logging level of BLE Daemon

#define CTRLM_BLE_IARM_CALL_GET_RCU_UNPAIR_REASON   "Ble_GetRcuUnpairReason"
#define CTRLM_BLE_IARM_CALL_GET_RCU_REBOOT_REASON   "Ble_GetRcuRebootReason"
#define CTRLM_BLE_IARM_CALL_GET_LAST_WAKEUP_KEY     "Ble_GetRcuLastWakeupKey"
#define CTRLM_BLE_IARM_CALL_SEND_RCU_ACTION         "Ble_SendRcuAction"
#define CTRLM_BLE_IARM_CALL_GET_IR_PROTOCOL_SUPPORT "Ble_GetIrProtocolSupport"  ///< IARM Call to get the IR protocol support characteristic
#define CTRLM_BLE_IARM_CALL_SET_IR_PROTOCOL_CONTROL "Ble_SetIrProtocolControl"  ///< IARM Call to set the IR protocol control characteristic

#endif   //_CTRLM_IPC_BTREMOTE_H_
