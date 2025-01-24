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
#include <cstring>
#include "ctrlm_rf4ce_rib.h"
#include "ctrlm_log.h"
#include <sstream>

ctrlm_rf4ce_rib_t::ctrlm_rf4ce_rib_t() {
    memset(this->rib, 0, sizeof(this->rib[0][0])*IDENTIFIER_MAX*INDEX_MAX);
}

ctrlm_rf4ce_rib_t::~ctrlm_rf4ce_rib_t() {

}

bool ctrlm_rf4ce_rib_t::add_attribute(ctrlm_rf4ce_rib_attr_t *attr) {
    bool ret = false;
    if(attr) {
        rf4ce_rib_attr_identifier_t id    = attr->get_identifier();
        rf4ce_rib_attr_index_t      index = attr->get_index();

        if(index != RIB_ATTR_INDEX_INVALID) {
            if(index == RIB_ATTR_INDEX_ALL) { // ALL INDEX
                ret = true;
                for(unsigned int i = 0; i < INDEX_MAX; i++) {
                    if(this->rib[id][i] == NULL) {
                        this->rib[id][i] = attr;
                    } else {
                        XLOGD_ERROR("RIB entry already exists <%02x, %02x>", id, i);
                        ret = false;
                    }
                }
            } else { // SINGLE INDEX
                if(this->rib[id][index] == NULL) {
                    this->rib[id][index] = attr;
                    ret = true;
                } else {
                    XLOGD_ERROR("RIB entry already exists <%02x, %02x>", id, index);
                }
            }
        } else {
            XLOGD_ERROR("attribute has invalid index");
        }
    } else {
        XLOGD_ERROR("attribute is NULL");
    }
    return(ret);
}

bool ctrlm_rf4ce_rib_t::remove_attribute(ctrlm_rf4ce_rib_attr_t *attr) {
    bool ret = false;
    if(attr) {
        rf4ce_rib_attr_identifier_t id    = attr->get_identifier();
        rf4ce_rib_attr_index_t      index = attr->get_index();

        if(index != RIB_ATTR_INDEX_INVALID) {
            if(index == RIB_ATTR_INDEX_ALL) { // ALL INDEXES
                for(unsigned int i = 0; i < INDEX_MAX; i++) {
                    if(this->rib[id][i] == attr) {
                        this->rib[id][i] = NULL;
                        ret = true;
                    } else {
                        XLOGD_ERROR("RIB entry has different attribute there <%02x, %02x>", id, i);
                    }
                }
            } else { // SINGLE INDEX
                if(this->rib[id][index] == attr) {
                    this->rib[id][index] = NULL;
                    ret = true;
                } else {
                    XLOGD_ERROR("RIB entry has different attribute there <%02x, %02x>", id, index);
                }
            }
        } else {
            XLOGD_ERROR("attribute has invalid index");
        }
    } else {
        XLOGD_ERROR("attribute is NULL");
    }
    return(ret);
}

ctrlm_rf4ce_rib_t::status ctrlm_rf4ce_rib_t::read_attribute(ctrlm_rf4ce_rib_attr_t::access accessor, uint8_t identifier, uint8_t index, char *data, size_t *length) {
    ctrlm_rf4ce_rib_t::status ret = ctrlm_rf4ce_rib_t::status::SUCCESS;
    ctrlm_rf4ce_rib_attr_t *attr = this->rib[identifier][index];
    if(attr != NULL) {
        if(attr->can_read(accessor)) {
            ctrlm_rf4ce_rib_attr_t::status status = attr->read_rib(accessor, index, data, length);
            if(status != ctrlm_rf4ce_rib_attr_t::status::SUCCESS) {
                ret = ctrlm_rf4ce_rib_t::status::FAILURE;
            }
        } else {
            ret = ctrlm_rf4ce_rib_t::status::BAD_PERMISSIONS;
        }
    } else {
        ret = ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST;
    }
    return(ret);
}

ctrlm_rf4ce_rib_t::status ctrlm_rf4ce_rib_t::write_attribute(ctrlm_rf4ce_rib_attr_t::access accessor, uint8_t identifier, uint8_t index, char *data, size_t length, rf4ce_rib_export_api_t *export_api, bool importing) {
    ctrlm_rf4ce_rib_t::status ret = ctrlm_rf4ce_rib_t::status::SUCCESS;
    ctrlm_rf4ce_rib_attr_t *attr = this->rib[identifier][index];
    if(attr != NULL) {
        if(attr->can_write(accessor)) {
            ctrlm_rf4ce_rib_attr_t::status status = attr->write_rib(accessor, index, data, length, importing);
            if(status == ctrlm_rf4ce_rib_attr_t::status::SUCCESS) {
                if(export_api) {
                    attr->export_rib(*export_api);
                }
            } else {
                ret = ctrlm_rf4ce_rib_t::status::FAILURE;
            }
        } else {
            ret = ctrlm_rf4ce_rib_t::status::BAD_PERMISSIONS;
        }
    } else {
        ret = ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST;
    }
    return(ret);
}

std::string ctrlm_rf4ce_rib_t::status_str(ctrlm_rf4ce_rib_t::status s) {
    std::stringstream ret; ret << "INVALID";
    switch(s) {
        case ctrlm_rf4ce_rib_t::status::SUCCESS:         {ret.str("SUCCESS"); break;}
        case ctrlm_rf4ce_rib_t::status::FAILURE:         {ret.str("FAILURE"); break;}
        case ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST:  {ret.str("DOES_NOT_EXIST"); break;}
        case ctrlm_rf4ce_rib_t::status::BAD_PERMISSIONS: {ret.str("BAD_PERMISSIONS"); break;}
        default: {ret << " (" << (int)s << ")"; break;}
    }
    return(ret.str());
}
