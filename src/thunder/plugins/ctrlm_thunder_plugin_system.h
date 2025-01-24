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
#ifndef __CTRLM_THUNDER_PLUGIN_SYSTEM_H__
#define __CTRLM_THUNDER_PLUGIN_SYSTEM_H__
#include "ctrlm_thunder_plugin.h"

namespace Thunder {
namespace System {

typedef enum {
    EVENT_FIRMWARE_UPDATE_STATE_CHANGED
} system_event_t;

typedef enum {
    STATE_UNINITIALIZED       = 0,
    STATE_REQUESTING          = 1,
    STATE_DOWNLOADING         = 2,
    STATE_FAILED              = 3,
    STATE_DOWNLOAD_COMPLETE   = 4,
    STATE_VALIDATION_COMPLETE = 5,
    STATE_PREPARING_REBOOT    = 6
} firmware_update_state_t;

typedef void (*system_event_handler_t)(system_event_t event, void *event_data, void *user_data);

/**
 * This class is used within ControlMgr to interact with the System Thunder Plugin. This implementation recieves system events.
 */
class ctrlm_thunder_plugin_system_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder System instance, as it is a Singleton.
     * @return The instance of the Thunder System, or NULL on error.
     */
    static ctrlm_thunder_plugin_system_t *getInstance();


    /**
     * System Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_system_t();

    /**
     * This function is used to register a handler for the System Thunder Plugin events.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_event_handler(system_event_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the System Thunder Plugin events.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_event_handler(system_event_handler_t handler);

public:
    /** 
     * This function is technically used internally but from static function. This is used to broadcast System update state change events.
     */
    void on_firmware_update_state_change(firmware_update_state_t state);

protected:
    /**
     * System Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_system_t();
    /**
     * This function is called when registering for Thunder events.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

private:
    std::vector<std::pair<system_event_handler_t, void *> > event_callbacks;
    bool registered_events;
};

const char *system_event_str(system_event_t event);
const char *system_firmware_update_state_str(firmware_update_state_t state);

};
};

#endif