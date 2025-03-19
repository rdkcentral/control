/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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

#ifndef JSON_CONFIG_H_
#define JSON_CONFIG_H_


#include <limits.h>
#include <string>
#include <vector>
#include <jansson.h>

class json_config {
   public:
      json_config();
      json_config(json_t* json_obj);
      virtual ~json_config();

      bool open_for_read(const char* json_file_name, const char* json_section_name);
      bool config_object_set(json_t *json_obj);

      bool config_array_get(const char* key, json_t **array) const;

      bool config_value_get(const char* key, bool& val, int index=-1) const;
      bool config_value_get(const char* key, int& val, int min_val=INT_MIN, int max_val=INT_MAX, int index=-1) const;
      bool config_value_get(const char* key, double& val, double min_val=0.0, double max_val=1.0, int index=-1) const;
      template <typename T>
      bool config_value_get(const char* key, T& val, int min_val=INT_MIN, int max_val=INT_MAX, int index=-1) const {
         int value = 0;
         if (config_value_get(key,value,min_val,max_val,index)){
            val = (T)value;
            return true;
         }
         return false;
      }
      bool config_value_get(const char* key, std::string& val) const;
      bool config_value_get(const char* key, std::vector<std::string> &val) const;
      bool config_object_get(const char* key, json_config& sub_object) const;
      json_t * current_object_get() const;

      std::string json_dump_string(void) const;

   private:
      json_t *json_root_obj;
      json_t *json_section_obj;

};

#endif /* JSON_CONFIG_H_ */
