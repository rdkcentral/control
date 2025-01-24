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
#include <algorithm>
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#if defined(CTRLM_THUNDER)
// STB Platforms
#include "ctrlm_thunder_controller.h"
#include "ctrlm_thunder_plugin_display_settings.h"
#include "ctrlm_thunder_plugin_cec.h"
#include "ctrlm_thunder_plugin_cec_source.h"
// TV Platforms
#include "ctrlm_thunder_plugin_hdmi_input.h"
#include "ctrlm_thunder_plugin_cec_sink.h"
#endif

typedef struct {
   #ifdef CTRLM_THUNDER
      #ifdef IRDB_HDMI_DISCOVERY
      Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t *display_settings;
      #endif
      #ifdef IRDB_CEC_DISCOVERY
         #ifdef IRDB_CEC_FLEX2
         Thunder::CEC::ctrlm_thunder_plugin_cec_source_t                *cec;
         #else
         Thunder::CEC::ctrlm_thunder_plugin_cec_t                       *cec;
         #endif
      #endif
      #ifdef IRDB_HDMI_DISCOVERY
      Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t             *hdmi_input;
      #endif
      #ifdef IRDB_CEC_DISCOVERY
      Thunder::CECSink::ctrlm_thunder_plugin_cec_sink_t                 *cec_sink;
      #endif
   #endif
} ctrlm_irdb_global_t;

ctrlm_irdb_global_t g_irdb;


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
    #if defined(IRDB_HDMI_DISCOVERY)
    XLOGD_INFO("HDMI Support Enabled");
    #endif
    #if defined(IRDB_CEC_DISCOVERY)
    XLOGD_INFO("CEC Support Enabled");
    #endif
}

ctrlm_irdb_t::~ctrlm_irdb_t() {
    if(this->ipc) {
        this->ipc->deregister_ipc();
        delete this->ipc;
        this->ipc = NULL;
    }
}

bool comp_autolookup_ranked_list (ctrlm_irdb_autolookup_entry_ranked_t i,ctrlm_irdb_autolookup_entry_ranked_t j) { 
    // Sort descending order
    return (i.rank > j.rank);
}

bool ctrlm_irdb_t::get_ir_codes_by_autolookup(ctrlm_irdb_autolookup_ranked_list_by_type_t &codes) {
    bool ret = false;

    #if defined(CTRLM_THUNDER)
    if(this->platform_tv == false) {
        #if defined(IRDB_HDMI_DISCOVERY)
        // Check EDID data
        std::vector<uint8_t> edid;
        if(g_irdb.display_settings) {
            g_irdb.display_settings->get_edid(edid);
            if(edid.size() > 0) {
                ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                if(this->get_ir_codes_by_edid(ir_codes, type, edid.data(), edid.size()) == true) {
                    if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                        if(ir_codes.size() > 0) {
                            codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                        } else {
                            XLOGD_WARN("no codes for edid data");
                        }
                        ret = true;
                    } else {
                        XLOGD_ERROR("edid dev type invalid");
                    }
                } else {
                    XLOGD_ERROR("Failed getting codes by edid");
                }
            } else {
                XLOGD_INFO("No EDID data");
            }
        } else {
            XLOGD_ERROR("display_settings is NULL");
        }
        #endif
        #if defined(IRDB_CEC_DISCOVERY)
        if(g_irdb.cec) {
            // Check CEC data
            std::vector<Thunder::CEC::cec_device_t> cec_devices;
            g_irdb.cec->get_cec_devices(cec_devices);
            if(cec_devices.size() > 0) {
                for(auto &itr : cec_devices) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(this->get_ir_codes_by_cec(ir_codes, type, itr.osd, (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                            if(ir_codes.size() > 0) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                            }
                            ret = true;
                        } else {
                            XLOGD_WARN("cec dev type invalid");
                        }
                    } else {
                        XLOGD_WARN("Failed to get codes for cec device <%s>", itr.osd.c_str());
                    }
                }
            } else {
                XLOGD_INFO("No CEC device data");
            }
        } else {
            XLOGD_ERROR("cec is NULL");
        }
        #endif
    } else {
        #if defined(IRDB_HDMI_DISCOVERY)
        if(g_irdb.hdmi_input) {
            // Check Infoframe data
            std::map<int, std::vector<uint8_t> > infoframes;
            g_irdb.hdmi_input->get_infoframes(infoframes);
            for(auto &itr : infoframes) {
                if(itr.second.size() > 0) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(this->get_ir_codes_by_infoframe(ir_codes, type, itr.second.data(), itr.second.size()) == true) {
                        if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                            if(ir_codes.size() > 0) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_WARN("no code for port %d infoframe", itr.first);
                            }
                            ret = true;
                        } else {
                            XLOGD_WARN("port %d infoframe dev type invalid", itr.first);
                        }
                    } else {
                        XLOGD_WARN("Failed to get codes for port %d infoframe", itr.first);
                    }
                } else {
                    XLOGD_INFO("no infoframe for port %d", itr.first);
                }
            }
        } else {
            XLOGD_ERROR("hdmi is NULL");
        }
        #endif
        #if defined(IRDB_CEC_DISCOVERY)
        if(g_irdb.cec_sink) {
            // Check CEC data
            std::vector<Thunder::CEC::cec_device_t> cec_devices;
            g_irdb.cec_sink->get_cec_devices(cec_devices);
            if(cec_devices.size() > 0) {
                for(auto &itr : cec_devices) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(this->get_ir_codes_by_cec(ir_codes, type, itr.osd, (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                            if(ir_codes.size() > 0) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                            }
                        } else {
                            XLOGD_WARN("cec dev type invalid");
                        }
                    } else {
                        XLOGD_WARN("Failed to get codes for cec device <%s>", itr.osd.c_str());
                    }
                }
            } else {
                XLOGD_INFO("No CEC device data");
            }
        } else {
            XLOGD_ERROR("cec_sink is NULL");
        }
        #endif
    }
    #endif

    // Sort the code lists by the rank value in descending order so that the best codes are listed first in the lists.
    if(codes.count(CTRLM_IRDB_DEV_TYPE_TV) > 0) {
        // sort, but keep the original order of items that have the same rank
        std::stable_sort(codes[CTRLM_IRDB_DEV_TYPE_TV].begin(), codes[CTRLM_IRDB_DEV_TYPE_TV].end(), comp_autolookup_ranked_list);
        // delete duplicate entries
        codes[CTRLM_IRDB_DEV_TYPE_TV].erase( unique( codes[CTRLM_IRDB_DEV_TYPE_TV].begin(), codes[CTRLM_IRDB_DEV_TYPE_TV].end() ), codes[CTRLM_IRDB_DEV_TYPE_TV].end() );
    }
    if(codes.count(CTRLM_IRDB_DEV_TYPE_AVR) > 0) {
        // sort, but keep the original order of items that have the same rank
        std::stable_sort(codes[CTRLM_IRDB_DEV_TYPE_AVR].begin(), codes[CTRLM_IRDB_DEV_TYPE_AVR].end(), comp_autolookup_ranked_list);
        // delete duplicate entries
        codes[CTRLM_IRDB_DEV_TYPE_AVR].erase( unique( codes[CTRLM_IRDB_DEV_TYPE_AVR].begin(), codes[CTRLM_IRDB_DEV_TYPE_AVR].end() ), codes[CTRLM_IRDB_DEV_TYPE_AVR].end() );
    }

    return(ret);
}

bool ctrlm_irdb_t::can_get_ir_codes_by_autolookup() { 
    return(false);
}

bool ctrlm_irdb_t::initialize_irdb() { 
    return(this->initialized > 0);
}

bool ctrlm_irdb_t::get_initialized() {
    return(this->initialized > 0);
}

void ctrlm_irdb_t::on_thunder_ready() {
    #if defined(CTRLM_THUNDER)
    if(this->platform_tv == false) {
        #if defined(IRDB_HDMI_DISCOVERY)
        g_irdb.display_settings = Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t::getInstance();
        #endif
        #if defined(IRDB_CEC_DISCOVERY)
        #if   defined(IRDB_CEC_FLEX2)
        g_irdb.cec = Thunder::CEC::ctrlm_thunder_plugin_cec_source_t::getInstance();
        #else
        g_irdb.cec = Thunder::CEC::ctrlm_thunder_plugin_cec_t::getInstance();
        #endif
        if(!g_irdb.cec->is_activated()) {
            XLOGD_INFO("CEC Thunder Plugin not activated.. Activating..");
            if(g_irdb.cec->activate()) {
                if(g_irdb.cec->enable(true)) {
                    XLOGD_INFO("CEC enabled");
                } else {
                    XLOGD_WARN("CEC failed to enable");
                }
            } else {
                XLOGD_TELEMETRY("failed to activate CEC Plugin");
            }
        }
        #endif
    } else {
        #if defined(IRDB_HDMI_DISCOVERY)
        g_irdb.hdmi_input = Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t::getInstance();
        #endif
        #if defined(IRDB_CEC_DISCOVERY)
        g_irdb.cec_sink = Thunder::CECSink::ctrlm_thunder_plugin_cec_sink_t::getInstance();
        #endif
    }
    #endif
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
