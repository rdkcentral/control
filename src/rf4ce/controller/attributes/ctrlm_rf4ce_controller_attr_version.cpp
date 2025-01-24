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
#include "ctrlm_rf4ce_controller_attr_version.h"
#include "ctrlm_db_types.h"
#include "ctrlm_log.h"
#include "ctrlm_database.h"
#include <cstring>
#include <sstream>
#include "rf4ce/ctrlm_rf4ce_controller.h"
#include "rf4ce/ctrlm_rf4ce_utils.h"

#define SW_VERSION_LEN                     (0x04)

#define ID_VERSIONING                      (0x02)
#define ID_UPDATE_VERSIONING               (0x31)

#define INDEX_VERSIONING_SOFTWARE          (0x00)
#define INDEX_VERSIONING_HARDWARE          (0x01)
#define INDEX_VERSIONING_IRDB              (0x02)
#define INDEX_VERSIONING_BUILD_ID          (0x03)
#define INDEX_VERSIONING_DSP               (0x04)
#define INDEX_VERSIONING_KEYWORD_MODEL     (0x05)
#define INDEX_VERSIONING_ARM               (0x06)
#define INDEX_VERSIONING_DSP_BUILD_ID      (0x07)
#define INDEX_UPDATE_VERSIONING_BOOTLOADER (0x01)
#define INDEX_UPDATE_VERSIONING_GOLDEN     (0x02)
#define INDEX_UPDATE_VERSIONING_AUDIO_DATA (0x10)

// ctrlm_rf4ce_sw_version_t
ctrlm_rf4ce_sw_version_t::ctrlm_rf4ce_sw_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) : 
ctrlm_sw_version_t(), 
ctrlm_db_attr_t(net, id), 
ctrlm_rf4ce_rib_attr_t(ID_VERSIONING, INDEX_VERSIONING_SOFTWARE, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)  {
    this->set_key("version_software");
    this->set_export(true);
}

ctrlm_rf4ce_sw_version_t::~ctrlm_rf4ce_sw_version_t() {

}

void ctrlm_rf4ce_sw_version_t::set_export(bool set_export) {
    this->exportable = set_export;
}

bool ctrlm_rf4ce_sw_version_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= SW_VERSION_LEN) {
        data[0] = this->major_    & 0xFF;
        data[1] = this->minor_    & 0xFF;
        data[2] = this->revision_ & 0xFF;
        data[3] = this->patch_    & 0xFF;
        ret = true;
    }
    return(ret);
}

bool ctrlm_rf4ce_sw_version_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());
    char buf[SW_VERSION_LEN];

    memset(buf, 0, sizeof(buf));
    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len == SW_VERSION_LEN) {
                this->major_    = buf[0] & 0xFF;
                this->minor_    = buf[1] & 0xFF;
                this->revision_ = buf[2] & 0xFF;
                this->patch_    = buf[3] & 0xFF;
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_sw_version_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());
    char buf[SW_VERSION_LEN];

    memset(buf, 0, sizeof(buf));
    this->to_buffer(buf, sizeof(buf));
    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_sw_version_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = SW_VERSION_LEN;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_sw_version_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == SW_VERSION_LEN) {
            ctrlm_sw_version_t temp(*this);
            this->major_    = data[0] & 0xFF;
            this->minor_    = data[1] & 0xFF;
            this->revision_ = data[2] & 0xFF;
            this->patch_    = data[3] & 0xFF;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(*this != temp && false == importing) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

void ctrlm_rf4ce_sw_version_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[SW_VERSION_LEN];
    if(this->exportable && this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}
// end ctrlm_rf4ce_sw_version_t

// ctrlm_rf4ce_dsp_version_t
ctrlm_rf4ce_dsp_version_t::ctrlm_rf4ce_dsp_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("DSP ");
    this->set_key("version_dsp");
    this->set_index(INDEX_VERSIONING_DSP);
    this->set_export(false);
}

ctrlm_rf4ce_dsp_version_t::~ctrlm_rf4ce_dsp_version_t() {

}
// end ctrlm_rf4ce_dsp_version_t

// ctrlm_rf4ce_keyword_model_version_t
ctrlm_rf4ce_keyword_model_version_t::ctrlm_rf4ce_keyword_model_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("Keyword Model ");
    this->set_key("version_keyword_model");
    this->set_index(INDEX_VERSIONING_KEYWORD_MODEL);
    this->set_export(false);
}

ctrlm_rf4ce_keyword_model_version_t::~ctrlm_rf4ce_keyword_model_version_t() {

}
// end ctrlm_rf4ce_keyword_model_version_t

// ctrlm_rf4ce_arm_version_t
ctrlm_rf4ce_arm_version_t::ctrlm_rf4ce_arm_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("ARM ");
    this->set_key("version_arm");
    this->set_index(INDEX_VERSIONING_ARM);
    this->set_export(false);
}

ctrlm_rf4ce_arm_version_t::~ctrlm_rf4ce_arm_version_t() {

}
// end ctrlm_rf4ce_arm_version_t

// ctrlm_rf4ce_irdb_version_t
ctrlm_rf4ce_irdb_version_t::ctrlm_rf4ce_irdb_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("IRDB ");
    this->set_key("version_irdb");
    this->set_index(INDEX_VERSIONING_IRDB);
}

ctrlm_rf4ce_irdb_version_t::~ctrlm_rf4ce_irdb_version_t() {

}
// end ctrlm_rf4ce_irdb_version_t

// ctrlm_rf4ce_bootloader_version_t
ctrlm_rf4ce_bootloader_version_t::ctrlm_rf4ce_bootloader_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("Bootloader ");
    this->set_key("update_version_bootloader");
    this->set_identifier(ID_UPDATE_VERSIONING);
    this->set_index(INDEX_UPDATE_VERSIONING_BOOTLOADER);
}

ctrlm_rf4ce_bootloader_version_t::~ctrlm_rf4ce_bootloader_version_t() {

}
// end ctrlm_rf4ce_bootloader_version_t

// ctrlm_rf4ce_golden_version_t
ctrlm_rf4ce_golden_version_t::ctrlm_rf4ce_golden_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("Golden ");
    this->set_key("update_version_golden");
    this->set_identifier(ID_UPDATE_VERSIONING);
    this->set_index(INDEX_UPDATE_VERSIONING_GOLDEN);
}

ctrlm_rf4ce_golden_version_t::~ctrlm_rf4ce_golden_version_t() {

}
// end ctrlm_rf4ce_golden_version_t

// ctrlm_rf4ce_audio_data_version_t
ctrlm_rf4ce_audio_data_version_t::ctrlm_rf4ce_audio_data_version_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_sw_version_t(net, id) {
    this->set_name_prefix("Audio Data ");
    this->set_key("update_version_audio_data");
    this->set_identifier(ID_UPDATE_VERSIONING);
    this->set_index(INDEX_UPDATE_VERSIONING_AUDIO_DATA);
}

ctrlm_rf4ce_audio_data_version_t::~ctrlm_rf4ce_audio_data_version_t() {

}
// end ctrlm_rf4ce_audio_data_version_t

// ctrlm_rf4ce_hw_version_t
#define HW_VERSION_LEN  (0x04)
ctrlm_rf4ce_hw_version_t::ctrlm_rf4ce_hw_version_t(ctrlm_obj_network_t *net, ctrlm_obj_controller_rf4ce_t *controller, ctrlm_controller_id_t id) : 
ctrlm_hw_version_t(), 
ctrlm_db_attr_t(net, id), 
ctrlm_rf4ce_rib_attr_t(ID_VERSIONING, INDEX_VERSIONING_HARDWARE, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)  {
    this->set_key("version_hardware");
    this->controller = controller;
}

ctrlm_rf4ce_hw_version_t::~ctrlm_rf4ce_hw_version_t() {

}


#define RF4CE_CONTROLLER_MANUFACTURER_NOBODY           (0)
#define RF4CE_CONTROLLER_MANUFACTURER_REMOTE_SOLUTION  (1)
#define RF4CE_CONTROLLER_MANUFACTURER_UEI              (2)
#define RF4CE_CONTROLLER_MANUFACTURER_OMNI             (3)
std::string ctrlm_rf4ce_hw_version_t::get_manufacturer_name() const {
    std::stringstream ret; ret << "INVALID";
    switch(this->manufacturer) {
        case RF4CE_CONTROLLER_MANUFACTURER_NOBODY:           {ret.str("NOBODY"); break;}
        case RF4CE_CONTROLLER_MANUFACTURER_REMOTE_SOLUTION:  {ret.str("RS");     break;}
        case RF4CE_CONTROLLER_MANUFACTURER_UEI:              {ret.str("UEI");    break;}
        case RF4CE_CONTROLLER_MANUFACTURER_OMNI:             {ret.str("OMNI");   break;}
        default:                                             {ret << " (" << (int)this->manufacturer << ")"; break;}
    }
    return(ret.str());
}

bool ctrlm_rf4ce_hw_version_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());
    char buf[HW_VERSION_LEN];

    memset(buf, 0, sizeof(buf));
    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len == HW_VERSION_LEN) {
                this->manufacturer =  (buf[0] >> 4);
                this->model        =  (buf[0] & 0xF);
                this->revision     =   buf[1];
                this->lot          = ((buf[2] & 0xF) << 8) | buf[3];

                /// @todo Add this to XR11/XR15v1/2 extended obj eventually
                if(this->controller) {
                    ctrlm_rf4ce_controller_type_t type = this->controller->controller_type_get();
                    if(type == RF4CE_CONTROLLER_TYPE_XR11 ||
                    type == RF4CE_CONTROLLER_TYPE_XR15 ||
                    type == RF4CE_CONTROLLER_TYPE_XR15V2) {
                        if(!this->is_valid()) {
                            this->fix_invalid_version();
                        }
                    }
                }
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_hw_version_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());
    char buf[HW_VERSION_LEN];

    memset(buf, 0, sizeof(buf));
    this->to_buffer(buf, sizeof(buf));
    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_hw_version_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = HW_VERSION_LEN;
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_hw_version_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == HW_VERSION_LEN) {
            ctrlm_hw_version_t temp(*this);
            this->manufacturer =  (data[0] >> 4);
            this->model        =  (data[0] & 0xF);
            this->revision     =   data[1];
            this->lot          = ((data[2] & 0xF) << 8) | data[3];
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(*this != temp && false == importing) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

void ctrlm_rf4ce_hw_version_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[HW_VERSION_LEN];
    if(this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}

bool ctrlm_rf4ce_hw_version_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= HW_VERSION_LEN) {
        data[0] = (this->manufacturer << 4) | (this->model & 0xF);
        data[1] =  this->revision  & 0xFF;
        data[2] = (this->lot >> 8) & 0xF;
        data[3] =  this->lot       & 0xFF;
        ret = true;
    }
    return(ret);
}

#define XR11V2_IEEE_PREFIX                 0x00124B0000000000
#define XR15V1_IEEE_PREFIX                 0x00155F0000000000
#define XR15V2_IEEE_PREFIX                 0x00155F0000000000
#define XR15V2_IEEE_PREFIX_UP_TO_FEB_2018  0x48D0CF0000000000
#define XR15V2_IEEE_PREFIX_THIRD           0x00CC3F0000000000
#define DOES_IEEE_PREFIX_MATCH(x,y)        ((x.get_value() & 0xFFFFFF0000000000) == y)

bool ctrlm_rf4ce_hw_version_t::is_valid() const {
    bool ret = false;
    bool valid_ieee = false, valid_hw = false;
    ctrlm_rf4ce_controller_type_t type = RF4CE_CONTROLLER_TYPE_INVALID;
    ctrlm_ieee_db_addr_t ieee;
    if(this->controller) {
        type = this->controller->controller_type_get();
        ieee = this->controller->ieee_address_get();
    }
    switch(type) {
        case RF4CE_CONTROLLER_TYPE_XR11: {
            if(DOES_IEEE_PREFIX_MATCH(ieee, XR11V2_IEEE_PREFIX)) {
                ctrlm_hw_version_t valid(2, 2, 1, 0);
                valid_ieee = true;
                if(*this == valid) {
                    valid_hw = true;
                }
            }
            break;
        }
        case RF4CE_CONTROLLER_TYPE_XR15: {
            if(DOES_IEEE_PREFIX_MATCH(ieee, XR15V1_IEEE_PREFIX)) {
                ctrlm_hw_version_t valid(2, 3, 1, 0);
                valid_ieee = true;
                if(*this == valid) {
                    valid_hw = true;
                }
            }
            break;
        }
        case RF4CE_CONTROLLER_TYPE_XR15V2: {
            if((DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX)) ||
               (DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX_UP_TO_FEB_2018)) ||
               (DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX_THIRD))) {
                ctrlm_hw_version_t valid(2, 3, 2, 0);
                ctrlm_hw_version_t valid2(1, 3, 2, 0);
                valid_ieee = true;
                if(*this == valid || *this == valid2) {
                    valid_hw = true;
                }
            }
            break;
        }
        default: {
            valid_ieee = valid_hw = true;
            break;
        }
    }
    if(valid_ieee == false) {
        XLOGD_INFO("Invalid ieee_address <0x%016llX>", ieee);
    } else {
        if(valid_hw == false) {
            XLOGD_INFO("Invalid hardware version: ieee_address 0x%016llX, controller_type <%s>, hardware_version <%s>", ieee.get_value(), ctrlm_rf4ce_controller_type_str(type), this->to_string().c_str());
        } else {
            ret = true;
        }
    }
    return(ret);
}

void ctrlm_rf4ce_hw_version_t::fix_invalid_version() {
    bool valid_ieee = false;
    ctrlm_rf4ce_controller_type_t type = RF4CE_CONTROLLER_TYPE_INVALID;
    ctrlm_ieee_db_addr_t ieee;
    if(this->controller) {
        type = this->controller->controller_type_get();
        ieee = this->controller->ieee_address_get();
    }
    switch(type) {
        case RF4CE_CONTROLLER_TYPE_XR11: {
            if(DOES_IEEE_PREFIX_MATCH(ieee, XR11V2_IEEE_PREFIX)) {
                ctrlm_hw_version_t valid(2, 2, 1, 0);
                valid_ieee = true;
                this->from(&valid);
            }
            break;
        }
        case RF4CE_CONTROLLER_TYPE_XR15: {
            if(DOES_IEEE_PREFIX_MATCH(ieee, XR15V1_IEEE_PREFIX)) {
                ctrlm_hw_version_t valid(2, 3, 1, 0);
                valid_ieee = true;
                this->from(&valid);
            }
            break;
        }
        case RF4CE_CONTROLLER_TYPE_XR15V2: {
            if((DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX)) ||
               (DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX_UP_TO_FEB_2018)) ||
               (DOES_IEEE_PREFIX_MATCH(ieee, XR15V2_IEEE_PREFIX_THIRD))) {
                ctrlm_hw_version_t valid(2, 3, 2, 0);
                valid_ieee = true;
                this->from(&valid);
            }
            break;
        }
        default: {
            valid_ieee = true;
            break;
        }
    }
    if(valid_ieee == false) {
        XLOGD_INFO("Invalid ieee_address <0x%016llX>", ieee);
    } else {
        XLOGD_INFO("ieee_address 0x%016llX, controller_type <%s>, hardware_version <%s>", ieee.get_value(), ctrlm_rf4ce_controller_type_str(type), this->to_string().c_str());
    }
}

const char *ctrlm_rf4ce_hw_version_t::rf4cemgr_product_name(ctrlm_hw_version_t *version) {
    if(version) {
        if(version->get_manufacturer() == 2 &&
           version->get_model()        == 2 &&
           version->get_revision()     >= 1) {
            return("XR11-20");
        }
        if( version->get_manufacturer() == 2 &&
          ( version->get_model()        == 0 || version->get_model()        == 3) &&
            version->get_revision()     >= 1) {
            return("XR15-10");
        }
    }
    return("XR5-40");
}
// end ctrlm_rf4ce_hw_version_t

// ctrlm_rf4ce_sw_build_id_t
#define RF4CE_BUILD_ID_MAX_LEN (92)

ctrlm_rf4ce_sw_build_id_t::ctrlm_rf4ce_sw_build_id_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) : 
ctrlm_sw_build_id_t(), 
ctrlm_db_attr_t(net, id), 
ctrlm_rf4ce_rib_attr_t(ID_VERSIONING, INDEX_VERSIONING_BUILD_ID, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)  {
    this->set_key("version_build_id");
}

ctrlm_rf4ce_sw_build_id_t::~ctrlm_rf4ce_sw_build_id_t() {

}

bool ctrlm_rf4ce_sw_build_id_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());

    if(blob.read_db(ctx)) {
        this->id = blob.to_string();
        if(!this->id.empty() && this->id.length() <= RF4CE_BUILD_ID_MAX_LEN) {
            ret = true;
            XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_WARN("invalid size read from db <%s, %d>", this->get_name().c_str(), this->id.length());
            this->id.clear();
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_sw_build_id_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->key, this->get_table());

    if(blob.from_string(this->id)) {
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

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_sw_build_id_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= this->id.length()) {
            memcpy(data, this->id.data(), this->id.length());
            *len = this->id.length();
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_sw_build_id_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len <= RF4CE_BUILD_ID_MAX_LEN) {
            ctrlm_sw_build_id_t temp(this->id);
            this->id = std::string(data);
            if(this->id.length() > RF4CE_BUILD_ID_MAX_LEN) {
                XLOGD_WARN("copied more than max allowed bytes, using substr");
                this->id = this->id.substr(RF4CE_BUILD_ID_MAX_LEN);
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(*this != temp) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_sw_build_id_t

// ctrlm_rf4ce_dsp_build_id_t
ctrlm_rf4ce_dsp_build_id_t::ctrlm_rf4ce_dsp_build_id_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) : 
ctrlm_rf4ce_sw_build_id_t(net, id) {
    this->set_name_prefix("DSP ");
    this->set_key("version_dsp_build_id");
    this->set_index(INDEX_VERSIONING_DSP_BUILD_ID);
}

ctrlm_rf4ce_dsp_build_id_t::~ctrlm_rf4ce_dsp_build_id_t() {

}
// end ctrlm_rf4ce_dsp_build_id_t
