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

#include "ctrlm_ipc_iarm.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_ipc_voice.h"

void ctrlm_ipc_iarm_t::set_api_revision(unsigned char api_revision) {
    api_revision_ = api_revision;
}

unsigned char ctrlm_ipc_iarm_t::get_api_revision(void) const {
    return api_revision_;
}

bool ctrlm_ipc_iarm_t::register_iarm_call(const char *call, IARM_BusCall_t handler) const
{
    bool ret = true;
    IARM_Result_t rc;

    XLOGD_INFO("Registering for %s", call);
    rc = IARM_Bus_RegisterCall(call, handler);
    if(rc != IARM_RESULT_SUCCESS) {
        XLOGD_ERROR("Failed to register for %s", call);
        ret = false;
    }
    return(ret);
}

bool ctrlm_ipc_iarm_t::broadcast_iarm_event_legacy(const char *bus_name, int event, void *data, size_t data_size) const {
    bool ret = true;
    IARM_Result_t result = IARM_Bus_BroadcastEvent(bus_name, event, data, data_size);
    if(IARM_RESULT_SUCCESS != result) {
        XLOGD_ERROR("IARM Bus Error!");
        ret = false;
    }
    return(ret);
}
