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

#include "hdmiIn.hpp"

class HdmiInputImplMock : public device::HdmiInputImpl {
public:
    virtual ~HdmiInputImplMock() = default;

    MOCK_METHOD(uint8_t, getNumberOfInputs, (), (const, override));
    MOCK_METHOD(bool, isPortConnected, (int8_t Port), (const, override));
    MOCK_METHOD(std::string, getCurrentVideoMode, (), (const, override));
    MOCK_METHOD(void, selectPort, (int8_t Port,bool audioMix, int videoPlane,bool topMost), (const, override));
    MOCK_METHOD(void, scaleVideo, (int32_t x, int32_t y, int32_t width, int32_t height), (const, override));
    MOCK_METHOD(void, getEDIDBytesInfo, (int iHdmiPort, std::vector<uint8_t> &edid), (const, override));
    MOCK_METHOD(void, getHDMISPDInfo, (int iHdmiPort, std::vector<uint8_t> &data), (const, override));
    MOCK_METHOD(void, setEdidVersion, (int iHdmiPort, int iEdidVersion), (const, override));
    MOCK_METHOD(void, getEdidVersion, (int iHdmiPort, int *iEdidVersion), (const, override));
    MOCK_METHOD(void, setEdid2AllmSupport, (int iHdmiPort, bool allmSupport), (const, override));
    MOCK_METHOD(void, getEdid2AllmSupport, (int iHdmiPort, bool *allmSupport), (const, override));
    MOCK_METHOD(void, setVRRSupport, (int iHdmiPort, bool vrrsupport), (const, override));
    MOCK_METHOD(void, getVRRSupport, (int iHdmiPort, bool *vrrsupport), (const, override));
    // MOCK_METHOD(void, setAudioMixerLevels, (dsAudioInput_t gain, int volume), (const, override));
    MOCK_METHOD(void, getHdmiALLMStatus, (int iHdmiPort, bool *allmStatus), (const, override));
    MOCK_METHOD(void, getVRRStatus, (int iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus), (const, override));
    MOCK_METHOD(void, getSupportedGameFeatures, (std::vector<std::string> &featureList), (const, override));
    MOCK_METHOD(void, getAVLatency, (int *audio_output_delay,int *video_latency), (const, override));
    MOCK_METHOD(void, getCurrentVideoModeObj, (dsVideoPortResolution_t& resolution), (const, override));
    MOCK_METHOD(void, getHdmiVersion, (int iHdmiPort, dsHdmiMaxCapabilityVersion_t *capVersion), (const, override));
    MOCK_METHOD(dsError_t, getHDMIARCPortId, (int &portId), (const, override));
};
