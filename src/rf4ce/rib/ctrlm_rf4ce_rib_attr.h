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
#ifndef __CTRLM_RF4CE_RIB_ATTR_H__
#define __CTRLM_RF4CE_RIB_ATTR_H__
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <functional>

/// Used to signify that a RIB attribute implemented all of the indexes for it's identifier
#define RIB_ATTR_INDEX_ALL     (0x0100)
#define RIB_ATTR_INDEX_INVALID (0xFFFF)

typedef uint8_t  rf4ce_rib_attr_identifier_t;
typedef uint16_t rf4ce_rib_attr_index_t;
typedef std::function<void(uint8_t, uint8_t, unsigned char *, unsigned char)> rf4ce_rib_export_api_t;

/**
 * @brief ControlMgr RF4CE RIB Attribute
 * 
 * This class is the interface for RF4CE RIB Attributes
 */
class ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * This is an enum which represents the status of an attribute read/write 
     */
    enum status {
        SUCCESS,
        WRONG_SIZE,
        INVALID_PARAM,
        FAILURE,
        NOT_IMPLEMENTED
    };
    /**
     * This is an enum which represents the type of RIB access     * 
     */
    enum access {
        TARGET,
        CONTROLLER
    };

public:
    /**
     * This is an enum which represents the 3 permission states for RIB accesses
     */
    enum permission {
        PERMISSION_TARGET,
        PERMISSION_CONTROLLER,
        PERMISSION_BOTH
    };
    /**
     * ControlMgr RF4CE RIB Attribute Constructor
     * @param identifier The RF4CE RIB identifier for the attribute
     * @param index The RF4CE RIB index for the attribute (uint8_t or special defined index)
     * @param read_permisson The permissions for reading this attribute via RIB
     * @param write_permisson The permissions for writing this attribute via RIB
     */
    ctrlm_rf4ce_rib_attr_t(rf4ce_rib_attr_identifier_t identifier = 0xFF, rf4ce_rib_attr_index_t index = 0xFF, permission read_permission = permission::PERMISSION_BOTH, permission write_permission = permission::PERMISSION_BOTH);

public:
    /**
     * ControlMgr RF4CE RIB Attribute Destructor
     */
    virtual ~ctrlm_rf4ce_rib_attr_t();

public:
    /**
     * Getter for the RIB identifier associated with the attribute
     * @return The RIB identifier associated with the attribute
     */
    rf4ce_rib_attr_identifier_t get_identifier() const;
    /**
     * Getter for the RIB index associated with the attribute
     * @return The RIB index associated with the attribute
     */
    rf4ce_rib_attr_index_t get_index() const;
    /**
     * Setter for the RIB identifier associated with the attribute
     * @param identifier The RIB identifier associated with the attribute
     */
    void set_identifier(rf4ce_rib_attr_identifier_t identifier);
    /**
     * Setter for the RIB index associated with the attribute
     * @param index The RIB index associated with the attribute
     */
    void set_index(rf4ce_rib_attr_index_t index);

public:
    /**
     * Function to check if the accessor is allowed to read attribute
     * @param accessor The type which is trying to access the attribute
     * @return True if correct permission, else False 
     */
    bool can_read(access accessor);
    /**
     * Interface function that the attribute will implement for a RIB read
     * @param accessor The device (controller or target) trying to access the attribute
     * @param index The index being read (some attributes implement multiple indexes)
     * @param data A buffer to place the attribute data
     * @param length A pointer to the length of the buffer. The value will be changed
     *               to the number of bytes placed in data on successful return
     * @return A status for the RIB read
     */
    virtual status read_rib(access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len);
    /**
     * Function to check if the accessor is allowed to write attribute
     * @param accessor The type which is trying to access the attribute
     * @return True if correct permission, else False 
     */
    bool can_write(access accessor);
    /**
     * Interface function that the attribute will implement for a RIB write
     * @param accessor The device (controller or target) trying to access the attribute
     * @param index The index being written (some attributes implement multiple indexes)
     * @param data A buffer to which contains the attribute data
     * @param length The length of the buffer
     * @param importing A variable signaling whether the write is due to an import of a controller (ONLY IMPORTABLE ATTRIBUTES CARE ABOUT THIS)
     * @return A status for the RIB read
     */
    virtual status write_rib(access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing = false);
    /**
     * Interface function that the attribute will implement exporting the attribute to the HAL
     * @param export_api The function used to export the data to the HAL
     */
    virtual void export_rib(rf4ce_rib_export_api_t export_api);

protected:
    /**
     * Function to compare an accessor with a specific permission
     * @param accessor The type trying to access the attribute
     * @param permission The permission to compare against
     * @return True if the accessor meets the permission, else False
     */
    static bool can_access(access a, permission p);

public:
    /**
     * Function to acquire string for specific permission
     * @return The permission string
     */
    static std::string permission_str(permission p);
    /**
     * Function to acquire string for specific status
     * @return The status string
     */
    static std::string status_str(status s);
    /**
     * Function to acquire string for access
     * @return The access string
     */
    static std::string access_str(access a);

private:
    rf4ce_rib_attr_identifier_t identifier;
    rf4ce_rib_attr_index_t      index;
    permission             read_permission;
    permission             write_permission;
};

#endif