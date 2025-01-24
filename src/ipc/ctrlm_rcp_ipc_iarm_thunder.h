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

#ifndef __CTRLM_RCP_IPC_IARM_THUNDER_H__
#define __CTRLM_RCP_IPC_IARM_THUNDER_H__

#include <semaphore.h>
#include <mutex>
#include <memory>
#include "ctrlm_ipc_iarm.h"
#include "ctrlm_ipc.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcp_ipc_event.h"

namespace rcp_json_keys
{
    constexpr char const* TIMEOUT              = "timeout";
    constexpr char const* LEVEL                = "level";
    constexpr char const* TIMESTAMP            = "timestamp";
    constexpr char const* SOURCE_NAME          = "sourceName";
    constexpr char const* SOURCE_TYPE          = "sourceType";
    constexpr char const* SOURCE_KEY_CODE      = "sourceKeyCode";
    constexpr char const* SCREENBIND_MODE      = "bIsScreenBindMode";
    constexpr char const* REMOTE_KEYPAD_CONFIG = "remoteKeypadConfig";
    constexpr char const* SUCCESS              = "success";
}

class ctrlm_rcp_ipc_iarm_thunder_t : public ctrlm_ipc_iarm_t
{
private:
    ctrlm_rcp_ipc_iarm_thunder_t();

    static std::mutex                                    instance_mutex_;
    static std::atomic_bool                              atomic_running_;
    static std::unique_ptr<ctrlm_rcp_ipc_iarm_thunder_t> instance_;

public:
    ~ctrlm_rcp_ipc_iarm_thunder_t();

    static ctrlm_rcp_ipc_iarm_thunder_t *get_instance();

    ctrlm_rcp_ipc_iarm_thunder_t(ctrlm_rcp_ipc_iarm_thunder_t &other) = delete;
    void operator=(const ctrlm_rcp_ipc_iarm_thunder_t &) = delete;

    bool register_ipc() const override;
    void deregister_ipc() const override;

    bool on_status(const ctrlm_rcp_ipc_net_status_t &net_status) const;

protected:
    static IARM_Result_t start_pairing(void *arg);
    static IARM_Result_t get_net_status(void *arg);
    static IARM_Result_t get_last_keypress(void *arg);
    static IARM_Result_t find_my_remote(void *arg);
    static IARM_Result_t factory_reset(void *arg);
    static IARM_Result_t write_rcu_wakeup_config(void *arg);

    template <typename T1, typename T2>
    static void sync_send_netw_handler_to_main_queue(T1 params, ctrlm_msg_handler_network_t handler)
    {
        T2 msg = {};

        msg.params         = params;
        msg.params->result = CTRLM_IARM_CALL_RESULT_ERROR;

        ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, handler, &msg, sizeof(msg), NULL, params->network_id, true);
    }

    template <typename T1, typename T2>
    static void sync_send_netw_handler_to_main_queue_new(std::shared_ptr<T1> params, ctrlm_msg_handler_network_t handler)
    {
        std::shared_ptr<T2> msg = std::make_shared<T2>();

        msg->params    = params;
        msg->semaphore = NULL;

        ctrlm_main_queue_handler_push_new<ctrlm_msg_handler_network_t, T2>(CTRLM_HANDLER_NETWORK, handler, std::move(msg), NULL, params->get_net_id(), true);
    }
};

#endif
