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

class FrontPanelTextDisplayMock : public device::FrontPanelTextDisplayImpl {
public:
    virtual ~FrontPanelTextDisplayMock() = default;
    MOCK_METHOD(device::FrontPanelTextDisplay&, getInstanceByName, (const std::string& name), (override));
    MOCK_METHOD(device::FrontPanelTextDisplay&, getInstanceById, (int id), (override));
    MOCK_METHOD(int,getCurrentTimeFormat, (), (const, override));
    MOCK_METHOD(void,setTimeFormat, (const int iTimeFormat), (const, override));
    MOCK_METHOD(int,getTextBrightness, (), (const, override));
    MOCK_METHOD(void,setTextBrightness, (const int brightness), (const, override));
    MOCK_METHOD(void,setText, (const std::string text), (const, override));
    MOCK_METHOD(void,setMode, (const int mode), (const, override));
};
