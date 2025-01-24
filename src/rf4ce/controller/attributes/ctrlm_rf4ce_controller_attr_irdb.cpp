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
#include "rf4ce/ctrlm_rf4ce_network.h"
#include "ctrlm_device_update.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_db_types.h"
#include "ctrlm_database.h"
#include <sstream>

// ctrlm_rf4ce_irdb_status_t
ctrlm_rf4ce_irdb_status_t::ctrlm_rf4ce_irdb_status_t() :
ctrlm_attr_t("IRDB Status") {
    this->flags = ctrlm_rf4ce_irdb_status_t::flag::NO_IR_PROGRAMMED;
}

ctrlm_rf4ce_irdb_status_t::ctrlm_rf4ce_irdb_status_t(const ctrlm_rf4ce_irdb_status_t& status) {
    this->flags           = status.flags;
    this->irdb_string_tv  = status.irdb_string_tv;
    this->irdb_string_avr = status.irdb_string_avr;
}

ctrlm_rf4ce_irdb_status_t::~ctrlm_rf4ce_irdb_status_t() {

}

bool ctrlm_rf4ce_irdb_status_t::is_flag_set(ctrlm_rf4ce_irdb_status_t::flag f) const {
    return(this->flags & f);
}

std::string ctrlm_rf4ce_irdb_status_t::flag_str(ctrlm_rf4ce_irdb_status_t::flag f) {
    std::stringstream ret; ret << "INVALID";
    switch(f) {
        case ctrlm_rf4ce_irdb_status_t::flag::NO_IR_PROGRAMMED:          {ret.str("NO_IR_PROGRAMMED"); break;}  
        case ctrlm_rf4ce_irdb_status_t::flag::IR_DB_TYPE:                {ret.str("IR_DB_TYPE"); break;}  
        case ctrlm_rf4ce_irdb_status_t::flag::IR_RF_DB:                  {ret.str("IR_RF_DB"); break;}  
        case ctrlm_rf4ce_irdb_status_t::flag::IR_DB_CODE_TV:             {ret.str("IR_DB_CODE_TV"); break;}  
        case ctrlm_rf4ce_irdb_status_t::flag::IR_DB_CODE_AVR:            {ret.str("IR_DB_CODE_AVR"); break;}  
        case ctrlm_rf4ce_irdb_status_t::flag::LOAD_5_DIGIT_CODE_SUPPORT: {ret.str("LOAD_5_DIGIT_CODE_SUPPORT"); break;}
        default: { ret << " (" << (int)f << ")"; break;}
    }
    return(ret.str());
}

uint8_t ctrlm_rf4ce_irdb_status_t::get_flags() const {
    return(this->flags);
}

std::string ctrlm_rf4ce_irdb_status_t::get_tv_code_str() const {
    return(this->irdb_string_tv);
}

std::string ctrlm_rf4ce_irdb_status_t::get_avr_code_str() const {
    return(this->irdb_string_avr);
}

std::string ctrlm_rf4ce_irdb_status_t::to_string() const {
    std::stringstream ss;
    std::ios_base::fmtflags ff(ss.flags());
    ss << "Flags 0x" << COUT_HEX_MODIFIER << std::uppercase << (int)this->flags << " "; ss.flags(ff);
    ss << "TV Code <" << this->irdb_string_tv << "> ";
    ss << "AVR Code <" << this->irdb_string_avr << "> ";
    return(ss.str());
}
// end ctrlm_rf4ce_irdb_status_t

// ctrlm_rf4ce_controller_irdb_status_t
#define CONTROLLER_IRDB_STATUS_ID           (0xDD)
#define CONTROLLER_IRDB_STATUS_INDEX        (0x00)
#define CONTROLLER_IRDB_STATUS_LEN          (13)
#define CONTROLLER_IRDB_STATUS_EXTENDED_LEN (15)
ctrlm_rf4ce_controller_irdb_status_t::ctrlm_rf4ce_controller_irdb_status_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_rf4ce_irdb_status_t(),
ctrlm_db_attr_t(net, id),
ctrlm_rf4ce_rib_attr_t(CONTROLLER_IRDB_STATUS_ID, CONTROLLER_IRDB_STATUS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER) {
    this->set_name_prefix("Controller ");
    this->tv_load_status   = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
    this->avr_load_status  = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
}

ctrlm_rf4ce_controller_irdb_status_t::ctrlm_rf4ce_controller_irdb_status_t(const ctrlm_rf4ce_controller_irdb_status_t& status) :
ctrlm_rf4ce_irdb_status_t(status) {
    this->tv_load_status   = status.tv_load_status;
    this->avr_load_status  = status.avr_load_status;
}

ctrlm_rf4ce_controller_irdb_status_t::~ctrlm_rf4ce_controller_irdb_status_t() {

}

ctrlm_rf4ce_controller_irdb_status_t::load_status ctrlm_rf4ce_controller_irdb_status_t::get_tv_load_status() const {
    return(this->tv_load_status);
}

ctrlm_rf4ce_controller_irdb_status_t::load_status ctrlm_rf4ce_controller_irdb_status_t::get_avr_load_status() const {
    return(this->avr_load_status);
}

void ctrlm_rf4ce_controller_irdb_status_t::set_updated_listener(ctrlm_rf4ce_controller_irdb_status_listener_t listener) {
    this->updated_listener = listener;
}

std::string ctrlm_rf4ce_controller_irdb_status_t::load_status_str(load_status s) {
    std::stringstream ret; ret << "INVALID";
    switch(s) {
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE:                             {ret.str("NONE"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::CODE_CLEARING_SUCCESS:            {ret.str("CODE_CLEARING_SUCCESS"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::CODE_SETTING_SUCCESS:             {ret.str("CODE_SETTING_SUCCESS"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::SET_AND_CLEAR_ERROR:              {ret.str("SET_AND_CLEAR_ERROR"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::CODE_CLEAR_FALIED:                {ret.str("CODE_CLEAR_FAILED"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::CODE_SET_FAILED_INVALID_CODE:     {ret.str("CODE_SET_FAILED_INVALID_CODE"); break;}
        case ctrlm_rf4ce_controller_irdb_status_t::load_status::CODE_SET_FAILED_UNKNOWN_REASON:   {ret.str("CODE_SET_FAILED_UNKNOWN_REASON"); break;}
        default: { ret << " (" << (int)s << ")"; break; }
    }
    return(ret.str());
}

std::string ctrlm_rf4ce_controller_irdb_status_t::to_string() const {
    std::stringstream ss;
    ss << ctrlm_rf4ce_irdb_status_t::to_string();
    ss << "TV Load Status <" << this->load_status_str(this->tv_load_status) << "> ";
    ss << "AVR Load Status <" << this->load_status_str(this->avr_load_status) << "> ";
    return(ss.str());
}

bool ctrlm_rf4ce_controller_irdb_status_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("controller_irdb_status", this->get_table());
    char buf[CONTROLLER_IRDB_STATUS_EXTENDED_LEN];

    memset(buf, 0, sizeof(buf));
    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len >= CONTROLLER_IRDB_STATUS_LEN) {
                this->flags               = buf[0];
                this->irdb_string_tv      = std::string(&buf[1]);
                if(this->irdb_string_tv.length() > 6) {
                    this->irdb_string_tv = this->irdb_string_tv.substr(6);;
                }
                this->irdb_string_avr     = std::string(&buf[7]);
                if(this->irdb_string_avr.length() > 6) {
                    this->irdb_string_avr = this->irdb_string_avr.substr(6);;
                }
                if(len == CONTROLLER_IRDB_STATUS_EXTENDED_LEN) {
                    this->tv_load_status  = (ctrlm_rf4ce_controller_irdb_status_t::load_status)buf[13];
                    this->avr_load_status = (ctrlm_rf4ce_controller_irdb_status_t::load_status)buf[14];
                } else {
                    XLOGD_INFO("Length <%d> does not include load status bytes.", len);
                    this->tv_load_status  = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
                    this->avr_load_status = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
                }
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db is too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_controller_irdb_status_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("controller_irdb_status", this->get_table());
    char buf[CONTROLLER_IRDB_STATUS_EXTENDED_LEN];

    memset(buf, 0, sizeof(buf));
    buf[0] = this->flags;
    this->irdb_string_tv.copy(&buf[1], 6);
    this->irdb_string_avr.copy(&buf[7], 6);
    buf[13] = (uint8_t)(this->tv_load_status & 0xFF);
    buf[14] = (uint8_t)(this->avr_load_status & 0xFF);
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

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_irdb_status_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= CONTROLLER_IRDB_STATUS_LEN) {
            data[0] = this->flags;
            this->irdb_string_tv.copy(&data[1], 5);
            this->irdb_string_avr.copy(&data[7], 5);
            if(*len >= CONTROLLER_IRDB_STATUS_EXTENDED_LEN) {
                data[13] = (uint8_t)(this->tv_load_status & 0xFF);
                data[14] = (uint8_t)(this->avr_load_status & 0xFF);
                *len = CONTROLLER_IRDB_STATUS_EXTENDED_LEN;
            } else {
                *len = CONTROLLER_IRDB_STATUS_LEN;
            }
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

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_irdb_status_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == CONTROLLER_IRDB_STATUS_LEN || len == CONTROLLER_IRDB_STATUS_EXTENDED_LEN) {
            ctrlm_rf4ce_controller_irdb_status_t temp(*this);
            this->flags               = data[0];
            this->irdb_string_tv      = std::string(&data[1]);
            if(this->irdb_string_tv.length() > 6) {
                this->irdb_string_tv = this->irdb_string_tv.substr(6);
            }
            this->irdb_string_avr     = std::string(&data[7]);
            if(this->irdb_string_avr.length() > 6) {
                this->irdb_string_avr = this->irdb_string_avr.substr(6);
            }
            if(len == 15) {
                this->tv_load_status  = (ctrlm_rf4ce_controller_irdb_status_t::load_status)data[13];
                this->avr_load_status = (ctrlm_rf4ce_controller_irdb_status_t::load_status)data[14];
            } else {
                XLOGD_INFO("Length <%d> does not include load status bytes.", len);
                this->tv_load_status  = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
                this->avr_load_status = ctrlm_rf4ce_controller_irdb_status_t::load_status::NONE;
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(*this != temp) {
                ctrlm_db_attr_write(shared_from_this());
                if(this->updated_listener) {
                    XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
                    this->updated_listener(*this);
                }
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
// end ctrlm_rf4ce_controller_irdb_status_t

// ctrlm_rf4ce_ir_rf_database_status_t
ctrlm_rf4ce_ir_rf_database_status_t::ctrlm_rf4ce_ir_rf_database_status_t(int status) : ctrlm_attr_t("IRRF Database Status") {
    *this = status; // handled by operator=
}

ctrlm_rf4ce_ir_rf_database_status_t::~ctrlm_rf4ce_ir_rf_database_status_t() {

}

std::string ctrlm_rf4ce_ir_rf_database_status_t::flag_str(ctrlm_rf4ce_ir_rf_database_status_t::flag f) {
    std::stringstream ss; ss << "INVALID";
    switch(f) {
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD:            {ss.str("FORCE_DOWNLOAD"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE:  {ss.str("DOWNLOAD_TV_5_DIGIT_CODE"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE: {ss.str("DOWNLOAD_AVR_5_DIGIT_CODE"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR:          {ss.str("TX_IR_DESCRIPTOR"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES:   {ss.str("CLEAR_ALL_5_DIGIT_CODES"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO:            {ss.str("DB_DOWNLOAD_NO"); break;}
        case ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES:           {ss.str("DB_DOWNLOAD_YES"); break;}
        default: {ss << "(" << (int)f << ")"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_ir_rf_database_status_t::to_string() const {
    std::stringstream ss;
    ss << "Status Download <";
    if(this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO &&
       this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) {
        ss << "YES & NO";
    } else if(this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO) {
        ss << "NO";
    } else if(this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) {
        ss << "YES";
    } else {
        ss << "NONE";
    }
    ss << "> ";
    ss << "Tx Ir <" << (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR ? "YES" : "NO") << "> ";
    ss << "Force <" << (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD ? "YES" : "NO") << "> ";
    ss << "Download TV 5-Digit IRDB Code <" << (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE ? "YES" : "NO") << "> ";
    ss << "Download AVR 5-Digit IRDB Code <" << (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE ? "YES" : "NO") << "> ";
    ss << "Clear All 5-Digit IRDB Codes <" << (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES ? "YES" : "NO") << "> ";
    return(ss.str());
}
// end ctrlm_rf4ce_ir_rf_database_status_t

// ctrlm_rf4ce_controller_ir_rf_database_status_t
#define IR_RF_STATUS_ID    (0xDA)
#define IR_RF_STATUS_INDEX (0x00)
#define IR_RF_STATUS_LEN   (1)
ctrlm_rf4ce_controller_ir_rf_database_status_t::ctrlm_rf4ce_controller_ir_rf_database_status_t(ctrlm_obj_controller_rf4ce_t *controller) : 
ctrlm_rf4ce_ir_rf_database_status_t(),
ctrlm_rf4ce_rib_attr_t(IR_RF_STATUS_ID, IR_RF_STATUS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET) 
{
    this->controller = controller;
}

ctrlm_rf4ce_controller_ir_rf_database_status_t::~ctrlm_rf4ce_controller_ir_rf_database_status_t() {

}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_ir_rf_database_status_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= IR_RF_STATUS_LEN) {
            // ROBIN -- Clear IR-RF database status
            if(this->controller && this->controller->controller_type_get() == RF4CE_CONTROLLER_TYPE_XR19) {
                if(accessor == ctrlm_rf4ce_rib_attr_t::access::TARGET) {
                    data[0] = IR_RF_DATABASE_STATUS_DEFAULT;
                } else {
                    data[0] = this->ir_rf_status;
                }
                ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            } else { // Normal Read
                data[0] = this->ir_rf_status;
                *len = IR_RF_STATUS_LEN;
#ifdef XR15_704
                if(this->controller && this->controller->needs_reset() && !ctrlm_device_update_is_controller_updating(this->controller->network_id_get(), this->controller->controller_id_get(), true)) {
                    if(ctrlm_device_update_xr15_crash_update_get()) {
                        XLOGD_INFO("ENTERING XR15 CRASH CODE: XR15-10 running less then 2.0.0.0, need to force reboot for device update... Setting proper IR RF Status bits");
                        data[0] = ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES | ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD;
                        XLOGD_INFO("MODIFIED Status Download <%s> Tx Ir <%s> Force <%s> Download TV 5-Digit IRDB Code <%s> Download AVR 5-Digit IRDB Code <%s> Clear All 5-Digit IRDB Codes <%s>",
                            (data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) ? "YES" : "NO", (data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR) ? "YES" : "NO",
                            (data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD) ? "YES" : "NO", (data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) ? "YES" : "NO",
                            (data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE) ? "YES" : "NO",(data[0] & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) ? "YES" : "NO");
                    }
                }
#endif
                if(this->controller && this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) {
                    XLOGD_INFO("Creating timer for download flag reset");
                    ctrlm_timeout_create(200, ir_rf_database_status_download_timeout, (void *)this->controller);
                }
                ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
                XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data/length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_ir_rf_database_status_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == IR_RF_STATUS_LEN) {
            uint8_t status = data[0];

            // ROBIN -- Add polling method if any bits are set 
            if(this->controller && this->controller->controller_type_get() == RF4CE_CONTROLLER_TYPE_XR19) {
                if((this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) || (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) || (this->ir_rf_status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE)) {
                    XLOGD_INFO("IR-RF Database Status has bits set for XR19 (0x%02X), adding polling action", this->ir_rf_status);
                    this->controller->push_ir_codes();
                }
                ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
                return(ret);
            }
    
            // FYI - Kept all the logging here the same in case of telemetry strings being used
            if(status & ctrlm_rf4ce_ir_rf_database_status_t::flag::RESERVED) {
                XLOGD_ERROR("INVALID PARAMETERS - Reserved bit is set 0x%02X", status);
            } else if((status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO) && (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES)) {
                XLOGD_ERROR("Status Download <YES and NO> Tx Ir <%s> Force <%s> Download TV 5-Digit IRDB Code <%s> Download AVR 5-Digit IRDB Code <%s> Clear All 5-Digit IRDB Codes <%s>",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD) ? "YES" : "NO",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE) ? "YES" : "NO",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) ? "YES" : "NO");
            } else if(0 == (status & (ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO | ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES))) {
                XLOGD_ERROR("Status Download <None> Tx Ir <%s> Force <%s> Download TV 5-Digit IRDB Code <%s> Download AVR 5-Digit IRDB Code <%s> Clear All 5-Digit IRDB Codes <%s>",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD) ? "YES" : "NO",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE) ? "YES" : "NO",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) ? "YES" : "NO");
            } else if((status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) && ((status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) || (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE))) {
                XLOGD_ERROR("Status Download <%s> Tx Ir <%s> Force <%s> 5-Digit IRDB Code <SET and CLEAR>",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::TX_IR_DESCRIPTOR) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::FORCE_DOWNLOAD) ? "YES" : "NO");
            } else if(((status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) || (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) || (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE)) && (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_YES)) {
                XLOGD_ERROR("Status Download <YES> Set OR Clear 5-Digit IRDB Codes <YES> Download TV 5-Digit IRDB Code <%s> Download AVR 5-Digit IRDB Code <%s> Clear All 5-Digit IRDB Codes <%s>",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_TV_5_DIGIT_CODE) ? "YES" : "NO", (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::DOWNLOAD_AVR_5_DIGIT_CODE) ? "YES" : "NO",
                        (status & ctrlm_rf4ce_ir_rf_database_status_t::flag::CLEAR_ALL_5_DIGIT_CODES) ? "YES" : "NO");
            } else {
                this->ir_rf_status = status;
                ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
                XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
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

void ctrlm_rf4ce_controller_ir_rf_database_status_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[IR_RF_STATUS_LEN];
    buf[0] = this->ir_rf_status;
    export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
}
// end ctrlm_rf4ce_controller_ir_rf_database_status_t
