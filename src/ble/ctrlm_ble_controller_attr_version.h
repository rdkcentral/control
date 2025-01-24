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
#ifndef __CTRLM_BLE_ATTR_VERSION_H__
#define __CTRLM_BLE_ATTR_VERSION_H__
#include "ctrlm_version.h"
#include "ctrlm_database.h"

/**
 * @brief ControlMgr BLE Software Version Class
 * 
 * This class is used within ControlMgr's BLE Network implementation
 */
class ctrlm_ble_sw_version_t : public ctrlm_sw_version_t, public ctrlm_db_attr_t {
public:
    /**
     * ControlMgr BLE Software Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_ble_sw_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF, const std::string &db_key = "sw_revision");
    /**
     * ControlMgr BLE Software Version Destructor
     */
    virtual ~ctrlm_ble_sw_version_t();

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


/**
 * @brief ControlMgr BLE Hardware Version Class
 * 
 * This class is used within ControlMgr's BLE Network implementation
 */
class ctrlm_ble_hw_version_t : public ctrlm_hw_version_t, public ctrlm_db_attr_t {
public:
    /**
     * ControlMgr BLE Hardware Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_ble_hw_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF, const std::string &db_key = "hw_revision");
    /**
     * ControlMgr BLE Hardware Version Destructor
     */
    virtual ~ctrlm_ble_hw_version_t();

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

#endif
