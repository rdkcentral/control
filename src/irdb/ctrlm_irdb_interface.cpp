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

#include "ctrlm_irdb_interface.h"
#include "ctrlm_network.h"
#include "ctrlm_log.h"
#include "ctrlm_irdb_stub.h"

#include "ctrlm_ipc_iarm.h"
#include "ctrlm_irdb_ipc_iarm_thunder.h"

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


#include <dlfcn.h>


static ctrlm_irdb_interface_t *_instance = NULL;

ctrlm_irdb_interface_t* ctrlm_irdb_interface_t::get_instance(bool platform_tv) {
    if (_instance == NULL) {
        _instance = new ctrlm_irdb_interface_t(platform_tv);
    }
    return(_instance);
}

void ctrlm_irdb_interface_t::destroy_instance() {
    if (_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}


typedef struct {
    bool (*pluginOpen)(bool, const char*) = NULL;
    bool (*pluginInitialize)() = NULL;
    bool (*pluginGetManufacturers)(ctrlm_irdb_manufacturer_list_t *manufacturers, ctrlm_irdb_dev_type_t type, const char *prefix) = NULL;
    bool (*pluginGetModels)(ctrlm_irdb_model_list_t *models, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *prefix) = NULL;
    bool (*pluginGetCodesByNames)(ctrlm_irdb_entry_id_list_t *codes, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *model) = NULL;
    bool (*pluginGetCodeSet)(ctrlm_irdb_ir_code_set_t *code_set, ctrlm_irdb_dev_type_t type, const char *id) = NULL;
    bool (*pluginGetCodesByEdid)(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *edid, unsigned int edid_len) = NULL;
    bool (*pluginGetCodesByCec)(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, const char *osd, unsigned int vendor_id, unsigned int logical_address) = NULL;
    bool (*pluginGetCodesByInfoframe)(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *infoframe, unsigned int infoframe_len) = NULL;

    ctrlm_ipc_iarm_t                                                    *irdb_ipc;
    #ifdef CTRLM_THUNDER
    Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t   *display_settings;
    Thunder::CEC::ctrlm_thunder_plugin_cec_t                            *cec;
    Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t               *hdmi_input;
    Thunder::CECSink::ctrlm_thunder_plugin_cec_sink_t                   *cec_sink;
   #endif
} ctrlm_irdb_global_t;

ctrlm_irdb_global_t g_irdb;


#if defined(CTRLM_THUNDER)
static int _on_thunder_ready_thread(void *data) {
    ctrlm_irdb_interface_t *irdb = (ctrlm_irdb_interface_t *)data;
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

ctrlm_irdb_interface_t::ctrlm_irdb_interface_t() : ctrlm_irdb_interface_t(false) {
}

ctrlm_irdb_interface_t::ctrlm_irdb_interface_t(bool platform_tv) {

    m_platform_tv = platform_tv;

    m_irdbPluginHandle = NULL;

    // m_irdbPluginHandle = dlopen ("libctrlm-hal-irdb.so", RTLD_LAZY);
    m_irdbPluginHandle = dlopen ("libctrlm-hal-irdb.so", RTLD_NOW);
    if (!m_irdbPluginHandle) {
        XLOGD_ERROR("Failed to dynamically load IR database library <%s>, using stub implementation.", dlerror());
        g_irdb.pluginOpen = STUB_ctrlm_irdb_open;
        g_irdb.pluginInitialize = STUB_ctrlm_irdb_initialize;
        g_irdb.pluginGetManufacturers = STUB_ctrlm_irdb_get_manufacturers;
        g_irdb.pluginGetModels = STUB_ctrlm_irdb_get_models;
        g_irdb.pluginGetCodesByNames = STUB_ctrlm_irdb_get_entry_ids;
        g_irdb.pluginGetCodeSet = STUB_ctrlm_irdb_get_ir_code_set;
        g_irdb.pluginGetCodesByEdid = STUB_ctrlm_irdb_get_ir_codes_by_edid;
        g_irdb.pluginGetCodesByCec = STUB_ctrlm_irdb_get_ir_codes_by_cec;
        g_irdb.pluginGetCodesByInfoframe = STUB_ctrlm_irdb_get_ir_codes_by_infoframe;

    } else {
        XLOGD_INFO("IR database plugin loaded successfully");

        char *error;

        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginOpen) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_open");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_open), error <%s>, Using STUB implementation", error);
            g_irdb.pluginOpen = STUB_ctrlm_irdb_open;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginInitialize) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_initialize");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_initialize), error <%s>, Using STUB implementation", error);
            g_irdb.pluginInitialize = STUB_ctrlm_irdb_initialize;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetManufacturers) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_manufacturers");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_manufacturers), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetManufacturers = STUB_ctrlm_irdb_get_manufacturers;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetModels) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_models");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_manufacturers), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetModels = STUB_ctrlm_irdb_get_models;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetCodesByNames) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_codes_by_names");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_codes_by_names), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetCodesByNames = STUB_ctrlm_irdb_get_entry_ids;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetCodeSet) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_ir_code_set");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_ir_code_set), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetCodeSet = STUB_ctrlm_irdb_get_ir_code_set;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetCodesByEdid) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_ir_codes_by_edid");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_ir_codes_by_edid), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetCodesByEdid = STUB_ctrlm_irdb_get_ir_codes_by_edid;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetCodesByCec) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_ir_codes_by_cec");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_ir_codes_by_cec), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetCodesByCec = STUB_ctrlm_irdb_get_ir_codes_by_cec;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetCodesByInfoframe) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_ir_codes_by_infoframe");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_ir_codes_by_infoframe), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetCodesByInfoframe = STUB_ctrlm_irdb_get_ir_codes_by_infoframe;
        }
    }
    (*g_irdb.pluginOpen)(m_platform_tv, ctrlm_device_mac_get().c_str());

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

    XLOGD_INFO("registering for IRDB IARM Thunder calls");
    g_irdb.irdb_ipc = new ctrlm_irdb_ipc_iarm_thunder_t();
    g_irdb.irdb_ipc->register_ipc();

}


ctrlm_irdb_interface_t::~ctrlm_irdb_interface_t() {
    if (g_irdb.irdb_ipc) {
        g_irdb.irdb_ipc->deregister_ipc();
        delete g_irdb.irdb_ipc;
        g_irdb.irdb_ipc = NULL;
    }

    if (m_irdbPluginHandle != NULL) {
        dlclose(m_irdbPluginHandle);
    }
}


void ctrlm_irdb_interface_t::on_thunder_ready() {
    #if defined(CTRLM_THUNDER)

    if(m_platform_tv == false) {
        g_irdb.display_settings = Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t::getInstance();
        g_irdb.cec = Thunder::CEC::ctrlm_thunder_plugin_cec_t::getInstance();
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
    } else {
        g_irdb.hdmi_input = Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t::getInstance();
        g_irdb.cec_sink = Thunder::CECSink::ctrlm_thunder_plugin_cec_sink_t::getInstance();
    }
    #endif
}


bool ctrlm_irdb_interface_t::initialize_irdb() {
    if (g_irdb.pluginInitialize) {
        return (*g_irdb.pluginInitialize)();
    }
    return false;
}

bool ctrlm_irdb_interface_t::get_initialized() { 
    // return(m_irdb->get_initialized());
    return true;
}

bool comp_autolookup_ranked_list (ctrlm_autolookup_entry_ranked_t i, ctrlm_autolookup_entry_ranked_t j) {
    // Sort descending order
    return (i.rank > j.rank);
}

bool ctrlm_irdb_interface_t::get_ir_codes_by_autolookup(ctrlm_autolookup_ranked_list_by_type_t &codes) {
    bool ret = false;

    #if defined(CTRLM_THUNDER)
    if(m_platform_tv == false) {
        // Check EDID data
        std::vector<uint8_t> edid;
        if(g_irdb.display_settings) {
            g_irdb.display_settings->get_edid(edid);
            if(edid.size() > 0) {
                ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                
                if(g_irdb.pluginGetCodesByEdid && (*g_irdb.pluginGetCodesByEdid)(&ir_codes, &type, edid.data(), edid.size()) == true) {
                    if(ir_codes.list_qty > 0) {
                        for (unsigned int i = 0; i < ir_codes.list_qty; i++) {
                            ctrlm_autolookup_entry_ranked_t temp;
                            if (ir_codes.ranked_list[i].manufacturer != NULL) {
                                temp.man = ir_codes.ranked_list[i].manufacturer;
                                free(ir_codes.ranked_list[i].manufacturer);
                            }
                            if (ir_codes.ranked_list[i].model != NULL) {
                                temp.model = ir_codes.ranked_list[i].model;
                                free(ir_codes.ranked_list[i].model);
                            }
                            if (ir_codes.ranked_list[i].id != NULL) {
                                temp.id = ir_codes.ranked_list[i].id;
                                free(ir_codes.ranked_list[i].id);
                            }
                            temp.rank = ir_codes.ranked_list[i].rank;

                            if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                codes[type].push_back(temp);
                            } else {
                                XLOGD_ERROR("edid dev type invalid");
                            }
                        }
                    } else {
                        XLOGD_WARN("no codes for edid data");
                    }
                    ret = true;
                    free(ir_codes.ranked_list);
                } else {
                    XLOGD_ERROR("Failed getting codes by edid");
                }
            } else {
                XLOGD_INFO("No EDID data");
            }
        } else {
            XLOGD_ERROR("display_settings is NULL");
        }
        if(g_irdb.cec) {
            // Check CEC data
            std::vector<Thunder::CEC::cec_device_t> cec_devices;
            g_irdb.cec->get_cec_devices(cec_devices);
            if(cec_devices.size() > 0) {
                for(auto &itr : cec_devices) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;

                    if(g_irdb.pluginGetCodesByCec && (*g_irdb.pluginGetCodesByCec)(&ir_codes, &type, itr.osd.c_str(), (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(ir_codes.list_qty > 0) {
                            for (unsigned int i = 0; i < ir_codes.list_qty; i++) {
                                ctrlm_autolookup_entry_ranked_t temp;
                                if (ir_codes.ranked_list[i].manufacturer != NULL) {
                                    temp.man = ir_codes.ranked_list[i].manufacturer;
                                    free(ir_codes.ranked_list[i].manufacturer);
                                }
                                if (ir_codes.ranked_list[i].model != NULL) {
                                    temp.model = ir_codes.ranked_list[i].model;
                                    free(ir_codes.ranked_list[i].model);
                                }
                                if (ir_codes.ranked_list[i].id != NULL) {
                                    temp.id = ir_codes.ranked_list[i].id;
                                    free(ir_codes.ranked_list[i].id);
                                }
                                temp.rank = ir_codes.ranked_list[i].rank;

                                if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                    codes[type].push_back(temp);
                                } else {
                                    XLOGD_ERROR("cec dev type invalid");
                                }
                            }
                        } else {
                            XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                        }
                        ret = true;
                        free(ir_codes.ranked_list);
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
    } else {
        if(g_irdb.hdmi_input) {
            // Check Infoframe data
            std::map<int, std::vector<uint8_t> > infoframes;
            g_irdb.hdmi_input->get_infoframes(infoframes);
            for(auto &itr : infoframes) {
                if(itr.second.size() > 0) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(g_irdb.pluginGetCodesByInfoframe && (*g_irdb.pluginGetCodesByInfoframe)(&ir_codes, &type, itr.second.data(), itr.second.size()) == true) {
                        if(ir_codes.list_qty > 0) {
                            for (unsigned int i = 0; i < ir_codes.list_qty; i++) {
                                ctrlm_autolookup_entry_ranked_t temp;
                                if (ir_codes.ranked_list[i].manufacturer != NULL) {
                                    temp.man = ir_codes.ranked_list[i].manufacturer;
                                    free(ir_codes.ranked_list[i].manufacturer);
                                }
                                if (ir_codes.ranked_list[i].model != NULL) {
                                    temp.model = ir_codes.ranked_list[i].model;
                                    free(ir_codes.ranked_list[i].model);
                                }
                                if (ir_codes.ranked_list[i].id != NULL) {
                                    temp.id = ir_codes.ranked_list[i].id;
                                    free(ir_codes.ranked_list[i].id);
                                }
                                temp.rank = ir_codes.ranked_list[i].rank;
                                
                                if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                    codes[type].push_back(temp);
                                } else {
                                    XLOGD_WARN("port %d infoframe dev type invalid", itr.first);
                                }
                            }
                        } else {
                            XLOGD_WARN("no code for port %d infoframe", itr.first);
                        }
                        ret = true;
                        free(ir_codes.ranked_list);
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
        if(g_irdb.cec_sink) {
            // Check CEC data
            std::vector<Thunder::CEC::cec_device_t> cec_devices;
            g_irdb.cec_sink->get_cec_devices(cec_devices);
            if(cec_devices.size() > 0) {
                for(auto &itr : cec_devices) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(g_irdb.pluginGetCodesByCec && (*g_irdb.pluginGetCodesByCec)(&ir_codes, &type, itr.osd.c_str(), (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(ir_codes.list_qty > 0) {
                            for (unsigned int i = 0; i < ir_codes.list_qty; i++) {
                                ctrlm_autolookup_entry_ranked_t temp;
                                if (ir_codes.ranked_list[i].manufacturer != NULL) {
                                    temp.man = ir_codes.ranked_list[i].manufacturer;
                                    free(ir_codes.ranked_list[i].manufacturer);
                                }
                                if (ir_codes.ranked_list[i].model != NULL) {
                                    temp.model = ir_codes.ranked_list[i].model;
                                    free(ir_codes.ranked_list[i].model);
                                }
                                if (ir_codes.ranked_list[i].id != NULL) {
                                    temp.id = ir_codes.ranked_list[i].id;
                                    free(ir_codes.ranked_list[i].id);
                                }
                                temp.rank = ir_codes.ranked_list[i].rank;

                                if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                    codes[type].push_back(temp);
                                } else {
                                    XLOGD_WARN("cec dev type invalid");
                                }
                            }
                        } else {
                            XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                        }
                        ret = true;
                        free(ir_codes.ranked_list);
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

bool ctrlm_irdb_interface_t::clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
    XLOGD_INFO("Clearing IR codes for (%u, %u)", network_id, controller_id);
    return(this->_clear_ir_codes(network_id, controller_id));
}

bool ctrlm_irdb_interface_t::_clear_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
    bool ret = false;

    ctrlm_main_queue_msg_ir_clear_t msg = {0};

    msg.network_id    = network_id;
    msg.controller_id = controller_id;
    msg.success       = &ret;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_ir_clear_codes, &msg, sizeof(msg), NULL, network_id, true);
    
    return(ret);
}

bool ctrlm_irdb_interface_t::get_manufacturers(ctrlm_irdb_manufacturer_list_t *manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix) {

    if (g_irdb.pluginGetManufacturers) {
        return (*g_irdb.pluginGetManufacturers)(manufacturers, type, prefix.c_str());
    }

    return false;
}

bool ctrlm_irdb_interface_t::get_models(ctrlm_irdb_model_list_t *models, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &prefix) {

    if (g_irdb.pluginGetModels) {
        return (*g_irdb.pluginGetModels)(models, type, manufacturer.c_str(), prefix.c_str());
    }

    return false;
}

bool ctrlm_irdb_interface_t::get_irdb_entry_ids(ctrlm_irdb_entry_id_list_t *codes, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &model) {

    if (g_irdb.pluginGetCodesByNames) {
        return (*g_irdb.pluginGetCodesByNames)(codes, type, manufacturer.c_str(), model.c_str());
    }

    return false;
}


bool ctrlm_irdb_interface_t::program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_dev_type_t type, const std::string &id) {
    bool ret = false;

    XLOGD_INFO("Programming IR codes for (%u, %u) with database id <%s>", network_id, controller_id, id.c_str());

    ctrlm_irdb_ir_code_set_t code_set;
    if (g_irdb.pluginGetCodeSet) {
        if ( (*g_irdb.pluginGetCodeSet)(&code_set, type, id.c_str()) == false) {
            XLOGD_ERROR("Failed getting IR code set");
        } else {
            ret = this->_program_ir_codes(network_id, controller_id, &code_set, CTRLM_IRDB_VENDOR_UEI);
        }
    }
    return(ret);
}


bool ctrlm_irdb_interface_t::_program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_ir_code_set_t *ir_codes, ctrlm_irdb_vendor_t vendor) {
    bool ret = false;

    ctrlm_main_queue_msg_program_ir_codes_t msg = {0};

    msg.network_id         = network_id;
    msg.controller_id      = controller_id;
    msg.ir_codes           = ir_codes;
    msg.success            = &ret;
    msg.vendor             = vendor;

    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_program_ir_codes, &msg, sizeof(msg), NULL, network_id, true);

    //Previous call is synchronous, so its safe to free memory here
    for(unsigned int i = 0; i < ir_codes->list_qty; i++) {
        free(ir_codes->waveforms[i].data);
    }
    free(ir_codes->waveforms);
    free(ir_codes->id);

    return(ret);
}
