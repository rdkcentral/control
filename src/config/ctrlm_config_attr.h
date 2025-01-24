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
#ifndef __CTRLM_CONFIG_ATTR_H__
#define __CTRLM_CONFIG_ATTR_H__
#include <string>

/**
 * @brief ControlMgr Config Attribute
 * 
 * This class is the interface for Config File Attributes
 */
class ctrlm_config_attr_t {
public:
    /**
     * ControlMgr Config Attribute Constructor
     * @param path The period seperated string which describes the objects path in the JSON config file
     */
    ctrlm_config_attr_t(const std::string &path);
    /**
     * ControlMgr Config Attribute Destructor
     */
    virtual ~ctrlm_config_attr_t();

public:
    /**
     * Interface for class extensions to implement reading their config
     * @return True if the value was read from the config, otherwise False
     */
    virtual bool read_config() = 0;

protected:
    std::string path;
};


#endif
