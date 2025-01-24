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
#ifndef __CTRLM_THUNDER_CONTROLLER_H__
#define __CTRLM_THUNDER_CONTROLLER_H__
#include <iostream>
#include <map>
#include <vector>
#include <utility>

/* Defines needed for Thunder */
#ifndef MODULE_NAME
#define MODULE_NAME controlMgr
#endif

#define SERVER_DETAILS            "127.0.0.1:9998"
#define CALL_TIMEOUT              (2000)
/* End Defines for Thunder */

namespace Thunder {

typedef enum {
    PLUGIN_BOOT_ACTIVATED,
    PLUGIN_ACTIVATING,
    PLUGIN_ACTIVATED,
    PLUGIN_DEACTIVATING,
    PLUGIN_DEACTIVATED,
    PLUGIN_INVALID
} plugin_state_t;

namespace Controller {


typedef void (*thunder_ready_handler_t)(void *user_data);
typedef void (*plugin_activation_handler_t)(plugin_state_t event, void *user_data);

/**
 * This class is used by the Thunder Plugin classes to interact with the Thunder Controller. The Thunder controller is used to Activate/Deactivate Plugins, along with receiving activation events.
 * This class MUST NOT be used within ControlMgr directly, rather by the base Thunder Plugin class.
 */
class ctrlm_thunder_controller_t {
public: 

    /**
     * This function is used to get the Thunder Controller instance, as it is a Singleton.
     * @return The instance of the Thunder Controller, or NULL on error.
     */
    static ctrlm_thunder_controller_t *getInstance();

    /**
     * Destructor for the Thunder Controller
     */
    virtual ~ctrlm_thunder_controller_t();

    /**
     * This function is used to see if Thunder is ready.
     * @return True if Thunder is ready otherwise False.
     */
    bool is_ready();

    /**
     * This functions is used to call a Thunder Controller method.
     * @param method The method in which the user wants to call.
     * @param params The WPEFramework JsonObject containing the parameters for the call. (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @param params The WPEFramework JsonObject containing the response from the call.  (We can't include WPEFramework headers in controlMgr .h files as their logging macros clash)
     * @return True if the call succeeded, otherwise False.
     */
    bool call(std::string method, void *params, void *response);

    /**
     * This function can be used to determine the state of a Thunder Plugin.
     * @param callsign The callsign of the Thunder Plugin.
     * @return The state that the plugin is in.
     */
    plugin_state_t get_plugin_state(std::string callsign);

    /**
     * This function is used to register a handler for the Thunder Controller events which contain Plugin state information.
     * @param callsign The callsign for the Thunder Plugin requesting the handler to be added.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_activation_handler(std::string callsign, plugin_activation_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the Thunder Controller events which contain Plugin state information.
     * @param callsign The callsign for the Thunder Plugin requesting the handler to be removed.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_activation_handler(std::string callsign, plugin_activation_handler_t handler);

    /**
     * This function is used to register a handler for the Thunder ready events.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_ready_handler(thunder_ready_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the Thunder ready events.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_ready_handler(thunder_ready_handler_t handler);

    /**
     * This function retrieves the security token for Thunder.
     * @return A string containing the security token or an empty string.
     */
    static std::string get_security_token();

public:
    /** 
     * This function is technically used internally but from static function. This is used to broadcast activation state changes.
     * @param callsign The callsign in which the activation state is in reference to.
     * @param state The ctrlm_thunder_plugin_state_t that the plugin transitioned to.
     */
    void on_activation_change(std::string callsign, plugin_state_t state);

protected:
    /**
     * Thunder Controller Default Constructor (Protected due to it being a Singleton)
     */
    ctrlm_thunder_controller_t();

    /**
     * This function is used to open the Thunder Controller Link.
     * @return True if successful otherwise False.
     */
    bool open_controller();

    /**
     * Static function used for retrying opening the Thunder Controller Link.
     * @param data The instance of ctrlm_thunder_controller_t
     * @return 1 if retry should occur, 0 if successful.
     */
    static int open_controller_retry(void *data);

    /**
     * Static function used to check if Thunder is active.
     * @return True if Thunder is active otherwise False.
     */
    static bool is_thunder_active();
private:
    void *controller_client;
    std::map<std::string, std::vector<std::pair<plugin_activation_handler_t, void *> > > activation_callbacks;
    std::vector<std::pair<thunder_ready_handler_t, void *> >                             ready_callbacks;
    unsigned int open_controller_retry_count;
    bool ready;
};
};

const char *plugin_state_str(plugin_state_t state);
};

#endif