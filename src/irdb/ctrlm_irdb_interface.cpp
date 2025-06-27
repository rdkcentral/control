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
#ifdef USE_DEPRECATED_HDMI_INPUT_PLUGIN
#include "ctrlm_thunder_plugin_hdmi_input.h"
#else
#include "ctrlm_thunder_plugin_av_input.h"
#endif
#include "ctrlm_thunder_plugin_cec_sink.h"
#endif

#include <chrono>
#include <dlfcn.h>

using namespace std;


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
    bool (*pluginOpen)(bool, const std::string&) = NULL;
    bool (*pluginClose)() = NULL;
    std::string (*pluginVersion)() = NULL;
    bool (*pluginInitialize)() = NULL;
    bool (*pluginGetVendorInfo)(ctrlm_irdb_vendor_info_t &info) = NULL;
    bool (*pluginGetManufacturers)(ctrlm_irdb_manufacturer_list_t &manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix) = NULL;
    bool (*pluginGetModels)(ctrlm_irdb_model_list_t &models, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &prefix) = NULL;
    bool (*pluginGetEntryIds)(ctrlm_irdb_entry_id_list_t &codes, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &model) = NULL;
    bool (*pluginGetCodeSet)(ctrlm_irdb_ir_code_set_t &code_set, ctrlm_irdb_dev_type_t type, const std::string &id) = NULL;
    bool (*pluginGetCodesByEdid)(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *edid, unsigned int edid_len) = NULL;
    bool (*pluginGetCodesByCec)(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, const std::string &osd, unsigned int vendor_id, unsigned int logical_address) = NULL;
    bool (*pluginGetCodesByInfoframe)(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *infoframe, unsigned int infoframe_len) = NULL;

    ctrlm_ipc_iarm_t                                                    *irdb_ipc;
    #ifdef CTRLM_THUNDER
    Thunder::DisplaySettings::ctrlm_thunder_plugin_display_settings_t   *display_settings;
    Thunder::CEC::ctrlm_thunder_plugin_cec_t                            *cec;
    #ifdef USE_DEPRECATED_HDMI_INPUT_PLUGIN
    Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t               *av_input;
    #else
    Thunder::AVInput::ctrlm_thunder_plugin_av_input_t                   *av_input;
    #endif
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

    m_irdbPluginHandle = dlopen ("/vendor/input/lib/universal_remote.so", RTLD_NOW);
    if (!m_irdbPluginHandle) {
        XLOGD_ERROR("Failed to load IR database plugin universal_remote.so <%s>, trying deprecated libctrlm-irdb-plugin.so.", dlerror());
        m_irdbPluginHandle = dlopen ("libctrlm-irdb-plugin.so", RTLD_NOW);
    } else {
        XLOGD_INFO("IR database plugin universal_remote.so loaded successfully");
    }
    
    if (!m_irdbPluginHandle) {
        XLOGD_ERROR("Failed to load IR database plugin <%s>, using stub implementation.", dlerror());
        g_irdb.pluginOpen = STUB_ctrlm_irdb_open;
        g_irdb.pluginClose = STUB_ctrlm_irdb_close;
        g_irdb.pluginVersion = STUB_irdb_version;
        g_irdb.pluginInitialize = STUB_ctrlm_irdb_initialize;
        g_irdb.pluginGetVendorInfo = STUB_ctrlm_irdb_get_vendor_info;
        g_irdb.pluginGetManufacturers = STUB_ctrlm_irdb_get_manufacturers;
        g_irdb.pluginGetModels = STUB_ctrlm_irdb_get_models;
        g_irdb.pluginGetEntryIds = STUB_ctrlm_irdb_get_entry_ids;
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

        *(void **) (&g_irdb.pluginClose) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_close");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_close), error <%s>, Using STUB implementation", error);
            g_irdb.pluginClose = STUB_ctrlm_irdb_close;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginVersion) = dlsym(m_irdbPluginHandle, "irdb_version");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (irdb_version), error <%s>, Using STUB implementation", error);
            g_irdb.pluginVersion = STUB_irdb_version;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginInitialize) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_initialize");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_initialize), error <%s>, Using STUB implementation", error);
            g_irdb.pluginInitialize = STUB_ctrlm_irdb_initialize;
        }
        dlerror();  // Clear any existing error

        *(void **) (&g_irdb.pluginGetVendorInfo) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_vendor_info");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_vendor_info), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetVendorInfo = STUB_ctrlm_irdb_get_vendor_info;
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

        *(void **) (&g_irdb.pluginGetEntryIds) = dlsym(m_irdbPluginHandle, "ctrlm_irdb_get_entry_ids");
        if ((error = dlerror()) != NULL)  {
            XLOGD_ERROR("Failed to find plugin method (ctrlm_irdb_get_entry_ids), error <%s>, Using STUB implementation", error);
            g_irdb.pluginGetEntryIds = STUB_ctrlm_irdb_get_entry_ids;
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

    open_plugin();

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

    close_plugin();

    if (m_irdbPluginHandle != NULL) {
        dlclose(m_irdbPluginHandle);
    }
}

bool ctrlm_irdb_interface_t::open_plugin() {
    std::unique_lock<std::mutex> guard(m_mutex);

    bool ret = false;
    if (g_irdb.pluginOpen) {
        if ((ret = (*g_irdb.pluginOpen)(m_platform_tv, ctrlm_device_mac_get())) == true) {
            string version = (*g_irdb.pluginVersion)();
            XLOGD_INFO("IRDB Version <%s>", version.c_str());
        }
    }
    XLOGD_INFO("IRDB plugin opened, ret = <%s>", ret ? "SUCCESS" : "ERROR");
    return ret;
}

bool ctrlm_irdb_interface_t::close_plugin() {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;
    if (g_irdb.pluginClose) {
        ret = (*g_irdb.pluginClose)();
    }
    XLOGD_INFO("IRDB plugin closed, ret = <%s>", ret ? "SUCCESS" : "ERROR, but ignoring");
    return ret;
}

bool ctrlm_irdb_interface_t::get_vendor_info(ctrlm_irdb_vendor_info_t &info) {
    std::unique_lock<std::mutex> guard(m_mutex);
    if (g_irdb.pluginGetVendorInfo) {
        return (*g_irdb.pluginGetVendorInfo)(info);
    }
    return false;
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
        #ifdef USE_DEPRECATED_HDMI_INPUT_PLUGIN
        g_irdb.av_input = Thunder::HDMIInput::ctrlm_thunder_plugin_hdmi_input_t::getInstance();
        #else
        g_irdb.av_input = Thunder::AVInput::ctrlm_thunder_plugin_av_input_t::getInstance();
        #endif
        g_irdb.cec_sink = Thunder::CECSink::ctrlm_thunder_plugin_cec_sink_t::getInstance();
    }
    #endif
}


bool ctrlm_irdb_interface_t::initialize_irdb() {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;

    if (g_irdb.pluginInitialize) {

        if ((ret = (*g_irdb.pluginInitialize)()) == true) {
            string version = (*g_irdb.pluginVersion)();
            XLOGD_INFO("IRDB Version <%s>", version.c_str());
        }
    }
    return ret;
}

bool ctrlm_irdb_interface_t::get_manufacturers(ctrlm_irdb_manufacturer_list_t &manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix) {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;

    if (g_irdb.pluginGetManufacturers) {
        ret = (*g_irdb.pluginGetManufacturers)(manufacturers, type, prefix);
    }
    return ret;
}

bool ctrlm_irdb_interface_t::get_models(ctrlm_irdb_model_list_t &models, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &prefix) {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;

    if (g_irdb.pluginGetModels) {
        ret = (*g_irdb.pluginGetModels)(models, type, manufacturer, prefix);
    }
    return ret;
}

bool ctrlm_irdb_interface_t::get_irdb_entry_ids(ctrlm_irdb_entry_id_list_t &codes, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &model) {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;

    if (g_irdb.pluginGetEntryIds) {
        ret = (*g_irdb.pluginGetEntryIds)(codes, type, manufacturer, model);
    }
    return ret;
}


bool comp_autolookup_ranked_list (ctrlm_irdb_autolookup_entry_ranked_t i, ctrlm_irdb_autolookup_entry_ranked_t j) {
    // Sort descending order
    return (i.rank > j.rank);
}

bool ctrlm_irdb_interface_t::get_ir_codes_by_autolookup(ctrlm_autolookup_ranked_list_by_type_t &codes) {
    std::unique_lock<std::mutex> guard(m_mutex);
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
                
                if(g_irdb.pluginGetCodesByEdid && (*g_irdb.pluginGetCodesByEdid)(ir_codes, type, edid.data(), edid.size()) == true) {
                    if(ir_codes.size() > 0) {
                        if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                            codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                        } else {
                            XLOGD_ERROR("edid dev type invalid");
                        }
                    } else {
                        XLOGD_WARN("no codes for edid data");
                    }
                    ret = true;
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

                    if(g_irdb.pluginGetCodesByCec && (*g_irdb.pluginGetCodesByCec)(ir_codes, type, itr.osd, (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(ir_codes.size() > 0) {
                            if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_ERROR("cec dev type invalid");
                            }
                        } else {
                            XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                        }
                        ret = true;
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
        if(g_irdb.av_input) {
            // Check Infoframe data
            std::map<int, std::vector<uint8_t> > infoframes;
            g_irdb.av_input->get_infoframes(infoframes);
            for(auto &itr : infoframes) {
                if(itr.second.size() > 0) {
                    ctrlm_irdb_dev_type_t type = CTRLM_IRDB_DEV_TYPE_INVALID;
                    ctrlm_irdb_autolookup_ranked_list_t ir_codes;
                    if(g_irdb.pluginGetCodesByInfoframe && (*g_irdb.pluginGetCodesByInfoframe)(ir_codes, type, itr.second.data(), itr.second.size()) == true) {
                        if(ir_codes.size() > 0) {
                            if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_WARN("port %d infoframe dev type invalid", itr.first);
                            }
                        } else {
                            XLOGD_WARN("no code for port %d infoframe", itr.first);
                        }
                        ret = true;
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
                    if(g_irdb.pluginGetCodesByCec && (*g_irdb.pluginGetCodesByCec)(ir_codes, type, itr.osd, (unsigned int)itr.vendor_id, itr.logical_address) == true) {
                        if(ir_codes.size() > 0) {
                            if(type != CTRLM_IRDB_DEV_TYPE_INVALID) {
                                codes[type].insert(codes[type].end(), ir_codes.begin(), ir_codes.end());
                            } else {
                                XLOGD_WARN("cec dev type invalid");
                            }
                        } else {
                            XLOGD_WARN("no code for cec device <%s>", itr.osd.c_str());
                        }
                        ret = true;
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

bool ctrlm_irdb_interface_t::program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_dev_type_t type, const std::string &id) {
    std::unique_lock<std::mutex> guard(m_mutex);
    bool ret = false;

    XLOGD_INFO("Programming IR codes for (%u, %u) with database id <%s>", network_id, controller_id, id.c_str());

    ctrlm_irdb_ir_code_set_t code_set;
    if (g_irdb.pluginGetCodeSet) {
        if ( (*g_irdb.pluginGetCodeSet)(code_set, type, id) == false) {
            XLOGD_ERROR("Failed getting IR code set");
        } else {
            guard.unlock();
            ret = this->_program_ir_codes(network_id, controller_id, &code_set);
        }
    }
    return(ret);
}


bool ctrlm_irdb_interface_t::_program_ir_codes(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_irdb_ir_code_set_t *ir_codes) {
    bool ret = false;

    ctrlm_main_queue_msg_program_ir_codes_t msg = {0};
    msg.network_id         = network_id;
    msg.controller_id      = controller_id;
    msg.ir_codes           = ir_codes;
    msg.success            = &ret;

    if (false == get_vendor_info(msg.vendor_info)) {
        msg.vendor_info.rcu_support_bitmask = 0;
    }
    ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_program_ir_codes, &msg, sizeof(msg), NULL, network_id, true);

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
