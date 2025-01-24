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

//
//  btrmgradapter.h
//

#ifndef BTRMGRADAPTER_H
#define BTRMGRADAPTER_H

#include <btmgr.h>

class BtrMgrAdapter
{
public:

    class ApiInitializer
    {
    public:
        ApiInitializer() noexcept;
        ~ApiInitializer() noexcept;
    };

    using OperationType = BTRMGR_DeviceOperationType_t;
    static constexpr OperationType unknownOperation = BTRMGR_DEVICE_OP_TYPE_UNKNOWN;

    BtrMgrAdapter() noexcept;

    void SetAdapterIndex() noexcept;
    void startDiscovery(OperationType requestedOperationType) noexcept;
    OperationType stopDiscovery() noexcept;
    bool isDiscoveryInProgress() noexcept;

private:
    unsigned char adapterIdx{};
};


#endif // !defined(BTRMGRADAPTER_H)
