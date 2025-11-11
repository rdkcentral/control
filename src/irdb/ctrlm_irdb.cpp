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

#include "ctrlm_irdb.h"
#include "ctrlm_network.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#if defined(CTRLM_THUNDER)
#include "ctrlm_thunder_controller.h"
#endif

#include <algorithm>


ctrlm_irdb_ir_code_set_t::ctrlm_irdb_ir_code_set_t(ctrlm_irdb_dev_type_t type, const ctrlm_irdb_ir_entry_id_t &id) {
   type_ = type;
   id_   = id;
}

void ctrlm_irdb_ir_code_set_t::add_key(ctrlm_key_code_t key, ctrlm_irdb_ir_code_t ir_code) {
   ir_codes_[key] = std::move(ir_code);
}

std::map<ctrlm_key_code_t, ctrlm_irdb_ir_code_t> *ctrlm_irdb_ir_code_set_t::get_key_map() {
   return &ir_codes_;
}

ctrlm_irdb_dev_type_t ctrlm_irdb_ir_code_set_t::get_type() {
   return type_;
}

ctrlm_irdb_ir_entry_id_t ctrlm_irdb_ir_code_set_t::get_id() {
   return id_;
}

#if defined(CTRLM_THUNDER)
static int _on_thunder_ready_thread(void *data) {
    ctrlm_irdb_t *irdb = (ctrlm_irdb_t *)data;
    if(irdb) {
        irdb->on_thunder_ready();
    } else {
        XLOGD_ERROR("irdb is null");
    }
    return(0);
} 

static void _on_thunder_ready(void *data) {
    g_idle_add(_on_thunder_ready_thread, data);
}
#endif

ctrlm_irdb_t::ctrlm_irdb_t(ctrlm_irdb_mode_t mode, bool platform_tv) {
    this->mode        = mode;
    this->platform_tv = platform_tv;
    this->ipc         = NULL;
    this->initialized = 0;
#if defined(CTRLM_THUNDER)
    Thunder::Controller::ctrlm_thunder_controller_t *controller = Thunder::Controller::ctrlm_thunder_controller_t::getInstance();
    if(controller) {
        if(controller->is_ready()) {
            this->on_thunder_ready();
        } else {
            controller->add_ready_handler(_on_thunder_ready, (void *)this);
        }
    } else {
        XLOGD_ERROR("Thunder controller is NULL");
    }
#endif
}

ctrlm_irdb_t::~ctrlm_irdb_t() {
    if(this->ipc) {
        this->ipc->deregister_ipc();
        delete this->ipc;
        this->ipc = NULL;
    }
}

bool ctrlm_irdb_t::initialize_irdb() { 
    return(this->initialized > 0);
}

bool ctrlm_irdb_t::get_initialized() {
    return(this->initialized > 0);
}


bool ctrlm_irdb_t::_set_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_ir_code_set_t &ir_codes, ctrlm_irdb_vendor_t vendor) {
    bool ret = false;

    XLOGD_INFO("Setting IR codes for (%u, %u)", network_id, controller_id);

    ctrlm_main_queue_msg_ir_set_code_t msg = {0};

    msg.network_id         = network_id;
    msg.controller_id      = controller_id;
    msg.ir_codes           = &ir_codes;
    msg.success            = &ret;
    msg.vendor             = vendor;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_ir_set_code, &msg, sizeof(msg), NULL, network_id, true);

    return(ret);
}

bool ctrlm_irdb_t::_clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
    bool ret = false;

    XLOGD_INFO("Clearing IR codes for (%u, %u)", network_id, controller_id);

    ctrlm_main_queue_msg_ir_clear_t msg = {0};

    msg.network_id    = network_id;
    msg.controller_id = controller_id;
    msg.success       = &ret;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_ir_clear_codes, &msg, sizeof(msg), NULL, network_id, true);
    
    return(ret);
}

void ctrlm_irdb_t::normalize_string(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        switch (c) {
            case u'à': case u'á': case u'ä': case u'â': case u'ã': case u'å':
            case u'À': case u'Á': case u'Ä': case u'Â': case u'Ã': case u'Å': {
                c = 'a';
                break;
            }
            case u'é': case u'è': case u'ê': case u'ë':
            case u'È': case u'É': case u'Ê': case u'Ë': {
                c = 'e';
                break;
            }
            case u'ì': case u'í': case u'î': case u'ï':
            case u'Ì': case u'Í': case u'Î': case u'Ï': {
                c = 'i';
                break;
            }
            case u'ò': case u'ó': case u'ô': case u'õ': case u'ö': case u'ø':
            case u'Ò': case u'Ó': case u'Ô': case u'Õ': case u'Ö': case u'Ø': {
                c = 'o';
                break;
            }
            case u'ù': case u'ú': case u'û': case u'ü':
            case u'Ù': case u'Ú': case u'Û': case u'Ü': {
                c = 'u';
                break;
            }
            case u'ß': {
                c = 'b';
                break;
            }
            case u'Ñ': case u'ñ': {
                c = 'n';
                break;
            }
            default: {
                c = std::tolower(c);
                break;
            }
        }
        return(c);
    });
}
