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
#ifndef __CTRLM_TELEMETRY_H__
#define __CTRLM_TELEMETRY_H__
#include "ctrlm_telemetry_event.h"
#include <functional>
#include <vector>
#include <map>

/**
 * Enum of telemetry reports
 */
typedef enum {
    GLOBAL,
    RF4CE,
    BLE,
    IP,
    VOICE
} ctrlm_telemetry_report_t;

/**
 * Function prototype for report callback. 
 */
typedef std::function<void()> ctrlm_telemetry_report_listener_t;

/**
 * @brief ControlMgr Telemetry Class
 * 
 * This class is a singleton that manages reporting ControlMgr events to
 * Telemetry 2.0.
 */
class ctrlm_telemetry_t {
public: 
    /**
     * This function is used to get the Telemetry instance, as it is a Singleton.
     * @return The instance of the Telemetry, or NULL on error.
     */
    static ctrlm_telemetry_t *get_instance();
    /**
     * This function is used to destroy the sole instance of the Telemetry object. 
     */
    static void destroy_instance();
    /**
     * Destructor for the Telemetry
     */
    virtual ~ctrlm_telemetry_t();
    /**
     * Function used to event a telemetry event.
     * @tparam T The type of event
     * @param event The event
     * @return True is reported successfully, otherwise False. 
     */
    template <typename T>
    bool event(ctrlm_telemetry_report_t report, ctrlm_telemetry_event_t<T> &event) {
        bool ret = (this->enabled ? event.event() : this->enabled);
        if(ret) {
            this->event_reported[report] = true;
        }
        return(ret);
    }
    /**
     * This static function is called by the timeout routine to trigger a telemetry report
     * @param data NULL
     * @return Return is 1, which makes this function repeat on a time-based interval.
     */
    static int report_timeout(void *data);
    /**
     * Set the duration between reports.
     * @param duration 
     */
    void set_duration(unsigned int duration);
    /**
     * Function to add a listener, which will be called after a report is generated.
     */
    void add_listener(ctrlm_telemetry_report_t report, ctrlm_telemetry_report_listener_t listener);

protected:
    /**
     * Thunder Controller Default Constructor (Protected due to it being a Singleton)
     */
    ctrlm_telemetry_t();
    /**
     * This function creates the telemetry report if at least 1 event was received. This
     * is called by the timeout routine.
     */
    void report();
    /**
     * This function maps the report types to their TR181 trigger variable
     * 
     * @param report The report type
     * @return The TR181 string for that report type
     */
    static const char *get_report_trigger(ctrlm_telemetry_report_t report);
    /**
     * This function maps the report types to a string
     * 
     * @param report The report type
     * @return The descriptive string
     */
    static const char *get_report_str(ctrlm_telemetry_report_t report);


private:
    bool          enabled;
    unsigned int  timeout_id;
    unsigned int  reporting_interval;
    std::map<ctrlm_telemetry_report_t,bool> event_reported;
    std::map<ctrlm_telemetry_report_t,std::vector<ctrlm_telemetry_report_listener_t> > listeners;
};


#endif