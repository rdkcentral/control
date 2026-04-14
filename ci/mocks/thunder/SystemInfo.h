/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/

#pragma once

#include <gmock/gmock.h>

#include "Module.h"

class SystemInfo : public WPEFramework::PluginHost::ISubSystem {
private:
    SystemInfo(const SystemInfo&) = delete;
    SystemInfo& operator=(const SystemInfo&) = delete;

public:
    SystemInfo()
        : _flags(0)
    {
        // Defaults:
        ON_CALL(*this, Set(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const subsystem type, WPEFramework::Core::IUnknown* information) {
                    _subsystems.emplace(type, information);

                    if (type >= NEGATIVE_START) {
                        _flags &= ~(1 << (type - NEGATIVE_START));
                    } else {
                        _flags |= (1 << type);
                    }
                }));
        ON_CALL(*this, Get(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const subsystem type) -> const WPEFramework::Core::IUnknown* {
                    const WPEFramework::Core::IUnknown* result(nullptr);

                    auto it = _subsystems.find(type);
                    if (it != _subsystems.end()) {
                        result = it->second;
                    }

                    return result;
                }));
        ON_CALL(*this, IsActive(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const subsystem type) -> bool {
                    return ((type < END_LIST) && ((_flags & (1 << type)) != 0));
                }));
    }
    virtual ~SystemInfo() = default;

public:
    MOCK_METHOD(void, Register, (WPEFramework::PluginHost::ISubSystem::INotification * notification), (override));
    MOCK_METHOD(void, Unregister, (WPEFramework::PluginHost::ISubSystem::INotification * notification), (override));
    MOCK_METHOD(string, BuildTreeHash, (), (const, override));
    MOCK_METHOD(void, Set, (const subsystem type, WPEFramework::Core::IUnknown* information), (override));
    MOCK_METHOD(const WPEFramework::Core::IUnknown*, Get, (const subsystem type), (const, override));
    MOCK_METHOD(bool, IsActive, (const subsystem type), (const, override));
#ifdef USE_THUNDER_R4
    MOCK_METHOD(string, Version, (), (const, override));
#endif /*USE_THUNDER_R4 */

    BEGIN_INTERFACE_MAP(SystemInfo)
    INTERFACE_ENTRY(WPEFramework::PluginHost::ISubSystem)
    END_INTERFACE_MAP

private:
    std::map<subsystem, WPEFramework::Core::IUnknown*> _subsystems;
    uint32_t _flags;
};
