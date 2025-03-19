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
#ifndef __CTRLMF_THUNDER_PLUGIN_H__
#define __CTRLMF_THUNDER_PLUGIN_H__
#include "ctrlmf_thunder_controller.h"
#include <iostream>
#include <map>
#include <vector>
#include <utility>

namespace Thunder {
namespace Plugin {

typedef void (*plugin_activation_handler_t)(plugin_state_t state, void *user_data);

/**
 * This class is the base class for thunder plugins within ControlMgr. This implementation handles calling Thunder for status on plugins, registering for plugin events, and making calls to thunder plugins.
 * This class MUST NOT be used within ControlMgr directly, rather extending this class for each Thunder Plugin ControlMgr needs support for.
 */
class ctrlm_thunder_plugin_t {
public:
    virtual ~ctrlm_thunder_plugin_t();

    /**
     * This function can be used to determine if a Thunder Plugin is activated.
     * @return True if the Plugin is activated, otherwise False.
     */
    virtual bool is_activated();

    /**
     * This function can be used to activate a Thunder Plugin.
     * @return True if the Plugin is activated, otherwise False.
     */
    virtual bool activate();

    /**
     * This function can be used to de-activate a Thunder Plugin.
     * @return True if the Plugin is de-activated, otherwise False.
     */
    bool deactivate();

    /**
     * This function is used to register a handler for the Thunder Controller events which contain Plugin state information.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_activation_handler(plugin_activation_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the Thunder Controller events which contain Plugin state information.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_activation_handler(plugin_activation_handler_t handler);

public:
    /** 
     * This function is technically used internally but from static function. This is used to broadcast activation state changes.
     * @param callsign The callsign in which the activation state is in reference to.
     * @param state The ctrlm_thunder_plugin_state_t that the plugin transitioned to.
     */
    virtual void on_activation_change(plugin_state_t state);

    /**
     * This function is technically used internally but from static function. This is used to catch Thunder ready events.
     * @param boot Boolean signifying if this call is happening during init.
     */
    virtual void on_thunder_ready(bool boot = false);

protected:
    /**
     * This is the Constructor for the base class object. It is protected, as the derived Thunder Plugins will be Singletons.
     * @param name Thunder Plugin name, mostly used for logging.
     * @param callsign The callsign for the plugin, which is used in every call.
     */
    ctrlm_thunder_plugin_t(std::string name, std::string callsign, int api_version);

    /**
     * This function returns the callsign string with the associated API version appended to it.
     * @return <callsign>.<api_version>
     */
    std::string callsign_with_api();

    /**
     * This functions is used to call a Thunder Plugin method.
     * @param method The method in which the user wants to call.
     * @param params The WPEFramework JsonObject containing the parameters for the call. (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @param params The WPEFramework JsonObject containing the response from the call.  (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @return True if the call succeeded, otherwise False.
     */
    bool call_plugin(std::string method, void *params, void *response);

    /**
     * This functions is used to call a Thunder Controller method.
     * @param method The method in which the user wants to call.
     * @param params The WPEFramework JsonObject containing the parameters for the call. (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @param params The WPEFramework JsonObject containing the response from the call.  (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @return True if the call succeeded, otherwise False.
     */
    bool call_controller(std::string method, void *params, void *response);

    /**
     * This function is called when registering for Thunder events. Plugin classes should override this method.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

    /**
     * This function is called when the initial activation of the plugin occurs. Plugin classes should override this method.
     */
    virtual void on_initial_activation();

    /**
     * This callback function is used to perform actions when the plugin is activated. This gets performed on a non-Thunder thread.
     * @return 0 if actions were performed successfully, or 1 if this function needs to be rerun.
     */
    static int on_plugin_activated(void *data);

protected:
    std::string name;
    void       *plugin_client;

private:
    Thunder::Controller::ctrlm_thunder_controller_t *controller;
    std::string callsign;
    int         api_version;
    std::vector<std::pair<plugin_activation_handler_t, void *> > activation_callbacks;
    int  register_events_retry;
};
};
};

#endif