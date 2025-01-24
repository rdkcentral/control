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
#ifndef __CTRLM_RF4CE_ATTR_IRDB_H__
#define __CTRLM_RF4CE_ATTR_IRDB_H__
#include <iostream>
#include <string>
#include <functional>
#include "ctrlm_attr.h"
#include "ctrlm_db_attr.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"

/**
 * @brief ControlMgr RF4CE IRDB Status Class
 * 
 * This class contains the implementation of the IRDB status attribute
 */
class ctrlm_rf4ce_irdb_status_t : public ctrlm_attr_t {
public:
    /**
     * ControlMgr RF4CE IRDB Status Constructor
     */
    ctrlm_rf4ce_irdb_status_t();
    /**
     * ControlMgr RF4CE IRDB Status Copy Constructor
     */
    ctrlm_rf4ce_irdb_status_t(const ctrlm_rf4ce_irdb_status_t& status);
    /**
     * ControlMgr RF4CE IRDB Status Destructor
     */
    virtual ~ctrlm_rf4ce_irdb_status_t();

public:
    /**
     * Enum of flag values 
     */
    enum flag {
        NO_IR_PROGRAMMED          = 0x01,
        IR_DB_TYPE                = 0x02,
        IR_RF_DB                  = 0x04,
        IR_DB_CODE_TV             = 0x08,
        IR_DB_CODE_AVR            = 0x10,
        LOAD_5_DIGIT_CODE_SUPPORT = 0x80
    };

public:
    bool is_flag_set(flag f) const;
    static std::string flag_str(flag f);
    uint8_t get_flags() const;
    std::string get_tv_code_str() const;
    std::string get_avr_code_str() const;

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the IRDB status
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

protected:
    uint8_t     flags;
    std::string irdb_string_tv; // length 7
    std::string irdb_string_avr; // length 7
};

inline bool operator==(const ctrlm_rf4ce_irdb_status_t& s1, const ctrlm_rf4ce_irdb_status_t& s2) {
    return(s1.get_flags() == s2.get_flags() && s1.get_tv_code_str() == s2.get_tv_code_str() && s1.get_avr_code_str() == s2.get_avr_code_str());
}


class ctrlm_rf4ce_controller_irdb_status_t;
typedef std::function<void(const ctrlm_rf4ce_controller_irdb_status_t&)> ctrlm_rf4ce_controller_irdb_status_listener_t;

/**
 * @brief ControlMgr RF4CE Controller IRDB Status Class
 * 
 * This class contains the implementation of the Controller IRDB status attribute
 */
class ctrlm_rf4ce_controller_irdb_status_t : public ctrlm_rf4ce_irdb_status_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * ControlMgr RF4CE Controller IRDB Status Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_controller_irdb_status_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Controller IRDB Status Copy Constructor
     */
    ctrlm_rf4ce_controller_irdb_status_t(const ctrlm_rf4ce_controller_irdb_status_t& status);
    /**
     * ControlMgr RF4CE Controller IRDB Status Destructor
     */
    virtual ~ctrlm_rf4ce_controller_irdb_status_t();

public:
    enum load_status {
        NONE                           = 0x00,
        CODE_CLEARING_SUCCESS          = 0x10,
        CODE_SETTING_SUCCESS           = 0x20,
        SET_AND_CLEAR_ERROR            = 0x30,
        CODE_CLEAR_FALIED              = 0x40,
        CODE_SET_FAILED_INVALID_CODE   = 0x50,
        CODE_SET_FAILED_UNKNOWN_REASON = 0x60
    };

public:
    load_status get_tv_load_status() const;
    load_status get_avr_load_status() const;
    void set_updated_listener(ctrlm_rf4ce_controller_irdb_status_listener_t listener);
    static std::string load_status_str(load_status s); 

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the IRDB status
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

public:
    /**
     * Interface implementation to read the data from DB
     * @see ctrlm_db_attr_t::read_db
     */
    virtual bool read_db(ctrlm_db_ctx_t ctx);
    /**
     * Interface implementation to write the data to DB
     * @see ctrlm_db_attr_t::write_db
     */
    virtual bool write_db(ctrlm_db_ctx_t ctx);

public:
    /**
     * Interface implementation for a RIB read
     * @see ctrlm_rf4ce_rib_attr_t::read_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len);
    /**
     * Interface implementation for a RIB write
     * @see ctrlm_rf4ce_rib_attr_t::write_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing);

protected:
    load_status tv_load_status;
    load_status avr_load_status;
    ctrlm_rf4ce_controller_irdb_status_listener_t updated_listener;
};

inline bool operator==(const ctrlm_rf4ce_controller_irdb_status_t& s1, const ctrlm_rf4ce_controller_irdb_status_t& s2) {
    return(((ctrlm_rf4ce_irdb_status_t)s1 == (ctrlm_rf4ce_irdb_status_t)s2) && s1.get_tv_load_status() == s2.get_tv_load_status() && s1.get_avr_load_status() == s2.get_avr_load_status());
}
inline bool operator!=(const ctrlm_rf4ce_controller_irdb_status_t& s1, const ctrlm_rf4ce_controller_irdb_status_t& s2) {
    return(!(s1 == s2));
}

#define IR_RF_DATABASE_STATUS_DEFAULT                   (ctrlm_rf4ce_ir_rf_database_status_t::flag::DB_DOWNLOAD_NO)
/**
 * @brief ControlMgr RF4CE IRRF Database Status
 * 
 * The class contains an RF4CE IRRF Database Status
 */
class ctrlm_rf4ce_ir_rf_database_status_t : public ctrlm_attr_t {
public:
    /**
     * Enum for the flags in the status 
     */
    enum flag {
        FORCE_DOWNLOAD             = 0x01,
        DOWNLOAD_TV_5_DIGIT_CODE   = 0x02,
        DOWNLOAD_AVR_5_DIGIT_CODE  = 0x04,
        TX_IR_DESCRIPTOR           = 0x08,
        CLEAR_ALL_5_DIGIT_CODES    = 0x10,
        DB_DOWNLOAD_NO             = 0x40,
        DB_DOWNLOAD_YES            = 0x80,
        RESERVED                   = 0x20,
    };

public:
    /**
     * RF4CE IRRF Database Status Constructor 
     * @param status The status bitmap to initialize to
     */
    ctrlm_rf4ce_ir_rf_database_status_t(int status = IR_RF_DATABASE_STATUS_DEFAULT);
    /**
     * RF4CE IRRF Database Status Destructor 
     */
    virtual ~ctrlm_rf4ce_ir_rf_database_status_t();
    // operators
    inline ctrlm_rf4ce_ir_rf_database_status_t& operator=(const int& s) {
        this->ir_rf_status = (s & 0xFF);
        return(*this);
    }
    inline bool operator==(const ctrlm_rf4ce_ir_rf_database_status_t& s) {
        return(this->ir_rf_status == s.ir_rf_status);
    }
    inline bool operator!=(const ctrlm_rf4ce_ir_rf_database_status_t& s) {
        return(!(this->ir_rf_status == s.ir_rf_status));
    }
    inline bool operator&(const int& f) {
        return(this->ir_rf_status & f);
    }
    inline bool operator|(const int& f) {
        return(this->ir_rf_status | f);
    }
    inline ctrlm_rf4ce_ir_rf_database_status_t& operator&=(const int& f) {
        this->ir_rf_status &= (f & 0xFF);
        return(*this);
    }
    inline ctrlm_rf4ce_ir_rf_database_status_t& operator|=(const int& f) {
        this->ir_rf_status |= (f & 0xFF);
        return(*this);
    }
    // end operators

public:
    /**
     * Static function to get the string associated with a flag
     * @param f The flag
     */
    static std::string flag_str(flag f);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the IRDB status
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

protected:
    uint8_t ir_rf_status;
};

class ctrlm_obj_controller_rf4ce_t;
/**
 * @brief ControlMgr RF4CE Controller IRRF Database Status
 * 
 * This class contains the implementation of the Controller IRRF Database Status
 */
class ctrlm_rf4ce_controller_ir_rf_database_status_t : public ctrlm_rf4ce_ir_rf_database_status_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Controller IRRF Database Status Constructor
     * @param controller The RF4CE controller object for this attribute
     * @todo Extend this object for any special cases when controller refactor is done
     */
    ctrlm_rf4ce_controller_ir_rf_database_status_t(ctrlm_obj_controller_rf4ce_t *controller = NULL);
    /**
     * RF4CE Controller IRRF Database Status Destructor
     */
    virtual ~ctrlm_rf4ce_controller_ir_rf_database_status_t();
    // operators
    inline ctrlm_rf4ce_controller_ir_rf_database_status_t& operator=(const int& s) {
        ctrlm_rf4ce_ir_rf_database_status_t::operator=(s);
        return(*this);
    }
    // end operators

public:
    /**
     * Interface implementation for a RIB read
     * @see ctrlm_rf4ce_rib_attr_t::read_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len);
    /**
     * Interface implementation for a RIB write
     * @see ctrlm_rf4ce_rib_attr_t::write_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing);
    /**
     * Interface implementation for a RIB export
     * @see ctrlm_rf4ce_rib_attr_t::export_rib
     */
    virtual void export_rib(rf4ce_rib_export_api_t export_api);

protected:
    ctrlm_obj_controller_rf4ce_t *controller;
};
#endif