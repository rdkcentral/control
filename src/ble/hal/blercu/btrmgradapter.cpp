/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
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

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

#include "btrmgradapter.h"

#include "ctrlm_log_ble.h"

#include <btmgr.h>

#include <thread>
#include <chrono>
#include "libIBus.h"

#define LOG_IF_FAILED(func, ... )   \
            do {    \
                auto __result = func(__VA_ARGS__);  \
                if(__result != BTRMGR_RESULT_SUCCESS)   \
                {   \
                    XLOGD_ERROR("call to %s failed with result %d", #func, (int)__result);  \
                }   \
            } while(false)


namespace
{
    unsigned char getNumberOfAdapters() noexcept
    {
        unsigned char result{};
        int isRegistered = 0;
        IARM_Result_t retCode = IARM_RESULT_SUCCESS;
        retCode = IARM_Bus_IsConnected("BTRMgrBus", &isRegistered);
        if (retCode != IARM_RESULT_SUCCESS || isRegistered == 0 ) {
            XLOGD_ERROR("BTRMgrBus not connected with IARM err code - %d, isRegistered - %d ...", retCode, isRegistered);
            return 0;
        }

        XLOGD_INFO("Method to get Number Of Adapters from btmgr : retCode - %d isRegistered - %d", retCode, isRegistered);
        if (isRegistered) {
           auto ret = BTRMGR_GetNumberOfAdapters(&result);
           if (ret != BTRMGR_RESULT_SUCCESS) {
               XLOGD_ERROR("call to BTRMGR_GetNumberOfAdapters failed with result %d, returning 0 adapters...", ret);
               return 0;
           } else {
               return result;
           }
       }
       return 0;
    }

    struct DiscoveryState
    {
        BTRMGR_DiscoveryStatus_t status = BTRMGR_DISCOVERY_STATUS_OFF;
        BTRMGR_DeviceOperationType_t operationType = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;
    };

    DiscoveryState getDiscoveryState(unsigned char adapterIdx) noexcept
    {
        auto state = DiscoveryState{};

        LOG_IF_FAILED(BTRMGR_GetDiscoveryStatus, adapterIdx, &state.status, &state.operationType);

        return state;
    }
}

BtrMgrAdapter::ApiInitializer::ApiInitializer() noexcept
{
    LOG_IF_FAILED(BTRMGR_Init);
}

BtrMgrAdapter::ApiInitializer::~ApiInitializer() noexcept
{
    LOG_IF_FAILED(BTRMGR_DeInit);
}

BtrMgrAdapter::BtrMgrAdapter() noexcept
{
    SetAdapterIndex();
}

void BtrMgrAdapter::SetAdapterIndex() noexcept
{
    const auto numOfAdapters = getNumberOfAdapters();
    adapterIdx = numOfAdapters-1;
}

void BtrMgrAdapter::startDiscovery(BtrMgrAdapter::OperationType requestedOperationType) noexcept
{
    if (adapterIdx == 0xFF) {
        SetAdapterIndex();
    }
    LOG_IF_FAILED(BTRMGR_StartDeviceDiscovery, adapterIdx, requestedOperationType);
}

BtrMgrAdapter::OperationType BtrMgrAdapter::stopDiscovery() noexcept
{
    if (adapterIdx == 0xFF) {
        SetAdapterIndex();
    }
    const auto state = getDiscoveryState(adapterIdx);

    LOG_IF_FAILED(BTRMGR_StopDeviceDiscovery, adapterIdx, state.operationType);
    return state.operationType;
}

bool BtrMgrAdapter::isDiscoveryInProgress() noexcept
{
    if (adapterIdx == 0xFF) {
        SetAdapterIndex();
    }
    return getDiscoveryState(adapterIdx).status  == BTRMGR_DISCOVERY_STATUS_IN_PROGRESS;
}
