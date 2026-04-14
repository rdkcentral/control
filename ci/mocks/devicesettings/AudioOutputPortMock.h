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
#include <list>

#include "audioOutputPort.hpp"

class AudioOutputPortMock : public device::AudioOutputPortImpl {
public:
    virtual ~AudioOutputPortMock() = default;

    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, getMS12AudioProfileList, (), (const, override));
    MOCK_METHOD(void, getAudioCapabilities, (int* capabilities), (override));
    MOCK_METHOD(void, getMS12Capabilities, (int* capabilities), (override));
    MOCK_METHOD(bool, isAudioMSDecode, (), (const,override));

    MOCK_METHOD(bool, getEnablePersist, (), (override));
    MOCK_METHOD(void, getHdmiArcPortId, (int *portId), (override));
    MOCK_METHOD(bool, isConnected, (), (override));
    MOCK_METHOD(void, setDRCMode, (int DRCMode), (override));
    MOCK_METHOD(void, setCompression, (int compressionLevel), (override));
    MOCK_METHOD(int, getCompression, (), (override));
    MOCK_METHOD(void, setDolbyVolumeMode, (bool dolbyVolumeMode), (override));
    MOCK_METHOD(void, setDialogEnhancement, (int enhancerlevel), (override));
    MOCK_METHOD(int, getDialogEnhancement, (), (override));
    MOCK_METHOD(void, setIntelligentEqualizerMode, (int intelligentEqualizerMode), (override));
    MOCK_METHOD(int, getIntelligentEqualizerMode, (), (override));
    MOCK_METHOD(void, setGraphicEqualizerMode, (int graphicEqualizerMode), (override));
    MOCK_METHOD(int, getGraphicEqualizerMode, (), (override));
    MOCK_METHOD(void, setMS12AudioProfile, (std::string audioProfileName), (override));
    MOCK_METHOD(void, resetVolumeLeveller, (), (override));
    MOCK_METHOD(void, resetSurroundVirtualizer, (), (override));
    MOCK_METHOD(void, resetBassEnhancer, (), (override));
    MOCK_METHOD(void, resetDialogEnhancement, (), (override));
    MOCK_METHOD(void, getSupportedARCTypes, (int *types), (override));
    MOCK_METHOD(void, setEnablePersist, (bool pEnable), (override));
    MOCK_METHOD(dsError_t, setEnablePort, (bool pEnable), (override));
    MOCK_METHOD(bool, isMuted, (), (override));
    MOCK_METHOD(void, setAudioAtmosOutputMode, (int enable), (override));
    MOCK_METHOD(bool, getSinkDeviceAtmosCapability, (dsATMOSCapability_t& atmosCapability), (override));
    MOCK_METHOD(void, setAudioDelayOffset, (int audioDelayOffsetMs), (override));
    MOCK_METHOD(void, setAudioDelay, (int audioDelayMs), (override));
    MOCK_METHOD(void, getAudioDelay, (uint32_t audioDelayMs), (override));
    MOCK_METHOD(void, getAudioDelayOffset, (uint32_t audioDelayOffsetMs), (override));
    MOCK_METHOD(void, getSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void,setSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void, setPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, getPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, getFaderControl, (int *mixerBalance), (override));
    MOCK_METHOD(void, setFaderControl, (int mixerBalance), (override));
    MOCK_METHOD(void, setAssociatedAudioMixing, (bool mixing), (override));
    MOCK_METHOD(void, setGain, (float newGain), (override));
    MOCK_METHOD(void, setMISteering, (bool MISteering), (override));
    MOCK_METHOD(void, setSurroundVirtualizer, (dsSurroundVirtualizer_t surroundVirtualizer), (override));
    MOCK_METHOD(void, setBassEnhancer, (int bassBoost), (override));
    MOCK_METHOD(void, enableSurroundDecoder, (bool enableSurroundDecoder), (override));
    MOCK_METHOD(void, setVolumeLeveller, (dsVolumeLeveller_t volumeLeveller), (override));
    MOCK_METHOD(bool, getMISteering, (), (override));
    MOCK_METHOD(dsSurroundVirtualizer_t, getSurroundVirtualizer, (), (override));
    MOCK_METHOD(int, getDRCMode, (), (override));
    MOCK_METHOD(float, getLevel, (), (override));
    MOCK_METHOD(float, getGain, (), (override));
    MOCK_METHOD(bool, isSurroundDecoderEnabled, (), (override));
    MOCK_METHOD(int, getBassEnhancer, (), (override));
    MOCK_METHOD(dsVolumeLeveller_t, getVolumeLeveller, (), (override));
    MOCK_METHOD(bool, getStereoAuto, (), (override));
    
    MOCK_METHOD(std::string, getMS12AudioProfile, (), (override));
    MOCK_METHOD(void, setMS12AudioProfileSetttingsOverride, (std::string audioProfileState,std::string audioProfileName,std::string audioProfileSettingsName, std::string audioProfileSettingValue), (override));

    MOCK_METHOD(void, setLevel, (float level), (override));
    MOCK_METHOD(void, setMuted, (bool muted), (override));
    MOCK_METHOD(void, getAssociatedAudioMixing, (bool *mixing), (override));
    MOCK_METHOD(bool, isEnabled, (), (override));
    MOCK_METHOD(void, setSAD, (std::vector<int> sad_list), (override));
    MOCK_METHOD(void, setStereoAuto, (bool stereoAuto, bool persist), (override));
    MOCK_METHOD(void, reInitializeAudioOutputPort, (), (override));

    MOCK_METHOD(std::vector<device::AudioStereoMode>, getSupportedStereoModes, (), (const, override));

    MOCK_METHOD(void, enableARC, (dsAudioARCTypes_t type, bool enable), (override));

    MOCK_METHOD(void, setStereoMode, (const std::string &mode, bool persist), (override));

    MOCK_METHOD(device::AudioStereoMode, getStereoMode, (const bool toPersist), (const, override));


    MOCK_METHOD(device::AudioStereoMode, getStereoMode, (), (const, override));

    MOCK_METHOD(uint32_t, getDolbyVolumeMode, (), (const, override));

    MOCK_METHOD(const device::AudioOutputPortType&, getType, (), (const, override));
    



};
