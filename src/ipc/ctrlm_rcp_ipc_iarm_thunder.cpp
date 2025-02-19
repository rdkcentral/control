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

#include "algorithm"
#include "ctrlm_rcp_ipc_iarm_thunder.h"
#include "ctrlm_network.h"
#include "ctrlm_log.h"
#include <uuid/uuid.h>

using namespace rcp_json_keys;
using namespace rcp_net_status_json_keys;

std::mutex                                     ctrlm_rcp_ipc_iarm_thunder_t::instance_mutex_;
std::atomic_bool                               ctrlm_rcp_ipc_iarm_thunder_t::atomic_running_{false};
bool                                           ctrlm_rcp_ipc_iarm_thunder_t::thunder_device_update_enabled_ = true;

static ctrlm_rcp_ipc_iarm_thunder_t *instance_ = nullptr;

ctrlm_rcp_ipc_iarm_thunder_t::ctrlm_rcp_ipc_iarm_thunder_t() : ctrlm_ipc_iarm_t()
{
    XLOGD_INFO("");
    configure();
}

ctrlm_rcp_ipc_iarm_thunder_t::~ctrlm_rcp_ipc_iarm_thunder_t()
{
    XLOGD_INFO("");
}

void ctrlm_rcp_ipc_iarm_thunder_t::configure(void)
{
    ctrlm_config_array_t device_update_array("device_update.methods");
    std::vector<std::string> methods;

    if (!device_update_array.get_config_array(methods)) {
        XLOGD_ERROR("Could not get the device update methods - disabling thunder device updates");
        thunder_device_update_enabled_ = false;
    } else {
        thunder_device_update_enabled_ = std::find(methods.begin(), methods.end(), "THUNDER") != methods.end() ? true : false;
        XLOGD_INFO("Device update via thunder is <%s>", thunder_device_update_enabled_ ? "true" : "false");
    }
}

ctrlm_rcp_ipc_iarm_thunder_t *ctrlm_rcp_ipc_iarm_thunder_t::get_instance()
{
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = new ctrlm_rcp_ipc_iarm_thunder_t();
    }
    return instance_;
}

bool ctrlm_rcp_ipc_iarm_thunder_t::register_ipc() const
{
    bool ret = true;

    if (is_running(atomic_running_)) {
        XLOGD_INFO("IARM calls for RCP methods already registered!");
        return ret;
    }

    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_START_PAIRING,           start_pairing))           { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_GET_RCU_STATUS,          get_net_status))          { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_LAST_KEYPRESS_GET,       get_last_keypress))       { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_FIND_MY_REMOTE,          find_my_remote))          { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_FACTORY_RESET,           factory_reset))           { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_WRITE_RCU_WAKEUP_CONFIG, write_rcu_wakeup_config)) { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_START_FIRMWARE_UPDATE,   start_fw_update))         { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_CANCEL_FIRMWARE_UPDATE,  cancel_fw_update))        { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_STATUS_FIRMWARE_UPDATE,  status_fw_update))        { ret = false; }
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_UNPAIR,                  unpair))                  { ret = false; }

    turn_on(atomic_running_);

    return(ret);
}

void ctrlm_rcp_ipc_iarm_thunder_t::deregister_ipc() const
{
    turn_off(atomic_running_);
}

bool ctrlm_rcp_ipc_iarm_thunder_t::on_status(const ctrlm_rcp_ipc_net_status_t &net_status) const
{
    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(false);
    }

    if (net_status.get_api_revision() != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Wrong ctrlm API revision - should be %d, event is %d", CTRLM_MAIN_IARM_BUS_API_REVISION, net_status.get_api_revision());
        return(false);
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, STATUS, net_status.to_json());

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(false);
    }

    return broadcast_iarm_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_RCU_STATUS, ret);
}

bool ctrlm_rcp_ipc_iarm_thunder_t::on_firmware_update_progress(const ctrlm_rcp_ipc_upgrade_status_t &upgrade_status) const
{
    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(false);
    }

    if (!thunder_device_update_enabled_) {
        XLOGD_WARN("This event is not currently enabled - discarding event");
        return(false);
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, STATUS, upgrade_status.to_json());

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(false);
    }

    return broadcast_iarm_event(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_FIRMWARE_UPDATE_PROGRESS, ret);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::start_pairing(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    int net_type = CTRLM_NETWORK_TYPE_INVALID;
    if (!config.config_value_get(NET_TYPE, net_type)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    int timeout = 0;
    if (!config.config_value_get(TIMEOUT, timeout)) {
        XLOGD_INFO("Missing %s parameter - defaulting to no timeout (0s)", TIMEOUT);
    }

    json_t *mac_addr_array = nullptr;
    std::vector<uint64_t> mac_addr_list;
    if (config.config_array_get(MAC_ADDRESS_LIST, &mac_addr_array)) {
        size_t index = 0;
        json_t *value = nullptr;

        json_array_foreach(mac_addr_array, index, value) {
            if (!json_is_string(value)) {
                XLOGD_ERROR("An element of the %s array is not a string", MAC_ADDRESS_LIST);
                return(IARM_RESULT_INVALID_PARAM);
            }

            uint64_t mac_addr = ctrlm_convert_mac_string_to_long(json_string_value(value));
            if (mac_addr == 0) {
                XLOGD_ERROR("An invalid mac address was provided <%s>", json_string_value(value));
                return(IARM_RESULT_INVALID_PARAM);
            }
            mac_addr_list.push_back(mac_addr);
        }
    }

    std::shared_ptr<ctrlm_iarm_call_StartPairing_params_t> params = std::make_shared<ctrlm_iarm_call_StartPairing_params_t>();
    params->set_net_id((net_type == CTRLM_NETWORK_TYPE_INVALID) ? CTRLM_MAIN_NETWORK_ID_ALL : ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(net_type)));
    params->timeout = timeout;
    params->ieee_address_list = mac_addr_list;

    sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_StartPairing_params_t,
                                             ctrlm_main_queue_msg_start_pairing_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_start_pairing);

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::get_net_status(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    int net_type = CTRLM_NETWORK_TYPE_INVALID;
    if (!config.config_value_get(NET_TYPE, net_type)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    std::shared_ptr<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>> params = std::make_shared<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>>();
    params->set_net_id((net_type == CTRLM_NETWORK_TYPE_INVALID) ? CTRLM_MAIN_NETWORK_ID_ALL : ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(net_type)));

    sync_send_netw_handler_to_main_queue_new<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>,
                                             ctrlm_main_queue_msg_get_rcu_status_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_get_rcu_status);

    json_t *ret = json_object();
    json_t *status_array = json_array();
    std::map<ctrlm_network_id_t, ctrlm_rcp_ipc_net_status_t> status_map = params->get_reply();
    int err = 0;

    if (params->get_net_id() != CTRLM_MAIN_NETWORK_ID_ALL) {
        err |= json_object_set_new_nocheck(ret, STATUS, status_map[params->get_net_id()].to_json());
    } else {
        for (const auto &it : status_map) {
            err |= json_array_append_new(status_array, it.second.to_json());
        }
        err |= json_object_set_new_nocheck(ret, STATUS, status_array);
    }
    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::get_last_keypress(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    int net_type = CTRLM_NETWORK_TYPE_INVALID;
    if (!config.config_value_get(NET_TYPE, net_type)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    std::shared_ptr<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_main_iarm_call_last_key_info_t>> params = std::make_shared<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_main_iarm_call_last_key_info_t>>();
    params->set_net_id((net_type == CTRLM_NETWORK_TYPE_INVALID) ? CTRLM_MAIN_NETWORK_ID_ALL : ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(net_type)));

    sync_send_netw_handler_to_main_queue_new<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_main_iarm_call_last_key_info_t>,
                                             ctrlm_main_queue_msg_get_last_keypress_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_get_last_keypress);

    json_t *ret = json_object();
    std::map<ctrlm_network_id_t, ctrlm_main_iarm_call_last_key_info_t> key_info_map = params->get_reply();
    std::map<ctrlm_network_id_t, ctrlm_main_iarm_call_last_key_info_t>::iterator itr;
    ctrlm_main_iarm_call_last_key_info_t key_info = {};
    long long          time_last_key = 0;
    ctrlm_network_id_t net_id_index = 0;
    int                err = 0;

    if (params->get_net_id() != CTRLM_MAIN_NETWORK_ID_ALL) {
        itr = key_info_map.find(params->get_net_id());
        key_info = itr->second;
    } else {
        for (itr = key_info_map.begin(); itr != key_info_map.end(); itr++) {
            if (itr->second.timestamp > time_last_key) {
                time_last_key = itr->second.timestamp;
                net_id_index = itr->first;
            }
        }
        key_info = key_info_map[net_id_index];
    }

    err |= json_object_set_new_nocheck(ret, CONTROLLER_ID,        json_integer(key_info.controller_id));
    err |= json_object_set_new_nocheck(ret, TIMESTAMP,            json_integer(key_info.timestamp));
    err |= json_object_set_new_nocheck(ret, SOURCE_NAME,          json_string(key_info.source_name));
    err |= json_object_set_new_nocheck(ret, SOURCE_TYPE,          json_string((key_info.source_type == CTRLM_KEY_SOURCE_RF) ? "RF" : (key_info.source_type == CTRLM_KEY_SOURCE_IR) ? "IR" : "INVALID"));
    err |= json_object_set_new_nocheck(ret, SOURCE_KEY_CODE,      json_integer(key_info.source_key_code));
    err |= json_object_set_new_nocheck(ret, SCREENBIND_MODE,      json_boolean(key_info.is_screen_bind_mode));
    err |= json_object_set_new_nocheck(ret, REMOTE_KEYPAD_CONFIG, json_integer(key_info.remote_keypad_config));
    err |= json_object_set_new_nocheck(ret, SUCCESS,              json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::find_my_remote(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    int net_type = CTRLM_NETWORK_TYPE_INVALID;
    if(!config.config_value_get(NET_TYPE, net_type)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    std::string level;
    if(!config.config_value_get(LEVEL, level)) {
        XLOGD_ERROR("Missing %s parameter", LEVEL);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::shared_ptr<ctrlm_iarm_call_FindMyRemote_params_t> params = std::make_shared<ctrlm_iarm_call_FindMyRemote_params_t>();
    params->set_net_id((net_type == CTRLM_NETWORK_TYPE_INVALID) ? CTRLM_MAIN_NETWORK_ID_ALL : ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(net_type)));
    params->level    = ctrlm_utils_str_to_fmr_level(level);
    params->duration = 0;

    sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_FindMyRemote_params_t,
                                             ctrlm_main_queue_msg_find_my_remote_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_find_my_remote);

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::factory_reset(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    ctrlm_main_iarm_call_factory_reset_t factory_reset = {};
    factory_reset.network_id = CTRLM_MAIN_NETWORK_ID_ALL;

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(ctrlm_main_iarm_call_factory_reset(&factory_reset)));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return (IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::write_rcu_wakeup_config(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
         XLOGD_ERROR("NULL parameter");
         return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
         XLOGD_ERROR("Bad payload from call data");
         return(IARM_RESULT_INVALID_PARAM);
    }

    int net_type = CTRLM_NETWORK_TYPE_INVALID;
    if(!config.config_value_get(NET_TYPE, net_type)) {
         XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    std::string wakeup_config;
    if(!config.config_value_get(WAKEUP_CONFIG, wakeup_config)) {
         XLOGD_ERROR("Missing %s parameter", WAKEUP_CONFIG);
         return(IARM_RESULT_INVALID_PARAM);
    }

    std::string custom_keys;
    if (ctrlm_utils_str_to_wakeup_config(wakeup_config) == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
        if(!config.config_value_get(CUSTOM_KEYS, custom_keys)) {
            XLOGD_ERROR("Missing %s parameter", CUSTOM_KEYS);
            return(IARM_RESULT_INVALID_PARAM);
        }
    }

    ctrlm_iarm_call_WriteRcuWakeupConfig_params_t params = {};
    params.network_id     = (net_type == CTRLM_NETWORK_TYPE_INVALID) ? CTRLM_MAIN_NETWORK_ID_ALL : ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(net_type));
    params.config         = ctrlm_utils_str_to_wakeup_config(wakeup_config);
    params.customListSize = ctrlm_utils_custom_key_str_to_array(custom_keys, params.customList);

    sync_send_netw_handler_to_main_queue<ctrlm_iarm_call_WriteRcuWakeupConfig_params_t *,
                                         ctrlm_main_queue_msg_write_advertising_config_t>
                                         (&params,
                                         static_cast<ctrlm_msg_handler_network_t>(&ctrlm_obj_network_t::req_process_write_rcu_wakeup_config));

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params.result == CTRLM_IARM_CALL_RESULT_SUCCESS));
    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::unpair(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *mac_addr_array = nullptr;
    std::vector<uint64_t> mac_addr_list;
    if (config.config_array_get(MAC_ADDRESS_LIST, &mac_addr_array)) {
        size_t index = 0;
        json_t *value = nullptr;

        json_array_foreach(mac_addr_array, index, value) {
            if (!json_is_string(value)) {
                XLOGD_ERROR("An element of the %s array is not a string", MAC_ADDRESS_LIST);
                return(IARM_RESULT_INVALID_PARAM);
            }

            uint64_t mac_addr = ctrlm_convert_mac_string_to_long(json_string_value(value));
            if (mac_addr == 0) {
                XLOGD_ERROR("An invalid mac address was provided <%s>", json_string_value(value));
                return(IARM_RESULT_INVALID_PARAM);
            }
            mac_addr_list.push_back(mac_addr);
        }
    }

    std::shared_ptr<ctrlm_iarm_call_Unpair_params_t> params = std::make_shared<ctrlm_iarm_call_Unpair_params_t>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    params->ieee_address_list = mac_addr_list;

    sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_Unpair_params_t,
                                             ctrlm_main_queue_msg_unpair_t>
                                             (params,
                                             static_cast<ctrlm_msg_handler_network_t>(&ctrlm_obj_network_t::req_process_unpair));

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::start_fw_update(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!thunder_device_update_enabled_) {
        XLOGD_ERROR("This API is currently disabled");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string mac_address;
    uint64_t    ieee_address = 0;
    if (config.config_value_get(MAC_ADDRESS, mac_address)) {
        ieee_address = ctrlm_convert_mac_string_to_long(mac_address.c_str());
    } else {
        XLOGD_INFO("%s parameter was omitted", MAC_ADDRESS);
    }

    std::string filename;
    if (!config.config_value_get(FILENAME, filename)) {
        XLOGD_ERROR("Missing %s parameter", FILENAME);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string filetype;
    if (!config.config_value_get(FILETYPE, filetype)) {
        XLOGD_INFO("%s parameter was omitted", FILETYPE);
    }

    uint8_t percent_increment = 0;
    if (!config.config_value_get(PERCENT_INCREMENT, percent_increment)) {
        XLOGD_INFO("%s parameter was omitted", PERCENT_INCREMENT);
    } else if (percent_increment < 0 || percent_increment > 100) {
        XLOGD_ERROR("%s parameter out of bounds", PERCENT_INCREMENT);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::shared_ptr<ctrlm_iarm_call_StartUpgrade_params_t> params = std::make_shared<ctrlm_iarm_call_StartUpgrade_params_t>();
    std::shared_ptr<std::vector<std::string>>              upgrade_sessions = std::make_shared<std::vector<std::string>>();

    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    params->ieee_address      = ieee_address;
    params->percent_increment = percent_increment;
    params->filetype          = filetype;
    params->filename          = filename;
    params->upgrade_sessions  = upgrade_sessions;

    sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_StartUpgrade_params_t,
                                             ctrlm_main_queue_msg_start_controller_upgrade_t>
                                             (params,
                                             static_cast<ctrlm_msg_handler_network_t>(&ctrlm_obj_network_t::req_process_start_controller_upgrade));

    json_t *ret = json_object();
    json_t *session_array = json_array();
    int err = 0;

    for (auto const &session : *upgrade_sessions) {
        err |= json_array_append_new(session_array, json_string(session.c_str()));
    }

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));
    err |= json_object_set_new_nocheck(ret, SESSION_ID_LIST, session_array);

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::cancel_fw_update(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!thunder_device_update_enabled_) {
        XLOGD_ERROR("This API is currently disabled");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string session_id;
    if (!config.config_value_get(SESSION_ID, session_id) || !ctrlm_utils_is_valid_uuid(session_id)) {
        XLOGD_ERROR("Missing or bad %s parameter", SESSION_ID);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::shared_ptr<ctrlm_iarm_call_CancelUpgrade_params_t> params = std::make_shared<ctrlm_iarm_call_CancelUpgrade_params_t>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    params->session_id = session_id;

    sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_CancelUpgrade_params_t,
                                             ctrlm_main_queue_msg_cancel_controller_upgrade_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_cancel_controller_upgrade);

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::status_fw_update(void *arg)
{
    XLOGD_INFO("");

    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!thunder_device_update_enabled_) {
        XLOGD_ERROR("This API is currently disabled");
        return(IARM_RESULT_INVALID_STATE);
    }

    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);

    if (!call_data || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("NULL parameter");
        return(IARM_RESULT_INVALID_PARAM);
    }

    json_t *payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);
    json_config config(payload);

    if (!payload || !config.current_object_get()) {
        XLOGD_ERROR("Bad payload from call data");
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string session_id;
    if (!config.config_value_get(SESSION_ID, session_id) || !ctrlm_utils_is_valid_uuid(session_id)) {
        XLOGD_ERROR("Missing or bad %s parameter", SESSION_ID);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::shared_ptr<ctrlm_rcp_ipc_upgrade_status_t> params = std::make_shared<ctrlm_rcp_ipc_upgrade_status_t>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    params->session_id = session_id;

    sync_send_netw_handler_to_main_queue_new<ctrlm_rcp_ipc_upgrade_status_t,
                                             ctrlm_main_queue_msg_status_controller_upgrade_t>
                                             (params,
                                             (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_status_controller_upgrade);

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, STATUS,  params->to_json());
    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}
