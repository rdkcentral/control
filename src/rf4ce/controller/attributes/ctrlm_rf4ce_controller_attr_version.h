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
#ifndef __CTRLM_RF4CE_ATTR_VERSION_H__
#define __CTRLM_RF4CE_ATTR_VERSION_H__
#include "ctrlm_version.h"
#include "ctrlm_database.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"

class ctrlm_obj_controller_rf4ce_t;

/**
 * @brief ControlMgr RF4CE Software Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_sw_version_t : public ctrlm_sw_version_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * ControlMgr RF4CE Software Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_sw_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Software Version Destructor
     */
    virtual ~ctrlm_rf4ce_sw_version_t();

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

public:
    /**
     * Function used to set whether or not to export this attribute
     * @param set_export True if the attribute should be exported, otherwise False
     */
    void set_export(bool set_export);

protected:
    /**
     * Function used by read_rib & export_rib to put attribute data into buffer
     * @param data The buffer to place the data -- this function assumes data is a valid pointer
     * @param len The length of the buffer
     * @return True if successfully placed data in buffer, otherwise False
     */
    bool to_buffer(char *data, size_t len);

private:
    bool exportable;
};

/**
 * @brief ControlMgr RF4CE DSP Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_dsp_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE DSP Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_dsp_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE DSP Version Destructor
     */
    virtual ~ctrlm_rf4ce_dsp_version_t();
};

/**
 * @brief ControlMgr RF4CE Keyword Model Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_keyword_model_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE Keyword Model Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_keyword_model_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Keyword Model Version Destructor
     */
    virtual ~ctrlm_rf4ce_keyword_model_version_t();
};

/**
 * @brief ControlMgr RF4CE ARM Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_arm_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE ARM Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_arm_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE ARM Version Destructor
     */
    virtual ~ctrlm_rf4ce_arm_version_t();
};

/**
 * @brief ControlMgr RF4CE IRDB Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_irdb_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE IRDB Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_irdb_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE IRDB Version Destructor
     */
    virtual ~ctrlm_rf4ce_irdb_version_t();
};

/**
 * @brief ControlMgr RF4CE Bootloader Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_bootloader_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE Bootloader Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_bootloader_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Bootloader Version Destructor
     */
    virtual ~ctrlm_rf4ce_bootloader_version_t();
};

/**
 * @brief ControlMgr RF4CE Golden Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_golden_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE Golden Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_golden_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Golden Version Destructor
     */
    virtual ~ctrlm_rf4ce_golden_version_t();
};

/**
 * @brief ControlMgr RF4CE Audio Data Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_audio_data_version_t : public ctrlm_rf4ce_sw_version_t {
public:
    /**
     * ControlMgr RF4CE Audio Data Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_audio_data_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Audio Data Version Destructor
     */
    virtual ~ctrlm_rf4ce_audio_data_version_t();
};

/**
 * @brief ControlMgr RF4CE Hardware Version Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_hw_version_t : public ctrlm_hw_version_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * ControlMgr RF4CE Hardware Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param controller The controller in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_hw_version_t(ctrlm_obj_network_t *net = NULL, ctrlm_obj_controller_rf4ce_t *controller = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Hardware Version Destructor
     */
    virtual ~ctrlm_rf4ce_hw_version_t();

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

public:
    /**
     * Function which gets a manufacturer name from the manufacturer field
     * @return A string containing the manufacturer name
     */
    std::string get_manufacturer_name() const;
    /**
     * Function to assume controller product name from hardware version (USED FOR LEGACY UPGRADE CODE ONLY)
     */
    static const char *rf4cemgr_product_name(ctrlm_hw_version_t *version);


protected:
    /**
     * Function to check if hardware version is valid
     * @return True if the hardware version is valid, else False
     * @todo Override this function on a per controller basis
     */
    virtual bool is_valid() const;
    /**
     * Function to fix the hardware version if invalid
     * @todo Override this function on a per controller basis 
     */
    virtual void fix_invalid_version();
    /**
     * Function used by read_rib & export_rib to put attribute data into buffer
     * @param data The buffer to place the data -- this function assumes data is a valid pointer
     * @param len The length of the buffer
     * @return True if successfully placed data in buffer, otherwise False
     */
    bool to_buffer(char *data, size_t len);

protected:
    ctrlm_obj_controller_rf4ce_t *controller;
};

/**
 * @brief ControlMgr RF4CE Software Build ID Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_sw_build_id_t : public ctrlm_sw_build_id_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
public:
    /**
     * ControlMgr RF4CE Build ID Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_sw_build_id_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE Build ID Version Destructor
     */
    virtual ~ctrlm_rf4ce_sw_build_id_t();

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
};

/**
 * @brief ControlMgr RF4CE DSP Build ID Class
 * 
 * This class is used within ControlMgr's RF4CE Network implementation
 */
class ctrlm_rf4ce_dsp_build_id_t : public ctrlm_rf4ce_sw_build_id_t {
public:
    /**
     * ControlMgr RF4CE DSP Build ID Version Constructor
     * @param net The controller's network in which this attribute belongs
     * @param id The controller's id in which this attribute belongs
     */
    ctrlm_rf4ce_dsp_build_id_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
    /**
     * ControlMgr RF4CE DSP Build ID Version Destructor
     */
    virtual ~ctrlm_rf4ce_dsp_build_id_t();
};

#endif