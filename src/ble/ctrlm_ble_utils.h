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
#ifndef _CTRLM_BLE_UTILS_H_
#define _CTRLM_BLE_UTILS_H_

#include <string.h>
#include <jansson.h>
#include "ctrlm_hal_ble.h"
#include "ctrlm_ble_controller.h"
#include "bleaddress.h"


typedef enum {
    CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_ADDED,
    CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_REMOVED,
    CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEEPSLEEP_WAKEUP,
    CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TICKLE,
    CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TERMINATE
} ctrlm_ble_key_queue_msg_type_t;

typedef struct {
    ctrlm_ble_key_queue_msg_type_t type;
} ctrlm_ble_key_queue_msg_header_t;

typedef struct {
    ctrlm_ble_key_queue_msg_header_t    header;
    BleAddress                          address;
} ctrlm_ble_key_queue_device_changed_msg_t;

typedef struct {
    ctrlm_ble_key_queue_msg_header_t    header;
    ctrlm_hal_thread_monitor_response_t *response;
} ctrlm_ble_key_thread_monitor_msg_t;



uint16_t ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(unsigned char usb_code);
unsigned char ctrlm_ble_utils_ConvertLinuxCodeToUsbKdb(uint16_t linux_code);
uint8_t ctrlm_ble_utils_VendorToIrControlBitmask(ctrlm_irdb_vendor_t vendor);
ctrlm_irdb_vendor_t ctrlm_ble_utils_IrControlToVendor(uint8_t irControl);

std::string ctrlm_ble_utils_BuildDBusDeviceObjectPath(const char *path_base, unsigned long long ieee_address);
unsigned long long ctrlm_ble_utils_GetIEEEAddressFromObjPath(std::string obj_path);
void ctrlm_ble_utils_PrintRCUStatus(ctrlm_hal_ble_RcuStatusData_t *status);
const char *ctrlm_ble_unpair_reason_str(ctrlm_ble_RcuUnpairReason_t reason);
const char *ctrlm_ble_reboot_reason_str(ctrlm_ble_RcuRebootReason_t reason);
const char *ctrlm_ble_rcu_action_str(ctrlm_ble_RcuAction_t reason);
std::string ctrlm_ble_irdbs_supported_str(std::vector<ctrlm_irdb_vendor_t> vendors);

#endif
