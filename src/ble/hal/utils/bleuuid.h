/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
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

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  bleuuid.h
//

#ifndef BLEUUID_H
#define BLEUUID_H

#include <string>
#include <uuid/uuid.h>

class BleUuid
{
public:
    enum ServiceType {
        GenericAccess = 0x1800,
        GenericAttribute = 0x1801,
        ImmediateAlert = 0x1802,
        LinkLoss = 0x1803,
        TxPower = 0x1804,
        DeviceInformation = 0x180a,
        BatteryService = 0x180f,
        HumanInterfaceDevice = 0x1812,
        ScanParameters = 0x1813,
    };

    enum CustomRdkServiceType {
        RdkVoice = 0xf800,
        RdkInfrared = 0xf801,
        RdkFirmwareUpgrade = 0xf802,
        RdkRemoteControl = 0xf803,
    };


    enum CharacteristicType {
        DeviceName = 0x2a00,
        Appearance = 0x2a01,
        PeripheralPreferredConnectionParameters = 0x2a04,
        ServiceChanged = 0x2a05,
        AlertLevel = 0x2a06,
        BatteryLevel = 0x2a19,
        SystemID = 0x2a23,
        ModelNumberString = 0x2a24,
        SerialNumberString = 0x2a25,
        FirmwareRevisionString = 0x2a26,
        HardwareRevisionString = 0x2a27,
        SoftwareRevisionString = 0x2a28,
        ManufacturerNameString = 0x2a29,
        IEEERegulatatoryCertificationDataList = 0x2a2a,
        ScanRefresh = 0x2a31,
        BootKeyboardOutputReport = 0x2a32,
        BootMouseInputReport = 0x2a33,
        HIDInformation = 0x2a4a,
        ReportMap = 0x2a4b,
        HIDControlPoint = 0x2a4c,
        Report = 0x2a4d,
        ProtocolMode = 0x2a4e,
        ScanIntervalWindow = 0x2a4f,
        PnPID = 0x2a50,
    };

    enum CustomRdkCharacteristicType {
        AudioCodecs = 0xea00,
        AudioGain = 0xea01,
        AudioControl = 0xea02,
        AudioData = 0xea03,
        InfraredStandby = 0xeb01,
        InfraredCodeId = 0xeb02,
        InfraredSignal = 0xeb03,
        EmitInfraredSignal = 0xeb06,
        InfraredSupport = 0xeb07,
        InfraredControl = 0xeb08,
        FirmwareControlPoint = 0xec01,
        FirmwarePacket = 0xec02,
        UnpairReason = 0xed01,
        RebootReason = 0xed02,
        RcuAction = 0xed03,
        LastKeypress = 0xed04,
        AdvertisingConfig = 0xed05,
        AdvertisingConfigCustomList = 0xed06,
        AssertReport = 0xed07,
    };

    enum DescriptorType {
        ClientCharacteristicConfiguration = 0x2902,
        ReportReference = 0x2908,
    };

    enum CustomRdkDescriptorType {
        InfraredSignalReference = 0xeb04,
        InfraredSignalConfiguration = 0xeb05,
        FirmwarePacketWindowSize = 0xec03,
    };

private:
    uuid_t m_uuid;
    std::string m_name;

public:
    BleUuid();
    BleUuid(ServiceType uuid, const std::string &name = std::string());
    BleUuid(CharacteristicType uuid, const std::string &name = std::string());
    BleUuid(DescriptorType uuid, const std::string &name = std::string());
    BleUuid(CustomRdkServiceType uuid, const std::string &name = std::string());
    BleUuid(CustomRdkCharacteristicType uuid, const std::string &name = std::string());
    BleUuid(CustomRdkDescriptorType uuid, const std::string &name = std::string());
    explicit BleUuid(uint16_t uuid, const std::string &name = std::string(), bool isService = false);
    explicit BleUuid(uint32_t uuid, const std::string &name = std::string(), bool isService = false);
    explicit BleUuid(const std::string &uuid, const std::string &name = std::string(), bool isService = false);
    BleUuid(const BleUuid &uuid, bool isService = false);
    ~BleUuid();

    std::string name() const;
    bool doesServiceExist(const std::string &name) const;

    enum UuidFormat { WithCurlyBraces, WithoutCurlyBraces };
    std::string toString(UuidFormat format = WithCurlyBraces) const;

    friend bool operator<(const BleUuid &lhs, const BleUuid &rhs);
    friend bool operator==(const BleUuid &lhs, const BleUuid &rhs);
    friend bool operator!=(const BleUuid &lhs, const BleUuid &rhs);

private:
    bool ConstructUuid(const uint32_t head, const char *base, uuid_t uuid, const std::string &name, bool isService = false);
};

inline bool operator<(const BleUuid &lhs, const BleUuid &rhs)
{
    return (uuid_compare(lhs.m_uuid, rhs.m_uuid) < 0);
}
inline bool operator==(const BleUuid &lhs, const BleUuid &rhs)
{
    return (uuid_compare(lhs.m_uuid, rhs.m_uuid) == 0);
}
inline bool operator!=(const BleUuid &lhs, const BleUuid &rhs)
{
    return !(lhs == rhs);
}

#endif // !defined(BLEUUID_H)
