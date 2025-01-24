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
#include "../ctrlm_vendor_network_factory.h"
#include "../ctrlm_log.h"


int ctrlm_vendor_network_factory_stub(unsigned long ignore_mask, json_t *json_config_root, networks_map_t& networks) {
   XLOGD_INFO("STUB");
   int networks_added = 0;
   #ifdef CTRLM_NETWORK_STUB
   // add DSP network if enabled
   if ( !(ignore_mask & (1 << CTRLM_NETWORK_TYPE_STUB)) ) {
      ctrlm_network_id_t network_id = network_id_get_next(CTRLM_NETWORK_TYPE_STUB);
      networks[network_id] = new ctrlm_stub_network_t(network_id, g_thread_self());
      ++networks_added;
   }
   #endif

   return networks_added;
}


// Add ctrlm_stub_network_factory to ctrlm_vendor_network_factory_func_chain automatically during init time
static class ctrlm_stub_network_factory_obj_t {
  public:
    ctrlm_stub_network_factory_obj_t(){
      ctrlm_vendor_network_factory_func_add(ctrlm_stub_network_factory);
    }
} _factory_obj;
