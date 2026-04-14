/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#ifndef MOCKAUTHSERVICES_H
#define MOCKAUTHSERVICES_H

#include <gmock/gmock.h>

#include "Module.h"

class MockAuthService : public WPEFramework::Exchange::IAuthService {
public:
    virtual ~MockAuthService() = default;
    MOCK_METHOD(uint32_t, GetActivationStatus, (ActivationStatusResult&), (override));
    MOCK_METHOD(uint32_t, SetActivationStatus, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearAuthToken, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearSessionToken, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearServiceAccessToken, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearLostAndFoundAccessToken, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearServiceAccountId, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, ClearCustomProperties, (SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetCustomProperties, (std::string&, bool&), (override));
    MOCK_METHOD(uint32_t, SetCustomProperties, (const std::string&, bool&), (override));
    MOCK_METHOD(uint32_t, GetAlternateIds, (std::string&, std::string&, bool&), (override));
    MOCK_METHOD(uint32_t, SetAlternateIds, (const std::string&, std::string&, bool&), (override));
    MOCK_METHOD(uint32_t, GetTransitionData, (std::string&, std::string&, bool&), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (uint32_t), (override));
    MOCK_METHOD(uint32_t, Register, (IAuthService::INotification*), (override));
    MOCK_METHOD(uint32_t, Unregister, (IAuthService::INotification*), (override));
    MOCK_METHOD(uint32_t, Configure, (), (override));
    MOCK_METHOD(uint32_t, GetInfo, (GetInfoResult&), (override));
    MOCK_METHOD(uint32_t, GetDeviceInfo, (GetDeviceInfoResult&), (override));
    MOCK_METHOD(uint32_t, GetDeviceId, (GetDeviceIdResult&), (override));
    MOCK_METHOD(uint32_t, SetDeviceId, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, SetPartnerId, (const std::string&, SetPartnerIdResult&), (override));
    MOCK_METHOD(uint32_t, GetAuthToken, (const bool, const bool, GetAuthTokenResult&), (override));
    MOCK_METHOD(uint32_t, GetSessionToken, (GetSessionTokenResult&), (override));
    MOCK_METHOD(uint32_t, SetSessionToken, (const int32_t&, const std::string&, uint32_t, const std::string&, const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetServiceAccessToken, (GetServiceAccessTokenResult&), (override));
    MOCK_METHOD(uint32_t, SetServiceAccessToken, (const int32_t&, const std::string&, uint32_t, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetServiceAccountId, (GetServiceAccountIdResult&), (override));
    MOCK_METHOD(uint32_t, SetServiceAccountId, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, SetAuthIdToken, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, Ready, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetBootstrapProperty, (const std::string&, GetBootstrapPropResult&), (override));
    MOCK_METHOD(uint32_t, ActivationStarted, (SuccessResult&), (override));
    MOCK_METHOD(uint32_t, ActivationComplete, (SuccessResult&), (override));
    MOCK_METHOD(uint32_t, GetLostAndFoundAccessToken, (std::string&, std::string&, bool&), (override));
    MOCK_METHOD(uint32_t, SetLostAndFoundAccessToken, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetXDeviceId, (GetXDeviceIdResult&), (override));
    MOCK_METHOD(uint32_t, SetXDeviceId, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetExperience, (GetExpResult&), (override));
    MOCK_METHOD(uint32_t, SetExperience, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetXifaId, (GetxifaIdResult&), (override));
    MOCK_METHOD(uint32_t, SetXifaId, (const std::string&, SuccessMsgResult&), (override));
    MOCK_METHOD(uint32_t, GetAdvtOptOut, (AdvtOptOutResult&), (override));
    MOCK_METHOD(uint32_t, SetAdvtOptOut, (const bool&, SuccessMsgResult&), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, SetContentAccessToken, (const std::string&, const uint32_t, bool&), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetContentAccessToken, (std::string&, uint32_t&), (override));
};

class MockIAuthenticate : public WPEFramework::PluginHost::IAuthenticate {
public:
    MOCK_METHOD(void*, QueryInterfaceByCallsign, (const uint32_t, const string&));
    MOCK_METHOD(uint32_t, CreateToken, (uint16_t, const uint8_t*, std::string&));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (uint32_t), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(WPEFramework::PluginHost::ISecurity*, Officer, (const std::string& token), (override));
};
#endif
