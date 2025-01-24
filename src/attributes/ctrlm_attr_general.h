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
#ifndef __CTRLM_ATTR_GENERAL_H__
#define __CTRLM_ATTR_GENERAL_H__
#include "ctrlm_attr.h"
#include "ctrlm_db_attr.h"
#include <stdint.h>


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr String Attribute Class
 * 
 * This is the class that contains a generic attribute stored as a string.
 */
class ctrlm_string_attr_t : public ctrlm_attr_t {
public:
    /**
     * Constructor
     * @param value The value stored as a string for this attribute
     */
    ctrlm_string_attr_t(const std::string &_name, const std::string &_value);
    /**
     * Destructor
     */
    virtual ~ctrlm_string_attr_t();

public:
    /**
     * Function for setting the value of the attribute
     * @param _value The value of the attribute
     */
    void set_value(const std::string &_value);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

protected:
    std::string value;
};

//-----------------------------------------------------------------------------------------

/**
 * @brief ControlMgr String Database Attribute Class
 * 
 * This is the class that contains a generic attribute stored as a string that needs to be stored in the database
 */
class ctrlm_string_db_attr_t : public ctrlm_string_attr_t, public ctrlm_db_attr_t {
public:
    /**
     * Constructor for a global attribute
     * @param value The value stored as a string for this attribute
     */
    ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, const std::string &db_table, const std::string &db_key);
    /**
     * Constructor for a global attribute associated with a network
     * @param value The value stored as a string for this attribute
     */
    ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, ctrlm_obj_network_t *net, const std::string &db_key);
    /**
     * Constructor for controller attribute
     * @param value The value stored as a string for this attribute
     */
    ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key = "");
    /**
     * Destructor
     */
    virtual ~ctrlm_string_db_attr_t();

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
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr Unsigned 64 Bit Integer Attribute Class
 * 
 * This is the class that contains a generic attribute stored as a uint64_t.
 */
class ctrlm_uint64_attr_t : public ctrlm_attr_t {
public:
    /**
     * Constructor
     * @param value The value stored as a uint64_t for this attribute
     */
    ctrlm_uint64_attr_t(uint64_t _value = 0, std::string _name = "");
    /**
     * Destructor
     */
    virtual ~ctrlm_uint64_attr_t();

public:
    /**
     * Function for setting the value of the attribute
     * @param _value The value of the attribute
     */
    void set_value(uint64_t _value);

    /**
     * Function for getting the value of the attribute
     * @param _value The value of the attribute
     */
    uint64_t get_value() const;

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

protected:
    uint64_t value;
};


/**
 * Overloaded Operators
 */
inline bool operator== (const ctrlm_uint64_attr_t& v1, const ctrlm_uint64_attr_t& v2) {
    return(v1.get_value() == v2.get_value());
}
inline bool operator== (const ctrlm_uint64_attr_t& v1, const uint64_t& v2) {
    return(v1 == ctrlm_uint64_attr_t(v2));
}
inline bool operator== (const uint64_t& v2, const ctrlm_uint64_attr_t& v1) {
    return(v1 == v2);
}
inline bool operator!= (const ctrlm_uint64_attr_t& v1, const uint64_t& v2) {
    return(!(v1 == v2));
}
inline bool operator!= (const uint64_t& v1, const ctrlm_uint64_attr_t& v2) {
    return(!(v1 == v2));
}


//-----------------------------------------------------------------------------------------

/**
 * @brief ControlMgr Unsigned 64 Bit Integer Database Attribute Class
 * 
 * This is the class that contains a generic attribute stored as a uint64_t that needs to be stored in the database
 */
class ctrlm_uint64_db_attr_t : public ctrlm_uint64_attr_t, public ctrlm_db_attr_t {
public:
    /**
     * Constructor for a global attribute
     * @param value The value stored as a uint64_t for this attribute
     */
    ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value, const std::string &db_table, const std::string &db_key);
    /**
     * Constructor for a global attribute associated with a network
     * @param value The value stored as a uint64_t for this attribute
     */
    ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value, ctrlm_obj_network_t *net, const std::string &db_key);

    /**
     * Constructor for a controller attribute
     * @param value The value stored as a uint64_t for this attribute
     */
    ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value = 0, ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF, const std::string &db_key = "");
    /**
     * Destructor
     */
    virtual ~ctrlm_uint64_db_attr_t();

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
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr IEEE Address Attribute Class
 * 
 * This class is used to handle the IEEE address attribute for ControlMgr.
 */
class ctrlm_ieee_addr_t : public ctrlm_uint64_attr_t {
public:
    /**
     * Constructor (uint64_t)
     * @param ieee A uint64_t containing an IEEE address
     */
    ctrlm_ieee_addr_t(uint64_t ieee = 0);
    /**
     * Destructor
     */
    virtual ~ctrlm_ieee_addr_t();

public:
    /**
     * Function to get the IEEE address string
     * @param colons Set to true if the ieee address string should include colons, otherwise False
     * @return The IEEE address string
     */
    std::string to_string(bool colons) const;
    
    /**
     * Function to set the number of bytes in the IEEE address.
     * Most devices use 6 byte MAC, but RF4CE uses 8.  Setting this
     * appropriately will make the to_string() function work best.
     * @param _num_bytes uint8_t containing the number of bytes in ieee address
     */
    void set_num_bytes(uint8_t _num_bytes);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

protected:
    uint8_t  num_bytes;
};

//-----------------------------------------------------------------------------------------

/**
 * @brief ControlMgr IEEE Address Database Attribute Class
 * 
 * This class is used to handle the IEEE address attribute for ControlMgr that needs to be stored in the database
 */
class ctrlm_ieee_db_addr_t : public ctrlm_ieee_addr_t, public ctrlm_db_attr_t {
public:
    /**
     * Constructor for a global ieee attribute associated with a network
     * @param ieee A uint64_t containing an IEEE address
     */
    ctrlm_ieee_db_addr_t(uint64_t ieee, ctrlm_obj_network_t *net, const std::string &db_key);
    /**
     * Constructor for a controller ieee attribute
     * @param ieee A uint64_t containing an IEEE address
     */
    ctrlm_ieee_db_addr_t(uint64_t ieee = 0, ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF, const std::string &db_key = "ieee_address");
    /**
     * Destructor
     */
    virtual ~ctrlm_ieee_db_addr_t();

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
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr Audio Profiles Class
 * 
 * This class contains the audio profiles supported by a device.
 */
class ctrlm_audio_profiles_t : public ctrlm_attr_t {
public:
    /**
     * Enum containing the available profiles 
     */
    enum profile{
        NONE                = 0x00,
        ADPCM_16BIT_16KHZ   = 0x01,
        PCM_16BIT_16KHZ     = 0x02,
        OPUS_16BIT_16KHZ    = 0x04
    };

public:
    /**
     * ControlMgr Audio Profiles Constructor
     * @param profiles The profiles supported by the device (bitfield)
     */
    ctrlm_audio_profiles_t(int profiles = NONE);
    /**
     * ControlMgr Audio Profiles Destructor
     */
    ~ctrlm_audio_profiles_t();

public:
    /**
     * Function to get the profiles
     * @return An integer with each profile bit set for the supported profiles
     */
    int get_profiles() const;
    /**
     * Function to set the profiles
     * @param profiles The profile bitfield to set the supported profiles
     */
    void set_profiles(int profiles);
    /**
     * Function to check if a specific profile is supported
     * @param p The profile in question
     * @return True if the profile is supported, otherwise False
     */
    bool supports(profile p) const;
    /**
     * Static function to get a string for a specific profile
     */
    static std::string profile_str(profile p);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

protected:
    int supported_profiles;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief ControlMgr Controller Capabilities Class
 * 
 * This class maintains the capabilties for a controller.
 */
class ctrlm_controller_capabilities_t : public ctrlm_attr_t {
public:
    /**
     * Enum of the different capabilities
     */
    enum capability{
        FMR      = 0,
        PAR,
        HAPTICS,
        INVALID
    };

public:
    /**
     * ControlMgr Controller Capabilities Constructor
     */
    ctrlm_controller_capabilities_t();
    /**
     * ControlMgr Controller Capabilities Copy Constructor
     */
    ctrlm_controller_capabilities_t(const ctrlm_controller_capabilities_t& cap);
    /**
     * ControlMgr Controller Capabilities Destructor
     */
    virtual ~ctrlm_controller_capabilities_t();
    // operators
    bool operator==(const ctrlm_controller_capabilities_t& cap);
    bool operator!=(const ctrlm_controller_capabilities_t& cap);
    // end operators

public:
    /**
     * Function to add a capability to the controller
     * @param c The capability
     */
    void add_capability(capability c);
    /**
     * Function to remove a capability to the controller
     * @param c The capability
     */
    void remove_capability(capability c);
    /**
     * Function to check if a controller support a capability
     * @param c The capability
     */
    bool has_capability(capability c) const;
    /**
     * Static function to get the string for a capability
     * @param c The capability
     */
    static std::string capability_str(capability c);
public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

private:
    bool capabilities[capability::INVALID];
};

#endif
