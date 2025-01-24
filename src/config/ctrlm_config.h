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
#ifndef __CTRLM_CONFIG_H__
#define __CTRLM_CONFIG_H__
#include <jansson.h>
#include <string>
#include "ctrlm.h"

/**
 * @brief ControlMgr Configuration File Class
 * 
 * This class is used within ControlMgr to interact with a configuration file
 */
class ctrlm_config_t {
public:
    /**
     * This function is used to get the ControlMgr Configuration instance, as it is a Singleton.
     * @return The instance of the ControlMgr Configuration object, or NULL on error.
     */
    static ctrlm_config_t *get_instance();
    /**
     * This function is used to destroy the ControlMgr Configuration instance, as it is a Singleton.
     */
    static void destroy_instance();
    /**
     * ControlMgr Configuration Destructor
     */
    virtual ~ctrlm_config_t();

public:
    /**
     * Function which loads a configuration file into the configuration object.
     * @param file_path The path to the configuration file
     * @return True on success, otherwise False
     */
    bool load_config(const std::string &file_path);
    /**
     * Function to check if object exists from a path
     * @param path A period seperated string used to navigate a JSON object i.e. "network_rf4ce.polling.enabled"
     * @return True if the object exists, else False
     */
    bool path_exists(const std::string &path);
    /**
     * Function which is used to get a Jansson JSON object from a path.
     * @param path A period seperated string used to navigate a JSON object i.e. "network_rf4ce.polling.enabled"
     * @return The json_t* object acquired via the path, or NULL if not found
     */
    json_t *json_from_path(const std::string &path, bool add_ref = false);
    /**
     * Function which is used to get a string from a path.
     * @param path A period seperated string used to navigate a JSON object i.e. "network_rf4ce.polling.enabled"
     * @return The decoded string of the object acquired via the path, or NULL if not found
     */
    std::string string_from_path(const std::string &path);

private:
    /**
     * ControlMgr Configuration Default Constructor (Private due to it being a Singleton)
     */
    ctrlm_config_t();

private:
    json_t     *root;
};

#endif
