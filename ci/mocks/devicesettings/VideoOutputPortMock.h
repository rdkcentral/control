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

#include "videoOutputPort.hpp"

class VideoOutputPortMock : public device::VideoOutputPortImpl {
public:
    virtual ~VideoOutputPortMock() = default;

    MOCK_METHOD(const device::VideoOutputPortType&, getType, (), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(const device::VideoResolution&, getDefaultResolution, (), (const, override));
    MOCK_METHOD(int, getHDCPProtocol, (), (override));
    MOCK_METHOD(int, getHDCPReceiverProtocol, (), (override));
    MOCK_METHOD(int, getHDCPCurrentProtocol, (), (override));
    MOCK_METHOD(device::AudioOutputPort&, getAudioOutputPort, (), (const, override));
    MOCK_METHOD(bool, isDisplayConnected, (), (override));
    MOCK_METHOD(int, getHDCPStatus, (), (override));
    MOCK_METHOD(bool, isContentProtected, (), (override));
    MOCK_METHOD(device::Display&, getDisplay, (), (override));
    MOCK_METHOD(bool, setScartParameter, (std::string sScartParameter, std::string sScartParameterData), (override));
    MOCK_METHOD(void, getTVHDRCapabilities, (int* capabilities), (override));
    MOCK_METHOD(bool, setForceHDRMode, (dsHDRStandard_t mode), (override));
    MOCK_METHOD(int, getPreferredColorDepth, (bool persist), (override));
    MOCK_METHOD(void, getColorDepthCapabilities, (unsigned int* capabilities), (override));
    MOCK_METHOD(void, setPreferredColorDepth, (dsDisplayColorDepth_t colorDepth, bool persist), (override));
    MOCK_METHOD(bool, getCurrentOutputSettings, (int& videoEOTF, int& matrixCoefficients, int& colorSpace, int& colorDepth, int& quantizationRange), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(void, setResolution, (std::string resolution, bool persist, bool isIgnoreEdid), (override));
    MOCK_METHOD(const device::VideoResolution&, getResolution, (), (const, override));
    MOCK_METHOD(void, getSupportedTvResolutions, (int* tvResolutions), (override));
    MOCK_METHOD(int, getVideoEOTF, (), (override));
    MOCK_METHOD(bool, isActive, (), (override));
    MOCK_METHOD(void, setAllmEnabled, (bool enable), (override));
    MOCK_METHOD(int, GetHdmiPreference, (), (override));
    MOCK_METHOD(bool, SetHdmiPreference, (dsHdcpProtocolVersion_t hdcpProtocol), (override));
    MOCK_METHOD(int, getColorSpace, (), (const, override));
    MOCK_METHOD(int, getColorDepth, (), (const, override));
    MOCK_METHOD(int, getQuantizationRange, (), (const, override));
    MOCK_METHOD(bool, IsOutputHDR, (), (override));
};
