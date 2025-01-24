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

#include "ctrlm_irdb_ipc_iarm_thunder.h"
#include "libIBus.h"
#include <iostream>
#include "ctrlm_ipc.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "json_config.h"
#include "ctrlm_utils.h"

namespace {
    constexpr char const* NET_TYPE      = "netType";
    constexpr char const* AV_DEV_TYPE   = "avDevType";
    constexpr char const* MANUFACTURER  = "manufacturer";
    constexpr char const* MANUFACTURERS = "manufacturers";
    constexpr char const* MODEL         = "model";
    constexpr char const* MODELS        = "models";
    constexpr char const* CODE          = "code";
    constexpr char const* CODES         = "codes";
    constexpr char const* REMOTE_ID     = "remoteId";
    constexpr char const* SUCCESS       = "success";
    constexpr char const* TV            = "TV";
    constexpr char const* AMP           = "AMP";
}

ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_ipc_iarm_thunder_t() : ctrlm_ipc_iarm_t() {
    XLOGD_INFO("");
}

ctrlm_irdb_ipc_iarm_thunder_t::~ctrlm_irdb_ipc_iarm_thunder_t() {
    XLOGD_INFO("");
}

bool ctrlm_irdb_ipc_iarm_thunder_t::register_ipc() const {
    bool ret = true;

    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_MANUFACTURERS, get_manufacturers)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_MODELS, get_models)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_CODES, get_ir_codes_by_names)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_AUTO_LOOKUP, get_ir_codes_by_auto_lookup)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_SET_CODE, set_ir_codes_by_name)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_CLEAR_CODE, clear_ir_codes)) ret = false;
    if(!this->register_iarm_call(CTRLM_MAIN_IARM_CALL_IR_INITIALIZE, initialize_irdb)) ret = false;

    return(ret);
}

void ctrlm_irdb_ipc_iarm_thunder_t::deregister_ipc() const {

}

ctrlm_irdb_dev_type_t ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_from_iarm(ctrlm_ir_device_type_t dev_type) {
    switch(dev_type) {
        case CTRLM_IR_DEVICE_TV:  return(CTRLM_IRDB_DEV_TYPE_TV);
        case CTRLM_IR_DEVICE_AMP: return(CTRLM_IRDB_DEV_TYPE_AVR);
        default: break;
    }
    return(CTRLM_IRDB_DEV_TYPE_INVALID);
}

const char *ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_str(ctrlm_irdb_dev_type_t dev_type) {
    switch(dev_type) {
        case CTRLM_IRDB_DEV_TYPE_TV:  return(TV);
        case CTRLM_IRDB_DEV_TYPE_AVR: return(AMP);
        default: break;
    }
    return("INVALID");
}

bool ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_is_valid(const std::string av_dev_str, ctrlm_ir_device_type_t &ir_dev_type) {
    bool ret = true;
    if(av_dev_str == AMP) {
        ir_dev_type = CTRLM_IR_DEVICE_AMP;
    } else if (av_dev_str == TV) {
        ir_dev_type = CTRLM_IR_DEVICE_TV;
    } else {
        ret = false;
    }
    return(ret);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::get_manufacturers(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                            = json_object();
    json_t *payload, *manufacturers        = NULL;
    ctrlm_irdb_t *irdb                     = ctrlm_main_irdb_get();
    bool success                           = false;
    ctrlm_ir_device_type_t ir_dev_type     = CTRLM_IR_DEVICE_UNKNOWN;
    ctrlm_irdb_dev_type_t dev_type         = CTRLM_IRDB_DEV_TYPE_INVALID;
    std::string av_dev_str;
    std::string manufacturer;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(AV_DEV_TYPE, av_dev_str)) {
        XLOGD_ERROR("Missing %s parameter", AV_DEV_TYPE);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(MANUFACTURER, manufacturer)){
        XLOGD_ERROR("Missing %s parameter", MANUFACTURER);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!ctrlm_irdb_dev_type_is_valid(av_dev_str, ir_dev_type)) {
        XLOGD_ERROR("Invalid %s type <%s>", AV_DEV_TYPE, av_dev_str.c_str());
        return(IARM_RESULT_INVALID_PARAM);
    }

    dev_type = ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_from_iarm(ir_dev_type);
    if(dev_type != CTRLM_IRDB_DEV_TYPE_INVALID) {
        success = true;
    }

    if(irdb && success) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            manufacturers = json_array();
            ctrlm_irdb_manufacturer_list_t mans;
            if(irdb->get_manufacturers(mans, dev_type, manufacturer) == false ) {
               XLOGD_ERROR("Failed getting manufacturers");
               success = false;
            } else {
               for(const auto &itr : mans) {
                   json_array_append_new(manufacturers, json_string(itr.c_str()));
               }
            }
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if(success) {
        json_object_set_new(ret, AV_DEV_TYPE,  json_string(ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_str(dev_type)));
        json_object_set_new(ret, MANUFACTURERS, (manufacturers != NULL ? manufacturers : json_null()));
    }
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::get_models(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                            = json_object();
    json_t *payload, *models               = NULL;
    ctrlm_irdb_t *irdb                     = ctrlm_main_irdb_get();
    bool success                           = false;
    ctrlm_ir_device_type_t ir_dev_type     = CTRLM_IR_DEVICE_UNKNOWN;
    ctrlm_irdb_dev_type_t dev_type         = CTRLM_IRDB_DEV_TYPE_INVALID;
    std::string av_dev_str, manufacturer, model;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(AV_DEV_TYPE, av_dev_str)) {
        XLOGD_ERROR("Missing %s parameter", AV_DEV_TYPE);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(MANUFACTURER, manufacturer)){
        XLOGD_ERROR("Missing %s parameter", MANUFACTURER);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(MODEL, model)){
        XLOGD_ERROR("Missing %s parameter", MODEL);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!ctrlm_irdb_dev_type_is_valid(av_dev_str, ir_dev_type)) {
        XLOGD_ERROR("Invalid %s type <%s>", AV_DEV_TYPE, av_dev_str.c_str());
        return(IARM_RESULT_INVALID_PARAM);
    }

    dev_type = ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_from_iarm(ir_dev_type);
    if(dev_type != CTRLM_IRDB_DEV_TYPE_INVALID) {
        success = true;
    }

    if(irdb && success) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            models = json_array();
            ctrlm_irdb_model_list_t mods;
            if(irdb->get_models(mods, dev_type, manufacturer, model) == false ) {
               XLOGD_ERROR("Failed getting models");
               success = false;
            } else {
               for(const auto &itr : mods) {
                   json_array_append_new(models, json_string(itr.c_str()));
               }
            }
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if(success) {
        json_object_set_new(ret, AV_DEV_TYPE,  json_string(ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_str(dev_type)));
        json_object_set_new(ret, MANUFACTURER, json_string(manufacturer.c_str()));
        json_object_set_new(ret, MODELS,       (models != NULL ? models : json_null()));
    }
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::get_ir_codes_by_auto_lookup(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data  = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                             = json_object();
    json_t *payload                         = NULL;
    json_t *tv_man, *tv_model, *tv_codes    = NULL;
    json_t *avr_man, *avr_model, *avr_codes = NULL;
    ctrlm_irdb_t *irdb                      = ctrlm_main_irdb_get();
    bool success                            = false;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(irdb) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            if(irdb->can_get_ir_codes_by_autolookup()) {
                ctrlm_irdb_autolookup_ranked_list_by_type_t cd_map;
                if(irdb->get_ir_codes_by_autolookup(cd_map) == false) {
                    XLOGD_ERROR("Failed getting codes");
                    success = false;
                } else {
                    if(cd_map.count(CTRLM_IRDB_DEV_TYPE_TV) > 0) {
                        // First entry in the list is the highest ranked code, so use the manufacturer and model from that entry
                        tv_man   = json_string(cd_map[CTRLM_IRDB_DEV_TYPE_TV].front().man.c_str());
                        tv_model = json_string(cd_map[CTRLM_IRDB_DEV_TYPE_TV].front().model.c_str());
                        tv_codes = json_array();
                        for (auto const &entry : cd_map[CTRLM_IRDB_DEV_TYPE_TV]) {
                            json_array_append_new(tv_codes, json_string(entry.id.c_str()));
                        }
                        json_object_set_new(ret, "tvManufacturer", tv_man);
                        json_object_set_new(ret, "tvModel", tv_model);
                        json_object_set_new(ret, "tvCodes", tv_codes);
                    }
                    if(cd_map.count(CTRLM_IRDB_DEV_TYPE_AVR) > 0) {
                        // First entry in the list is the highest ranked code, so use the manufacturer and model from that entry
                        avr_man   = json_string(cd_map[CTRLM_IRDB_DEV_TYPE_AVR].front().man.c_str());
                        avr_model = json_string(cd_map[CTRLM_IRDB_DEV_TYPE_AVR].front().model.c_str());
                        avr_codes = json_array();
                        for (auto const &entry : cd_map[CTRLM_IRDB_DEV_TYPE_AVR]) {
                            json_array_append_new(avr_codes, json_string(entry.id.c_str()));
                        }
                        json_object_set_new(ret, "avrManufacturer", avr_man);
                        json_object_set_new(ret, "avrModel", avr_model);
                        json_object_set_new(ret, "avrCodes", avr_codes);
                    }
                    success = true;
                }
            } else {
                XLOGD_WARN("IRDB doesn't support get_ir_codes_by_autolookup");
            }
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::get_ir_codes_by_names(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data  = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                             = json_object();
    json_t *payload, *codes                 = NULL;
    ctrlm_irdb_t *irdb                      = ctrlm_main_irdb_get();
    bool success                            = false;
    ctrlm_ir_device_type_t ir_dev_type      = CTRLM_IR_DEVICE_UNKNOWN;
    ctrlm_irdb_dev_type_t dev_type          = CTRLM_IRDB_DEV_TYPE_INVALID;
    std::string av_dev_str, manufacturer, model;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(AV_DEV_TYPE, av_dev_str)) {
        XLOGD_ERROR("Missing %s parameter", AV_DEV_TYPE);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(MANUFACTURER, manufacturer)){
        XLOGD_ERROR("Missing %s parameter", MANUFACTURER);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(MODEL, model)){
        XLOGD_WARN("Missing %s parameter - will default to empty string", MODEL);
        model = "";
    }

    if(!ctrlm_irdb_dev_type_is_valid(av_dev_str, ir_dev_type)) {
        XLOGD_ERROR("Invalid %s type <%s>", AV_DEV_TYPE, av_dev_str.c_str());
        return(IARM_RESULT_INVALID_PARAM);
    }

    dev_type = ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_from_iarm(ir_dev_type);
    if(dev_type != CTRLM_IRDB_DEV_TYPE_INVALID) {
        success = true;
    }

    if(irdb && success) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            codes = json_array();
            ctrlm_irdb_ir_entry_id_list_t cds;
            if(irdb->get_ir_codes_by_names(cds, dev_type, manufacturer, model) == false) {
               XLOGD_ERROR("Failed getting codes");
               success = false;
            } else {
               for(const auto &itr : cds) {
                    json_array_append_new(codes, json_string(itr.c_str()));
               }
            }
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if(success) {
        json_object_set_new(ret, AV_DEV_TYPE,  json_string(ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_str(dev_type)));
        json_object_set_new(ret, MANUFACTURER, json_string(manufacturer.c_str()));
        json_object_set_new(ret, MODEL,        json_string(model.c_str()));
        json_object_set_new(ret, CODES,        (codes != NULL ? codes : json_null()));
    }
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::set_ir_codes_by_name(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data  = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                             = json_object();
    json_t *payload                         = NULL;
    ctrlm_irdb_t *irdb                      = ctrlm_main_irdb_get();
    bool success                            = false;
    ctrlm_irdb_dev_type_t dev_type          = CTRLM_IRDB_DEV_TYPE_INVALID;
    ctrlm_ir_device_type_t ir_dev_type      = CTRLM_IR_DEVICE_UNKNOWN;
    int network_type_val                    = CTRLM_NETWORK_TYPE_INVALID;
    int remote_id                           = 0;
    std::string av_dev_str, code;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(NET_TYPE, network_type_val)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    if(!conf.config_value_get(AV_DEV_TYPE, av_dev_str)) {
        XLOGD_ERROR("Missing %s parameter", AV_DEV_TYPE);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(REMOTE_ID, remote_id)){
        XLOGD_ERROR("Missing %s parameter", REMOTE_ID);
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(CODE, code)){
        XLOGD_ERROR("Missing %s parameter", CODE);
        return(IARM_RESULT_INVALID_PARAM);
    }

    ctrlm_network_id_t network_id       = (network_type_val == CTRLM_NETWORK_TYPE_INVALID) ?
                                          CTRLM_MAIN_NETWORK_ID_ALL :
                                          ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(network_type_val));
    ctrlm_controller_id_t controller_id = static_cast<ctrlm_controller_id_t>(remote_id);

    if(!ctrlm_irdb_dev_type_is_valid(av_dev_str, ir_dev_type)) {
        XLOGD_ERROR("Invalid %s type <%s>", AV_DEV_TYPE, av_dev_str.c_str());
        return(IARM_RESULT_INVALID_PARAM);
    }

    dev_type = ctrlm_irdb_ipc_iarm_thunder_t::ctrlm_irdb_dev_type_from_iarm(ir_dev_type);
    if(dev_type != CTRLM_IRDB_DEV_TYPE_INVALID) {
        success = true;
    }

    if(irdb && success) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            success = irdb->set_ir_codes_by_name(network_id, controller_id, dev_type, code);
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::clear_ir_codes(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data  = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                             = json_object();
    json_t *payload                         = NULL;
    ctrlm_irdb_t *irdb                      = ctrlm_main_irdb_get();
    bool success                            = false;
    int network_type_val                    = CTRLM_NETWORK_TYPE_INVALID;
    int remote_id                           = 0;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(!conf.config_value_get(NET_TYPE, network_type_val)) {
        XLOGD_INFO("Missing %s parameter - defaulting to all networks", NET_TYPE);
    }

    if(!conf.config_value_get(REMOTE_ID, remote_id)){
        XLOGD_ERROR("Missing %s parameter", REMOTE_ID);
        return(IARM_RESULT_INVALID_PARAM);
    }

    ctrlm_network_id_t network_id       = (network_type_val == CTRLM_NETWORK_TYPE_INVALID) ?
                                          CTRLM_MAIN_NETWORK_ID_ALL :
                                          ctrlm_network_id_get(static_cast<ctrlm_network_type_t>(network_type_val));
    ctrlm_controller_id_t controller_id = static_cast<ctrlm_controller_id_t>(remote_id);

    if(irdb) {
        if(!irdb->get_initialized()) {
            XLOGD_WARN("IRDB is not initialized");
            success = false;
        } else {
            success = irdb->clear_ir_codes(network_id, controller_id);
        }
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_irdb_ipc_iarm_thunder_t::initialize_irdb(void *arg) {
    XLOGD_INFO("");
    ctrlm_main_iarm_call_json_t *call_data  = static_cast<ctrlm_main_iarm_call_json_t *>(arg);
    json_t *ret                             = json_object();
    json_t *payload                         = NULL;
    ctrlm_irdb_t *irdb                      = ctrlm_main_irdb_get();
    bool success                            = false;
    json_config conf;

    if(call_data == NULL || call_data->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
        XLOGD_ERROR("Invalid parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }
    payload = json_loads(call_data->payload, JSON_DECODE_ANY, NULL);

    if(payload == NULL || !conf.config_object_set(payload)) {
        XLOGD_ERROR("Invalid payload from parameters");
        return(IARM_RESULT_INVALID_PARAM);
    }

    if(irdb) {
        success = irdb->initialize_irdb();
    }

    // Assemble the return
    json_object_set_new(ret, SUCCESS, json_boolean(success));
    if (!ctrlm_json_to_iarm_call_data_result(ret, call_data)) {
        json_decref(ret);
        return(IARM_RESULT_INVALID_STATE);
    }
    return(IARM_RESULT_SUCCESS);
}
