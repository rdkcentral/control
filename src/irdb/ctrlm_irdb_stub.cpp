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

#include "ctrlm_log.h"
#include "ctrlm_irdb_stub.h"
#include "ctrlm_irdb_ipc_iarm_thunder.h"

ctrlm_irdb_stub_t::ctrlm_irdb_stub_t(ctrlm_irdb_mode_t mode, bool platform_tv) : ctrlm_irdb_t(mode, platform_tv) {
    XLOGD_INFO("registering for IARM Thunder calls");
    this->ipc = new ctrlm_irdb_ipc_iarm_thunder_t();
    this->ipc->register_ipc();
}

ctrlm_irdb_stub_t::~ctrlm_irdb_stub_t() {
    XLOGD_ERROR("not implemented");
}

bool ctrlm_irdb_stub_t::get_manufacturers(ctrlm_irdb_manufacturer_list_t &manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_models(ctrlm_irdb_model_list_t &models, ctrlm_irdb_dev_type_t type, ctrlm_irdb_manufacturer_t manufacturer, const std::string &prefix) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_ir_codes_by_infoframe(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *infoframe, size_t infoframe_len) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_ir_codes_by_edid(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *edid, size_t edid_len) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_ir_codes_by_cec(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, const std::string &osd, unsigned int vendor_id, unsigned int logical_address) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_ir_codes_by_autolookup(ctrlm_irdb_autolookup_ranked_list_by_type_t &codes) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::get_ir_codes_by_names(ctrlm_irdb_ir_entry_id_list_t &codes, ctrlm_irdb_dev_type_t type, ctrlm_irdb_manufacturer_t manufacturer, const ctrlm_irdb_model_t &model) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::set_ir_codes_by_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_dev_type_t type, const ctrlm_irdb_ir_entry_id_t &name) {
    XLOGD_ERROR("not implemented");
    return(false);
}

bool ctrlm_irdb_stub_t::clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
    XLOGD_ERROR("not implemented");
    return(false);
}
    
bool ctrlm_irdb_stub_t::initialize_irdb() {
    XLOGD_ERROR("not implemented");
    return(false);
}

ctrlm_irdb_vendor_t ctrlm_irdb_stub_t::get_vendor() {
    XLOGD_ERROR("not implemented");
    return(CTRLM_IRDB_VENDOR_INVALID);
}
