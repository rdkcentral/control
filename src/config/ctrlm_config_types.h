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
#ifndef __CTRLM_CONFIG_INT_H__
#define __CTRLM_CONFIG_INT_H__
#include <string>
#include <limits.h>

/**
 * @brief ControlMgr Config Object
 * 
 * This class is the base object for all config objects. This should not be used by itself.
 */
class ctrlm_config_obj_t {
public:
    /**
     * ControlMgr Config Object Constructor
     * @param path The period seperated string which describes the objects path in the JSON config file
     */
    ctrlm_config_obj_t(const std::string &path);
    /**
     * ControlMgr Config Object Destructor
     */
    virtual ~ctrlm_config_obj_t();

protected:
    std::string path;
};

/**
 * @brief ControlMgr Config Integer
 * 
 * This class helps retrieve an integer from the config file
 */
class ctrlm_config_int_t : public ctrlm_config_obj_t {
public:
    /**
     * ControlMgr Config Integer Constructor
     * @param path The period seperated string which describes the objects path in the JSON config file
     */
    ctrlm_config_int_t(const std::string &path);
    /**
     * ControlMgr Config Integer Destructor
     */
    virtual ~ctrlm_config_int_t();
    
public:
    /**
     * Function that is used to get the value from the config file
     * @param value The reference to where the value from the config file should be placed
     * @param min The minimum value that should be allowed
     * @param max The maximum value that should be allowed
     * @return True if the value has been placed in value, otherwise False
     */
    bool get_config_value(int &value, int min = INT_MIN, int max = INT_MAX);
    /**
     * Function template for all int types that is used to get the value from the config file
     * @param value The reference to where the value from the config file should be placed
     * @param min The minimum value that should be allowed
     * @param max The maximum value that should be allowed
     * @return True if the value has been placed in value, otherwise False
     */
    template <typename T>
    bool get_config_value(T &value, int min = INT_MIN, int max = INT_MAX) {
         int temp = 0;
         if(get_config_value(temp, min, max)) {
            value = (T)temp;
            return true;
         }
         return false;
    }
};

class ctrlm_config_string_t : public ctrlm_config_obj_t {
public:
    /**
     * ControlMgr Config String Constructor
     * @param path The period seperated string which describes the objects path in the JSON config file
     */
    ctrlm_config_string_t(const std::string &path);
    /**
     * ControlMgr Config String Destructor
     */
    virtual ~ctrlm_config_string_t();
    
public:
    /**
     * Function that is used to get the value from the config file
     * @param value The reference to where the value from the config file should be placed
     * @return True if the value has been placed in value, otherwise False
     */
    bool get_config_value(std::string &value);
};

class ctrlm_config_bool_t : public ctrlm_config_obj_t {
public:
    /**
     * ControlMgr Config Boolean Constructor
     * @param path The period seperated string which describes the objects path in the JSON config file
     */
    ctrlm_config_bool_t(const std::string &path);
    /**
     * ControlMgr Config Boolean Destructor
     */
    virtual ~ctrlm_config_bool_t();
    
public:
    /**
     * Function that is used to get the value from the config file
     * @param value The reference to where the value from the config file should be placed
     * @return True if the value has been placed in value, otherwise False
     */
    bool get_config_value(bool &value);
};


#endif
