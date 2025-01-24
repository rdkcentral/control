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

#ifndef __CTRLM_IPC_IARM_H__
#define __CTRLM_IPC_IARM_H__

#include "jansson.h"
#include "libIBus.h"
#include <cstring>
#include <atomic>

class ctrlm_ipc_iarm_t {
public:
    ctrlm_ipc_iarm_t() {};
    virtual ~ctrlm_ipc_iarm_t() {};

    virtual bool register_ipc() const = 0;
    virtual void deregister_ipc() const = 0;

protected:
    static bool is_running(std::atomic_bool &abool) { return (abool.load() == true) ? true : false; }
    static void turn_on(std::atomic_bool &abool) { abool.store(true); }
    static void turn_off(std::atomic_bool &abool) { abool.store(false); }

    bool register_iarm_call(const char *call, IARM_BusCall_t handler) const;
    bool broadcast_iarm_event(const char *bus_name, int event, json_t* event_data) const;
};

#endif
