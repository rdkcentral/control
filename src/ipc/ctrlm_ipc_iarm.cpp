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

bool ctrlm_ipc_iarm_t::broadcast_iarm_event(const char *bus_name, int event, json_t* event_data) const
{
    bool ret = false;
    if(!event_data) {
        return(ret);
    }

    char *payload_str = json_dumps(event_data, JSON_COMPACT);

    if(payload_str != NULL) {
        size_t str_size = strlen(payload_str) + 1;
        size_t size = sizeof(ctrlm_main_iarm_event_json_t) + str_size;

        ctrlm_main_iarm_event_json_t *data = (ctrlm_main_iarm_event_json_t *)calloc(1, size);
        if (data == NULL) {
            XLOGD_ERROR("failed to allocate memory for the IARM event, so cannot broadcast....");
        } else {

            data->api_revision = CTRLM_MAIN_IARM_BUS_API_REVISION;
            //Can't be replaced with safeC version of this, as safeC string functions doesn't allow string size more than 4K
            snprintf(data->payload, str_size, "%s", payload_str);

            IARM_Result_t res = IARM_Bus_BroadcastEvent(bus_name, event, data, size);
            if(res != IARM_RESULT_SUCCESS) {
                XLOGD_ERROR("IARM Bus Error %d", res);
            } else {
                ret = true;
            }
            
            free(data);
        }

        free(payload_str);
    }

    if(event_data) {
        json_decref(event_data);
    }

    return(ret);
}
