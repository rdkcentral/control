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
#ifndef __CTRLM_RF4CE_ATTR_BATTERY_H__
#define __CTRLM_RF4CE_ATTR_BATTERY_H__
#include "ctrlm_attr.h"
#include "ctrlm_db_attr.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"
#include "ctrlm_version.h"
#include <functional>

#define VOLTAGE_CALC(x) ((float)(x * 4.0 / 255))

#define CTRLM_RF4CE_RIB_ATTR_LEN_BATTERY_STATUS            (11)

class ctrlm_obj_controller_rf4ce_t;

// RF4CE Battery Percent Function Declaration
// INFO: had to change type to int, as using rf4ce_controller_type causes circular dependency..
//       this should be cleaned up when we refactor controllers in OO way.
uint8_t battery_level_percent(int type, ctrlm_hw_version_t hw_version, unsigned char voltage_unloaded);

// Battery Milestones are updated based on battery status updates
class ctrlm_rf4ce_battery_status_t;
typedef std::function<void(const ctrlm_rf4ce_battery_status_t&, const ctrlm_rf4ce_battery_status_t&, bool)> ctrlm_rf4ce_battery_status_listener_t;

/**
 * @brief ControlMgr RF4CE Controller Battery Status Class
 * 
 * This class contains the battery status reported from the remote
 */
class ctrlm_rf4ce_battery_status_t : public ctrlm_attr_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * ControlMgr RF4CE Controller Battery Status Constructor
     * @param net The controller's network in which this attribute belongs
     * @param controller The controller in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_battery_status_t(ctrlm_obj_network_t *net = NULL, ctrlm_obj_controller_rf4ce_t *controller = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Controller Battery Status Copy Constructor
     * @param status The status that should be copied to this status object
     */
    ctrlm_rf4ce_battery_status_t(const ctrlm_rf4ce_battery_status_t& status);
    /**
     * ControlMgr RF4CE Controller Battery Status Destructor
     */
    virtual ~ctrlm_rf4ce_battery_status_t();

public:
    /**
     * Getter function for the update time
     * @return The time the battery status was last updated
     */
    time_t   get_update_time() const;
    /**
     * Getter function for the flags
     * @return The flags
     */
    uint8_t  get_flags() const;
    /**
     * Function which provides a string describing the flags
     * @return The flags string
     */
    std::string get_flags_str() const;
    /**
     * Getter function for the loaded voltage
     * @return The loaded voltage
     */
    uint8_t  get_voltage_loaded() const;
    /**
     * Getter function for the unloaded voltage
     * @return The unloaded voltage
     */
    uint8_t  get_voltage_unloaded() const;
    /**
     * Getter function for the TX RF codes
     * @return The number of RF codes sent
     */
    uint32_t get_codes_txd_rf() const;
    /**
     * Getter function for the TX IR codes
     * @return The number of IR codes sent
     */
    uint32_t get_codes_txd_ir() const;
    /**
     * Getter function for the battery percent
     * @return The battery percent
     */
    uint8_t  get_percent() const;
    /**
     * Function to register a listener to update events for this attribute
     * @param listener Function pointer to the callback
     * @param user_data Data that the caller would like passed to the callback
     */
    void set_updated_listener(ctrlm_rf4ce_battery_status_listener_t listener);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

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
    time_t   update_time;
    uint8_t  flags;
    uint8_t  voltage_loaded;
    uint8_t  voltage_unloaded;
    uint32_t codes_txd_rf;
    uint32_t codes_txd_ir;
    uint8_t  percent;
    ctrlm_rf4ce_battery_status_listener_t listener;
    ctrlm_obj_controller_rf4ce_t *controller;
};

inline bool operator==(const ctrlm_rf4ce_battery_status_t& s1, const ctrlm_rf4ce_battery_status_t& s2) {
    return((s1.get_flags() == s2.get_flags())                       &&
           (s1.get_voltage_loaded() == s2.get_voltage_loaded())     &&
           (s1.get_voltage_unloaded() == s2.get_voltage_unloaded()) &&
           (s1.get_codes_txd_rf() == s2.get_codes_txd_rf())         &&
           (s1.get_codes_txd_ir() == s2.get_codes_txd_ir()));
}

inline bool operator!=(const ctrlm_rf4ce_battery_status_t& s1, const ctrlm_rf4ce_battery_status_t& s2) {
    return(!(s1 == s2));
}

/**
 * @brief ControlMgr RF4CE Controller Battery Large Jump Counter
 * 
 * This class is a simple wrapper for the large jump counter that gets written to the DB.
 * This was pushed out into it's own class, as it sometimes gets updated individually from 
 * the battery milestones.
 */
class ctrlm_rf4ce_battery_voltage_large_jump_counter_t : public ctrlm_db_attr_t {
public:
    /**
     * ControlMgr RF4CE Controller Battery Large Jump Counter Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_battery_voltage_large_jump_counter_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Controller Battery Large Jump Counter Destructor
     */
    virtual ~ctrlm_rf4ce_battery_voltage_large_jump_counter_t();

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
    uint8_t counter;
};

/**
 * @brief ControlMgr RF4CE Controller Battery Milestones Class
 * 
 * This class contains all of the battery milestones and logic associated 
 * with the milestones. 
 */
class ctrlm_rf4ce_battery_milestones_t : public ctrlm_attr_t, public ctrlm_db_attr_t {
public:
    /**
     * ControlMgr RF4CE Controller Battery Milestones Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_battery_milestones_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Controller Battery Milestones Destructor
     */
    virtual ~ctrlm_rf4ce_battery_milestones_t();

public:
    /**
     * Getter function for the timestamp when the battery was changed.
     * @return Battery changed timestamp
     */
    time_t   get_timestamp_battery_changed()   const;
    /**
     * Getter function for the timestamp when the battery was 75 percent.
     * @return Battery 75% timestamp
     */
    time_t   get_timestamp_battery_percent75() const;
    /**
     * Getter function for the timestamp when the battery was 50 percent.
     * @return Battery 50% timestamp
     */
    time_t   get_timestamp_battery_percent50() const;
    /**
     * Getter function for the timestamp when the battery was 25 percent.
     * @return Battery 25% timestamp
     */
    time_t   get_timestamp_battery_percent25() const;
    /**
     * Getter function for the timestamp when the battery was 5 percent.
     * @return Battery 5% timestamp
     */
    time_t   get_timestamp_battery_percent5()  const;
    /**
     * Getter function for the timestamp when the battery was 0 percent.
     * @return Battery 0% timestamp
     */
    time_t   get_timestamp_battery_percent0()  const;
    /**
     * Getter function for the last good battery timestamp.
     * @return Last good battery timestamp
     */
    time_t   get_timestamp_battery_last_good() const;

    /**
     * Getter function for the actual percent when the battery was changed.
     * @return Battery changed percent
     */
    uint8_t  get_actual_battery_changed()            const;
    /**
     * Getter function for the actual percent when the battery was 75 percent.
     * @return Battery 75% percent
     */
    uint8_t  get_actual_battery_percent75()          const;
    /**
     * Getter function for the actual percent when the battery was 50 percent.
     * @return Battery 50% percent
     */
    uint8_t  get_actual_battery_percent50()          const;
    /**
     * Getter function for the actual percent when the battery was 25 percent.
     * @return Battery 25% percent
     */
    uint8_t  get_actual_battery_percent25()          const;
    /**
     * Getter function for the actual percent when the battery was 5 percent.
     * @return Battery 5% percent
     */
    uint8_t  get_actual_battery_percent5()           const;
    /**
     * Getter function for the actual percent when the battery was 0 percent.
     * @return Battery 0% percent
     */
    uint8_t  get_actual_battery_percent0()           const;
    /**
     * Getter function for the last good actual percent.
     * @return Battery percent last good
     */
    uint8_t  get_actual_battery_percent_last_good()  const;

    /**
     * Getter function for the unloaded voltage when the battery was changed.
     * @return Battery changed unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_changed()   const;
    /**
     * Getter function for the unloaded voltage when the battery was 75 percent.
     * @return Battery 75% unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_percent75() const;
    /**
     * Getter function for the unloaded voltage when the battery was 50 percent.
     * @return Battery 50% unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_percent50() const;
    /**
     * Getter function for the unloaded voltage when the battery was 25 percent.
     * @return Battery 25% unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_percent25() const;
    /**
     * Getter function for the unloaded voltage when the battery was 5 percent.
     * @return Battery 5% unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_percent5()  const;
    /**
     * Getter function for the unloaded voltage when the battery was 0 percent.
     * @return Battery 0% unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_percent0()  const;
    /**
     * Getter function for the last good unloaded voltage.
     * @return Last good unloaded voltage
     */
    uint8_t get_unloaded_voltage_battery_last_good() const;
    /**
     * Getter function for the last good loaded voltage.
     * @return Last good loaded voltage
     */
    uint8_t get_loaded_voltage_battery_last_good()   const;

    /**
     * Getter function for the large jump counter.
     * @return Large jump counter
     */
    uint8_t get_battery_voltage_large_jump_counter()     const;
    /**
     * Getter function for the large decline detect
     * @return Large decline detected
     */
    uint8_t get_battery_voltage_large_decline_detected() const;

public:
    /**
     * Function used to update last good fields with battery status
     * @param status The battery status
     */
    void update_last_good(const ctrlm_rf4ce_battery_status_t& status);
    /**
     * Callback used when the battery status is updated
     * @param status_old The old battery status before the update
     * @param status_new The new battery status after the update
     */
    void battery_status_updated(const ctrlm_rf4ce_battery_status_t& status_old, const ctrlm_rf4ce_battery_status_t& status_new, bool importing);
    /**
     * Helper function to determine if battery was changed
     * @param new_unloaded_voltage The unloaded voltage now being reported by the remote
     * @param old_unloaded_voltage The previous voltage reported by the remote
     * @return True if the batteries have been changed, else False
     */
    static bool is_batteries_changed(uint8_t new_unloaded_voltage, uint8_t old_unloaded_voltage);
    /**
     * Helper function to determine if large voltage jump occurred
     * @param new_unloaded_voltage The unloaded voltage now being reported by the remote
     * @param old_unloaded_voltage The previous voltage reported by the remote
     * @return True if large voltage jump, else False
     */
    static bool is_batteries_large_voltage_jump(uint8_t new_unloaded_voltage, uint8_t old_unloaded_voltage);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @see ctrlm_attr_t::to_string()
     */
    std::string to_string() const;

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

private:
    ctrlm_network_id_t    network_id;
    ctrlm_controller_id_t controller_id;

    // Original Milestones
    time_t    battery_changed_timestamp;
    time_t    battery_75_percent_timestamp;
    time_t    battery_50_percent_timestamp;
    time_t    battery_25_percent_timestamp;
    time_t    battery_5_percent_timestamp;
    time_t    battery_0_percent_timestamp;
    uint8_t   battery_changed_actual_percent;
    uint8_t   battery_75_percent_actual_percent;
    uint8_t   battery_50_percent_actual_percent;
    uint8_t   battery_25_percent_actual_percent;
    uint8_t   battery_5_percent_actual_percent;
    uint8_t   battery_0_percent_actual_percent;

    // Updated Milestones
    time_t    battery_last_good_timestamp;
    uint8_t   battery_last_good_percent;
    uint8_t   battery_last_good_loaded_voltage;
    uint8_t   battery_last_good_unloaded_voltage;
    uint8_t   battery_changed_unloaded_voltage;
    uint8_t   battery_75_percent_unloaded_voltage;
    uint8_t   battery_50_percent_unloaded_voltage;
    uint8_t   battery_25_percent_unloaded_voltage;
    uint8_t   battery_5_percent_unloaded_voltage;
    uint8_t   battery_0_percent_unloaded_voltage;
    uint8_t   battery_voltage_large_decline_detected;
    bool      battery_first_write;
    std::shared_ptr<ctrlm_rf4ce_battery_voltage_large_jump_counter_t> battery_voltage_large_jump_counter;
};

#endif
