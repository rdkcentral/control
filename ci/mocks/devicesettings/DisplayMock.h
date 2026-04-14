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

#include "devicesettings.h"

class DisplayMock : public device::DisplayImpl {
public:
    virtual ~DisplayMock() = default;

    MOCK_METHOD(void, getEDIDBytes, (std::vector<uint8_t> &edid), (const, override));
    MOCK_METHOD(bool, isConnectedDeviceRepeater, (), (override));
    MOCK_METHOD(int, getSurroundMode, (), (override));
    MOCK_METHOD(void, setAllmEnabled, (bool enable), (override));
    MOCK_METHOD(void, setAVIContentType, (int contentType), (override));
    MOCK_METHOD(void, setAVIScanInformation, (int scanInfo), (override));
};
