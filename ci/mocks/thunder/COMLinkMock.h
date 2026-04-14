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

#ifndef RDKSERVICES_TESTS_MOCKS_COMLINKMOCK_H_
#define RDKSERVICES_TESTS_MOCKS_COMLINKMOCK_H_

#include <gmock/gmock.h>

#include "Module.h"

#ifndef USE_THUNDER_R4
class COMLinkMock : public WPEFramework::PluginHost::IShell::ICOMLink {
public:
    virtual ~COMLinkMock() = default;

    MOCK_METHOD(void, Register, (WPEFramework::RPC::IRemoteConnection::INotification*), (override));
    MOCK_METHOD(void, Unregister, (WPEFramework::RPC::IRemoteConnection::INotification*), (override));
    MOCK_METHOD(WPEFramework::RPC::IRemoteConnection*, RemoteConnection, (const uint32_t), (override));
    MOCK_METHOD(void*, Instantiate, (const WPEFramework::RPC::Object&, const uint32_t, uint32_t&, const string&, const string&), (override));
};
#else
class COMLinkMock : public WPEFramework::PluginHost::IShell::ICOMLink {
public:
    virtual ~COMLinkMock() = default;

    MOCK_METHOD(void, Register, (WPEFramework::RPC::IRemoteConnection::INotification*), (override));
    MOCK_METHOD(void, Unregister, (const WPEFramework::RPC::IRemoteConnection::INotification*), (override));
    MOCK_METHOD(void, Register, (INotification*), (override));
    MOCK_METHOD(void, Unregister, (INotification*), (override));
    MOCK_METHOD(WPEFramework::RPC::IRemoteConnection*, RemoteConnection, (const uint32_t), (override));
    MOCK_METHOD(void*, Instantiate, (const WPEFramework::RPC::Object&, const uint32_t, uint32_t&), (override));
};
#endif /* ! USE_THUNDER_R4 */
#endif //RDKSERVICES_TESTS_MOCKS_COMLINKMOCK_H_
