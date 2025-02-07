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
#ifndef _CTRLM_IR_CONTROLLER_H_
#define _CTRLM_IR_CONTROLLER_H_


#include <string>
#include <xr_mq.h>
#include <ctrlm_utils.h>
#include <ctrlm_attr_general.h>
#include <ctrlm_controller.h>


/**
 * @brief ControlMgr IR Controller Class
 * 
 * This class is a singleton that manages the IR controller
 */
class ctrlm_ir_controller_t {
public:

   /**
    * This function is used to get the IR Controller instance, as it is a Singleton.
    * @return The instance of the IR Controller, or NULL on error.
    */
   static ctrlm_ir_controller_t *get_instance();
   /**
    * This function is used to destroy the sole instance of the IR Controller object. 
    */
   static void destroy_instance();

   virtual ~ctrlm_ir_controller_t();
   

   /**
    * This function is used to read the IR section of the ctrlm config. 
    */
   bool read_config();

   void print_status();

   std::string input_device_name_get(void);
   std::string name_get(void);

   /**
    * These functions are used to mask key codes in log messages. 
    * Key codes are masked in production builds to avoid printing PII
    * like usernames and passwords
    */
   void mask_key_codes_set(bool mask_key_codes);
   bool mask_key_codes_get() const;


   time_t last_key_time_get();
   void last_key_time_set(time_t val);
   uint16_t last_key_code_get();
   void last_key_code_set(uint16_t val);
   void last_key_time_update();

   void process_event_key(uint16_t key_code);

   xr_mq_t key_thread_msgq_get() const;
   void thread_poll(void *data);
   /**
    * This function is used to set the database table name where params will be stored. 
    */
   void db_table_set(const std::string &table);
   void db_load();
   void db_store();

   void set_scan_code(int code);

private:
   /**
    *  Default Constructor (Private due to it being a Singleton)
    */
   ctrlm_ir_controller_t();

   std::string                             input_device_name_;

   std::shared_ptr<ctrlm_uint64_db_attr_t> last_key_time_;
   std::shared_ptr<ctrlm_uint64_db_attr_t> last_key_code_;
   time_t                                  last_key_time_flush_;

   bool                                    mask_key_codes_;

   ctrlm_thread_t                          key_thread_        = { 0 };
   xr_mq_t                                 key_thread_msgq_;

   int                                     scan_code_;
};

// End Class ctrlm_ir_controller_t

#endif
