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
#include "ctrlm_rf4ce_rib_attr.h"
#include "ctrlm_log.h"
#include <sstream>

#define VALID_INDEX(x) ((x == RIB_ATTR_INDEX_ALL || x <= 0xFF) ? x : RIB_ATTR_INDEX_INVALID)

ctrlm_rf4ce_rib_attr_t::ctrlm_rf4ce_rib_attr_t(rf4ce_rib_attr_identifier_t identifier, rf4ce_rib_attr_index_t index, permission read_permission, permission write_permission) {
    this->identifier       = identifier;
    this->index            = VALID_INDEX(index);
    this->read_permission  = read_permission;
    this->write_permission = write_permission;
}

ctrlm_rf4ce_rib_attr_t::~ctrlm_rf4ce_rib_attr_t() {

}

rf4ce_rib_attr_identifier_t ctrlm_rf4ce_rib_attr_t::get_identifier() const {
    return(this->identifier);
}

rf4ce_rib_attr_index_t ctrlm_rf4ce_rib_attr_t::get_index() const {
    return(this->index);
}

void ctrlm_rf4ce_rib_attr_t::set_identifier(rf4ce_rib_attr_identifier_t identifier) {
    this->identifier = identifier;
}

void ctrlm_rf4ce_rib_attr_t::set_index(rf4ce_rib_attr_index_t index) {
    this->index = VALID_INDEX(index);
}

bool ctrlm_rf4ce_rib_attr_t::can_read(ctrlm_rf4ce_rib_attr_t::access a) {
    return(can_access(a, this->read_permission));
}

bool ctrlm_rf4ce_rib_attr_t::can_write(ctrlm_rf4ce_rib_attr_t::access a) {
    return(can_access(a, this->write_permission));
}

bool ctrlm_rf4ce_rib_attr_t::can_access(ctrlm_rf4ce_rib_attr_t::access a, ctrlm_rf4ce_rib_attr_t::permission p) {
    bool ret = false;
    if (p == ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH ||
       (a == ctrlm_rf4ce_rib_attr_t::access::CONTROLLER && p == ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER) ||
       (a == ctrlm_rf4ce_rib_attr_t::access::TARGET && p == ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET)) {
        ret = true;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_rib_attr_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    return(ctrlm_rf4ce_rib_attr_t::status::NOT_IMPLEMENTED);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_rib_attr_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    return(ctrlm_rf4ce_rib_attr_t::status::NOT_IMPLEMENTED);
}

void ctrlm_rf4ce_rib_attr_t::export_rib(rf4ce_rib_export_api_t export_api) {
    XLOGD_DEBUG("attribute export not implemented for this attribute");
}

std::string ctrlm_rf4ce_rib_attr_t::status_str(ctrlm_rf4ce_rib_attr_t::status s) {
    std::stringstream ret; ret << "INVALID";
    switch(s) {
        case ctrlm_rf4ce_rib_attr_t::status::SUCCESS:             {ret.str("SUCCESS"); break;}
        case ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE:          {ret.str("WRONG_SIZE"); break;}
        case ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM:       {ret.str("INVALID_PARAM"); break;}
        case ctrlm_rf4ce_rib_attr_t::status::FAILURE:             {ret.str("FAILURE"); break;}
        case ctrlm_rf4ce_rib_attr_t::status::NOT_IMPLEMENTED:     {ret.str("NOT_IMPLEMENTED"); break;}
        default: {ret << " (" << (int)s << ")"; break;}
    }
    return(ret.str());
}

std::string ctrlm_rf4ce_rib_attr_t::permission_str(ctrlm_rf4ce_rib_attr_t::permission p) {
    std::stringstream ret; ret << "INVALID";
    switch(p) {
        case ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER: {ret.str("CONTROLLER"); break;}
        case ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET:     {ret.str("TARGET"); break;}
        case ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH:       {ret.str("BOTH"); break;}
        default: {ret << " (" << (int)p << ")"; break;}
    }
    return(ret.str());
}

std::string ctrlm_rf4ce_rib_attr_t::access_str(ctrlm_rf4ce_rib_attr_t::access a) {
    std::stringstream ret; ret << "INVALID";
    switch(a) {
        case ctrlm_rf4ce_rib_attr_t::access::CONTROLLER: {ret.str("CONTROLLER"); break;}
        case ctrlm_rf4ce_rib_attr_t::access::TARGET:     {ret.str("TARGET"); break;}
        default: {ret << " (" << (int)a << ")"; break;}
    }
    return(ret.str());
}
