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

#include "ctrlm_telemetry.h"
#include <stdlib.h>
#include "ctrlm_log.h"
#include "ctrlm_tr181.h"
#include "ctrlm.h"
#include "ctrlm_utils.h"
#include "ctrlm_config_default.h"
#include <glib.h>
#include <sstream>
#include <uuid/uuid.h>

static ctrlm_telemetry_t *_instance = NULL;

ctrlm_telemetry_t* ctrlm_telemetry_t::get_instance() {
    if(_instance == NULL) {
        _instance = new ctrlm_telemetry_t();
    }
    return(_instance);
}

void ctrlm_telemetry_t::destroy_instance() {
    if(_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}

ctrlm_telemetry_t::ctrlm_telemetry_t() {
    char component[] = "ctrlm";
    XLOGD_INFO("Telemetry 2.0 init");

    this->enabled = true;

    XLOGD_INFO("Telemetry is %s", this->enabled ? "enabled" : "disabled");
    if(this->enabled) {
        t2_init(component);
    }

    // Launch report timeout
    this->reporting_interval = JSON_INT_VALUE_CTRLM_GLOBAL_TELEMETRY_REPORT_INTERVAL;
    this->timeout_id = 0;
    this->set_duration(this->reporting_interval);
    this->event_reported[ctrlm_telemetry_report_t::GLOBAL] = false;
    this->event_reported[ctrlm_telemetry_report_t::RF4CE]  = false;
    this->event_reported[ctrlm_telemetry_report_t::BLE]    = false;
    this->event_reported[ctrlm_telemetry_report_t::IP]     = false;
    this->event_reported[ctrlm_telemetry_report_t::VOICE]  = false;
}

ctrlm_telemetry_t::~ctrlm_telemetry_t() {
    t2_uninit();
}

void ctrlm_telemetry_t::set_duration(unsigned int duration) {
    if((duration != this->reporting_interval || this->timeout_id == 0) && this->enabled) {
        this->reporting_interval = duration;
        ctrlm_timeout_destroy(&this->timeout_id);
        this->timeout_id = ctrlm_timeout_create(this->reporting_interval, ctrlm_telemetry_t::report_timeout, (void *)NULL);
    }
}

void ctrlm_telemetry_t::add_listener(ctrlm_telemetry_report_t report, ctrlm_telemetry_report_listener_t listener) {
    this->listeners[report].push_back(listener);
}

int ctrlm_telemetry_t::report_timeout(void *data) {
    ctrlm_telemetry_t *telemetry = ctrlm_telemetry_t::get_instance();
    if(telemetry) {
        telemetry->report();
    }
    return(1);
}

void ctrlm_telemetry_t::report() {
    for(auto &itr : this->event_reported) {
        if(itr.second) {
            itr.second = false;
            XLOGD_INFO("triggering %s telemetry report", this->get_report_str(itr.first));
            uuid_t uuid;
            char uuid_string[64];
            uuid_generate(uuid);
            uuid_unparse_lower(uuid, uuid_string);
            ctrlm_tr181_string_set(this->get_report_trigger(itr.first), uuid_string, sizeof(uuid_string));
            for(auto &itr : this->listeners[itr.first]) {
                itr();
            }
        } else {
            XLOGD_DEBUG("no events reported for %s.. telemetry report not created.", this->get_report_str(itr.first));
        }
    }
}

const char *ctrlm_telemetry_t::get_report_trigger(ctrlm_telemetry_report_t report) {
    switch(report) {
        case ctrlm_telemetry_report_t::GLOBAL: return(CTRLM_TR181_TELEMETRY_REPORT_GLOBAL);
        case ctrlm_telemetry_report_t::RF4CE:  return(CTRLM_TR181_TELEMETRY_REPORT_RF4CE);
        case ctrlm_telemetry_report_t::BLE:    return(CTRLM_TR181_TELEMETRY_REPORT_BLE);
        case ctrlm_telemetry_report_t::IP:     return(CTRLM_TR181_TELEMETRY_REPORT_IP);
        case ctrlm_telemetry_report_t::VOICE:  return(CTRLM_TR181_TELEMETRY_REPORT_VOICE);
    }
    return("INVALID");
}

const char *ctrlm_telemetry_t::get_report_str(ctrlm_telemetry_report_t report) {
    switch(report) {
        case ctrlm_telemetry_report_t::GLOBAL: return("GLOBAL");
        case ctrlm_telemetry_report_t::RF4CE:  return("RF4CE");
        case ctrlm_telemetry_report_t::BLE:    return("BLE");
        case ctrlm_telemetry_report_t::IP:     return("IP");
        case ctrlm_telemetry_report_t::VOICE:  return("VOICE");
    }
    return("INVALID");
}
