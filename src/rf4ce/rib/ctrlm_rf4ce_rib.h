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
#ifndef __CTRLM_RF4CE_RIB_H__
#define __CTRLM_RF4CE_RIB_H__
#include <map>
#include <stdint.h>
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"

#define IDENTIFIER_MAX (0x100)
#define INDEX_MAX      (0x100)

/**
 * @brief ControlMgr RF4CE RIB
 * 
 * This class is that implements the Remote Infomation Base (RIB)
 */
class ctrlm_rf4ce_rib_t {
public:
    /**
     * ControlMgr RF4CE RIB Constructor
     */
    ctrlm_rf4ce_rib_t();
    /**
     * ControlMgr RF4CE RIB Destructor
     */
    virtual ~ctrlm_rf4ce_rib_t();

public:
    /**
     * This is an enum which represents the resulting status of a RIB read or write call.
     */
    enum status {
        SUCCESS,
        BAD_PERMISSIONS,
        DOES_NOT_EXIST,
        FAILURE
    };

public:
    /**
     * Function to add a ctrlm_rf4ce_rib_attr_t to the RIB
     * @param attr A pointer to the RIB attribute
     * @return True if successfully added, else False 
     */
    virtual bool add_attribute(ctrlm_rf4ce_rib_attr_t *attr);
    /**
     * Function to remove a ctrlm_rf4ce_rib_attr_t to the RIB
     * @param attr A pointer to the RIB attribute
     * @return True if successfully removed, else False 
     */
    virtual bool remove_attribute(ctrlm_rf4ce_rib_attr_t *attr);

public:
    /**
     * Function to read a specific RIB attribute
     * @param accessor The type that is trying to access the RIB attribute
     * @param identifier The identifier for the RIB attribute
     * @param index The index of the RIB attribute
     * @param data The data buffer that the data should be written to.
     * @param length A pointer to a length variable which contains the length of the buffer.
     * @return The appropriete ctrlm_rf4ce_rib_t::status for the read. If successful, the length of written data will be placed in length 
     */
    virtual status read_attribute(ctrlm_rf4ce_rib_attr_t::access accessor, uint8_t identifier, uint8_t index, char *data, size_t *length);
    /**
     * Function to write a specific RIB attribute
     * @param accessor The type that is trying to access the RIB attribute
     * @param identifier The identifier for the RIB attribute
     * @param index The index of the RIB attribute
     * @param data The data buffer that the data should be read from.
     * @param length The length of the buffer.
     * @param export_api The API used to export the RIB attribute
     * @param importing A variable signalling if we are currently importing attributes
     * @return The appropriete ctrlm_rf4ce_rib_t::status for the write.
     */
    virtual status write_attribute(ctrlm_rf4ce_rib_attr_t::access accessor, uint8_t identifier, uint8_t index, char *data, size_t length, rf4ce_rib_export_api_t *export_api = NULL, bool importing = false);

public:
    /**
     * Helper function to translate a status to a string
     * @param s The status enum
     * @return The string associated with status s
     */
    static std::string status_str(status s);

private:
    ctrlm_rf4ce_rib_attr_t *rib[IDENTIFIER_MAX][INDEX_MAX]; // Using 2D array as it takes up more space, but the lookup times are fast which is needed when getting data for controller.
};

#endif