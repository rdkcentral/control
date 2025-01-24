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
#ifndef __CTRLM_RFC_ATTR_H__
#define __CTRLM_RFC_ATTR_H__
#include <string>
#include <vector>
#include <functional>
#include <jansson.h>
#include <climits>
#include <float.h>

class ctrlm_rfc_attr_t;
/**
 * This is the callback type that will be called when an RFC attribute value has changed
 */
typedef std::function<void(const ctrlm_rfc_attr_t&)> ctrlm_rfc_attr_changed_t;

/**
 * @brief ControlMgr RFC Attribute
 * 
 * This class is the interface for RFC Attributes
 */
class ctrlm_rfc_attr_t {
public:
    /**
     * ControlMgr RFC Attribute Constructor
     * @param tr181_string The TR181 string associated with the attribute
     * @param enabled An optional parameter to enable/disable RFC for this attribute (mostly for Development)
     */
    ctrlm_rfc_attr_t(const std::string &tr181_string, const std::string &config_key, bool enabled = true);
    /**
     * ControlMgr RFC Attribute Destructor
     */
    virtual ~ctrlm_rfc_attr_t();

public:
    /**
     * Getter for the TR181 string associated with the attribute
     * @return The TR181 string associated with the attribute
     */
    std::string get_tr181_string() const;
    /**
     * Getter for the enabled value for this attribute
     * @return True if enabled, otherwise False
     */
    bool        is_enabled() const;
    /**
     * Setter for the enabled value for this attribute
     * @param enabled True if enabled, otherwise False
     */
    void        set_enabled(bool enabled);
    /**
     * Adds the callback that is called when the attributes value has changed
     * @param listener The callback
     */
    void        add_changed_listener(const ctrlm_rfc_attr_changed_t &listener);
    /**
     * Removes the callback that is called when the attributes value has changed
     * @param listener The callback
     * @param user_data Application data that the developer wants to pass to the callback
     */
    void        remove_changed_listener(const ctrlm_rfc_attr_changed_t &listener);

public:
    /**
     * Function that sets the value recieved from XCONF
     * @param value The string received from RFC
     */
    void set_rfc_value(const std::string &value);
    /**
     * Function used to retrieve JSON object for the attribute
     * 
     * @param val The pointer which will be set to the JSON object. This adds a reference to the JSON object, must call json_decref after use.
     * @return True if val is pointing to the JSON object, False otherwise.
     */
    bool get_rfc_json_value(json_t **val) const;
    /**
     * Function used to retrieve the decoded value for this atttribute
     * @param path The path to the JSON object
     * @return JSON object or NULL if value doesn't exist.
     */
    bool get_rfc_value(const std::string &path, bool &val, int index = -1) const;
    bool get_rfc_value(const std::string &path, std::string &val) const;
    bool get_rfc_value(const std::string &path, std::vector<std::string> &val) const;
    bool get_rfc_value(const std::string &path, int &val, int min = INT_MIN, int max = INT_MAX, int index = -1) const;
    bool get_rfc_value(const std::string &path, double &val, double min = -DBL_MAX, double max = DBL_MAX) const;
    template <typename T>
    bool get_rfc_value(const std::string &path, T &val, int min = INT_MIN, int max = INT_MAX, int index = -1) const {
         int value = 0;
         if(get_rfc_value(path,value,min,max,index)) {
            val = (T)value;
            return true;
         }
         return false;
    }

protected:
    /**
     * This function is called when the attribute has detected that a change in value
     * has occured, which will trigger the user callback
     */
    void notify_changed();
    /**
     * This function checks whether a specific path is defined in the config file
     * @param path The path of the config object
     * @return True if the object is defined in the config file, else False.
     */
    bool check_config_file(const std::string &path) const;

private:
    std::string tr181_string;
    std::string config_key;
    std::string value;
    json_t *value_json;
    bool enabled;
    std::vector<ctrlm_rfc_attr_changed_t> listeners;
};

#endif
