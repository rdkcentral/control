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

class FactoriesImplementation : public WPEFramework::PluginHost::IFactories {
public:
    FactoriesImplementation(const FactoriesImplementation&) = delete;
    FactoriesImplementation& operator=(const FactoriesImplementation&) = delete;

    FactoriesImplementation()
        : _requestFactory(5)
        , _responseFactory(5)
        , _fileBodyFactory(5)
        , _jsonRPCFactory(5)
    {
        // Defaults:
        ON_CALL(*this, Request())
            .WillByDefault(::testing::Invoke(
                [&]() { return (_requestFactory.Element()); }));
        ON_CALL(*this, Response())
            .WillByDefault(::testing::Invoke(
                [&]() { return (_responseFactory.Element()); }));
        ON_CALL(*this, FileBody())
            .WillByDefault(::testing::Invoke(
                [&]() { return (_fileBodyFactory.Element()); }));
        ON_CALL(*this, JSONRPC())
            .WillByDefault(::testing::Invoke(
                [&]() {
                    return (WPEFramework::Core::ProxyType<WPEFramework::Web::JSONBodyType<WPEFramework::Core::JSONRPC::Message>>(_jsonRPCFactory.Element()));
                }));
    }

    virtual ~FactoriesImplementation() = default;

    MOCK_METHOD(WPEFramework::Core::ProxyType<WPEFramework::Web::Request>, Request, (), (override));
    MOCK_METHOD(WPEFramework::Core::ProxyType<WPEFramework::Web::Response>, Response, (), (override));
    MOCK_METHOD(WPEFramework::Core::ProxyType<WPEFramework::Web::FileBody>, FileBody, (), (override));
    MOCK_METHOD(WPEFramework::Core::ProxyType<WPEFramework::Web::JSONBodyType<WPEFramework::Core::JSONRPC::Message>>, JSONRPC, (), (override));

private:
    WPEFramework::Core::ProxyPoolType<WPEFramework::Web::Request> _requestFactory;
    WPEFramework::Core::ProxyPoolType<WPEFramework::Web::Response> _responseFactory;
    WPEFramework::Core::ProxyPoolType<WPEFramework::Web::FileBody> _fileBodyFactory;
    WPEFramework::Core::ProxyPoolType<WPEFramework::PluginHost::JSONRPCMessage> _jsonRPCFactory;
};
