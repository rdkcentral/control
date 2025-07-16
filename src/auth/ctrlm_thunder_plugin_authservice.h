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
#ifndef __CTRLM_THUNDER_PLUGIN_AUTHSERVICE_H__
#define __CTRLM_THUNDER_PLUGIN_AUTHSERVICE_H__
#include "ctrlm_thunder_plugin.h"

namespace Thunder {
namespace AuthService {

typedef enum {
    EVENT_SAT_CHANGED,
    EVENT_ACCOUNT_ID_CHANGED,
    EVENT_ACTIVATION_STATUS_ACTIVATED
} authservice_event_t;

typedef void (*authservice_event_handler_t)(authservice_event_t event, void *event_data, void *user_data);

/**
 * This class is used within ControlMgr to interact with the Authservice Thunder Plugin. This implementation exposes methods to call for Authentication information,
 * along with some events that are important for the ControlMgr implementation.
 */
class ctrlm_thunder_plugin_authservice_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder Auth Service instance, as it is a Singleton.
     * @return The instance of the Thunder Remote Control, or NULL on error.
     */
    static ctrlm_thunder_plugin_authservice_t *getInstance();

    /**
     * Authservice Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_authservice_t();

    /**
     * Function that retrieves the Activation Status of the device.
     * @return True is the device is in the activated state otherwise False
     */
    bool is_device_activated();

    /**
     * Function that retrieves the Device ID from Authservice.
     * @param device_id The reference to a string which will contain the Device ID.
     * @return True on success otherwise False.
     */
    bool get_device_id(std::string &device_id);

    /**
     * Function that retrieves the Partner ID from Authservice.
     * @param partner_id The reference to a string which will contain the Partner ID.
     * @return True on success otherwise False.
     */
    bool get_partner_id(std::string &partner_id);

    /**
     * Function that retrieves the Account ID from Authservice.
     * @param account_id The reference to a string which will contain the Account ID.
     * @return True on success otherwise False.
     */
    bool get_account_id(std::string &account_id);

    /**
     * Function that retrieves the Experience String from Authservice.
     * @param experience The reference to a string which will contain the Experience String.
     * @return True on success otherwise False.
     */
    bool get_experience(std::string &experience);

    /**
     * Function that retrieves the SAT Token from Authservice.
     * @param sat The reference to a string which will contain the SAT Token.
     * @param expiration The reference to a time_t object containing the expiration time of the SAT token.
     * @return True on success otherwise False.
     */
    bool get_sat(std::string &sat, time_t &expiration);

    /**
     * This function is used to register a handler for the Authservice Thunder Plugin events.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_event_handler(authservice_event_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the Authservice Thunder Plugin events.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_event_handler(authservice_event_handler_t handler);

public:
    /** 
     * This function is technically used internally but from static function. This is used to broadcast SAT change events.
     */
    void on_sat_change();

    /** 
     * This function is technically used internally but from static function. This is used to broadcast Account ID change events.
     */
    void on_account_id_change(std::string new_account_id);

    /** 
     * This function is technically used internally but from static function. This is used to broadcast activation status change events.
     * @param status The activation status from the event.
     */
    void on_activation_status_change(std::string status);

protected:
    /**
     * This function is called when registering for Thunder events.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

    /**
     * Authservice Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_authservice_t();

private:
    std::vector<std::pair<authservice_event_handler_t, void *> > event_callbacks;
    bool registered_events;
};

const char *authservice_event_str(authservice_event_t event);

};
};

#endif