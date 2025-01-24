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

#ifndef __CTRLM_IRDB_IPC_IARM_THUNDER_H__
#define __CTRLM_IRDB_IPC_IARM_THUNDER_H__
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_iarm.h"
#include "ctrlm_irdb.h"
#include "libIBus.h"

class ctrlm_irdb_ipc_iarm_thunder_t : public ctrlm_ipc_iarm_t {
public:
    ctrlm_irdb_ipc_iarm_thunder_t();
    virtual ~ctrlm_irdb_ipc_iarm_thunder_t();

    bool register_ipc() const override;
    void deregister_ipc() const override;

protected:
    static ctrlm_irdb_dev_type_t ctrlm_irdb_dev_type_from_iarm(ctrlm_ir_device_type_t dev_type);
    static bool ctrlm_irdb_dev_type_is_valid(const std::string av_dev_str, ctrlm_ir_device_type_t &ir_dev_type);
    static const char *ctrlm_irdb_dev_type_str(ctrlm_irdb_dev_type_t dev_type);

    static IARM_Result_t get_manufacturers(void *arg);
    static IARM_Result_t get_models(void *arg);
    static IARM_Result_t get_ir_codes_by_names(void *arg);
    static IARM_Result_t get_ir_codes_by_auto_lookup(void *arg);
    static IARM_Result_t set_ir_codes_by_name(void *arg);
    static IARM_Result_t clear_ir_codes(void *arg);
    static IARM_Result_t initialize_irdb(void *arg);
};

#endif
