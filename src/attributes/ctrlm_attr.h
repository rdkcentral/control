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
#ifndef __CTRLM_ATTR_H__
#define __CTRLM_ATTR_H__
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

#define COUT_HEX_MODIFIER std::setfill('0') << std::setw(2) << std::hex

/**
 * @brief ControlMgr Attribute Class
 * 
 * This interface is used for all ControlMgr attributes. This interface provides
 * methods to simply get the string name and value for the attribute.
 */
class ctrlm_attr_t {
public:
    /**
     * ControlMgr Attribute Default Constructor
     */
    ctrlm_attr_t(const std::string &name = "Unknown Attribute");
    /**
     * ControlMgr Attribute Destructor
     */
    virtual ~ctrlm_attr_t();

public:
    /**
     * Function to provide name of the attribute
     * @return The string containing the name of the attibute
     */
    std::string get_name() const;
    /**
     * Function to set name of the attribute
     * @param name The string containing the name of the attibute
     */
    void set_name(const std::string &name);
    /**
     * Function to set a prefix for the attribute name
     */
    void set_name_prefix(const std::string &prefix);

public:
    /**
     * Interface for class extensions to provide the string value of the attribute.
     * @return The string containing the value of the attribute
     */
    virtual std::string to_string() const = 0;

private:
    std::string prefix;
    std::string name;
};

#endif
