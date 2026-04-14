/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

#pragma once

#include <gmock/gmock.h>
#include "Communicator.h"

#if 0
class CommunicatorClientMock : public ICommunicatorClient
{
public:
    virtual ~CommunicatorClientMock() = default;

    MOCK_METHOD(uint32_t, Open, (const uint32_t waitTime), (override));
    MOCK_METHOD(INTERFACE*, Open, (const std::string& className, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(INTERFACE*, Acquire, (const uint32_t waitTime, const std::string& className, const uint32_t versionId), (override));
    MOCK_METHOD(uint32_t, Offer, (INTERFACE* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Revoke, (INTERFACE* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Close, (const uint32_t waitTime), (override));
};
#else
class CommunicatorClientMock : public ICommunicatorClient {
public:
    virtual ~CommunicatorClientMock() = default;
    MOCK_METHOD(void, AddRef, (), (const));
    MOCK_METHOD(uint32_t, Release, (), (const));
    MOCK_METHOD(uint32_t, Open, (const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Open, (const std::string& className, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Acquire, (const uint32_t waitTime, const std::string& className, const uint32_t versionId), (override));
    MOCK_METHOD(uint32_t, Offer, (void* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Revoke, (void* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Close, (const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Acquire, (const std::string& className, const uint32_t interfaceId, const uint32_t versionId), (override));
};

#endif
