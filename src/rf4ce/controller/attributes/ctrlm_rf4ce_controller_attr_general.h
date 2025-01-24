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
#ifndef __CTRLM_RF4CE_ATTR_GENERAL_H__
#define __CTRLM_RF4CE_ATTR_GENERAL_H__
#include "ctrlm_attr_general.h"
#include "ctrlm_db_attr.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"
#include "ctrlm_version.h"
#include <vector>
#include <functional>


class ctrlm_obj_controller_rf4ce_t;
/**
 * @brief ControlMgr RF4CE Controller Memory Dump Class
 * 
 * This class implements the controller memory dump RIB attribute
 */
class ctrlm_rf4ce_controller_memory_dump_t : public ctrlm_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    ctrlm_rf4ce_controller_memory_dump_t(ctrlm_obj_controller_rf4ce_t *controller = NULL);
    virtual ~ctrlm_rf4ce_controller_memory_dump_t();

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the Memory Dump info
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

public:
    // ONLY NEED TO IMPLEMENT RIB WRITE, RIB READ NOT IMPLEMENTED
    /**
     * Interface implementation for a RIB write
     * @see ctrlm_rf4ce_rib_attr_t::write_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing);

private:
    uint8_t *crash_dump_buf;
    uint16_t crash_dump_size;
    uint16_t crash_dump_expected_size;
    ctrlm_obj_controller_rf4ce_t *controller;
};

class ctrlm_rf4ce_product_name_t;
/// The type of listener for product name changes
typedef std::function<void(const ctrlm_rf4ce_product_name_t&)> ctrlm_rf4ce_product_name_listener_t;

/**
 * @brief ControlMgr RF4CE Product Name Class
 * 
 * The product name class implements the RIB / DB interfaces for RF4CE products 
 */
class ctrlm_rf4ce_product_name_t : public ctrlm_string_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Product Name Constructor
     * @param net The network that the attribute is associated with
     * @param id The controller id that the attribute is associated with
     */
    ctrlm_rf4ce_product_name_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * RF4CE Product Name Destructor
     */
    virtual ~ctrlm_rf4ce_product_name_t();

public:
    /**
     * Function for setting the product name - has some additional logic for RF4CE
     * @param product_name The name of the product
     */
    virtual void set_product_name(std::string product_name);
    /**
     * Function for setting the listener for product name updates
     * @param listener Function to be used as the listener
     */
    void set_updated_listener(ctrlm_rf4ce_product_name_listener_t listener);

public:
    /**
     * Function prediciting the chipset based off of the product name
     * @param product_name The name of the product
     * @todo Controller object should have a function for chipset, extended by each controller object type
     */
    virtual std::string get_predicted_chipset() const;

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
    /**
     * Interface implementation for a RIB export
     * @see ctrlm_rf4ce_rib_attr_t::export_rib
     */
    virtual void export_rib(rf4ce_rib_export_api_t export_api);

protected:
    /**
     * Function used by read_rib & export_rib to put attribute data into buffer
     * @param data The buffer to place the data -- this function assumes data is a valid pointer
     * @param len The length of the buffer
     * @return True if successfully placed data in buffer, otherwise False
     */
    bool to_buffer(char *data, size_t len);

private:
    ctrlm_rf4ce_product_name_listener_t updated_listener;
};

class ctrlm_rf4ce_voice_command_length_t;
/**
 * @brief Controlmgr RF4CE RIB Entries Updated Class
 * 
 * This attribute tells the controller if specific RIB entries have been updated.
 */
class ctrlm_rf4ce_rib_entries_updated_t : public ctrlm_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * Enum of the different updated values
     */
    enum updated {
        UPDATED_FALSE = 0,
        UPDATED_TRUE  = 1,
        UPDATED_STOP  = 2
    };

public:
    /**
     * RF4CE RIB Entries Updated Constructor 
     */
    ctrlm_rf4ce_rib_entries_updated_t();
    /**
     * RF4CE RIB Entries Updated Destructor 
     */
    virtual ~ctrlm_rf4ce_rib_entries_updated_t();
    /**
     * RF4CE RIB Entries Updated Bool Equals operator
     */
    inline ctrlm_rf4ce_rib_entries_updated_t& operator=(bool b) {
        if(b) this->reu = updated::UPDATED_TRUE;
        else  this->reu = updated::UPDATED_FALSE;
        return(*this);
    };

public:
    /**
     * Listener for the voice command length RIB attribute
     * @param length The voice command length RIB entry that was updated
     */
    void voice_command_length_updated(const ctrlm_rf4ce_voice_command_length_t& length);
    /**
     * Static function which returns the string for an updated value
     * @param u The updated value
     * @return The updated value string
     */
    static std::string updated_str(updated u);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the rib entries updated info
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

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

private:
    updated reu;
};

/**
 * @brief ControlMgr RF4CE Controller Capabilities Class
 * 
 * This class maintains the list of capabilities for a controller
 */
class ctrlm_rf4ce_controller_capabilities_t : public ctrlm_controller_capabilities_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Controller Capabilities Constructor
     * @param net The network that the attribute is associated with
     * @param id The controller id that the attribute is associated with
     */
    ctrlm_rf4ce_controller_capabilities_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * RF4CE Controller Capabilities Destructor
     */
    virtual ~ctrlm_rf4ce_controller_capabilities_t();


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
};
#endif