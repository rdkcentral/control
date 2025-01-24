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
#include "ctrlm_config_types.h"
#include "ctrlm_config.h"
#include "ctrlm_log.h"

// ctrlm_config_obj_t
ctrlm_config_obj_t::ctrlm_config_obj_t(const std::string &path) {
    this->path = path;
}

ctrlm_config_obj_t::~ctrlm_config_obj_t() {
    
}
// end ctrlm_config_obj_t

// ctrlm_config_int_t
ctrlm_config_int_t::ctrlm_config_int_t(const std::string &path) : ctrlm_config_obj_t(path) {

}

ctrlm_config_int_t::~ctrlm_config_int_t() {

}

bool ctrlm_config_int_t::get_config_value(int &value, int min, int max) {
    bool ret = false;
    ctrlm_config_t *config = ctrlm_config_t::get_instance();
    if(config) {
        json_t *obj = config->json_from_path(this->path);
        if(obj) {
            if(json_is_integer(obj)) {
                int temp = json_integer_value(obj);
                if(temp >= min && temp <= max) {
                    value = temp;
                    ret = true;
                } else {
                    XLOGD_WARN("config value for <%s> is out of bounds (%d, %d) (%d)", this->path.c_str(), min, max, temp);
                }
            } else {
                XLOGD_ERROR("config value for <%s> is not an integer", this->path.c_str());
            }
        } else {
            XLOGD_WARN("config value for <%s> was not found", this->path.c_str());
        }
    } else {
        XLOGD_ERROR("ctrlm_config_t is NULL");
    }
    return(ret);
}
// end ctrlm_config_int_t

// ctrlm_config_string_t
ctrlm_config_string_t::ctrlm_config_string_t(const std::string &path) : ctrlm_config_obj_t(path) {

}

ctrlm_config_string_t::~ctrlm_config_string_t() {

}

bool ctrlm_config_string_t::get_config_value(std::string &value) {
    bool ret = false;
    ctrlm_config_t *config = ctrlm_config_t::get_instance();
    if(config) {
        json_t *obj = config->json_from_path(this->path);
        if(obj) {
            if(json_is_string(obj)) {
                value = json_string_value(obj);
                ret = true;
            } else {
                XLOGD_ERROR("config value for <%s> is not a string", this->path.c_str());
            }
        } else {
            XLOGD_WARN("config value for <%s> was not found", this->path.c_str());
        }
    } else {
        XLOGD_ERROR("ctrlm_config_t is NULL");
    }
    return(ret);
}
// end ctrlm_config_string_t

// ctrlm_config_bool_t
ctrlm_config_bool_t::ctrlm_config_bool_t(const std::string &path) : ctrlm_config_obj_t(path) {

}

ctrlm_config_bool_t::~ctrlm_config_bool_t() {

}

bool ctrlm_config_bool_t::get_config_value(bool &value) {
    bool ret = false;
    ctrlm_config_t *config = ctrlm_config_t::get_instance();
    if(config) {
        json_t *obj = config->json_from_path(this->path);
        if(obj) {
            if(json_is_boolean(obj)) {
                if(json_is_true(obj)) {
                    value = true;
                } else {
                    value = false;
                }
                ret = true;
            } else {
                XLOGD_ERROR("config value for <%s> is not a boolean", this->path.c_str());
            }
        } else {
            XLOGD_WARN("config value for <%s> was not found", this->path.c_str());
        }
    } else {
        XLOGD_ERROR("ctrlm_config_t is NULL");
    }
    return(ret);
}
// end ctrlm_config_bool_t
