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

class HostImplMock : public device::HostImpl {
public:
    virtual ~HostImplMock() = default;

    MOCK_METHOD(device::SleepMode, getPreferredSleepMode, (), (override));
    MOCK_METHOD(int, setPreferredSleepMode, (const device::SleepMode), (override));
    MOCK_METHOD(device::List<device::SleepMode>, getAvailableSleepModes, (), (override));
    MOCK_METHOD(device::List<device::VideoOutputPort>, getVideoOutputPorts, (), (override));
    MOCK_METHOD(device::List<device::AudioOutputPort>, getAudioOutputPorts, (), (override));
    MOCK_METHOD(device::List<device::VideoDevice>, getVideoDevices, (), (override));
    MOCK_METHOD(device::VideoOutputPort&, getVideoOutputPort, (const std::string& name), (override));
    MOCK_METHOD(device::AudioOutputPort&, getAudioOutputPort, (const std::string& name), (override));
    MOCK_METHOD(void, getHostEDID, (std::vector<uint8_t> & edid), (const, override));
    MOCK_METHOD(std::string, getDefaultVideoPortName, (), (override));
    //MOCK_METHOD(void, setAudioMixerLevels, (dsAudioInput_t aInput, int volume), (override));
    MOCK_METHOD(std::string, getDefaultAudioPortName, (), (override));
    
    MOCK_METHOD(void, getMS12ConfigDetails, (std::string type), (override));
    MOCK_METHOD(bool, isHDMIOutPortPresent, (), (override));
    MOCK_METHOD(void, getSinkDeviceAtmosCapability, (dsATMOSCapability_t atmosCapability), (override));
    MOCK_METHOD(void, getSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void,setSecondaryLanguage, (std::string secondaryLanguage), (override));
    MOCK_METHOD(void, getFaderControl, (int *mixerBalance), (override));
    MOCK_METHOD(void, setFaderControl, (int mixerBalance), (override));
    MOCK_METHOD(void, setPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, getPrimaryLanguage, (std::string primaryLanguage), (override));
    MOCK_METHOD(void, setAudioAtmosOutputMode, (int enable), (override));
    MOCK_METHOD(void, setAssociatedAudioMixing, (bool mixing), (override));
    MOCK_METHOD(void, getCurrentAudioFormat, (dsAudioFormat_t audioFormat), (override));
    MOCK_METHOD(void, getAssociatedAudioMixing, (bool *mixing), (override));
    MOCK_METHOD(void, setAudioMixerLevels, (dsAudioInput_t gain, int volume), (override));

    MOCK_METHOD(dsError_t, Register, (device::Host::IDisplayEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IDisplayEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::IAudioOutputPortEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IAudioOutputPortEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::IDisplayDeviceEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IDisplayDeviceEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::IHdmiInEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IHdmiInEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::IVideoDeviceEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IVideoDeviceEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::IVideoOutputPortEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::IVideoOutputPortEvents* listener), (override));
    MOCK_METHOD(dsError_t, Register, (device::Host::ICompositeInEvents* listener), (override));
    MOCK_METHOD(dsError_t, UnRegister, (device::Host::ICompositeInEvents* listener), (override));
};

class IDisplayEventsImplMock : public device::IDisplayEventsImpl {
public:
    virtual ~IDisplayEventsImplMock() = default;

    MOCK_METHOD(void, OnDisplayRxSense, (dsDisplayEvent_t displayEvent), (override));
};

class IAudioOutputPortEventsImplMock : public device::IAudioOutputPortEventsImpl {
public:
    virtual ~IAudioOutputPortEventsImplMock() = default;

    MOCK_METHOD(void, OnAudioOutHotPlug, (dsAudioPortType_t portType, uint32_t uiPortNumber, bool isPortConnected), (override));
    MOCK_METHOD(void, OnAudioFormatUpdate, (dsAudioFormat_t audioFormat), (override));
    MOCK_METHOD(void, OnDolbyAtmosCapabilitiesChanged, (dsATMOSCapability_t atmosCapability, bool status), (override));
    MOCK_METHOD(void, OnAudioPortStateChanged, (dsAudioPortState_t audioPortState), (override));
    MOCK_METHOD(void, OnAssociatedAudioMixingChanged, (bool mixing), (override));
    MOCK_METHOD(void, OnAudioFaderControlChanged, (int mixerBalance), (override));
    MOCK_METHOD(void, OnAudioPrimaryLanguageChanged, (const std::string& primaryLanguage), (override));
    MOCK_METHOD(void, OnAudioSecondaryLanguageChanged, (const std::string& secondaryLanguage), (override));
    MOCK_METHOD(void, OnAudioModeEvent, (dsAudioPortType_t audioPortType, dsAudioStereoMode_t audioStereoMode), (override));
    MOCK_METHOD(void, OnAudioLevelChangedEvent, (int audioLevel), (override));
};

class IDisplayDeviceEventsImplMock : public device::IDisplayDeviceEventsImpl {
public:
    virtual ~IDisplayDeviceEventsImplMock() = default;

    MOCK_METHOD(void, OnDisplayHDMIHotPlug, (dsDisplayEvent_t displayEvent), (override));
};

class IHdmiInEventsImplMock : public device::IHdmiInEventsImpl {
public:
    virtual ~IHdmiInEventsImplMock() = default;

    MOCK_METHOD(void, OnHdmiInEventHotPlug, (dsHdmiInPort_t port, bool isConnected), (override));
    MOCK_METHOD(void, OnHdmiInEventSignalStatus, (dsHdmiInPort_t port, dsHdmiInSignalStatus_t signalStatus), (override));
    MOCK_METHOD(void, OnHdmiInEventStatus, (dsHdmiInPort_t activePort, bool isPresented), (override));
    MOCK_METHOD(void, OnHdmiInVideoModeUpdate, (dsHdmiInPort_t port, const dsVideoPortResolution_t& videoPortResolution), (override));
    MOCK_METHOD(void, OnHdmiInAllmStatus, (dsHdmiInPort_t port, bool allmStatus), (override));
    MOCK_METHOD(void, OnHdmiInAVIContentType, (dsHdmiInPort_t port, dsAviContentType_t aviContentType), (override));
    MOCK_METHOD(void, OnHdmiInVRRStatus, (dsHdmiInPort_t port, dsVRRType_t vrrType), (override));
    MOCK_METHOD(void, OnHdmiInAVLatency, (int audioDelay, int videoDelay), (override));

};

class ICompositeInEventsImplMock : public device::ICompositeInEventsImpl {
public:
    virtual ~ICompositeInEventsImplMock() = default;

    MOCK_METHOD(void, OnCompositeInHotPlug, (dsCompositeInPort_t port, bool isConnected), (override));
    MOCK_METHOD(void, OnCompositeInSignalStatus, (dsCompositeInPort_t port, dsCompInSignalStatus_t signalStatus), (override));
    MOCK_METHOD(void, OnCompositeInStatus, (dsCompositeInPort_t activePort, bool isPresented), (override));
    MOCK_METHOD(void, OnCompositeInVideoModeUpdate, (dsCompositeInPort_t activePort, dsVideoPortResolution_t videoResolution), (override));
};

class IVideoDeviceEventsImplMock : public device::IVideoDeviceEventsImpl {
public:
    virtual ~IVideoDeviceEventsImplMock() = default;

    MOCK_METHOD(void, OnZoomSettingsChanged, (dsVideoZoom_t zoomSetting), (override));
    MOCK_METHOD(void, OnDisplayFrameratePreChange, (const std::string& frameRate), (override));
    MOCK_METHOD(void, OnDisplayFrameratePostChange, (const std::string& frameRate), (override));

};

class IVideoOutputPortEventsImplMock : public device::IVideoOutputPortEventsImpl {
public:
    virtual ~IVideoOutputPortEventsImplMock() = default;

    MOCK_METHOD(void, OnResolutionPreChange, (const int width, const int height), (override));
    MOCK_METHOD(void, OnResolutionPostChange, (const int width, const int height), (override));
    MOCK_METHOD(void, OnVideoFormatUpdate,(dsHDRStandard_t videoFormatHDR),(override));
    MOCK_METHOD(void, OnHDCPStatusChange,(dsHdcpStatus_t hdcpStatus),(override));

};
