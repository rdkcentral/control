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

#ifndef __CTRLM_IR_RF_DB_H__
#define __CTRLM_IR_RF_DB_H__
#include <iostream>
#include <vector>
#include <map>
#include "ctrlm.h"
#include "ctrlm_ir_rf_db_entry.h"

/**
 * This class contains the implementation of the XRC IR RF Database. Utilizing this class and the IR RF Database Entry class, we can support
 * IR setup with both the Legacy RAMS service code and the new architecture utilizing the ControlMgr IRDB component.
 */
class ctrlm_ir_rf_db_t {
public:

    /**
     * Default Constructor
     * 
     * @param power_toggle_favor_tv True if TV is favored for power toggle, False if AVR is favored for power toggle
     * @param power_discrete_favor_tv True if TV is favored for power discrete, False if AVR is favored for power discrete
     * 
     * @note power_toggle_favor_tv = true, power_discrete_favor_tv = false is the defaults for RF4CE
     */
    ctrlm_ir_rf_db_t(bool power_toggle_favor_tv = true, bool power_discrete_favor_tv = false);

    /**
     * Default Deconstructor
     */
    virtual ~ctrlm_ir_rf_db_t();

    /**
     * Function to add an IR code entry to the IR RF Database
     * @param entry A pointer to the IR RF Database entry. The IR RF Database will handle freeing this memory once it no longer needs the entry.
     * @param key The key parameter will determine which key slot to add the entry to. Setting key to invalid utlizes the ir_rf db logic to add the entry to the proper slots. This should only be set for old RAMS implementation.
     * @return True if the entry was added to the IRRF Database, False otherwise.
     */
    bool add_ir_code_entry(ctrlm_ir_rf_db_entry_t *entry, ctrlm_key_code_t key = CTRLM_KEY_CODE_INVALID);

    /**
     * Function to add a CTRLM IRDB ir_codes to the IR RF Database
     * @param ir_codes A keymap supplied from the CTRLM IRDB component
     * @reutrn True if the entries were added to the IRRF Database, False otherwise.
     */
    bool add_irdb_codes(ctrlm_irdb_ir_code_set_t *ir_codes);

    /**
     * Function to clear all TV IR codes stored in the IR RF Database
     */
    void clear_tv_ir_codes();

    /**
     * Function to clear all AVR IR codes stored in the IR RF Database
     */
    void clear_avr_ir_codes();

    /**
     * Function to clear all IR codes stored in the IR RF Database
     */
    void clear_ir_codes();

    /**
     * Function to retrieve an IR RF Database Entry for a given key code slot.
     * @param key The key code slot for the desired IR RF Database Entry
     * @return Returns the IR RF Database Entry object or NULL if it doesn't exist.
     */
    ctrlm_ir_rf_db_entry_t *get_ir_code(ctrlm_key_code_t key);

    /**
     * Function used to check if a IR RF Database Entry for a specfic key slot exists.
     * @param key The desired key slot.
     * @return True if an IR RF Database Entry exists otherwise False.
     */
    bool has_entry(ctrlm_key_code_t key);

    /**
     * Function used to remove a IR RF Database Entry for a specfic key slot.
     * @param key The desired key slot.
     */
    void remove_entry(ctrlm_key_code_t key);

    /**
     * Function to retrieve a string of what is currently in the IR RF Database
     * @param debug This option allows for the user to request a more verbose string, including hex strings of the contents of each entry.
     */
    std::string to_string(bool debug = false) const;

    /**
     * Function used to load the IR RF Database from the ControlMgr Database.
     */
    void load_db();

    /**
     * Function used to store the IR RF Database in the ControlMgr Database.
     * @return True if every IRRF Database entry was stored in the Database, otherwise False.
     */
    bool store_db();

    /**
     * Function used to get the TV IR code ID in the ControlMgr Database.
     * @return the TV IR code ID in string form.
     */
    std::string get_tv_ir_code_id();

    /**
     * Function used to get the AVR IR code ID in the ControlMgr Database.
     * @return the AVR IR code ID in string form.
     */
    std::string get_avr_ir_code_id();

private:
    /**
     * Internal function used to replace a IR RF Database Entry for a specfic key slot.
     * @param key The desired key slot.
     * @param entry The pointer to the IR RF Database Entry.
     */
    void replace_entry(ctrlm_key_code_t key, ctrlm_ir_rf_db_entry_t *entry);

    /**
     * Internal function used to store an individual entry to the database. 
     * @param key The desired key slot.
     * @return True if the entry is successfully stored otherwise False.
     */
    bool store_entry(ctrlm_key_code_t key);

    /**
     * Internal function used to check if key code represents a valid IR RF Database key slot.
     * @param key The desired key slot.
     */
    bool check_key_code(ctrlm_key_code_t key);

    /**
     * Internal function used to set the common IR RF Database entries, along with containing the logic to fix IR flags to maintain current functionality.
     */
    void fix_common_slots_and_ir_flags();

private:
    std::map<ctrlm_key_code_t, ctrlm_ir_rf_db_entry_t*> ir_rf_db;
    bool power_toggle_favor_tv;
    bool power_discrete_favor_tv;
    std::string tv_ir_code_id_;
    std::string avr_ir_code_id_;
};


#endif
