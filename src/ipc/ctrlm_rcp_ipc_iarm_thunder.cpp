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
#include "ctrlm.h"
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
    set_api_revision(CTRLM_MAIN_IARM_BUS_API_REVISION);
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
    if(!register_iarm_call(CTRLM_MAIN_IARM_CALL_STOP_PAIRING,            stop_pairing))            { ret = false; }
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

bool ctrlm_rcp_ipc_iarm_thunder_t::is_thunder_device_update_enabled() const
{
    return (thunder_device_update_enabled_);
}

json_t *ctrlm_rcp_ipc_iarm_thunder_t::build_rcu_status_json(
    const std::map<ctrlm_network_id_t, ctrlm_rcp_ipc_net_status_t> &status_map,
    ctrlm_ir_state_t      ir_prog_state,
    ctrlm_rf_pair_state_t rf_pair_state,
    ctrlm_network_type_t  type)
{
    json_t *status             = json_object();
    json_t *net_type_supported = json_array();
    json_t *remote_array       = json_array();
    std::vector<ctrlm_rcp_ipc_controller_status_t> remotes;
    int err = 0;

    for (auto const &it : ctrlm_network_types_get()) {
        err |= json_array_append_new(net_type_supported, json_integer(it));
    }
    for (auto &it : status_map) {
        it.second.get_controller_status_list(remotes);
    }
    for (const auto &remote : remotes) {
        err |= json_array_append_new(remote_array, remote.to_json());
    }

    err |= json_object_set_new_nocheck(status, REMOTE_DATA,         remote_array);
    err |= json_object_set_new_nocheck(status, NET_TYPES_SUPPORTED, net_type_supported);
    err |= json_object_set_new_nocheck(status, NET_TYPE,            json_integer(type));
    err |= json_object_set_new_nocheck(status, IR_PROG_STATE,       json_string(ctrlm_ir_state_str(ir_prog_state)));
    err |= json_object_set_new_nocheck(status, PAIRING_STATE,       json_string(ctrlm_rf_pair_state_str(rf_pair_state)));

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(status);
        return nullptr;
    }
    return status;
}

static const char *ctrlm_legacy_pairing_type(ctrlm_rcu_binding_type_t type)
{
    switch(type) {
        case CTRLM_RCU_BINDING_TYPE_INTERACTIVE: return "manual";
        case CTRLM_RCU_BINDING_TYPE_AUTOMATIC:   return "auto-bind";
        case CTRLM_RCU_BINDING_TYPE_BUTTON:      return "button-button";
        case CTRLM_RCU_BINDING_TYPE_SCREEN_BIND: return "screen-bind";
        default:                                 return "invalid";
    }
}

static json_t *ctrlm_legacy_controller_status_json(ctrlm_controller_id_t controller_id, const ctrlm_controller_status_t &status)
{
    json_t *remote = json_object();
    char mac_address[32];
    std::string type(status.type);
    size_t separator = type.find('-');
    std::string model = type.substr(0, separator);
    std::string model_version = "v";
    model_version += separator != std::string::npos && separator + 1 < type.length() ? type[separator + 1] : '?';

    snprintf(mac_address, sizeof(mac_address), "0x%016llX", status.ieee_address);
    json_object_set_new_nocheck(remote, "remoteId", json_integer(controller_id));
    json_object_set_new_nocheck(remote, "remoteMACAddress", json_string(mac_address));
    json_object_set_new_nocheck(remote, "remoteModel", json_string(model.c_str()));
    json_object_set_new_nocheck(remote, "remoteModelVersion", json_string(model_version.c_str()));
    json_object_set_new_nocheck(remote, "howRemoteIsPaired", json_string(ctrlm_legacy_pairing_type(status.binding_type)));
    json_object_set_new_nocheck(remote, "pairingTimestamp", json_integer(status.time_binding * 1000LL));
    json_object_set_new_nocheck(remote, "batteryLevelLoaded", json_string(std::to_string(status.battery_voltage_loaded).c_str()));
    json_object_set_new_nocheck(remote, "batteryLevelUnloaded", json_string(std::to_string(status.battery_voltage_unloaded).c_str()));
    json_object_set_new_nocheck(remote, "batteryLevelPercentage", json_integer(status.battery_level_percent));
    json_object_set_new_nocheck(remote, "batteryLastEvent", json_integer(status.battery_event));
    json_object_set_new_nocheck(remote, "batteryLastEventTimestamp", json_integer(status.time_battery_update * 1000LL));
    json_object_set_new_nocheck(remote, "numVoiceCommandsPreviousDay", json_integer(status.voice_cmd_count_yesterday));
    json_object_set_new_nocheck(remote, "numVoiceCommandsCurrentDay", json_integer(status.voice_cmd_count_today));
    json_object_set_new_nocheck(remote, "numVoiceShortUtterancesPreviousDay", json_integer(status.voice_cmd_short_yesterday));
    json_object_set_new_nocheck(remote, "numVoiceShortUtterancesCurrentDay", json_integer(status.voice_cmd_short_today));
    json_object_set_new_nocheck(remote, "numVoicePacketsSentPreviousDay", json_integer(status.voice_packets_sent_yesterday));
    json_object_set_new_nocheck(remote, "numVoicePacketsSentCurrentDay", json_integer(status.voice_packets_sent_today));
    json_object_set_new_nocheck(remote, "numVoicePacketsLostPreviousDay", json_integer(status.voice_packets_lost_yesterday));
    json_object_set_new_nocheck(remote, "numVoicePacketsLostCurrentDay", json_integer(status.voice_packets_lost_today));
    json_object_set_new_nocheck(remote, "aveVoicePacketLossPreviousDay", json_string(std::to_string(status.voice_packet_loss_average_yesterday).c_str()));
    json_object_set_new_nocheck(remote, "aveVoicePacketLossCurrentDay", json_string(std::to_string(status.voice_packet_loss_average_today).c_str()));
    json_object_set_new_nocheck(remote, "numVoiceCmdsHighLossPreviousDay", json_integer(status.utterances_exceeding_packet_loss_threshold_yesterday));
    json_object_set_new_nocheck(remote, "numVoiceCmdsHighLossCurrentDay", json_integer(status.utterances_exceeding_packet_loss_threshold_today));
    json_object_set_new_nocheck(remote, "lastRebootErrorCode", json_integer(status.reboot_reason));
    json_object_set_new_nocheck(remote, "lastRebootTimestamp", json_integer(status.reboot_timestamp * 1000LL));
    json_object_set_new_nocheck(remote, "versionInfoSw", json_string(status.version_software));
    json_object_set_new_nocheck(remote, "versionInfoHw", json_string(status.version_hardware));
    json_object_set_new_nocheck(remote, "versionInfoIrdb", json_string(status.version_irdb));
    json_object_set_new_nocheck(remote, "irdbType", json_integer(status.ir_db_type));
    json_object_set_new_nocheck(remote, "irdbState", json_integer(status.ir_db_state));
    json_object_set_new_nocheck(remote, "programmedTvIRCode", json_string(status.ir_db_code_tv));
    json_object_set_new_nocheck(remote, "programmedAvrIRCode", json_string(status.ir_db_code_avr));
    json_object_set_new_nocheck(remote, "bHasRemoteBeenUpdated", json_boolean(status.firmware_updated));
    json_object_set_new_nocheck(remote, "lastCommandTimeDate", json_integer(status.time_last_key * 1000LL));
    json_object_set_new_nocheck(remote, "rf4ceRemoteSocMfr", json_string(status.chipset));
    json_object_set_new_nocheck(remote, "remoteMfr", json_string(status.manufacturer));
    json_object_set_new_nocheck(remote, "signalStrengthPercentage", json_integer(status.link_quality_percent));
    json_object_set_new_nocheck(remote, "linkQuality", json_integer(status.link_quality));
    json_object_set_new_nocheck(remote, "bHasCheckedIn", json_boolean(status.checkin_for_device_update));
    json_object_set_new_nocheck(remote, "bIrdbDownloadSupported", json_boolean(status.ir_db_code_download_supported));
    json_object_set_new_nocheck(remote, "securityType", json_integer(status.security_type));
    json_object_set_new_nocheck(remote, "bHasBattery", json_boolean(status.has_battery));
    if(status.has_battery) {
        json_object_set_new_nocheck(remote, "batteryChangedTimestamp", json_integer(status.time_battery_changed * 1000LL));
        json_object_set_new_nocheck(remote, "batteryChangedActualPercentage", json_integer(status.battery_changed_actual_percentage));
        json_object_set_new_nocheck(remote, "batteryChangedUnloadedVoltage", json_string(std::to_string(status.battery_changed_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "battery75PercentTimestamp", json_integer(status.time_battery_75_percent * 1000LL));
        json_object_set_new_nocheck(remote, "battery75PercentActualPercentage", json_integer(status.battery_75_percent_actual_percentage));
        json_object_set_new_nocheck(remote, "battery75PercentUnloadedVoltage", json_string(std::to_string(status.battery_75_percent_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "battery50PercentTimestamp", json_integer(status.time_battery_50_percent * 1000LL));
        json_object_set_new_nocheck(remote, "battery50PercentActualPercentage", json_integer(status.battery_50_percent_actual_percentage));
        json_object_set_new_nocheck(remote, "battery50PercentUnloadedVoltage", json_string(std::to_string(status.battery_50_percent_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "battery25PercentTimestamp", json_integer(status.time_battery_25_percent * 1000LL));
        json_object_set_new_nocheck(remote, "battery25PercentActualPercentage", json_integer(status.battery_25_percent_actual_percentage));
        json_object_set_new_nocheck(remote, "battery25PercentUnloadedVoltage", json_string(std::to_string(status.battery_25_percent_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "battery5PercentTimestamp", json_integer(status.time_battery_5_percent * 1000LL));
        json_object_set_new_nocheck(remote, "battery5PercentActualPercentage", json_integer(status.battery_5_percent_actual_percentage));
        json_object_set_new_nocheck(remote, "battery5PercentUnloadedVoltage", json_string(std::to_string(status.battery_5_percent_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "battery0PercentTimestamp", json_integer(status.time_battery_0_percent * 1000LL));
        json_object_set_new_nocheck(remote, "battery0PercentActualPercentage", json_integer(status.battery_0_percent_actual_percentage));
        json_object_set_new_nocheck(remote, "battery0PercentUnloadedVoltage", json_string(std::to_string(status.battery_0_percent_unloaded_voltage).c_str()));
        json_object_set_new_nocheck(remote, "batteryVoltageLargeJumpCounter", json_integer(status.battery_voltage_large_jump_counter));
        json_object_set_new_nocheck(remote, "batteryVoltageLargeDeclineDetected", json_boolean(status.battery_voltage_large_decline_detected));
    }
    json_object_set_new_nocheck(remote, "bHasDSP", json_boolean(status.has_dsp));
    if(status.has_dsp) {
        json_object_set_new_nocheck(remote, "averageTimeInPrivacyMode", json_integer(status.average_time_in_privacy_mode));
        json_object_set_new_nocheck(remote, "bInPrivacyMode", json_boolean(status.in_privacy_mode));
        json_object_set_new_nocheck(remote, "averageSNR", json_integer(status.average_snr));
        json_object_set_new_nocheck(remote, "averageKeywordConfidence", json_integer(status.average_keyword_confidence));
        json_object_set_new_nocheck(remote, "totalNumberOfMicsWorking", json_integer(status.total_number_of_mics_working));
        json_object_set_new_nocheck(remote, "totalNumberOfSpeakersWorking", json_integer(status.total_number_of_speakers_working));
        json_object_set_new_nocheck(remote, "endOfSpeechInitialTimeoutCount", json_integer(status.end_of_speech_initial_timeout_count));
        json_object_set_new_nocheck(remote, "endOfSpeechTimeoutCount", json_integer(status.end_of_speech_timeout_count));
        json_object_set_new_nocheck(remote, "uptimeStartTime", json_integer(status.time_uptime_start * 1000LL));
        json_object_set_new_nocheck(remote, "uptimeInSeconds", json_integer(status.uptime_seconds));
        json_object_set_new_nocheck(remote, "privacyTimeInSeconds", json_integer(status.privacy_time_seconds));
        json_object_set_new_nocheck(remote, "versionDSPBuildId", json_string(status.version_dsp_build_id));
    }
    return remote;
}

static json_t *ctrlm_legacy_remote_data_json(const std::map<ctrlm_network_id_t, ctrlm_rcp_ipc_net_status_t> &status_map,
                                              const ctrlm_pairing_metrics_status_t &pairing_metrics,
                                              const ctrlm_ir_remote_usage_t &ir_remote_usage)
{
    ctrlm_legacy_rf4ce_network_status_t network_status = {};
    std::vector<std::pair<ctrlm_controller_id_t, ctrlm_controller_status_t>> controller_status;
    bool found = false;
    for(const auto &it : status_map) {
        if(it.second.get_type() == CTRLM_NETWORK_TYPE_RF4CE && it.second.get_legacy_remote_data(network_status, controller_status)) {
            found = true;
            break;
        }
    }
    if(!found) {
        return NULL;
    }

    json_t *data = json_object();
    json_t *remotes = json_array();
    char mac_address[32];
    snprintf(mac_address, sizeof(mac_address), "0x%016llX", network_status.ieee_address);
    json_object_set_new_nocheck(data, "stbRf4ceMACAddress", json_string(mac_address));
    json_object_set_new_nocheck(data, "stbRf4ceSocMfr", json_string(network_status.chipset));
    json_object_set_new_nocheck(data, "stbHALVersion", json_string(network_status.version_hal));
    json_object_set_new_nocheck(data, "stbRf4ceShortAddress", json_integer(network_status.short_address));
    json_object_set_new_nocheck(data, "stbPanId", json_integer(network_status.pan_id));
    json_object_set_new_nocheck(data, "stbActiveChannel", json_integer(network_status.rf_channel_active.number));
    json_object_set_new_nocheck(data, "stbNumPairedRemotes", json_integer(network_status.controller_qty));
    json_object_set_new_nocheck(data, "stbNumScreenBindFailures", json_integer(pairing_metrics.num_screenbind_failures));
    json_object_set_new_nocheck(data, "stbLastScreenBindErrorCode", json_integer(pairing_metrics.last_screenbind_error_code));
    json_object_set_new_nocheck(data, "stbLastScreenBindErrorRemoteType", json_string(pairing_metrics.last_screenbind_remote_type));
    json_object_set_new_nocheck(data, "stbLastScreenBindErrorTimestamp", json_integer(pairing_metrics.last_screenbind_error_timestamp * 1000LL));
    json_object_set_new_nocheck(data, "stbNumOtherBindFailures", json_integer(pairing_metrics.num_non_screenbind_failures));
    json_object_set_new_nocheck(data, "stbLastOtherBindErrorCode", json_integer(pairing_metrics.last_non_screenbind_error_code));
    json_object_set_new_nocheck(data, "stbLastOtherBindErrorRemoteType", json_string(pairing_metrics.last_non_screenbind_remote_type));
    json_object_set_new_nocheck(data, "stbLastOtherBindErrorBindType", json_integer(pairing_metrics.last_non_screenbind_error_binding_type));
    json_object_set_new_nocheck(data, "stbLastOtherBindErrorTimestamp", json_integer(pairing_metrics.last_non_screenbind_error_timestamp * 1000LL));
    bool ir_previous = ir_remote_usage.has_ir_xr2_yesterday || ir_remote_usage.has_ir_xr5_yesterday || ir_remote_usage.has_ir_xr11_yesterday || ir_remote_usage.has_ir_xr15_yesterday || ir_remote_usage.has_ir_remote_yesterday;
    bool ir_current = ir_remote_usage.has_ir_xr2_today || ir_remote_usage.has_ir_xr5_today || ir_remote_usage.has_ir_xr11_today || ir_remote_usage.has_ir_xr15_today || ir_remote_usage.has_ir_remote_today;
    json_object_set_new_nocheck(data, "bHasIrRemotePreviousDay", json_boolean(ir_previous));
    json_object_set_new_nocheck(data, "bHasIrRemoteCurrentDay", json_boolean(ir_current));
    for(const auto &controller : controller_status) {
        json_array_append_new(remotes, ctrlm_legacy_controller_status_json(controller.first, controller.second));
    }
    if(!controller_status.empty()) {
        json_object_set_new_nocheck(data, REMOTE_DATA, remotes);
    } else {
        json_decref(remotes);
    }
    return data;
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

    std::shared_ptr<void> ptr = ctrlm_main_all_network_rcu_status_get();
    if (ptr == nullptr) {
        XLOGD_ERROR("Failed to get RCU status from main thread");
        return false;
    }
    std::shared_ptr<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>> params =
        std::static_pointer_cast<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>>(ptr);

    const std::map<ctrlm_network_id_t, ctrlm_rcp_ipc_net_status_t> status_map = params->get_reply();

    json_t *status = build_rcu_status_json(status_map, net_status.get_ir_prog_state(), net_status.get_rf_pair_state(), net_status.get_type());
    if (status == nullptr) {
        return(false);
    }

    json_t *ret = json_object();
    int err = json_object_set_new_nocheck(ret, STATUS, status);
    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(false);
    }

    return broadcast_iarm_event<ctrlm_main_iarm_event_json_t>(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_RCU_STATUS, ret);
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

    bool verbose = false;
    if(call_data->payload[0] != '\0') {
        json_error_t error;
        json_t *payload = json_loads(call_data->payload, JSON_REJECT_DUPLICATES, &error);
        json_t *verbose_value = payload && json_is_object(payload) ? json_object_get(payload, "verbose") : NULL;
        if(payload == NULL || !json_is_object(payload) || (verbose_value != NULL && !json_is_boolean(verbose_value))) {
            XLOGD_ERROR("Invalid payload");
            if(payload != NULL) {
                json_decref(payload);
            }
            return(IARM_RESULT_INVALID_PARAM);
        }
        if(verbose_value != NULL) {
            verbose = json_is_true(verbose_value);
        }
        json_decref(payload);
    }

    ctrlm_pairing_metrics_status_t pairing_metrics = {};
    ctrlm_ir_remote_usage_t ir_remote_usage = {};
    if(verbose && (!ctrlm_main_pairing_metrics_get(&pairing_metrics) || !ctrlm_main_ir_remote_usage_get(&ir_remote_usage))) {
        return(IARM_RESULT_INVALID_STATE);
    }

    std::shared_ptr<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>> params = std::make_shared<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_rcp_ipc_net_status_t>>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    std::shared_ptr<ctrlm_main_queue_msg_get_rcu_status_t> msg = std::make_shared<ctrlm_main_queue_msg_get_rcu_status_t>();
    msg->params = params;
    msg->semaphore = NULL;
    msg->verbose = verbose;
    ctrlm_main_queue_handler_push_new<ctrlm_msg_handler_network_t, ctrlm_main_queue_msg_get_rcu_status_t>(CTRLM_HANDLER_NETWORK,
                                                                                                          (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_get_rcu_status,
                                                                                                          std::move(msg), NULL, params->get_net_id(), true);

    std::map<ctrlm_network_id_t, ctrlm_rcp_ipc_net_status_t> status_map = params->get_reply();
    
    
    ctrlm_network_type_t  type = CTRLM_NETWORK_TYPE_INVALID;
    ctrlm_ir_state_t      ir_prog_state = CTRLM_IR_STATE_UNKNOWN;
    ctrlm_rf_pair_state_t rf_pair_state = CTRLM_RF_PAIR_STATE_UNKNOWN;

    // For now default to RF4CE network reporting if available
    for (auto &it : status_map) {
        ir_prog_state = it.second.get_ir_prog_state();
        rf_pair_state = it.second.get_rf_pair_state();
        type          = it.second.get_type();

        if (type == CTRLM_NETWORK_TYPE_RF4CE) {
            break;
        }
    }

    json_t *status = build_rcu_status_json(status_map, ir_prog_state, rf_pair_state, type);
    if (status == nullptr) {
        return(IARM_RESULT_INVALID_STATE);
    }
    if(verbose) {
        json_t *remote_data = ctrlm_legacy_remote_data_json(status_map, pairing_metrics, ir_remote_usage);
        if(remote_data == NULL || json_object_set_new_nocheck(status, REMOTE_DATA, remote_data) != 0) {
            json_decref(status);
            return(IARM_RESULT_INVALID_STATE);
        }
    }

    json_t *ret = json_object();

    int err = 0;
    err |= json_object_set_new_nocheck(ret, STATUS, status);
    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(params->get_result()));

    if (err || !ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }

    return(IARM_RESULT_SUCCESS);
}

bool ctrlm_rcp_ipc_iarm_thunder_t::on_validation_status(const ctrlm_rcp_ipc_validation_status_t &validation_status) const
{
    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(false);
    }

    if (validation_status.get_api_revision() != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Wrong ctrlm API revision - should be %d, event is %d", CTRLM_MAIN_IARM_BUS_API_REVISION, validation_status.get_api_revision());
        return(false);
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, STATUS, validation_status.to_json());

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(false);
    }

    return broadcast_iarm_event<ctrlm_main_iarm_event_json_t>(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_VALIDATION_STATUS, ret);
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

    return broadcast_iarm_event<ctrlm_main_iarm_event_json_t>(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_FIRMWARE_UPDATE_PROGRESS, ret);
}

bool ctrlm_rcp_ipc_iarm_thunder_t::on_validation(const ctrlm_rcp_ipc_validation_status_t &validation_status) const
{
    if (!is_running(atomic_running_)) {
        XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state");
        return(false);
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, STATUS, validation_status.to_json());

    if (err) {
        XLOGD_ERROR("JSON object set error");
        json_decref(ret);
        return(false);
    }

    return broadcast_iarm_event<ctrlm_main_iarm_event_json_t>(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_RCU_IARM_EVENT_VALIDATION_STATUS, ret);
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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    int timeout = 0;
    if (!config.config_value_get(TIMEOUT, timeout)) {
        XLOGD_INFO("Missing %s parameter - defaulting to no timeout (0s)", TIMEOUT);
    }

    bool screenBindEnable = true;
    if (!config.config_value_get(SCREEN_BIND_ENABLE, screenBindEnable)) {
        XLOGD_INFO("Missing %s parameter - defaulting to %s", SCREEN_BIND_ENABLE, screenBindEnable ? "true" : "false");
    }

    bool scanEnable       = true;
    if (!config.config_value_get(SCAN_ENABLE, scanEnable)) {
        XLOGD_INFO("Missing %s parameter - defaulting to %s", SCAN_ENABLE, scanEnable ? "true" : "false");
    }

    json_t *mac_addr_array = nullptr;
    std::vector<uint64_t> mac_addr_list;
    if (config.config_array_get(MAC_ADDRESS_LIST, &mac_addr_array)) {
        size_t index = 0;
        json_t *value = nullptr;

        json_array_foreach(mac_addr_array, index, value) {
            if (!json_is_string(value)) {
                XLOGD_ERROR("An element of the %s array is not a string", MAC_ADDRESS_LIST);
                json_decref(payload);
                return(IARM_RESULT_INVALID_PARAM);
            }

            uint64_t mac_addr = ctrlm_convert_mac_string_to_long(json_string_value(value));
            if (mac_addr == 0) {
                XLOGD_ERROR("An invalid mac address was provided <%s>", json_string_value(value));
                json_decref(payload);
                return(IARM_RESULT_INVALID_PARAM);
            }
            mac_addr_list.push_back(mac_addr);
        }
    }
    json_decref(payload);

    if(!scanEnable && mac_addr_list.size() > 0) {
        XLOGD_WARN("scanEnable is false but macAddressList is not empty.  Ignoring macAddressList.");
        mac_addr_list.clear();
    }

    bool result = true;
    if(!screenBindEnable && !scanEnable) {
        XLOGD_WARN("screen bind and scan enable are both false.  Nothing to do.");
    } else {
        std::shared_ptr<ctrlm_iarm_call_StartPairing_params_t> params = std::make_shared<ctrlm_iarm_call_StartPairing_params_t>();
        params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
        params->timeout            = timeout;
        params->screen_bind_enable = screenBindEnable;
        params->scan_enable        = scanEnable;
        params->ieee_address_list  = mac_addr_list;

        sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_StartPairing_params_t,
                                                ctrlm_main_queue_msg_start_pairing_t>
                                                (params,
                                                (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_start_pairing);
        result = params->get_result();
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(result));

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

IARM_Result_t ctrlm_rcp_ipc_iarm_thunder_t::stop_pairing(void *arg)
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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    bool screenBindDisable = true;
    if (!config.config_value_get(SCREEN_BIND_DISABLE, screenBindDisable)) {
        XLOGD_INFO("Missing %s parameter - defaulting to %s", SCREEN_BIND_DISABLE, screenBindDisable ? "true" : "false");
    }

    bool scanDisable       = true;
    if (!config.config_value_get(SCAN_DISABLE, scanDisable)) {
        XLOGD_INFO("Missing %s parameter - defaulting to %s", SCAN_DISABLE, scanDisable ? "true" : "false");
    }
    json_decref(payload);


    bool result = true;
    if(!screenBindDisable && !scanDisable) {
        XLOGD_WARN("screen bind and scan disable are both false.  Nothing to do.");
    } else {
        std::shared_ptr<ctrlm_iarm_call_StopPairing_params_t> params = std::make_shared<ctrlm_iarm_call_StopPairing_params_t>();
        params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
        params->screen_bind_disable = screenBindDisable;
        params->scan_disable        = scanDisable;

        sync_send_netw_handler_to_main_queue_new<ctrlm_iarm_call_StopPairing_params_t,
                                                ctrlm_main_queue_msg_stop_pairing_t>
                                                (params,
                                                (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_stop_pairing);
        result = params->get_result();
    }

    json_t *ret = json_object();
    int err = 0;

    err |= json_object_set_new_nocheck(ret, SUCCESS, json_boolean(result));

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

    std::shared_ptr<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_main_iarm_call_last_key_info_t>> params = std::make_shared<ctrlm_network_all_ipc_reply_wrapper_t<ctrlm_main_iarm_call_last_key_info_t>>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);

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

    for (itr = key_info_map.begin(); itr != key_info_map.end(); itr++) {
        if (itr->second.timestamp > time_last_key) {
            time_last_key = itr->second.timestamp;
            net_id_index = itr->first;
        }
    }
    key_info = key_info_map[net_id_index];

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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string level;
    if(!config.config_value_get(LEVEL, level)) {
        XLOGD_ERROR("Missing %s parameter", LEVEL);
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }
    json_decref(payload);

    std::shared_ptr<ctrlm_iarm_call_FindMyRemote_params_t> params = std::make_shared<ctrlm_iarm_call_FindMyRemote_params_t>();
    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
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
         json_decref(payload);
         return(IARM_RESULT_INVALID_PARAM);
    }


    std::string wakeup_config;
    if(!config.config_value_get(WAKEUP_CONFIG, wakeup_config)) {
         XLOGD_ERROR("Missing %s parameter", WAKEUP_CONFIG);
         json_decref(payload);
         return(IARM_RESULT_INVALID_PARAM);
    }

    std::string custom_keys;
    if (ctrlm_utils_str_to_wakeup_config(wakeup_config) == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
        if(!config.config_value_get(CUSTOM_KEYS, custom_keys)) {
            XLOGD_ERROR("Missing %s parameter", CUSTOM_KEYS);
            json_decref(payload);
            return(IARM_RESULT_INVALID_PARAM);
        }
    }
    json_decref(payload);

    ctrlm_iarm_call_WriteRcuWakeupConfig_params_t params = {};
    params.network_id     = CTRLM_MAIN_NETWORK_ID_ALL;
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
        json_decref(payload);
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
                json_decref(payload);
                return(IARM_RESULT_INVALID_PARAM);
            }

            uint64_t mac_addr = ctrlm_convert_mac_string_to_long(json_string_value(value));
            if (mac_addr == 0) {
                XLOGD_ERROR("An invalid mac address was provided <%s>", json_string_value(value));
                json_decref(payload);
                return(IARM_RESULT_INVALID_PARAM);
            }
            mac_addr_list.push_back(mac_addr);
        }
    }
    json_decref(payload);

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
        json_decref(payload);
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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string filetype;
    if (!config.config_value_get(FILETYPE, filetype)) {
        XLOGD_INFO("%s parameter was omitted", FILETYPE);
    }

    int percent_increment = 0;
    if (!config.config_value_get(PERCENT_INCREMENT, percent_increment)) {
        XLOGD_INFO("%s parameter was omitted", PERCENT_INCREMENT);
    } else if (percent_increment < 0 || percent_increment > 100) {
        XLOGD_ERROR("%s parameter out of bounds", PERCENT_INCREMENT);
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }
    json_decref(payload);

    std::shared_ptr<ctrlm_iarm_call_StartUpgrade_params_t> params = std::make_shared<ctrlm_iarm_call_StartUpgrade_params_t>();
    std::shared_ptr<std::vector<std::string>>              upgrade_sessions = std::make_shared<std::vector<std::string>>();

    params->set_net_id(CTRLM_MAIN_NETWORK_ID_ALL);
    params->ieee_address      = ieee_address;
    params->percent_increment = (uint8_t)percent_increment;
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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string session_id;
    if (!config.config_value_get(SESSION_ID, session_id) || !ctrlm_utils_is_valid_uuid(session_id)) {
        XLOGD_ERROR("Missing or bad %s parameter", SESSION_ID);
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }
    json_decref(payload);

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
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }

    std::string session_id;
    if (!config.config_value_get(SESSION_ID, session_id) || !ctrlm_utils_is_valid_uuid(session_id)) {
        XLOGD_ERROR("Missing or bad %s parameter", SESSION_ID);
        json_decref(payload);
        return(IARM_RESULT_INVALID_PARAM);
    }
    json_decref(payload);

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
