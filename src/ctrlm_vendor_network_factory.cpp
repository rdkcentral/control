/*
 * If not stated otherwise in this file or this component's LICENSE file
 * the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#include <glib.h>
#include <vector>
#include "ctrlm_vendor_network_factory.h"
#include "ctrlm_log.h"

// vector contains vendor's network factory functions
// vector is declared as pointer, because vendor's network factory initializers objects
// constructors might be called before vector constructor if declare it as object
typedef std::vector<ctrlm_vendor_network_factory_func_t*> ctrlm_vendor_network_factory_func_chain_t;
static ctrlm_vendor_network_factory_func_chain_t* ctrlm_vendor_network_factory_func_chain = 0;

static class factory_chain_garbage_collector {
  public:
  ~factory_chain_garbage_collector(){
      if (0 != ctrlm_vendor_network_factory_func_chain){
         delete ctrlm_vendor_network_factory_func_chain;
         ctrlm_vendor_network_factory_func_chain = 0;
      }
  }
} _garbage_collector;;

// vendors should add to chain network factory function their with this call
void ctrlm_vendor_network_factory_func_add(ctrlm_vendor_network_factory_func_t* func) {
  if (0 == ctrlm_vendor_network_factory_func_chain){
    ctrlm_vendor_network_factory_func_chain = new ctrlm_vendor_network_factory_func_chain_t;
  }
  ctrlm_vendor_network_factory_func_chain->push_back(func);
}


// by the time of this function call, ctrlm_vendor_network_factory_func_chain will be initialized with
// vendor factory functions
int ctrlm_vendor_network_factory(vendor_network_opts_t *opts, json_t *json_config_root, networks_map_t& networks) {
  if (0 == ctrlm_vendor_network_factory_func_chain){
    return 0;
  }
  for (auto factory_func : *ctrlm_vendor_network_factory_func_chain){
    factory_func(opts, json_config_root, networks);
  }
  return networks.size();
}
