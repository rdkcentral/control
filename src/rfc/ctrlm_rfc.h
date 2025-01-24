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
#ifndef __CTRLM_RFC_H__
#define __CTRLM_RFC_H__
#include <map>
#include "ctrlm_rfc_attr.h"

/**
 * @brief ControlMgr RFC Class
 * 
 * This class is a singleton that manages all RFC interactions
 */
class ctrlm_rfc_t {
/**
 * This is the callback type that will be called when an RFC fetch has completed
 */
typedef std::function<void(void)> ctrlm_rfc_fetch_completed_t;

public:
    enum attrs {
        VSDK,
        VOICE,
        RF4CE,
        BLE,
        IP,
        GLOBAL,
        DEVICE_UPDATE
    };
public:
    /**
     * This function is used to get the ControlMgr RFC instance, as it is a Singleton.
     * @return The instance of the ControlMgr RFC class, or NULL on error.
     */
    static ctrlm_rfc_t *get_instance();
    /**
     * This function is used to destroy the ControlMgr RFC instance, as it is a Singleton.
     */
    static void destroy_instance();
    /**
     * ControlMgr RFC Destructor
     */
    virtual ~ctrlm_rfc_t();

public:
    /**
     * This function initiates the RFC component to retrieve all of the RFC values.
     */
    void fetch_attributes();

public:
    /**
     * Adds the callback that is called when an RFC fetch has been completed
     * @param listener The callback
     */
    void add_fetch_completed_listener(const ctrlm_rfc_fetch_completed_t &listener);
    /**
     * Removes the callback that is called when an RFC fetch has been completed
     * @param listener The callback
     */
    void remove_fetch_completed_listener(const ctrlm_rfc_fetch_completed_t &listener);

    /**
     * Adds the callback that is called when an RFC attribute value has changed
     * @param type The RFC attribute type
     * @param listener The callback
     */
    void add_changed_listener(ctrlm_rfc_t::attrs type, const ctrlm_rfc_attr_changed_t &listener);
    /**
     * Removes the callback that is called when an RFC attribute value has changed
     * @param type The RFC attribute type
     * @param listener The callback
     * @param user_data Application data that the developer wants to pass to the callback
     */
    void remove_changed_listener(ctrlm_rfc_t::attrs type, const ctrlm_rfc_attr_changed_t &listener);

protected:
    /**
     * This function is used to add an Attribute to the RFC component
     * @param attr The attribute to be added 
     */
    void add_attribute(attrs type, ctrlm_rfc_attr_t *attr);
    /**
     * This function is used to remove an Attribute from the RFC component
     * @param attr The attribute to be removed 
     */
    void remove_attribute(attrs type);

private:
    /**
     * ControlMgr RFC Default Constructor (Private due to it being a Singleton)
     */
    ctrlm_rfc_t();

private:
    /**
     * Helper function which calls the TR181 manager to get the value of an attribute
     * @param attr The attribute we want the value from
     */
    std::string tr181_call(ctrlm_rfc_attr_t *attr);

private:
    std::map<attrs, ctrlm_rfc_attr_t *> attributes;
    std::vector<ctrlm_rfc_fetch_completed_t> listeners;
};

#endif
