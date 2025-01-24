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

#ifndef __CTRLM_IR_RF_DB_ENTRY_H__
#define __CTRLM_IR_RF_DB_ENTRY_H__
#include "ctrlm.h"
#include "ctrlm_irdb.h"
#include <vector>

/**
 * The supported device types for the IR RF Database Entries.
 */
typedef enum {
    CTRLM_IR_RF_DB_DEV_TV,
    CTRLM_IR_RF_DB_DEV_AVR
} ctrlm_ir_rf_db_dev_type_t;

/**
 * This class is a container for each entry in the IR RF Database. The class handles creating itself from RIB Binary for RAMS service support, raw
 * IR codes which come from the ControlMgr IRDB component, and duplicating the entries.
 */
class ctrlm_ir_rf_db_entry_t {
public:
    /**
     * Default Destructor
     */
    virtual ~ctrlm_ir_rf_db_entry_t();

    /**
     * Static function which creates an IR RF Database Entry from the binary data sent from RAMS Service.
     */
    static ctrlm_ir_rf_db_entry_t* from_rib_binary(uint8_t *data, uint16_t length);

    /**
     * Static function which creates an IR RF Database Entry from the raw IR code from ControlMgr IRDB Component.
     */
    static ctrlm_ir_rf_db_entry_t* from_raw_ir_code(ctrlm_ir_rf_db_dev_type_t type, ctrlm_key_code_t key, uint8_t *data, uint16_t length);

    /**
     * Static function which creates a duplicate IR RF Database Entry. Mainly used by the IR RF Database implementation.
     */
    static ctrlm_ir_rf_db_entry_t* from_ir_rf_db_entry(ctrlm_ir_rf_db_entry_t *entry);

    /**
     * Static function which reads the key slot entry from the ControlMgr Database.
     */
    static ctrlm_ir_rf_db_entry_t* from_db(ctrlm_key_code_t key);

    /**
     * Static function to convert a ctrlm_irdb_dev_type to a ctrlm_ir_rf_db_dev_type
     */
    static ctrlm_ir_rf_db_dev_type_t type_from_irdb(ctrlm_irdb_dev_type_t type);

    /**
     * Function to get a buffer containing the binary IR RF Database Entry. This data will be sent directly to the remote, and used for DB storage.
     * @param data Pointer to a buffer pointer. This is used to give the user access to a buffer containing the binary blob for the entry. This will need to be freed after use.
     * @param length Pointer to an unsiged integer representing the length of the returned buffer.
     * @param include_header RF4CE RIB header added if true, else raw IR code is returned.
     * @return True if the buffer was successfully created and the data was copied to the buffer, False otherwise.
     */
    bool to_binary(uint8_t **data, uint16_t *length, bool include_header = true);

    /**
     * Function to retrieve a string of what is currently in the IR RF Database Entry
     * @param debug This option allows for the user to request a more verbose string, including hex strings of the contents of the entry.
     */
    std::string to_string(bool debug, bool include_header = true) const;

    /**
     * Function to get the device type of the entry.
     */
    ctrlm_ir_rf_db_dev_type_t get_type() const;

    /**
     * Function to get the key of the entry.
     */
    ctrlm_key_code_t get_key() const;

    /**
     * Function to get the code vector 
     */
    std::vector<uint8_t> get_code() const;

    /**
     * Function to set the IR Flags for the entry. The IR RF Database implementation contains the logic for calculating this value.
     */
    void set_ir_flags(uint8_t flags);

private:
    /**
     * Default Constructor - Private, use one of the static methods to create this object.
     */
    ctrlm_ir_rf_db_entry_t();

    /**
     * Copy Constructor - Private, use one of the static methods to create this object.
     */
    ctrlm_ir_rf_db_entry_t(ctrlm_ir_rf_db_entry_t *entry);

    /**
     * Helper function to get the RF Descriptor based on the entry's key.
     */
    void set_descriptor_via_key();

    /**
     * Helper function to get the key based on the entry's RF Descriptors.
     */
    void set_key_via_descriptor();

private:
    ctrlm_ir_rf_db_dev_type_t type;
    ctrlm_key_code_t                key;
    uint8_t                         rf_descriptor[2];
    uint8_t                         rf_config;
    uint8_t                         ir_flags;
    std::vector<uint8_t>            ir_code;
};

#endif