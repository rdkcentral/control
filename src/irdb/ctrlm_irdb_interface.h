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

#ifndef __CTRLM_IRDB_INTERFACE_H__
#define __CTRLM_IRDB_INTERFACE_H__

#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <mutex>
#include <semaphore.h>
#include <string>

#include "ctrlm_hal.h"
#include "ctrlm_irdb_plugin.h"


inline bool operator==(const ctrlm_irdb_autolookup_entry_ranked_t& lhs, const ctrlm_irdb_autolookup_entry_ranked_t& rhs) {
      return (lhs.id.compare(rhs.id) == 0);
}
   
typedef std::map<ctrlm_irdb_dev_type_t,ctrlm_irdb_autolookup_ranked_list_t> ctrlm_autolookup_ranked_list_by_type_t;

typedef struct {
   ctrlm_network_id_t         network_id;
   ctrlm_controller_id_t      controller_id;
   ctrlm_irdb_ir_code_set_t * ir_codes;
   bool *                     success;
   sem_t *                    semaphore;
   ctrlm_irdb_vendor_info_t   vendor_info;
} ctrlm_main_queue_msg_program_ir_codes_t;

typedef struct {
   ctrlm_network_id_t    network_id;
   ctrlm_controller_id_t controller_id;
   bool *                success;
   sem_t *               semaphore;
} ctrlm_main_queue_msg_ir_clear_t;

class ctrlm_irdb_interface_t {
public:

   /**
    * This function is used to get the IRDB interface instance, as it is a Singleton.
    * @return The instance of the IRDB interface, or NULL on error.
    */
   static ctrlm_irdb_interface_t *get_instance(bool platform_tv = false);
   /**
    * This function is used to destroy the sole instance of the IRDB interface object. 
    */
   static void destroy_instance();

   virtual ~ctrlm_irdb_interface_t();

   bool get_vendor_info(ctrlm_irdb_vendor_info_t &info);
   bool get_manufacturers(ctrlm_irdb_manufacturer_list_t &manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix = "");
   bool get_models(ctrlm_irdb_model_list_t &models, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &prefix = "");
   bool get_irdb_entry_ids(ctrlm_irdb_entry_id_list_t &codes, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &model = "");
   bool program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_dev_type_t type, const std::string &name);
   bool clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
   
   bool get_ir_codes_by_autolookup(ctrlm_autolookup_ranked_list_by_type_t &codes);
   bool initialize_irdb();
   void on_thunder_ready();
    
private:

   /**
    *  Default Constructor (Private due to it being a Singleton)
    */
   ctrlm_irdb_interface_t();
   ctrlm_irdb_interface_t(bool platform_tv);

   bool lock_mutex();
   void unlock_mutex();

   bool open_plugin();
   bool close_plugin();

   bool _program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_ir_code_set_t *ir_codes);
   bool _clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

   void *m_irdbPluginHandle;

   ctrlm_irdb_mode_t mode;
   bool              m_platform_tv;
   std::timed_mutex  m_mutex;
};

#endif
