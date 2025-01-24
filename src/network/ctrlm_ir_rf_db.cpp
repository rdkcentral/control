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

#include "ctrlm_ir_rf_db.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"
#include "ctrlm_log.h"
#include <sstream>

ctrlm_ir_rf_db_t::ctrlm_ir_rf_db_t(bool power_toggle_favor_tv, bool power_discrete_favor_tv) {
    // Setup empty slots
    this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_ON]      = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_OFF]     = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]         = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_ON]     = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_OFF]    = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE] = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_POWER_ON]         = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_POWER_OFF]        = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_POWER_TOGGLE]     = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_VOL_UP]           = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_VOL_DOWN]         = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_MUTE]             = NULL;
    this->ir_rf_db[CTRLM_KEY_CODE_INPUT_SELECT]     = NULL;

    this->power_toggle_favor_tv   = power_toggle_favor_tv;
    this->power_discrete_favor_tv = power_discrete_favor_tv;
    this->tv_ir_code_id_ = "0";
    this->avr_ir_code_id_ = "0";
}

ctrlm_ir_rf_db_t::~ctrlm_ir_rf_db_t() {
    for(auto itr = this->ir_rf_db.begin(); itr != this->ir_rf_db.end(); itr++) {
        this->remove_entry(itr->first);
    }
}

bool ctrlm_ir_rf_db_t::add_ir_code_entry(ctrlm_ir_rf_db_entry_t *entry, ctrlm_key_code_t key) {
    bool ret = false;
    if(this->check_key_code(entry->get_key())) {
        if(key == CTRLM_KEY_CODE_INVALID) { // This is the new way of adding IR RF Entries. This logic handles setting up the proper key slots for each key.
            switch(entry->get_type()) {
                case CTRLM_IR_RF_DB_DEV_TV: {
                    switch(entry->get_key()) {
                        case CTRLM_KEY_CODE_POWER_ON: {
                            this->replace_entry(CTRLM_KEY_CODE_TV_POWER_ON, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_POWER_OFF: {
                            this->replace_entry(CTRLM_KEY_CODE_TV_POWER_OFF, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_POWER_TOGGLE: {
                            this->replace_entry(CTRLM_KEY_CODE_TV_POWER, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_VOL_UP:
                        case CTRLM_KEY_CODE_VOL_DOWN:
                        case CTRLM_KEY_CODE_MUTE: {
                            if(this->ir_rf_db[entry->get_key()]) {
                                if(this->ir_rf_db[entry->get_key()]->get_type() == CTRLM_IR_RF_DB_DEV_AVR) {
                                    XLOGD_WARN("VOL/MUTE slot has AVR code, skipping");
                                    break;
                                }
                            }
                            this->replace_entry(entry->get_key(), entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_INPUT_SELECT: {
                            this->replace_entry(CTRLM_KEY_CODE_INPUT_SELECT, entry);
                            break;
                        }
                        default: {
                            // Never should happen
                            XLOGD_ERROR("Invalid key code <%d, %d>", entry->get_type(), entry->get_key());
                            break;
                        }
                    }
                    break;
                }
                case CTRLM_IR_RF_DB_DEV_AVR: {
                    switch(entry->get_key()) {
                        case CTRLM_KEY_CODE_POWER_ON: {
                            this->replace_entry(CTRLM_KEY_CODE_AVR_POWER_ON, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_POWER_OFF: {
                            this->replace_entry(CTRLM_KEY_CODE_AVR_POWER_OFF, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_POWER_TOGGLE: {
                            this->replace_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE, entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_VOL_UP:
                        case CTRLM_KEY_CODE_VOL_DOWN:
                        case CTRLM_KEY_CODE_MUTE: {
                            this->replace_entry(entry->get_key(), entry);
                            break;
                        }
                        case CTRLM_KEY_CODE_INPUT_SELECT: {
                            XLOGD_WARN("INPUT Select for AVR not used");
                            break;
                        }
                        default: {
                            // Never should happen
                            XLOGD_ERROR("Invalid key code <%d, %d>", entry->get_type(), entry->get_key());
                            break;
                        }
                    }
                    break;
                }
                default: {
                    // Never should happen
                    XLOGD_ERROR("Invalid dev type <%d>", entry->get_type());
                    break;
                }
            }
            this->fix_common_slots_and_ir_flags();
        } else { // This forces the entry into a specific slot without fixing any entries. This is used to maintain current RAMS behavior. Also writes the entry to the DB.
            this->replace_entry(key, entry);
            this->store_entry(key);
        }
        ret = true;
    } else {
        XLOGD_ERROR("invalid key type");
    }

    return(ret);
}

bool ctrlm_ir_rf_db_t::add_irdb_codes(ctrlm_irdb_ir_code_set_t *ir_codes) {
    bool ret = false;
    if(ir_codes) {
        ctrlm_ir_rf_db_dev_type_t type = ctrlm_ir_rf_db_entry_t::type_from_irdb(ir_codes->get_type());
        switch(type) {
            case CTRLM_IR_RF_DB_DEV_TV: {
                this->tv_ir_code_id_ = ir_codes->get_id();
                break;
            }
            case CTRLM_IR_RF_DB_DEV_AVR: {
                this->avr_ir_code_id_ = ir_codes->get_id();
                break;
            }
            default: {
                break;
            }
        }
        for(auto &itr : *ir_codes->get_key_map()) {
            ctrlm_ir_rf_db_entry_t *entry = ctrlm_ir_rf_db_entry_t::from_raw_ir_code(type, itr.first, (uint8_t *)itr.second.data(), (uint16_t)itr.second.size());
            if(entry) {
               this->add_ir_code_entry(entry);
            } else {
               XLOGD_WARN("failed to create entry");
            }
         }
         ret = true;
    }
    return(ret);
}

void ctrlm_ir_rf_db_t::clear_tv_ir_codes() {
    this->remove_entry(CTRLM_KEY_CODE_TV_POWER_ON);
    this->remove_entry(CTRLM_KEY_CODE_TV_POWER_OFF);
    this->remove_entry(CTRLM_KEY_CODE_TV_POWER);
    if(this->has_entry(CTRLM_KEY_CODE_VOL_UP)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_VOL_UP]->get_type() == CTRLM_IR_RF_DB_DEV_TV) {
            this->remove_entry(CTRLM_KEY_CODE_VOL_UP);
        }
    }
    if(this->has_entry(CTRLM_KEY_CODE_VOL_DOWN)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_VOL_DOWN]->get_type() == CTRLM_IR_RF_DB_DEV_TV) {
            this->remove_entry(CTRLM_KEY_CODE_VOL_DOWN);
        }
    }
    if(this->has_entry(CTRLM_KEY_CODE_MUTE)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_MUTE]->get_type() == CTRLM_IR_RF_DB_DEV_TV) {
            this->remove_entry(CTRLM_KEY_CODE_MUTE);
        }
    }
    this->remove_entry(CTRLM_KEY_CODE_INPUT_SELECT);

    this->fix_common_slots_and_ir_flags();
    this->tv_ir_code_id_ = "0";
}

void ctrlm_ir_rf_db_t::clear_avr_ir_codes() {
    this->remove_entry(CTRLM_KEY_CODE_AVR_POWER_ON);
    this->remove_entry(CTRLM_KEY_CODE_AVR_POWER_OFF);
    this->remove_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE);
    if(this->has_entry(CTRLM_KEY_CODE_VOL_UP)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_VOL_UP]->get_type() == CTRLM_IR_RF_DB_DEV_AVR) {
            this->remove_entry(CTRLM_KEY_CODE_VOL_UP);
        }
    }
    if(this->has_entry(CTRLM_KEY_CODE_VOL_DOWN)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_VOL_DOWN]->get_type() == CTRLM_IR_RF_DB_DEV_AVR) {
            this->remove_entry(CTRLM_KEY_CODE_VOL_DOWN);
        }
    }
    if(this->has_entry(CTRLM_KEY_CODE_MUTE)) {
        if(this->ir_rf_db[CTRLM_KEY_CODE_MUTE]->get_type() == CTRLM_IR_RF_DB_DEV_AVR) {
            this->remove_entry(CTRLM_KEY_CODE_MUTE);
        }
    }

    this->fix_common_slots_and_ir_flags();
    this->avr_ir_code_id_ = "0";
}

void ctrlm_ir_rf_db_t::clear_ir_codes() {
    for(auto itr = this->ir_rf_db.begin(); itr != this->ir_rf_db.end(); itr++) {
        ctrlm_db_ir_rf_database_delete(itr->first);
        this->remove_entry(itr->first);
    }
    this->tv_ir_code_id_ = "0";
    this->avr_ir_code_id_ = "0";
}

ctrlm_ir_rf_db_entry_t *ctrlm_ir_rf_db_t::get_ir_code(ctrlm_key_code_t key) {
    ctrlm_ir_rf_db_entry_t *ret = NULL;
    if(this->has_entry(key)) {
        ret = this->ir_rf_db[key];
    }
    return(ret);
}

std::string ctrlm_ir_rf_db_t::to_string(bool debug) const {
    std::stringstream ss;
    ss << "IR RF Database: "<< std::endl;
    ss << "\tTV  IR Code ID <" << tv_ir_code_id_ << ">" << std::endl;
    ss << "\tAVR IR Code ID <" << avr_ir_code_id_ << ">" << std::endl;
    for(auto itr = this->ir_rf_db.begin(); itr != this->ir_rf_db.end(); itr++) {
        if(itr->second != NULL) {
            ss << "\tKeySlot <" << ctrlm_key_code_str(itr->first) << ">, " << itr->second->to_string(debug) << std::endl;
        } else {
            ss << "\tKeySlot <" << ctrlm_key_code_str(itr->first) << "> EMPTY" << std::endl;
        }
    }
    return(ss.str());
}

void ctrlm_ir_rf_db_t::fix_common_slots_and_ir_flags() {
    // Clear common spots
    this->remove_entry(CTRLM_KEY_CODE_POWER_ON);
    this->remove_entry(CTRLM_KEY_CODE_POWER_OFF);
    this->remove_entry(CTRLM_KEY_CODE_POWER_TOGGLE);
    // First fix common slots
    // Power On slot
    if(this->power_discrete_favor_tv) { // Favors TV
        if(this->has_entry(CTRLM_KEY_CODE_TV_POWER_ON)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_ON]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_ON)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_ON]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        }
    } else { // Favors AVR
        if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_ON)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_ON]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER_ON)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_ON]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_ON, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        }
    }
    // Power Off slot
    if(this->power_discrete_favor_tv) { // Favors TV
        if(this->has_entry(CTRLM_KEY_CODE_TV_POWER_OFF)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_OFF]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_OFF)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_OFF]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        }
    } else { // Favors AVR
        if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_OFF)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_OFF]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER_OFF)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER_OFF]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_OFF, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        }
    }
    // Power Toggle slot (Favors TV)
    if(this->power_toggle_favor_tv) { // Favors TV
        if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_TOGGLE, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        } else if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_TOGGLE, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        }
    } else { // Favors AVR
        if(this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_TOGGLE, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_AVR_POWER_TOGGLE]));
        } else if(this->has_entry(CTRLM_KEY_CODE_TV_POWER)) {
            this->replace_entry(CTRLM_KEY_CODE_POWER_TOGGLE, ctrlm_ir_rf_db_entry_t::from_ir_rf_db_entry(this->ir_rf_db[CTRLM_KEY_CODE_TV_POWER]));
        }
    }

    // Now fix IR flags
    // Power On slot
    if(this->has_entry(CTRLM_KEY_CODE_POWER_ON) && (this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE) != this->has_entry(CTRLM_KEY_CODE_TV_POWER))) {
        this->ir_rf_db[CTRLM_KEY_CODE_POWER_ON]->set_ir_flags(0x4F);
    } else if(this->has_entry(CTRLM_KEY_CODE_POWER_ON)) {
        this->ir_rf_db[CTRLM_KEY_CODE_POWER_ON]->set_ir_flags(0x00);
    }
    // Power Off slot
    if(this->has_entry(CTRLM_KEY_CODE_POWER_OFF) && (this->has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE) != this->has_entry(CTRLM_KEY_CODE_TV_POWER))) {
        this->ir_rf_db[CTRLM_KEY_CODE_POWER_OFF]->set_ir_flags(0x4F);
    } else if(this->has_entry(CTRLM_KEY_CODE_POWER_OFF)) {
        this->ir_rf_db[CTRLM_KEY_CODE_POWER_OFF]->set_ir_flags(0x00);
    }

}

void ctrlm_ir_rf_db_t::load_db() {
    for(auto itr = this->ir_rf_db.begin(); itr != this->ir_rf_db.end(); itr++) {
        ctrlm_ir_rf_db_entry_t* entry = ctrlm_ir_rf_db_entry_t::from_db(itr->first);
        if(entry) {
            this->replace_entry(itr->first, entry);
        }
    }
    ctrlm_db_tv_ir_code_id_read(tv_ir_code_id_);
    ctrlm_db_avr_ir_code_id_read(avr_ir_code_id_);
}

bool ctrlm_ir_rf_db_t::store_db() {
    for(auto itr = this->ir_rf_db.begin(); itr != this->ir_rf_db.end(); itr++) {
        this->store_entry(itr->first);
    }
    ctrlm_db_tv_ir_code_id_write(tv_ir_code_id_);
    ctrlm_db_avr_ir_code_id_write(avr_ir_code_id_);

    return(true); // TODO, maybe change to void
}

void ctrlm_ir_rf_db_t::replace_entry(ctrlm_key_code_t key, ctrlm_ir_rf_db_entry_t *entry) {
    this->remove_entry(key);
    this->ir_rf_db[key] = entry;
}

bool ctrlm_ir_rf_db_t::has_entry(ctrlm_key_code_t key) {
    bool ret = false;
    if(this->ir_rf_db[key] != NULL) {
        ret = true;
    }
    return(ret);
}

bool ctrlm_ir_rf_db_t::store_entry(ctrlm_key_code_t key) {
    bool ret = false;
    uint8_t   *data = NULL;
    uint16_t  size = 0;

    if(this->ir_rf_db[key]) {
        if(this->ir_rf_db[key]->to_binary(&data, &size)) {
            if(data) { // Sanity
                ctrlm_db_ir_rf_database_write(key, data, size);
                free(data);
                ret = true;
            }
        }
    }
    return(ret);
}

void ctrlm_ir_rf_db_t::remove_entry(ctrlm_key_code_t key) {
    if(this->ir_rf_db[key] != NULL) {
        delete this->ir_rf_db[key];
        this->ir_rf_db[key] = NULL;
    }
}

bool ctrlm_ir_rf_db_t::check_key_code(ctrlm_key_code_t key) {
    bool ret = false;
    switch(key) {
        case CTRLM_KEY_CODE_POWER_ON:
        case CTRLM_KEY_CODE_POWER_OFF:
        case CTRLM_KEY_CODE_POWER_TOGGLE:
        case CTRLM_KEY_CODE_VOL_UP:
        case CTRLM_KEY_CODE_VOL_DOWN:
        case CTRLM_KEY_CODE_MUTE:
        case CTRLM_KEY_CODE_INPUT_SELECT: {
            ret = true;
            break;
        }
        default: {
            XLOGD_ERROR("Invalid key code <%s, %d>", ctrlm_key_code_str(key), key);
            break;
        }
    }
    return(ret);
}

std::string ctrlm_ir_rf_db_t::get_tv_ir_code_id() {
   return tv_ir_code_id_;
}

std::string ctrlm_ir_rf_db_t::get_avr_ir_code_id() {
   return avr_ir_code_id_;
}

