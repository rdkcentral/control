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
#include "ctrlm_rf4ce_controller_attr_battery.h"
#include "ctrlm_database.h"
#include "ctrlm_db_types.h"
#include "ctrlm_network.h"
#include "rf4ce/ctrlm_rf4ce_controller.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include <iostream>
#include <sstream>

// ctrlm_rf4ce_battery_status_t
#define RF4CE_BATTERY_STATUS_VOLTAGE_LOADED_DEFAULT        (3000*255/4000) //3000 millivolts
#define RF4CE_BATTERY_STATUS_VOLTAGE_UNLOADED_DEFAULT      (3000*255/4000) //3000 millivolts
#define RF4CE_BATTERY_STATUS_VOLTAGE_PERCENTAGE_DEFAULT    (100)

#define BATTERY_STATUS_FLAGS_REPLACEMENT    (0x01)
#define BATTERY_STATUS_FLAGS_CHARGING       (0x02)
#define BATTERY_STATUS_FLAGS_IMPENDING_DOOM (0x04)

#define BATTERY_STATUS_ID     (0x03)
#define BATTERY_STATUS_INDEX  (0x00)
#define BATTERY_STATUS_LEN    (11)
ctrlm_rf4ce_battery_status_t::ctrlm_rf4ce_battery_status_t(ctrlm_obj_network_t *net, ctrlm_obj_controller_rf4ce_t *controller, ctrlm_controller_id_t id) :
ctrlm_attr_t("Battery Status"),
ctrlm_db_attr_t(net, id),
ctrlm_rf4ce_rib_attr_t(BATTERY_STATUS_ID, BATTERY_STATUS_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER) {
    this->update_time      = 0;
    this->flags            = 0;
    this->voltage_loaded   = RF4CE_BATTERY_STATUS_VOLTAGE_LOADED_DEFAULT;
    this->voltage_unloaded = RF4CE_BATTERY_STATUS_VOLTAGE_UNLOADED_DEFAULT;
    this->codes_txd_rf     = 0;
    this->codes_txd_ir     = 0;
    this->percent          = RF4CE_BATTERY_STATUS_VOLTAGE_PERCENTAGE_DEFAULT;
    this->controller       = controller;
}

ctrlm_rf4ce_battery_status_t::ctrlm_rf4ce_battery_status_t(const ctrlm_rf4ce_battery_status_t& status) {
    this->update_time      = status.update_time;
    this->flags            = status.flags;
    this->voltage_loaded   = status.voltage_loaded;
    this->voltage_unloaded = status.voltage_unloaded;
    this->codes_txd_rf     = status.codes_txd_rf;
    this->codes_txd_ir     = status.codes_txd_ir;
    this->percent          = status.percent;
    this->controller       = status.controller;
}

ctrlm_rf4ce_battery_status_t::~ctrlm_rf4ce_battery_status_t() {

}

time_t ctrlm_rf4ce_battery_status_t::get_update_time() const {
    return(this->update_time);
}

uint8_t ctrlm_rf4ce_battery_status_t::get_flags() const {
    return(this->flags);
}

std::string ctrlm_rf4ce_battery_status_t::get_flags_str() const {
    std::stringstream ss;
    std::ios_base::fmtflags f(ss.flags());
    //Flags 0x%02X Voltage (old:%4.2fv, new:%4.2fv) Battery Replacement <%u>, Battery Charging <%u>, Impending Doom <%u>
    ss << "Flags 0x" << COUT_HEX_MODIFIER << std::uppercase << (int)this->flags << " "; ss.flags(f);
    ss << "Battery Replacement <" << (this->flags & BATTERY_STATUS_FLAGS_REPLACEMENT) << ">, ";
    ss << "Battery Charging <" << (this->flags & BATTERY_STATUS_FLAGS_CHARGING) << ">, ";
    ss << "Impending Doom <" << (this->flags & BATTERY_STATUS_FLAGS_IMPENDING_DOOM) << ">";
    return(ss.str());
}

uint8_t ctrlm_rf4ce_battery_status_t::get_voltage_loaded() const {
    return(this->voltage_loaded);
}

uint8_t ctrlm_rf4ce_battery_status_t::get_voltage_unloaded() const {
    return(this->voltage_unloaded);
}

uint32_t ctrlm_rf4ce_battery_status_t::get_codes_txd_rf() const {
    return(this->codes_txd_rf);
}

uint32_t ctrlm_rf4ce_battery_status_t::get_codes_txd_ir() const {
    return(this->codes_txd_ir);
}

uint8_t ctrlm_rf4ce_battery_status_t::get_percent() const {
    return(this->percent);
}

void ctrlm_rf4ce_battery_status_t::set_updated_listener(ctrlm_rf4ce_battery_status_listener_t listener) {
    this->listener  = listener;
}

bool ctrlm_rf4ce_battery_status_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= BATTERY_STATUS_LEN) {
        data[0]  = this->flags;
        data[1]  = this->voltage_loaded;
        data[2]  = (uint8_t)(this->codes_txd_rf);
        data[3]  = (uint8_t)(this->codes_txd_rf >> 8);
        data[4]  = (uint8_t)(this->codes_txd_rf >> 16);
        data[5]  = (uint8_t)(this->codes_txd_rf >> 24);
        data[6]  = (uint8_t)(this->codes_txd_ir);
        data[7]  = (uint8_t)(this->codes_txd_ir >> 8);
        data[8]  = (uint8_t)(this->codes_txd_ir >> 16);
        data[9]  = (uint8_t)(this->codes_txd_ir >> 24);
        data[10] = this->voltage_unloaded;
        ret = true;
    }
    return(ret);
}

std::string ctrlm_rf4ce_battery_status_t::to_string() const {
    std::stringstream ss;
    std::ios_base::fmtflags f(ss.flags());
    ss << "Flags 0x" << COUT_HEX_MODIFIER << std::uppercase << (int)this->flags << " "; ss.flags(f);
    ss << "Voltage (" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->voltage_loaded) << "v, " << VOLTAGE_CALC(this->voltage_unloaded) << "v) "; ss.flags(f);
    ss << "Codes Txd (RF, IR) (" << this->codes_txd_rf << ", " << this->codes_txd_ir << ")";
    return(ss.str());
}

bool ctrlm_rf4ce_battery_status_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t status("battery_status", this->get_table());
    ctrlm_db_uint64_t status_time("time_battery_status", this->get_table());
    char buf[BATTERY_STATUS_LEN];

    memset(buf, 0, sizeof(buf));
    if(status.read_db(ctx)) {
        size_t len = status.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len >= 11) {
                this->flags             = buf[0];
                this->voltage_loaded    = buf[1];
                this->codes_txd_rf      = ((buf[5] << 24) | (buf[4] << 16) | (buf[3] << 8) | buf[2]);
                this->codes_txd_ir      = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]);
                this->voltage_unloaded  = buf[10];
                ret = true;
            } else {
                XLOGD_ERROR("data from db is too small <%s - status>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to copy data to buffer <%s - status>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s - status>", this->get_name().c_str());
    }
    if(status_time.read_db(ctx)) {
        this->update_time = status_time.get_uint64();
    } else {
        XLOGD_ERROR("failed to read from db <%s - time>", this->get_name().c_str());
        ret = false;
    }
    if(ret == true) {
        XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_battery_status_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t status("battery_status", this->get_table());
    ctrlm_db_uint64_t status_time("time_battery_status", this->get_table());
    char buf[BATTERY_STATUS_LEN];

    memset(buf, 0, sizeof(buf));
    this->to_buffer(buf, sizeof(buf));
    if(status.from_buffer(buf, sizeof(buf))) {
        if(status.write_db(ctx)) {
            ret = true;
        } else {
            XLOGD_ERROR("failed to write to db <%s - status>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s - status>", this->get_name().c_str());
    }
    status_time.set_uint64(this->update_time);
    if(!status_time.write_db(ctx)) {
        XLOGD_ERROR("failed to write to db <%s - time>", this->get_name().c_str());
        ret = false;
    }
    if(ret == true) {
        XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_battery_status_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = BATTERY_STATUS_LEN;
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

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_battery_status_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == BATTERY_STATUS_LEN) {
            ctrlm_rf4ce_battery_status_t temp(*this);
            this->flags             = data[0];
            this->voltage_loaded    = data[1];
            this->codes_txd_rf      = ((data[5] << 24) | (data[4] << 16) | (data[3] << 8) | data[2]);
            this->codes_txd_ir      = ((data[9] << 24) | (data[8] << 16) | (data[7] << 8) | data[6]);
            this->voltage_unloaded  = data[10];
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(this->flags) {
                XLOGD_WARN("%s", this->get_flags_str().c_str());
            }
            if(*this != temp) {
                // Update percent
                ctrlm_rf4ce_controller_type_t type = RF4CE_CONTROLLER_TYPE_INVALID;
                ctrlm_hw_version_t hw_version;
                if(this->controller) {
                    hw_version = this->controller->version_hardware_get();
                    type       = this->controller->controller_type_get();
                }
                this->update_time = time(NULL); // Update time
                this->percent     = battery_level_percent(type, hw_version, this->voltage_unloaded);
                if(importing == false) {
                    ctrlm_db_attr_write(shared_from_this());
                }
                if(this->listener) {
                    XLOGD_INFO("Battery Status updated.. alerting milestones object");
                    this->listener(temp, *this, importing);
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

void ctrlm_rf4ce_battery_status_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[BATTERY_STATUS_LEN];
    if(ctrlm_rf4ce_rib_attr_t::status::SUCCESS == this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}
// end ctrlm_rf4ce_battery_status_t

// ctrlm_rf4ce_battery_voltage_large_jump_counter_t 
ctrlm_rf4ce_battery_voltage_large_jump_counter_t::ctrlm_rf4ce_battery_voltage_large_jump_counter_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) : ctrlm_db_attr_t(net, id) {
    this->counter = 0;
}

ctrlm_rf4ce_battery_voltage_large_jump_counter_t::~ctrlm_rf4ce_battery_voltage_large_jump_counter_t() {

}

bool ctrlm_rf4ce_battery_voltage_large_jump_counter_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data("battery_voltage_large_jump_counter", this->get_table());
    if(data.read_db(ctx)) {
        this->counter = data.get_uint64();
        ret = true;
    } else {
        XLOGD_ERROR("failed to write to db <battery_voltage_large_jump_counter>");
    }
    return(ret);
}

bool ctrlm_rf4ce_battery_voltage_large_jump_counter_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data("battery_voltage_large_jump_counter", this->get_table(), this->counter);
    if(data.write_db(ctx)) {
        ret = true;
    } else {
        XLOGD_ERROR("failed to read from db <battery_voltage_large_jump_counter>");
    }
    return(ret);
}
// end ctrlm_rf4ce_battery_voltage_large_jump_counter_t

// ctrlm_rf4ce_battery_milestones_t
#define CTRLM_BATTERY_75_PERCENT_THRESHOLD 75
#define CTRLM_BATTERY_50_PERCENT_THRESHOLD 50
#define CTRLM_BATTERY_25_PERCENT_THRESHOLD 25
#define CTRLM_BATTERY_5_PERCENT_THRESHOLD  5
#define CTRLM_BATTERY_0_PERCENT_THRESHOLD  0


ctrlm_rf4ce_battery_milestones_t::ctrlm_rf4ce_battery_milestones_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_attr_t("Battery Milestones"),
ctrlm_db_attr_t(net, id),
battery_voltage_large_jump_counter(std::make_shared<ctrlm_rf4ce_battery_voltage_large_jump_counter_t>(net, id))
{
    if(net) {
        this->network_id = net->network_id_get();
    } else {
        this->network_id = CTRLM_MAIN_NETWORK_ID_INVALID;
    }
    this->controller_id  = id;

    this->battery_changed_timestamp                = 0;
    this->battery_75_percent_timestamp             = 0;
    this->battery_50_percent_timestamp             = 0;
    this->battery_25_percent_timestamp             = 0;
    this->battery_5_percent_timestamp              = 0;
    this->battery_0_percent_timestamp              = 0;
    this->battery_changed_actual_percent           = 0;
    this->battery_75_percent_actual_percent        = 0;
    this->battery_50_percent_actual_percent        = 0;
    this->battery_25_percent_actual_percent        = 0;
    this->battery_5_percent_actual_percent         = 0;
    this->battery_0_percent_actual_percent         = 0;
    this->battery_last_good_timestamp              = 0;
    this->battery_last_good_percent                = 0;
    this->battery_last_good_loaded_voltage         = 0;
    this->battery_last_good_unloaded_voltage       = 0;
    this->battery_changed_unloaded_voltage         = 0;
    this->battery_75_percent_unloaded_voltage      = 0;
    this->battery_50_percent_unloaded_voltage      = 0;
    this->battery_25_percent_unloaded_voltage      = 0;
    this->battery_5_percent_unloaded_voltage       = 0;
    this->battery_0_percent_unloaded_voltage       = 0;
    this->battery_voltage_large_decline_detected   = 0;
    this->battery_first_write                      = true;
}

ctrlm_rf4ce_battery_milestones_t::~ctrlm_rf4ce_battery_milestones_t() {

}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_changed()   const {
    return(this->battery_changed_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_percent75() const {
    return(this->battery_75_percent_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_percent50() const {
    return(this->battery_50_percent_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_percent25() const {
    return(this->battery_25_percent_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_percent5()  const {
    return(this->battery_5_percent_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_percent0()  const {
    return(this->battery_0_percent_timestamp);
}

time_t ctrlm_rf4ce_battery_milestones_t::get_timestamp_battery_last_good() const {
    return(this->battery_last_good_timestamp);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_changed()   const {
    return(this->battery_changed_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent75() const {
    return(this->battery_75_percent_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent50() const {
    return(this->battery_50_percent_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent25() const {
    return(this->battery_25_percent_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent5()  const {
    return(this->battery_5_percent_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent0()  const {
    return(this->battery_0_percent_actual_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_actual_battery_percent_last_good()  const {
    return(this->battery_last_good_percent);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_changed() const {
    return(this->battery_changed_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_percent75() const {
    return(this->battery_75_percent_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_percent50() const {
    return(this->battery_50_percent_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_percent25() const {
    return(this->battery_25_percent_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_percent5()  const {
    return(this->battery_5_percent_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_percent0()  const {
    return(this->battery_0_percent_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_unloaded_voltage_battery_last_good() const {
    return(this->battery_last_good_unloaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_loaded_voltage_battery_last_good()   const {
    return(this->battery_last_good_loaded_voltage);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_battery_voltage_large_jump_counter() const {
    return(this->battery_voltage_large_jump_counter->counter);
}

uint8_t ctrlm_rf4ce_battery_milestones_t::get_battery_voltage_large_decline_detected() const {
    return(this->battery_voltage_large_decline_detected);
}

void ctrlm_rf4ce_battery_milestones_t::update_last_good(const ctrlm_rf4ce_battery_status_t& status) {
    //battery_last_good... must be set to the last battery_status_... in case they are not currently in the database.
   this->battery_last_good_timestamp         = status.get_update_time();
   this->battery_last_good_percent           = status.get_percent();
   this->battery_last_good_loaded_voltage    = status.get_voltage_loaded();
   this->battery_last_good_unloaded_voltage  = status.get_voltage_unloaded();
   //If the battery_last_good_timestamp_ is 0 then we have not written battery info yet
   if(this->battery_last_good_timestamp != 0) {
      this->battery_first_write = false;
   }
}

void ctrlm_rf4ce_battery_milestones_t::battery_status_updated(const ctrlm_rf4ce_battery_status_t& status_old, const ctrlm_rf4ce_battery_status_t& status_new, bool importing) {
    bool update_milestones = true;
    bool batteries_changed = false;
    //If this is the first battery status write, this could be a newly paired remote so treat this as batteries changed
    if(this->battery_first_write) {
        XLOGD_WARN("This is the first battery status write so this could be a newly paired remote.  Voltage (%4.2fv)", VOLTAGE_CALC(status_new.get_voltage_unloaded()));
        batteries_changed = true;
        this->battery_first_write = false;
    } else if((status_new.get_voltage_unloaded() > this->battery_last_good_unloaded_voltage) || (status_new.get_voltage_loaded() > this->battery_last_good_loaded_voltage)) {
        if(is_batteries_changed(status_new.get_voltage_unloaded(), status_old.get_voltage_unloaded())) {
            XLOGD_WARN("It appears the batteries have been changed. Voltage (old:%4.2fv, new:%4.2fv)", VOLTAGE_CALC(status_old.get_voltage_unloaded()), VOLTAGE_CALC(status_new.get_voltage_unloaded()));
            batteries_changed = true;
        } else if(is_batteries_large_voltage_jump(status_new.get_voltage_unloaded(), status_old.get_voltage_unloaded())) {
            update_milestones = false;
            this->battery_voltage_large_jump_counter->counter++;
            XLOGD_WARN("The battery voltage had a large increase but we don't consider the batteries to have changed. battery_voltage_large_jump_counter <%u>  Milestones not updated.  Voltage (old:%4.2fv, new:%4.2fv)", this->battery_voltage_large_jump_counter->counter, VOLTAGE_CALC(status_old.get_voltage_unloaded()), VOLTAGE_CALC(status_new.get_voltage_unloaded()));
            if(false == importing) {
                ctrlm_db_attr_write(this->battery_voltage_large_jump_counter);
            }
        } else if(((status_new.get_voltage_unloaded() < status_old.get_voltage_unloaded()) && (status_old.get_voltage_unloaded() > this->battery_last_good_unloaded_voltage)) ||
                ((status_new.get_voltage_loaded() < status_old.get_voltage_loaded()) && (status_old.get_voltage_loaded() > this->battery_last_good_loaded_voltage))) {
            XLOGD_WARN("The battery voltage went up previously, has now gone down, but not to it's lowest point.  Milestones not updated.");
            update_milestones = false;
        } else {
            XLOGD_WARN("The battery voltage has gone up but we don't consider the batteries to have changed.  Milestones not updated.  Unloaded Voltage (old:%4.2fv, new:%4.2fv)  Loaded Voltage (old:%4.2fv, new:%4.2fv)", VOLTAGE_CALC(this->battery_last_good_unloaded_voltage), VOLTAGE_CALC(status_new.get_voltage_unloaded()), VOLTAGE_CALC(this->battery_last_good_loaded_voltage), VOLTAGE_CALC(status_new.get_voltage_loaded()));
            update_milestones = false;
        }
    }

    // If we want to update the milestones
    if(update_milestones) {
        ctrlm_rcu_battery_event_t battery_event = CTRLM_RCU_BATTERY_EVENT_NONE;
        bool    send_event = false;
        uint8_t threshold_counter = 0;
        if(batteries_changed) {
            this->battery_changed_actual_percent             = status_new.get_percent();
            this->battery_changed_timestamp                  = status_new.get_update_time();
            this->battery_75_percent_actual_percent          = 0;
            this->battery_75_percent_timestamp               = 0;
            this->battery_50_percent_actual_percent          = 0;
            this->battery_50_percent_timestamp               = 0;
            this->battery_25_percent_actual_percent          = 0;
            this->battery_25_percent_timestamp               = 0;
            this->battery_5_percent_actual_percent           = 0;
            this->battery_5_percent_timestamp                = 0;
            this->battery_0_percent_actual_percent           = 0;
            this->battery_0_percent_timestamp                = 0;
            this->battery_changed_unloaded_voltage           = status_new.get_voltage_unloaded();
            this->battery_75_percent_unloaded_voltage        = 0;
            this->battery_50_percent_unloaded_voltage        = 0;
            this->battery_25_percent_unloaded_voltage        = 0;
            this->battery_5_percent_unloaded_voltage         = 0;
            this->battery_0_percent_unloaded_voltage         = 0;
            this->battery_voltage_large_jump_counter->counter = 0;
            this->battery_voltage_large_decline_detected     = 0;
            threshold_counter                                      = 0;
            battery_event                                          = CTRLM_RCU_BATTERY_EVENT_REPLACED;
            send_event                                             = true;
            // Need to clear battery_voltage_large_jump_counter in the database
            // UPDATE: This will be done below when the milestones are written to DB
        }

        //These check variables are to help us transition from images prior to RDK-32347
        unsigned long battery_75_percent_old_image_check = this->battery_50_percent_timestamp + this->battery_25_percent_timestamp + this->battery_5_percent_timestamp + this->battery_0_percent_timestamp;
        unsigned long battery_50_percent_old_image_check = this->battery_25_percent_timestamp + this->battery_5_percent_timestamp + this->battery_0_percent_timestamp;
        unsigned long battery_25_percent_old_image_check = this->battery_5_percent_timestamp + this->battery_0_percent_timestamp;
        unsigned long battery_5_percent_old_image_check  = this->battery_0_percent_timestamp;
        if((status_new.get_percent() <= CTRLM_BATTERY_75_PERCENT_THRESHOLD) && (this->battery_75_percent_timestamp == 0) && (battery_75_percent_old_image_check == 0)) {
            this->battery_75_percent_actual_percent    = status_new.get_percent();
            this->battery_75_percent_timestamp         = status_new.get_update_time();
            this->battery_75_percent_unloaded_voltage  = status_new.get_voltage_unloaded();
            threshold_counter++;
            battery_event                                    = CTRLM_RCU_BATTERY_EVENT_75_PERCENT;
            send_event                                       = true;
        }
        if((status_new.get_percent() <= CTRLM_BATTERY_50_PERCENT_THRESHOLD) && (this->battery_50_percent_timestamp == 0) && (battery_50_percent_old_image_check == 0)) {
            this->battery_50_percent_actual_percent    = status_new.get_percent();
            this->battery_50_percent_timestamp         = status_new.get_update_time();
            this->battery_50_percent_unloaded_voltage  = status_new.get_voltage_unloaded();
            threshold_counter++;
            battery_event                                    = CTRLM_RCU_BATTERY_EVENT_50_PERCENT;
            send_event                                       = true;
        }
        if((status_new.get_percent() <= CTRLM_BATTERY_25_PERCENT_THRESHOLD) && (this->battery_25_percent_timestamp == 0) && (battery_25_percent_old_image_check == 0)) {
            this->battery_25_percent_actual_percent    = status_new.get_percent();
            this->battery_25_percent_timestamp         = status_new.get_update_time();
            this->battery_25_percent_unloaded_voltage  = status_new.get_voltage_unloaded();
            threshold_counter++;
            battery_event                                    = CTRLM_RCU_BATTERY_EVENT_25_PERCENT;
            send_event                                       = true;
        }
        if((status_new.get_percent() <= CTRLM_BATTERY_5_PERCENT_THRESHOLD) && (this->battery_5_percent_timestamp == 0) && (battery_5_percent_old_image_check == 0)) {
            this->battery_5_percent_actual_percent     = status_new.get_percent();
            this->battery_5_percent_timestamp          = status_new.get_update_time();
            this->battery_5_percent_unloaded_voltage   = status_new.get_voltage_unloaded();
            threshold_counter++;
            battery_event                                    = CTRLM_RCU_BATTERY_EVENT_PENDING_DOOM;
            send_event                                       = true;
        }
        if((status_new.get_percent() == CTRLM_BATTERY_0_PERCENT_THRESHOLD)           && (this->battery_0_percent_timestamp == 0)) {
            this->battery_0_percent_actual_percent     = status_new.get_percent();
            this->battery_0_percent_timestamp          = status_new.get_update_time();
            this->battery_0_percent_unloaded_voltage   = status_new.get_voltage_unloaded();
            threshold_counter++;
            battery_event                                    = CTRLM_RCU_BATTERY_EVENT_0_PERCENT;
            send_event                                       = true;
        }
        this->battery_last_good_loaded_voltage         = status_new.get_voltage_loaded();
        this->battery_last_good_unloaded_voltage       = status_new.get_voltage_unloaded();
        this->battery_last_good_percent                = status_new.get_percent();
        this->battery_last_good_timestamp              = status_new.get_update_time();
        if(!batteries_changed && (threshold_counter>1)) {
            this->battery_voltage_large_decline_detected = 1;
        }

        // Write milestones to DB
        if(false == importing) {
            ctrlm_db_attr_write(shared_from_this());
            if(send_event) {
                // Send message to MAIN QUEUE
                ctrlm_main_queue_msg_rf4ce_battery_milestone_t *msg = (ctrlm_main_queue_msg_rf4ce_battery_milestone_t *)g_malloc(sizeof(ctrlm_main_queue_msg_rf4ce_battery_milestone_t));
                if(NULL == msg) {
                    XLOGD_ERROR("Couldn't allocate memory");
                    g_assert(0);
                }

                msg->type              = CTRLM_MAIN_QUEUE_MSG_TYPE_BATTERY_MILESTONE_EVENT;
                msg->network_id        = this->network_id;
                msg->controller_id     = this->controller_id;
                msg->battery_event     = battery_event;
                msg->percent           = status_new.get_percent();
                ctrlm_main_queue_msg_push(msg);
            }
        }

        XLOGD_INFO("battery_changed <%ld>, <%d%%>, <%4.2fv> battery_75_percent_ <%ld>, <%d%%>, <%4.2fv> battery_50_percent_ <%ld>, <%d%%>, <%4.2fv> battery_25_percent_ <%ld>, <%d%%>, <%4.2fv> battery_5_percent_ <%ld>, <%d%%>, <%4.2fv> battery_0_percent_ <%ld>, <%d%%>, <%4.2fv>",
            this->battery_changed_timestamp, this->battery_changed_actual_percent, VOLTAGE_CALC(this->battery_changed_unloaded_voltage),
            this->battery_75_percent_timestamp, this->battery_75_percent_actual_percent, VOLTAGE_CALC(this->battery_75_percent_unloaded_voltage), 
            this->battery_50_percent_timestamp, this->battery_50_percent_actual_percent, VOLTAGE_CALC(this->battery_50_percent_unloaded_voltage),
            this->battery_25_percent_timestamp, this->battery_25_percent_actual_percent, VOLTAGE_CALC(this->battery_25_percent_unloaded_voltage), 
            this->battery_5_percent_timestamp, this->battery_5_percent_actual_percent, VOLTAGE_CALC(this->battery_5_percent_unloaded_voltage), 
            this->battery_0_percent_timestamp, this->battery_0_percent_actual_percent, VOLTAGE_CALC(this->battery_0_percent_unloaded_voltage));
        XLOGD_INFO("battery_last_good_timestamp <%ld> battery_last_good_percent_<%d%%> battery_last_good_loaded_voltage <%4.2fv> battery_last_good_unloaded_voltage <%4.2fv> battery_voltage_large_jump_counter <%u> battery_voltage_large_decline_detected_<%d> battery_first_write_ <%d>",
            this->battery_last_good_timestamp, this->battery_last_good_percent, VOLTAGE_CALC(this->battery_last_good_loaded_voltage), VOLTAGE_CALC(this->battery_last_good_unloaded_voltage),
            this->battery_voltage_large_jump_counter->counter, this->battery_voltage_large_decline_detected, this->battery_first_write);
    }
}

bool ctrlm_rf4ce_battery_milestones_t::is_batteries_changed(uint8_t new_unloaded_voltage, uint8_t old_unloaded_voltage) {
    guchar battery_increase = (0.3 * 255 / 4);
    //If the new voltage goes up by 0.3v or more, consider this a batteries changed situation
    return(new_unloaded_voltage >= (old_unloaded_voltage + battery_increase));
}
bool ctrlm_rf4ce_battery_milestones_t::is_batteries_large_voltage_jump(uint8_t new_unloaded_voltage, uint8_t old_unloaded_voltage) {
    guchar battery_increase = (0.2 * 255 / 4);
    //If the new voltage goes up by 0.2v or more but less than 0.3v, don't set the new voltage but report as a large jump.
    return(new_unloaded_voltage >= (old_unloaded_voltage + battery_increase));
}

std::string ctrlm_rf4ce_battery_milestones_t::to_string() const {
    std::stringstream ss;
    std::ios f(NULL);
    f.copyfmt(ss);
    ss << "Battery Changed: " << "ts <" << this->battery_changed_timestamp    << "> " <<  "bat % <" << (unsigned int)this->battery_changed_actual_percent    << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_changed_unloaded_voltage)    << "v>" << std::endl; ss.copyfmt(f);
    ss << "Battery 75%: "     << "ts <" << this->battery_75_percent_timestamp << "> " <<  "bat % <" << (unsigned int)this->battery_75_percent_actual_percent << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_75_percent_unloaded_voltage) << "v>" << std::endl; ss.copyfmt(f);
    ss << "Battery 50%: "     << "ts <" << this->battery_50_percent_timestamp << "> " <<  "bat % <" << (unsigned int)this->battery_50_percent_actual_percent << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_50_percent_unloaded_voltage) << "v>" << std::endl; ss.copyfmt(f);
    ss << "Battery 25%: "     << "ts <" << this->battery_25_percent_timestamp << "> " <<  "bat % <" << (unsigned int)this->battery_25_percent_actual_percent << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_25_percent_unloaded_voltage) << "v>" << std::endl; ss.copyfmt(f);
    ss << "Battery 5%: "      << "ts <" << this->battery_5_percent_timestamp  << "> " <<  "bat % <" << (unsigned int)this->battery_5_percent_actual_percent  << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_5_percent_unloaded_voltage)  << "v>" << std::endl; ss.copyfmt(f);
    ss << "Battery 0%: "      << "ts <" << this->battery_0_percent_timestamp  << "> " <<  "bat % <" << (unsigned int)this->battery_0_percent_actual_percent  << "> " <<  "unloaded <" << std::setprecision(3) << std::setfill('0') << VOLTAGE_CALC(this->battery_0_percent_unloaded_voltage)  << "v>" << std::endl; ss.copyfmt(f);
    ss << "Large Jump Counter: " << (unsigned int)this->battery_voltage_large_jump_counter->counter << std::endl;
    ss << "Large Decline Detected: " << (this->battery_voltage_large_decline_detected > 0 ? "YES" : "NO") << std::endl;   

    return(ss.str());
}

// FOR DB COMPATIBILITY
typedef struct {
   time_t   battery_changed_timestamp;
   time_t   battery_75_percent_timestamp;
   time_t   battery_50_percent_timestamp;
   time_t   battery_25_percent_timestamp;
   time_t   battery_5_percent_timestamp;
   time_t   battery_0_percent_timestamp;
   guchar   battery_changed_actual_percent;
   guchar   battery_75_percent_actual_percent;
   guchar   battery_50_percent_actual_percent;
   guchar   battery_25_percent_actual_percent;
   guchar   battery_5_percent_actual_percent;
   guchar   battery_0_percent_actual_percent;
} battery_voltage_milestones_t;
// FOR DB COMPATIBILITY

bool ctrlm_rf4ce_battery_milestones_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = true;
    battery_voltage_milestones_t milestones_s;
    ctrlm_db_blob_t milestones("battery_milestones", this->get_table());

    memset(&milestones_s, 0, sizeof(milestones_s));
    if(milestones.read_db(ctx)) {
        size_t len = milestones.to_buffer((char *)&milestones_s, sizeof(milestones_s));
        if(len >= 0) {
            if(len >= sizeof(battery_voltage_milestones_t)) {
                this->battery_changed_timestamp         = milestones_s.battery_changed_timestamp;
                this->battery_75_percent_timestamp      = milestones_s.battery_75_percent_timestamp;
                this->battery_50_percent_timestamp      = milestones_s.battery_50_percent_timestamp;
                this->battery_25_percent_timestamp      = milestones_s.battery_25_percent_timestamp;
                this->battery_5_percent_timestamp       = milestones_s.battery_5_percent_timestamp;
                this->battery_0_percent_timestamp       = milestones_s.battery_0_percent_timestamp;
                this->battery_changed_actual_percent    = milestones_s.battery_changed_actual_percent;
                this->battery_75_percent_actual_percent = milestones_s.battery_75_percent_actual_percent;
                this->battery_50_percent_actual_percent = milestones_s.battery_50_percent_actual_percent;
                this->battery_25_percent_actual_percent = milestones_s.battery_25_percent_actual_percent;
                this->battery_5_percent_actual_percent  = milestones_s.battery_5_percent_actual_percent;
                this->battery_0_percent_actual_percent  = milestones_s.battery_0_percent_actual_percent;
            } else {
                XLOGD_WARN("data from db too small <%s - battery_voltage_milestones_t>", this->get_name().c_str());
                ret = false;
            }
        } else {
            XLOGD_WARN("failed to copy into buffer <%s - battery_voltage_milestones_t>", this->get_name().c_str());
            ret = false;
        }
    } else {
        XLOGD_WARN("failed to read <%s - battery_voltage_milestones_t>", this->get_name().c_str());
        ret = false;
    }

    ctrlm_db_uint64_t last_good_timestamp("battery_last_good_timestamp", this->get_table());
    if(last_good_timestamp.read_db(ctx)) {
        this->battery_last_good_timestamp = last_good_timestamp.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_last_good_timestamp>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_pecent("battery_last_good_percent", this->get_table());
    if(last_good_pecent.read_db(ctx)) {
        this->battery_last_good_percent = (uint8_t)last_good_pecent.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_last_good_percent>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_loaded_voltage("battery_last_good_loaded_voltage", this->get_table());
    if(last_good_loaded_voltage.read_db(ctx)) {
        this->battery_last_good_loaded_voltage = (uint8_t)last_good_loaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_last_good_loaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_unloaded_voltage("battery_last_good_unloaded_voltage", this->get_table());
    if(last_good_unloaded_voltage.read_db(ctx)) {
        this->battery_last_good_unloaded_voltage = (uint8_t)last_good_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_last_good_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t voltage_large_decline_detected("battery_voltage_large_decline_detected", this->get_table());
    if(voltage_large_decline_detected.read_db(ctx)) {
        this->battery_voltage_large_decline_detected = (bool)voltage_large_decline_detected.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_voltage_large_decline_detected>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_changed_unloaded_voltage("battery_changed_unloaded_voltage", this->get_table());
    if(battery_changed_unloaded_voltage.read_db(ctx)) {
        this->battery_changed_unloaded_voltage = (uint8_t)battery_changed_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_changed_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_75_percent_unloaded_voltage("battery_75_percent_unloaded_voltage", this->get_table());
    if(battery_75_percent_unloaded_voltage.read_db(ctx)) {
        this->battery_75_percent_unloaded_voltage = (uint8_t)battery_75_percent_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_75_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_50_percent_unloaded_voltage("battery_50_percent_unloaded_voltage", this->get_table());
    if(battery_50_percent_unloaded_voltage.read_db(ctx)) {
        this->battery_50_percent_unloaded_voltage = (uint8_t)battery_50_percent_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_50_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_25_percent_unloaded_voltage("battery_25_percent_unloaded_voltage", this->get_table());
    if(battery_25_percent_unloaded_voltage.read_db(ctx)) {
        this->battery_25_percent_unloaded_voltage = (uint8_t)battery_25_percent_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_25_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_5_percent_unloaded_voltage("battery_5_percent_unloaded_voltage", this->get_table());
    if(battery_5_percent_unloaded_voltage.read_db(ctx)) {
        this->battery_5_percent_unloaded_voltage = (uint8_t)battery_5_percent_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_5_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_0_percent_unloaded_voltage("battery_0_percent_unloaded_voltage", this->get_table());
    if(battery_0_percent_unloaded_voltage.read_db(ctx)) {
        this->battery_0_percent_unloaded_voltage = (uint8_t)battery_0_percent_unloaded_voltage.get_uint64();
    } else {
        XLOGD_WARN("failed to read <%s - battery_0_percent_unloaded_voltage>", this->get_name().c_str());
    }
    this->battery_voltage_large_jump_counter->read_db(ctx);

    if(ret) {
        XLOGD_INFO("%s read from database:\n%s", this->get_name().c_str(), this->to_string().c_str());
    }

    return(ret);
}

bool ctrlm_rf4ce_battery_milestones_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    battery_voltage_milestones_t milestones_s;
    ctrlm_db_blob_t milestones("battery_milestones", this->get_table());

    milestones_s.battery_changed_timestamp         = this->battery_changed_timestamp;
    milestones_s.battery_75_percent_timestamp      = this->battery_75_percent_timestamp;
    milestones_s.battery_50_percent_timestamp      = this->battery_50_percent_timestamp;
    milestones_s.battery_25_percent_timestamp      = this->battery_25_percent_timestamp;
    milestones_s.battery_5_percent_timestamp       = this->battery_5_percent_timestamp;
    milestones_s.battery_0_percent_timestamp       = this->battery_0_percent_timestamp;
    milestones_s.battery_changed_actual_percent    = this->battery_changed_actual_percent;
    milestones_s.battery_75_percent_actual_percent = this->battery_75_percent_actual_percent;
    milestones_s.battery_50_percent_actual_percent = this->battery_50_percent_actual_percent;
    milestones_s.battery_25_percent_actual_percent = this->battery_25_percent_actual_percent;
    milestones_s.battery_5_percent_actual_percent  = this->battery_5_percent_actual_percent;
    milestones_s.battery_0_percent_actual_percent  = this->battery_0_percent_actual_percent;

    if(milestones.from_buffer((char *)&milestones_s, sizeof(milestones_s))) {
        if(milestones.write_db(ctx)) {
            ret = true;
        } else {
            XLOGD_WARN("failed to write <%s - battery_voltage_milestones_t>", this->get_name().c_str());
        }
    } else {
        XLOGD_WARN("failed to create blob from <%s - battery_voltage_milestones_t>", this->get_name().c_str());
    }

    ctrlm_db_uint64_t last_good_timestamp("battery_last_good_timestamp", this->get_table(), this->battery_last_good_timestamp);
    if(last_good_timestamp.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_last_good_timestamp>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_pecent("battery_last_good_percent", this->get_table(), this->battery_last_good_percent);
    if(last_good_pecent.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_last_good_percent>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_loaded_voltage("battery_last_good_loaded_voltage", this->get_table(), this->battery_last_good_loaded_voltage);
    if(last_good_loaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_last_good_loaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t last_good_unloaded_voltage("battery_last_good_unloaded_voltage", this->get_table(), this->battery_last_good_unloaded_voltage);
    if(last_good_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_last_good_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t voltage_large_decline_detected("battery_voltage_large_decline_detected", this->get_table(), this->battery_voltage_large_decline_detected);
    if(voltage_large_decline_detected.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_voltage_large_decline_detected>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_changed_unloaded_voltage("battery_changed_unloaded_voltage", this->get_table(), this->battery_changed_unloaded_voltage);
    if(battery_changed_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_changed_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_75_percent_unloaded_voltage("battery_75_percent_unloaded_voltage", this->get_table(), this->battery_75_percent_unloaded_voltage);
    if(battery_75_percent_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_75_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_50_percent_unloaded_voltage("battery_50_percent_unloaded_voltage", this->get_table(), this->battery_50_percent_unloaded_voltage);
    if(battery_50_percent_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_50_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_25_percent_unloaded_voltage("battery_25_percent_unloaded_voltage", this->get_table(), this->battery_25_percent_unloaded_voltage);
    if(battery_25_percent_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_25_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_5_percent_unloaded_voltage("battery_5_percent_unloaded_voltage", this->get_table(), this->battery_5_percent_unloaded_voltage);
    if(battery_5_percent_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_5_percent_unloaded_voltage>", this->get_name().c_str());
    }
    ctrlm_db_uint64_t battery_0_percent_unloaded_voltage("battery_0_percent_unloaded_voltage", this->get_table(), this->battery_0_percent_unloaded_voltage);
    if(battery_0_percent_unloaded_voltage.write_db(ctx)) {
    } else {
        XLOGD_WARN("failed to write <%s - battery_0_percent_unloaded_voltage>", this->get_name().c_str());
    }
    this->battery_voltage_large_jump_counter->write_db(ctx);

    if(ret) {
        XLOGD_INFO("%s written to database:\n%s", this->get_name().c_str(), this->to_string().c_str());
    }

    return(ret);
}
// end ctrlm_rf4ce_battery_milestones_t
