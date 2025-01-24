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
#ifndef CTRLM_VENDOR_NETWORK_FACTORY_H_
#define CTRLM_VENDOR_NETWORK_FACTORY_H_

#include <map>
#include <jansson.h>
#include <stdint.h>
#include <glib.h>
#include "ctrlm_hal.h"
#include "ctrlm_network.h"

typedef struct {
   uint32_t ignore_mask;
   gboolean mask_key_codes;
} vendor_network_opts_t;

void ctrlm_ip_network_create(void);

// ctrlm_vendor_network_factory() is called by ctrlm to initialize networks
typedef std::map<ctrlm_network_id_t, ctrlm_obj_network_t *> networks_map_t;
int ctrlm_vendor_network_factory(vendor_network_opts_t *opts, json_t *json_config_root, networks_map_t& networks);

// vendors should add to chain network factory function their with this call
typedef int (ctrlm_vendor_network_factory_func_t)(vendor_network_opts_t *opts, json_t *json_config_root, networks_map_t& networks);
void ctrlm_vendor_network_factory_func_add(ctrlm_vendor_network_factory_func_t* func);

#endif /* CTRLM_VENDOR_NETWORK_FACTORY_H_ */
