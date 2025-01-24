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
#ifndef __CTRLM_RF4CE_NETWORK_ATTR_CONFIG_H__
#define __CTRLM_RF4CE_NETWORK_ATTR_CONFIG_H__
#include "ctrlm_attr_general.h"
#include "ctrlm_db_attr.h"
#include "ctrlm_rfc.h"
#include "rf4ce/rib/ctrlm_rf4ce_rib_attr.h"
#include "ctrlm_version.h"
#include "ctrlm_config_attr.h"
#include <vector>
#include <map>
#include <functional>

/**
 * @brief ControlMgr RF4CE Controller Response Time 
 * 
 * This class is used to handle the response time window for controllers
 */
class ctrlm_rf4ce_rsp_time_t;
typedef std::function<void(const ctrlm_rf4ce_rsp_time_t&)> ctrlm_rf4ce_rsp_time_listener_t;

class ctrlm_rf4ce_rsp_time_t : public ctrlm_attr_t, public ctrlm_rf4ce_rib_attr_t, public ctrlm_config_attr_t {
public:
    /**
     * ControlMgr RF4CE Controller Response Time Constructor
     */
    ctrlm_rf4ce_rsp_time_t();
    /**
     * ControlMgr RF4CE Controller Response Time Destructor
     */
    virtual ~ctrlm_rf4ce_rsp_time_t();

public:
    void rfc_value_retrieved(const ctrlm_rfc_attr_t &attr);
    void set_updated_listener(ctrlm_rf4ce_rsp_time_listener_t listener);

public:
    unsigned int get_ms(uint8_t profile_id) const;
    unsigned int get_us(uint8_t profile_id) const;
    void         legacy_rfc();

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the Memory Dump info
     * @see ctrlm_attr_t::to_string
     */
    virtual std::string to_string() const;

public:
    // ONLY NEED TO IMPLEMENT RIB READ, RIB WRITE NOT IMPLEMENTED
    /**
     * Interface implementation for a RIB read
     * @see ctrlm_rf4ce_rib_attr_t::read_rib
     */
    virtual ctrlm_rf4ce_rib_attr_t::status read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len);

public:
    /**
     * Interface implementation to read config values
     * @see ctrlm_config_attr_t::read_config
     */
    virtual bool read_config();

protected:
    std::map<uint8_t, unsigned int> rsp_times;
    ctrlm_rf4ce_rsp_time_listener_t updated_listener;
};
#endif