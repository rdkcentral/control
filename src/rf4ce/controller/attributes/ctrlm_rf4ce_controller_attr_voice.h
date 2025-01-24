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
#ifndef __CTRLM_RF4CE_ATTR_VOICE_H__
#define __CTRLM_RF4CE_ATTR_VOICE_H__
#include "ctrlm_attr_general.h"
#include "ctrlm_db_attr.h"
#include "ctrlm_config_attr.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"
#include "ctrlm_version.h"
#include "ctrlm_rfc.h"
#include <vector>
#include <functional>

/**
 * @brief ControlMgr RF4CE Controller Audio Profiles Class
 * 
 * This class implements the audio profiles for the RF4CE products
 */
class ctrlm_rf4ce_controller_audio_profiles_t : public ctrlm_audio_profiles_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Audio Profiles Constructor
     * @param net The network that the attribute is associated with
     * @param id The controller id that the attribute is associated with
     */
    ctrlm_rf4ce_controller_audio_profiles_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * RF4CE Audio Profiles Destructor
     */
    virtual ~ctrlm_rf4ce_controller_audio_profiles_t();

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

};

/**
 * @brief ControlMgr RF4CE Voice Statistics
 * 
 * The class that implements the RF4CE Voice Statistics RIB attribute
 */
class ctrlm_rf4ce_voice_statistics_t : public ctrlm_attr_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Voice Statistics Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_voice_statistics_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * RF4CE Voice Statistics Destructor
     */
    virtual ~ctrlm_rf4ce_voice_statistics_t();

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the voice statistics info
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

protected:
    uint32_t voice_sessions;
    uint32_t tx_time;
};

/**
 * @brief ControlMgr RF4CE Voice Session Statistics
 * 
 * This class handles when the voice session stats are written to the RIB
 */
class ctrlm_rf4ce_voice_session_statistics_t : public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * RF4CE Voice Session Statistics Constructor
     * @param network_id The network id the stats are associated with
     * @param controller_id The controller id the stats are associated with
     */
    ctrlm_rf4ce_voice_session_statistics_t(ctrlm_network_id_t network_id = CTRLM_MAIN_NETWORK_ID_INVALID, ctrlm_controller_id_t controller_id = CTRLM_MAIN_CONTROLLER_ID_INVALID);
    /**
     * RF4CE Voice Session Statistics Destructor
     */
    virtual ~ctrlm_rf4ce_voice_session_statistics_t();

public:
    /**
     * Interface implementation for a RIB write, DO NOT NEED RIB READ FOR THIS ATTRIBUTE
     * @see ctrlm_rf4ce_rib_attr_t::write_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing);

private:
    ctrlm_network_id_t network_id;
    ctrlm_controller_id_t controller_id;
};

class ctrlm_obj_controller_rf4ce_t;
/**
 * @brief ControlMgr RF4CE Voice Command Status
 * 
 * This class contains the implementation for the RF4CE voice command status attribute.
 */
class ctrlm_rf4ce_voice_command_status_t : public ctrlm_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * Enum of the status values
     */
    enum status {
        PENDING            = 0x00,
        TIMEOUT            = 0x01,
        OFFLINE            = 0x02,
        SUCCESS            = 0x03,
        FAILURE            = 0x04,
        NO_COMMAND_SENT    = 0x05,
        TV_AVR_COMMAND     = 0x06,
        MICROPHONE_COMMAND = 0x07,
        AUDIO_COMMAND      = 0x08
    };

    /**
     * Enum of the Flags for TV/AVR commands
     */
    enum tv_avr_command_flag {
        TOGGLE_FALLBACK = 0x01
    };

    /**
     * Enum of the TV/AVR commands
     */
    enum tv_avr_command {
        POWER_OFF    = 0x00,
        POWER_ON     = 0x01,
        VOLUME_UP    = 0x02,
        VOLUME_DOWN  = 0x03,
        VOLUME_MUTE  = 0x04,
        POWER_TOGGLE = 0x05
    };

public:
    /**
     * RF4CE Voice Command Status Constructor
     * @param controller The controller this attribute is associated with.
     * @todo Extend this class once we do the controller refactor
     */
    ctrlm_rf4ce_voice_command_status_t(ctrlm_obj_controller_rf4ce_t *controller = NULL);
    /**
     * RF4CE Voice Command Status Destructor
     */
    virtual ~ctrlm_rf4ce_voice_command_status_t();

public:
    /**
     * Function to reset the voice command status
     */
    void reset();
    /**
     * Static function to get the status string
     * @return The status string
     */
    static std::string status_str(status s);
    /**
     * Static function to get the TV/AVR flag string
     * @return The TV/AVR flag string
     */
    static std::string tv_avr_command_flag_str(tv_avr_command_flag f);
    /**
     * Static function to get the TV/AVR command string
     * @return The TV/AVR command string
     */
    static std::string tv_avr_command_str(tv_avr_command c);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the voice command status info
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

protected:
    ctrlm_obj_controller_rf4ce_t *controller;
    status vcs;
    uint8_t flags;
    tv_avr_command tac;
    uint8_t ir_repeats;
};

class ctrlm_rf4ce_voice_command_length_t;
typedef std::function<void(const ctrlm_rf4ce_voice_command_length_t&)> ctrlm_rf4ce_voice_command_length_listener_t;
/**
 * @brief ControlMgr RF4CE Voice Command Length Class
 * 
 * The RF4CE Voice Command Length RIB attribute
 */
class ctrlm_rf4ce_voice_command_length_t : public ctrlm_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * Enum for the length values
     */
    enum length {
        CONTROLLER_DEFAULT  = 0,
        PROFILE_NEGOTIATION = 1,
        VALUE               = 182
    };

public:
    /**
     * RF4CE Voice Command Length Constructor
     */
    ctrlm_rf4ce_voice_command_length_t();
    /**
     * RF4CE Voice Command Length Destructor
     */
    virtual ~ctrlm_rf4ce_voice_command_length_t();

public:
    /**
     * Function to set the listener for when this attribute is updated
     * @param listener The listener for the updated event.
     */
    void set_updated_listener(ctrlm_rf4ce_voice_command_length_listener_t listener);
    static std::string length_str(length l);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the voice command length info
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
    length vcl;
    ctrlm_rf4ce_voice_command_length_listener_t updated_listener;
};

#endif