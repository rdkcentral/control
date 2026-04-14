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

#include "host.hpp"

class VideoDeviceMock : public device::VideoDeviceImpl {
public:
    virtual ~VideoDeviceMock() = default;

    MOCK_METHOD(int, getFRFMode, (int* frfmode), (const, override));
    MOCK_METHOD(int, setFRFMode, (int frfmode), (const, override));
    MOCK_METHOD(int, getCurrentDisframerate, (char* framerate), (const, override));
    MOCK_METHOD(int, setDisplayframerate, (const char* framerate), (const, override));
    MOCK_METHOD(void, getHDRCapabilities, (int* capabilities), (override));
    MOCK_METHOD(void, getSettopSupportedResolutions, (std::list<std::string>& resolutions), (override));
    MOCK_METHOD(void, setDFC, (std::string zoomSetting), (override));
    // MOCK_METHOD(const VideoDFC&, getDFC, (), (const, override));
     MOCK_METHOD(const device::VideoDFC&, getDFC, (), (override));
};
