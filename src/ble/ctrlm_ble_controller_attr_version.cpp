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

#include "ctrlm_ble_controller_attr_version.h"
#include "ctrlm_db_types.h"
#include "ctrlm_log.h"
#include <sstream>

// ctrlm_ble_sw_version_t
ctrlm_ble_sw_version_t::ctrlm_ble_sw_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key) :
ctrlm_sw_version_t(), 
ctrlm_db_attr_t(net, id, db_key) {
    this->num_components_ = 3;
}

ctrlm_ble_sw_version_t::~ctrlm_ble_sw_version_t() {

}

bool ctrlm_ble_sw_version_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    if(blob.read_db(ctx)) {
        if (this->from_string(blob.to_string())) {
            ret = true;
            XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("%s read from database failed to parse <%s> ", this->get_name().c_str(), blob.to_string().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_ble_sw_version_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    std::string ver_str = this->to_string();
    if(blob.from_string(ver_str)) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert string to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

// end ctrlm_ble_sw_version_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_ble_hw_version_t
ctrlm_ble_hw_version_t::ctrlm_ble_hw_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key) :
ctrlm_hw_version_t(), 
ctrlm_db_attr_t(net, id, db_key) {

}

ctrlm_ble_hw_version_t::~ctrlm_ble_hw_version_t() {

}

bool ctrlm_ble_hw_version_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    if(blob.read_db(ctx)) {
        if (this->from_string(blob.to_string())) {
            ret = true;
            XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("%s read from database failed to parse <%s> ", this->get_name().c_str(), blob.to_string().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_ble_hw_version_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    std::string ver_str = this->to_string();
    if(blob.from_string(ver_str)) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert string to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

// end ctrlm_ble_hw_version_t
