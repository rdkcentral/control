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
#include "json_config.h"
#include "ctrlm_log.h"
#include <stdexcept>

json_config::json_config() : json_root_obj(0),
                             json_section_obj(0) {
}

json_config::json_config(json_t *json_obj) : json_root_obj(0) {
   if(json_obj == 0 || !json_is_object(json_obj)) {
       json_section_obj = 0;
   }
   json_section_obj = json_obj;
}

json_config::~json_config() {
   json_decref(json_root_obj);
}

bool json_config::open_for_read(const char* json_file_name, const char* json_section_name){

   json_error_t json_error;
   json_root_obj = json_load_file(json_file_name, JSON_REJECT_DUPLICATES,  &json_error);
   if (json_root_obj == 0) {
      XLOGD_INFO("Cannot open %-25s for read", json_file_name);
      return false;
   }
   json_section_obj = json_object_get(json_root_obj, json_section_name);
   if(json_section_obj == NULL || !json_is_object(json_section_obj)) {
      XLOGD_WARN("lson object %s not found", json_section_name);
      return false;
   }
   return true;
}

bool json_config::config_object_set(json_t *json_obj){
   if(json_obj == 0 || !json_is_object(json_obj)) {
      XLOGD_INFO("use default configuration");
      return false;
   }
   json_section_obj = json_obj;
   return true;
}

bool json_config::config_array_get(const char* key, json_t **array) const {
   if (!array) {
      XLOGD_ERROR("Array is nullptr", key);
      return false;
   }

   *array = json_object_get(json_section_obj, key);
   if (!*array || !json_is_array(*array)) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }

   XLOGD_INFO("%-25s - PRESENT", key);
   return true;
}

bool json_config::config_value_get(const char* key, bool& val, int index) const {

   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj != 0 && index >= 0) { // Handle array index
      if(!json_is_array(json_obj)) {
         XLOGD_ERROR("%-25s - not an array", key);
         json_obj = 0;
      } else {
         json_t *json_element = json_array_get(json_obj, index);
         if(json_element == 0) {
            XLOGD_ERROR("%-25s - array index not found.  index <%u> size <%u>", key, index, json_array_size(json_obj));
         }
         json_obj = json_element;
      }
   }

   if(json_obj == 0 || !json_is_boolean(json_obj)) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }
   val = json_is_true(json_obj);
   XLOGD_INFO("%-25s - PRESENT <%s>", key, val ? "true" : "false");
   return true;
}

bool json_config::config_value_get(const char* key, int& val, int min_val, int max_val, int index) const {

   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj != 0 && index >= 0) { // Handle array index
      if(!json_is_array(json_obj)) {
         XLOGD_ERROR("%-25s - not an array", key);
         json_obj = 0;
      } else {
         json_t *json_element = json_array_get(json_obj, index);
         if(json_element == 0) {
            XLOGD_ERROR("%-25s - array index not found.  index <%u> size <%u>", key, index, json_array_size(json_obj));
         }
         json_obj = json_element;
      }
   }

   if(json_obj == 0 || (!json_is_integer(json_obj) && !json_is_boolean(json_obj) && !json_is_string(json_obj))) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }
   // handle gboolean or integer boolean types
   if (json_is_boolean(json_obj)){
      bool value=false;
      if (config_value_get(key, value)){
         val=value;
         return true;
      }
      return false;
   }

   int value = 0;
   if (json_is_string(json_obj)) {
      try {
         value = std::stoi(json_string_value(json_obj));
      } catch(std::invalid_argument &e) {
         XLOGD_ERROR("std::invalid_argument: %s", e.what());
         return false;
      } catch(std::out_of_range &e) {
         XLOGD_ERROR("std::out_of_range: %s", e.what());
         return false;
      }
   } else {
      value = static_cast<int>(json_integer_value(json_obj));
   }
   XLOGD_INFO("%-25s - PRESENT <%d>", key, value);

   if(value < min_val || value > max_val) {
      XLOGD_INFO("%-25s - OUT OF RANGE %d", key, value);
      return false;
   }
   val = value;
   return true;
}

bool json_config::config_value_get(const char* key, double& val, double min_val, double max_val, int index) const {

   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj != 0 && index >= 0) { // Handle array index
      if(!json_is_array(json_obj)) {
         XLOGD_ERROR("%-25s - not an array", key);
         json_obj = 0;
      } else {
         json_t *json_element = json_array_get(json_obj, index);
         if(json_element == 0) {
            XLOGD_ERROR("%-25s - array index not found.  index <%u> size <%u>", key, index, json_array_size(json_obj));
         }
         json_obj = json_element;
      }
   }

   if(json_obj == 0 || !json_is_real(json_obj) ) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }
   // handle real number types
   double value = json_real_value(json_obj);
   XLOGD_INFO("%-25s - PRESENT <%f>", key, value);
   if(value < min_val || value > max_val) {
      XLOGD_INFO("%-25s - OUT OF RANGE %f", key, value);
      return false;
   }
   val = value;
   return true;
}

bool json_config::config_value_get(const char* key, std::string& val) const{

   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj == 0 || !json_is_string(json_obj)) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }
   val = json_string_value(json_obj);
   XLOGD_INFO("%-25s - PRESENT <%s>", key, val.c_str());
   return true;
}

bool json_config::config_value_get(const char* key, std::vector<std::string> &val) const {
   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj == NULL || !json_is_array(json_obj)) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return(false);
   }

   XLOGD_INFO("%-25s - PRESENT", key);

   size_t index  = 0;
   json_t *value = NULL;

   // add new contents
   json_array_foreach(json_obj, index, value) {
      if(json_is_string(value)) {
          val.push_back(std::string(json_string_value(value)));
      } else {
          XLOGD_ERROR("not a string, skipping..");
      }
   }

   return(true);
}

bool json_config::config_object_get(const char* key, json_config& config_object) const {

   json_t *json_obj = json_object_get(json_section_obj, key);
   if(json_obj == 0 || !json_is_object(json_obj)) {
      XLOGD_INFO("%-25s - ABSENT", key);
      return false;
   }
   XLOGD_INFO("%-25s - PRESENT", key);
   (void)config_object.config_object_set(json_obj);
   return true;
}

json_t* json_config::current_object_get() const {
   return json_section_obj;
}

std::string json_config::json_dump_string(void) const {
   return std::string(json_dumps(json_section_obj, JSON_COMPACT));
}
