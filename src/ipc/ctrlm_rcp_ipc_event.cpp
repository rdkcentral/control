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

#include <algorithm>
#include "ctrlm_rcp_ipc_event.h"
#include "ctrlm_network.h"
#include "ctrlm_utils.h"
#include "ctrlm_validation.h"

using namespace rcp_net_status_json_keys;

ctrlm_rcp_ipc_controller_status_t::ctrlm_rcp_ipc_controller_status_t(const ctrlm_obj_controller_t &controller) :
    ieee_address_(controller.ieee_address_get()),
    connected_(controller.get_connected()),
    controller_id_(controller.controller_id_get()),
    device_id_(controller.get_device_minor_id()),
    name_(controller.get_name()),
    manufacturer_(controller.get_manufacturer()),
    model_(controller.get_model()),
    software_version_(controller.get_sw_revision().to_string()),
    hardware_version_(controller.get_hw_revision().to_string()),
    bootloader_version_(controller.get_fw_revision()),
    serial_number_(controller.get_serial_number()),
    battery_level_(controller.get_battery_percent()),
    tv_irdb_code_(controller.get_irdb_entry_id_name_tv()),
    avr_irdb_code_(controller.get_irdb_entry_id_name_avr()),
    wakeup_key_code_((CTRLM_KEY_CODE_INVALID == controller.get_last_wakeup_key()) ? -1 : controller.get_last_wakeup_key()),
    wakeup_config_(controller.get_wakeup_config()),
    wakeup_custom_list_(controller.get_wakeup_custom_list()),
    upgrade_session_id_(controller.get_upgrade_session_uuid())
{
}

ctrlm_rcp_ipc_controller_status_t::~ctrlm_rcp_ipc_controller_status_t()
{
}

json_t *ctrlm_rcp_ipc_controller_status_t::to_json() const
{
    json_t *remote_data = json_object();
    int err             = 0;

    err |= json_object_set_new_nocheck(remote_data, MAC_ADDRESS,        json_string(ieee_address_.to_string().c_str()));
    err |= json_object_set_new_nocheck(remote_data, CONNECTED,          json_boolean(connected_));
    err |= json_object_set_new_nocheck(remote_data, NAME,               json_string(name_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, REMOTE_ID,          json_integer(controller_id_));
    err |= json_object_set_new_nocheck(remote_data, DEVICE_ID,          json_integer(device_id_));
    err |= json_object_set_new_nocheck(remote_data, MAKE,               json_string(manufacturer_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, MODEL,              json_string(model_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, HW_VERSION,         json_string(hardware_version_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, SW_VERSION,         json_string(software_version_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, BTL_VERSION,        json_string(bootloader_version_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, SERIAL_NUMBER,      json_string(serial_number_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, BATTERY_PERCENT,    json_integer(battery_level_));
    err |= json_object_set_new_nocheck(remote_data, TV_IR_CODE,         json_string(tv_irdb_code_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, AMP_IR_CODE,        json_string(avr_irdb_code_.c_str()));
    err |= json_object_set_new_nocheck(remote_data, WAKEUP_KEY_CODE,    json_integer(wakeup_key_code_));
    err |= json_object_set_new_nocheck(remote_data, UPGRADE_SESSION_ID, json_string(upgrade_session_id_.c_str()));

    std::string config_str = ctrlm_rcu_wakeup_config_str(wakeup_config_);
    std::transform(config_str.begin(), config_str.end(), config_str.begin(), [](unsigned char c) {return std::tolower(c);});

    err |= json_object_set_new_nocheck(remote_data, WAKEUP_CONFIG,   json_string(config_str.c_str()));

    if (wakeup_config_ == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
        json_t *wakeup_custom_array = json_array();

        err |= json_object_set_new_nocheck(remote_data, WAKEUP_CUSTOM_LIST, wakeup_custom_array);
        for (size_t j = 0; j < wakeup_custom_list_.size(); j++) {
            err |= json_array_append_new(wakeup_custom_array, json_integer(wakeup_custom_list_[j]));
        }
    }
    return (err) ? NULL : remote_data;
}

ctrlm_rcp_ipc_net_status_t::~ctrlm_rcp_ipc_net_status_t()
{
}

json_t *ctrlm_rcp_ipc_net_status_t::to_json() const
{
    json_t *status             = json_object();
    json_t *net_type_supported = json_array();
    json_t *remote_data_array  = json_array();
    json_t *manual_pair_code   = json_array();
    int err                    = 0;

    for (auto const &it : ctrlm_network_types_get()) {
        err |= json_array_append_new(net_type_supported, json_integer(it));
    }

    for (auto const &controller_status : controller_status_list_) {
        err |= json_array_append_new(remote_data_array, controller_status.to_json());
    }

    err |= json_object_set_new_nocheck(status, REMOTE_DATA,         remote_data_array);
    err |= json_object_set_new_nocheck(status, NET_TYPES_SUPPORTED, net_type_supported);
    err |= json_object_set_new_nocheck(status, NET_TYPE,            json_integer(net_type_));
    err |= json_object_set_new_nocheck(status, PAIRING_STATE,       json_string(ctrlm_rf_pair_state_str(pair_state_)));
    err |= json_object_set_new_nocheck(status, IR_PROG_STATE,       json_string(ctrlm_ir_state_str(irdb_state_)));

    std::vector<ctrlm_key_code_t> golden_code = ctrlm_validation_golden_code_get();
    if (!golden_code.empty()) {
       for (auto digit : golden_code) {
           err |= json_array_append_new(manual_pair_code, json_integer(atoi(ctrlm_key_code_str(digit))));
       }
       err |= json_object_set_new_nocheck(status, MANUAL_PAIR_CODE, manual_pair_code);
    }

    return (err) ? NULL : status;
}

void ctrlm_rcp_ipc_net_status_t::populate_status(const ctrlm_obj_network_t &network)
{
    api_revision_ = CTRLM_MAIN_IARM_BUS_API_REVISION;
    net_type_     = network.type_get();
    net_id_       = network.network_id_get();
    result_       = CTRLM_IARM_CALL_RESULT_INVALID;

    std::vector<ctrlm_obj_controller_t *> controller_list = network.get_controller_obj_list();

    controller_qty_ = controller_list.size();
    irdb_state_ = network.get_ir_prog_state();
    pair_state_ = network.get_rf_pair_state();

    for (auto controller : controller_list) {
        controller_status_list_.push_back(ctrlm_rcp_ipc_controller_status_t(*controller));
    }
}

char *ctrlm_rcp_ipc_net_status_t::to_string() const
{
    return json_dumps(to_json(), JSON_ENCODE_ANY);
}

ctrlm_rcp_ipc_upgrade_status_t::~ctrlm_rcp_ipc_upgrade_status_t()
{
}

void ctrlm_rcp_ipc_upgrade_status_t::populate_status(const ctrlm_obj_controller_t &rcu)
{
    session_id        = rcu.get_upgrade_session_uuid();
    ieee_address_     = rcu.ieee_address_get();
    percent_complete_ = rcu.get_upgrade_progress();
    state_            = rcu.get_upgrade_state();
    error_msg_        = rcu.get_upgrade_error();
}

json_t *ctrlm_rcp_ipc_upgrade_status_t::to_json() const
{
    json_t *status = json_object();
    int err        = 0;

    err |= json_object_set_new_nocheck(status, UPGRADE_SESSION_ID, json_string(session_id.c_str()));
    err |= json_object_set_new_nocheck(status, MAC_ADDRESS,        json_string(ieee_address_.to_string().c_str()));
    err |= json_object_set_new_nocheck(status, PERCENT_COMPLETE,   json_integer(percent_complete_));
    err |= json_object_set_new_nocheck(status, UPGRADE_STATE,      json_string(ctrlm_rcu_upgrade_state_str(state_)));

    if (!error_msg_.empty()) {
        err |= json_object_set_new_nocheck(status, ERROR_STRING, json_string(error_msg_.c_str()));
    }

    return (err) ? NULL : status;
}

char *ctrlm_rcp_ipc_upgrade_status_t::to_string() const
{
    return json_dumps(to_json(), JSON_ENCODE_ANY);
}
