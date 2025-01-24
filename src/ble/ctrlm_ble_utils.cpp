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

#include "ctrlm_ble_utils.h"

#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_hal.h"
#include "ctrlm_utils.h"

using namespace std;


/// @brief keymapping for USB Keyboard usage page 0x07.  Index is the keyboard
/// code sent from the remote, and the value is the linux code
static const unsigned char hid_keyboard_to_linux[256] = {
      0,   0,   0,   0,   30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,  38,
      50,  49,  24,  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,  2,   3,
      4,   5,   6,   7,   8,   9,   10,  11,  28,  1,   14,  15,  57,  12,  13,  26,
      27,  43,  43,  39,  40,  41,  51,  52,  53,  58,  59,  60,  61,  62,  63,  64,
      65,  66,  67,  68,  87,  88,  99,  70,  119, 110, 102, 104, 111, 107, 109, 106,
      105, 108, 103, 69,  98,  55,  74,  78,  96,  79,  80,  81,  75,  76,  77,  71,
      72,  73,  82,  83,  86,  127, 116, 117, 183, 184, 185, 186, 187, 188, 189, 190,
      191, 192, 193, 194, 134, 138, 130, 132, 128, 129, 131, 137, 133, 135, 136, 113,
      115, 114, 0,   0,   0,   121, 0,   89,  93,  124, 92,  94,  95,  0,   0,   0,
      122, 123, 90,  91,  85,  0,   0,   0,   0,   0,   0,   0,   111, 0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   179, 180, 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   111, 0,   0,   0,   0,   0,   0,   0,
      29,  42,  56,  125, 97,  54,  100, 126, 164, 166, 165, 163, 161, 115, 114, 113,
      150, 158, 159, 128, 136, 177, 178, 176, 142, 152, 173, 140, 0,   0,   0,   0
};

uint16_t ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(unsigned char usb_code) {
    return hid_keyboard_to_linux[usb_code] ? hid_keyboard_to_linux[usb_code] : CTRLM_KEY_CODE_INVALID;
}
unsigned char ctrlm_ble_utils_ConvertLinuxCodeToUsbKdb(uint16_t linux_code) {
    for (unsigned int i = 0; i < sizeof(hid_keyboard_to_linux)/sizeof(hid_keyboard_to_linux[0]); i++) {
        if (hid_keyboard_to_linux[i] == linux_code) {
            return i;
        }
    }
    return 0;
}


const std::map<uint8_t, ctrlm_irdb_vendor_t> irdbVendorMap
{
   {0x2, CTRLM_IRDB_VENDOR_UEI},
   {0x4, CTRLM_IRDB_VENDOR_RUWIDO},
};

uint8_t ctrlm_ble_utils_VendorToIrControlBitmask(ctrlm_irdb_vendor_t vendor)
{
    for (auto const &it : irdbVendorMap) {
        if (it.second == vendor) {
           return it.first;
        }
    }
    return 0;
}

ctrlm_irdb_vendor_t ctrlm_ble_utils_IrControlToVendor(uint8_t irControl)
{
    auto it = irdbVendorMap.find(irControl);
    return (it != irdbVendorMap.end()) ? it->second : CTRLM_IRDB_VENDOR_INVALID;
}


string ctrlm_ble_utils_BuildDBusDeviceObjectPath(const char *path_base, unsigned long long ieee_address)
{
    char objectPath[CTRLM_MAX_PARAM_STR_LEN];
    errno_t safec_rc = sprintf_s(objectPath, sizeof(objectPath), "%s/device_%02X_%02X_%02X_%02X_%02X_%02X", path_base,
                                                            0xFF & (unsigned int)(ieee_address >> 40),
                                                            0xFF & (unsigned int)(ieee_address >> 32),
                                                            0xFF & (unsigned int)(ieee_address >> 24),
                                                            0xFF & (unsigned int)(ieee_address >> 16),
                                                            0xFF & (unsigned int)(ieee_address >> 8),
                                                            0xFF & (unsigned int)(ieee_address));
    if(safec_rc < EOK) {
       ERR_CHK(safec_rc);
    }

    XLOGD_DEBUG("device_path: <%s>", objectPath);
    return string(objectPath);
}

unsigned long long ctrlm_ble_utils_GetIEEEAddressFromObjPath(std::string obj_path)
{
    // Dbus object path is of the form .../blercu/device_E8_0F_C8_10_33_D8
    // Parse out the IEEE address

    string addr_start_delim("device_");
    size_t addr_start_pos = obj_path.find(addr_start_delim);
    if (string::npos != addr_start_pos) {
        string addr_str = obj_path.substr(addr_start_pos + addr_start_delim.length(), 17);
        // XLOGD_DEBUG("addr_str: <%s>", addr_str.c_str());
        return ctrlm_convert_mac_string_to_long(addr_str.c_str());
    } else {
        return -1;
    }
}

const char *ctrlm_ble_unpair_reason_str(ctrlm_ble_RcuUnpairReason_t reason) {
    switch(reason) {
        case CTRLM_BLE_RCU_UNPAIR_REASON_NONE:          return("NONE");
        case CTRLM_BLE_RCU_UNPAIR_REASON_SFM:           return("SPECIAL_FUNCTION_MODE");
        case CTRLM_BLE_RCU_UNPAIR_REASON_FACTORY_RESET: return("FACTORY_RESET");
        case CTRLM_BLE_RCU_UNPAIR_REASON_RCU_ACTION:    return("RCU_ACTION");
        case CTRLM_BLE_RCU_UNPAIR_REASON_INVALID:       return("INVALID");
        default:                                        return("INVALID__TYPE");
    }
}

const char *ctrlm_ble_reboot_reason_str(ctrlm_ble_RcuRebootReason_t reason) {
    switch(reason) {
        case CTRLM_BLE_RCU_REBOOT_REASON_FIRST_BOOT:        return("FIRST_BOOT");
        case CTRLM_BLE_RCU_REBOOT_REASON_FACTORY_RESET:     return("FACTORY_RESET");
        case CTRLM_BLE_RCU_REBOOT_REASON_NEW_BATTERIES:     return("NEW_BATTERIES");
        case CTRLM_BLE_RCU_REBOOT_REASON_ASSERT:            return("ASSERT");
        case CTRLM_BLE_RCU_REBOOT_REASON_BATTERY_INSERTION: return("BATTERY_INSERTION");
        case CTRLM_BLE_RCU_REBOOT_REASON_RCU_ACTION:        return("RCU_ACTION");
        case CTRLM_BLE_RCU_REBOOT_REASON_FW_UPDATE:         return("FW_UPDATE");
        case CTRLM_BLE_RCU_REBOOT_REASON_INVALID:           return("INVALID");
        default:                                            return("INVALID__TYPE");
    }
}

const char *ctrlm_ble_rcu_action_str(ctrlm_ble_RcuAction_t reason) {
    switch(reason) {
        case CTRLM_BLE_RCU_ACTION_REBOOT:           return("REBOOT");
        case CTRLM_BLE_RCU_ACTION_FACTORY_RESET:    return("FACTORY_RESET");
        case CTRLM_BLE_RCU_ACTION_UNPAIR:           return("UNPAIR");
        case CTRLM_BLE_RCU_ACTION_INVALID:          return("INVALID");
        default:                                    return("INVALID__TYPE");
    }
}

string ctrlm_ble_irdbs_supported_str(std::vector<ctrlm_irdb_vendor_t> vendors) {
   string supported_irdbs;

   for (auto const &vendor : vendors) {
      supported_irdbs += (supported_irdbs.empty()) ? "" : ", ";
      supported_irdbs += ctrlm_irdb_vendor_str(vendor);
   }
   return supported_irdbs;
}
