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
#ifndef _CTRLM_RCP_IPC_EVENT_H
#define _CTRLM_RCP_IPC_EVENT_H

#include <string>
#include <map>
#include <vector>
#include "ctrlm_ipc.h"
#include "ctrlm_attr_general.h"
#include "jansson.h"

namespace rcp_net_status_json_keys
{
    constexpr char const* STATUS               = "status";
    constexpr char const* NET_TYPE             = "netType";
    constexpr char const* NET_TYPES_SUPPORTED  = "netTypesSupported";
    constexpr char const* PAIRING_STATE        = "pairingState";
    constexpr char const* IR_PROG_STATE        = "irProgState";
    constexpr char const* MANUAL_PAIR_CODE     = "manualPairCode";
    constexpr char const* REMOTE_DATA          = "remoteData";

    constexpr char const* MAC_ADDRESS          = "macAddress";
    constexpr char const* CONNECTED            = "connected";
    constexpr char const* NAME                 = "name";
    constexpr char const* REMOTE_ID            = "remoteId";
    constexpr char const* DEVICE_ID            = "deviceId";
    constexpr char const* MAKE                 = "make";
    constexpr char const* MODEL                = "model";
    constexpr char const* HW_VERSION           = "hwVersion";
    constexpr char const* SW_VERSION           = "swVersion";
    constexpr char const* BTL_VERSION          = "btlVersion";
    constexpr char const* SERIAL_NUMBER        = "serialNumber";
    constexpr char const* BATTERY_PERCENT      = "batteryPercent";
    constexpr char const* TV_IR_CODE           = "tvIRCode";
    constexpr char const* AMP_IR_CODE          = "ampIRCode";
    constexpr char const* WAKEUP_KEY_CODE      = "wakeupKeyCode";
    constexpr char const* WAKEUP_CONFIG        = "wakeupConfig";
    constexpr char const* WAKEUP_CUSTOM_LIST   = "wakeupCustomList";
    constexpr char const* CUSTOM_KEYS          = "customKeys";
    constexpr char const* CONTROLLER_ID        = "controllerId";
}

class ctrlm_virtual_json_t
{
public:
    virtual ~ctrlm_virtual_json_t() {};
    virtual json_t *to_json() const = 0;
};

class ctrlm_obj_controller_t;
class ctrlm_obj_network_t;

class ctrlm_rcp_ipc_controller_status_t : public ctrlm_virtual_json_t
{
public:
    ctrlm_rcp_ipc_controller_status_t() = default;
    ctrlm_rcp_ipc_controller_status_t(const ctrlm_obj_controller_t &controller);
    ~ctrlm_rcp_ipc_controller_status_t();

    virtual json_t *to_json() const;

private:
    ctrlm_ieee_addr_t         ieee_address_;
    bool                      connected_          = false;
    ctrlm_controller_id_t     controller_id_      = 0;
    uint8_t                   device_id_          = 0;
    std::string               name_;
    std::string               manufacturer_;
    std::string               model_;
    std::string               software_version_;
    std::string               hardware_version_;
    std::string               bootloader_version_;
    std::string               serial_number_;
    uint8_t                   battery_level_      = 0;
    std::string               tv_irdb_code_;
    std::string               avr_irdb_code_;
    uint8_t                   wakeup_key_code_    = 0;
    ctrlm_rcu_wakeup_config_t wakeup_config_      = CTRLM_RCU_WAKEUP_CONFIG_INVALID;
    std::vector<uint16_t>     wakeup_custom_list_;
};

class ctrlm_rcp_ipc_net_status_t : public ctrlm_virtual_json_t
{
public:
    ctrlm_rcp_ipc_net_status_t() = default;
    ~ctrlm_rcp_ipc_net_status_t();

    virtual json_t *to_json() const;

    char              *to_string() const;
    uint8_t            get_api_revision() const                             { return api_revision_; }
    bool               get_result() const                                   { return (result_ == CTRLM_IARM_CALL_RESULT_SUCCESS) ? true : false; }
    void               set_result(ctrlm_iarm_call_result_t result)          { result_ = result; }
    ctrlm_network_id_t get_net_id() const                                   { return net_id_; }
    void               set_net_id(ctrlm_network_id_t net_id)                { net_id_ = net_id; }
    void               populate_status(const ctrlm_obj_network_t &network);

private:
    uint8_t                  api_revision_   = 0;
    ctrlm_network_type_t     net_type_       = CTRLM_NETWORK_TYPE_INVALID;
    ctrlm_network_id_t       net_id_         = 0;
    uint8_t                  controller_qty_ = 0;
    ctrlm_ir_state_t         irdb_state_     = CTRLM_IR_STATE_UNKNOWN;
    ctrlm_rf_pair_state_t    pair_state_     = CTRLM_RF_PAIR_STATE_UNKNOWN;
    ctrlm_iarm_call_result_t result_         = CTRLM_IARM_CALL_RESULT_INVALID;

    std::vector<ctrlm_rcp_ipc_controller_status_t> controller_status_list_;
};

class ctrlm_network_all_ipc_result_wrapper_t {
private:
    ctrlm_network_id_t network_id_;
    std::map<ctrlm_network_id_t, ctrlm_iarm_call_result_t> result_map_;

public:
    void set_net_id(ctrlm_network_id_t net_id) {
        network_id_ = net_id;
    }
    ctrlm_network_id_t get_net_id(void) { return network_id_; }

    void set_result(ctrlm_iarm_call_result_t res, ctrlm_network_id_t net_id) {
       result_map_[net_id] = res;
    }
    bool get_result(void) {
        if (network_id_ != CTRLM_MAIN_NETWORK_ID_ALL) {
            return (result_map_[network_id_] == CTRLM_IARM_CALL_RESULT_SUCCESS);
        } else {
            for (const auto &it : result_map_) {
                if (it.second != CTRLM_IARM_CALL_RESULT_SUCCESS) {
                    return false;
                }
            }
            return true;
        }
    }
};

template <typename T>
class ctrlm_network_all_ipc_reply_wrapper_t : public ctrlm_network_all_ipc_result_wrapper_t {
private:
    std::map<ctrlm_network_id_t, T> reply_map_;

public:
    void set_reply(T reply, ctrlm_network_id_t net_id) {
        reply_map_[net_id] = reply;
    }

    std::map<ctrlm_network_id_t, T> get_reply(void) {
        return reply_map_;
    }
};

#endif
