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
//  hcisocket.h
//

#ifndef HCISOCKET_H
#define HCISOCKET_H

#include "bleaddress.h"
#include "bleconnectionparameters.h"

#include <memory>
#include <vector>

class HciSocket
{

public:
    static std::shared_ptr<HciSocket> create(uint deviceId = 0,
                                            int netNsFd = -1);
    static std::shared_ptr<HciSocket> createFromSocket(int socketFd,
                                                      uint deviceId = 0);

protected:
    HciSocket() = default;

public:
    virtual ~HciSocket() = default;

public:
    enum HciStatus {
        Success = 0x00,
        UnknownHCICommand = 0x01,
        UnknownConnectionIdentifier = 0x02,
        HardwareFailure = 0x03,
        PageTimeout = 0x04,
        AuthenticationFailure = 0x05,
        PINorKeyMissing = 0x06,
        MemoryCapacityExceeded = 0x07,
        ConnectionTimeout = 0x08,
        ConnectionLimitExceeded = 0x09,
        SynchronousConnectionLimitToADeviceExceeded = 0x0A,
        ACLConnectionAlreadyExists = 0x0B,
        CommandDisallowed = 0x0C,
        ConnectionRejectetLimitedResources = 0x0D,
        ConnectionRejectedSecurityReasons = 0x0E,
        ConnectionRejectedUnacceptableAddr = 0x0F,
        ConnectionAcceptTimeoutExceeded = 0x10,
        UnsupportedFeatureOrParameterValue = 0x11,
        InvalidHCICommandParameters = 0x12,
        RemoteUserTerminatedConnection = 0x13,
        RemoteDeviceTerminatedConnectionLowResources = 0x14,
        RemoteDeviceTerminatedConnectionPowerOff = 0x15,
        ConnectionTerminatedByLocalHost = 0x16,
        RepeatedAttempts = 0x17,
        PairingNotAllowed = 0x18,
        UnknownLMP_PDU = 0x19,
        UnsupportedRemoteFeatureUnsupportedLMPFeature = 0x1A,
        SCOOffsetRejected = 0x1B,
        SCOIntervalRejected = 0x1C,
        SCOAirModeRejected = 0x1D,
        InvalidLMPParameters = 0x1E,
        UnspecifiedError = 0x1F,
        UnsupportedLMPParameterValue = 0x20,
        RoleChangeNotAllowed = 0x21,
        LMPResponseTimeoutLLResponseTimeout = 0x22,
        LMPErrorTransactionCollision = 0x23,
        LMP_PDUNotAllowed = 0x24,
        EncryptionModeNotAcceptable = 0x25,
        LinkKeyCannotBeChanged = 0x26,
        RequestedQoSNotSupported = 0x27,
        InstantPassed = 0x28,
        PairingWithUnitKeyNotSupported = 0x29,
        DifferentTransactionCollision = 0x2A,
        Reserved1 = 0x2B,
        QoSUnacceptableParameter = 0x2C,
        QoSRejected = 0x2D,
        ChannelClassificationNotSupported = 0x2E,
        InsufficientSecurity = 0x2F,
        ParameterOutOfMandatoryRange = 0x30,
        Reserved2 = 0x31,
        RoleSwitchPending = 0x32,
        Reserved3 = 0x33,
        ReservedSlotViolation = 0x34,
        RoleSwitchFailed = 0x35,
        ExtendedInquiryResponseTooLarge = 0x36,
        SecureSimplePairingNotSupportedByHost = 0x37,
        HostBusyPairing = 0x38,
        ConnectionRejectedNoSuitableChannelFound = 0x39,
        ControllerBusy = 0x3A,
        UnacceptableConnectionInterval = 0x3B,
        DirectedAdvertisingTimeout = 0x3C,
        ConnectionTerminatedMICFailure = 0x3D,
        ConnectionFailedEstablished = 0x3E,
        MACConnectionFailed = 0x3F,

        UnknownError = 0x100
    };

public:
    class ConnectedDeviceInfo
    {
    public:
        ConnectedDeviceInfo(const BleAddress &&address_, uint16_t handle_,
                            uint16_t state_, uint32_t linkMode_)
            : address(std::move(address_))
            , handle(handle_)
            , state(state_)
            , linkMode(linkMode_)
        { }

    public:
        BleAddress address;
        uint16_t handle;
        uint16_t state;
        uint32_t linkMode;
    };

public:
    virtual bool isValid() const = 0;

    virtual bool requestConnectionUpdate(uint16_t connHandle,
                                         const BleConnectionParameters &params) = 0;

    virtual std::vector<ConnectedDeviceInfo> getConnectedDevices() const = 0;

    virtual bool sendIncreaseDataCapability(uint16_t connHandle) = 0;

};


#endif // !defined(HCISOCKET_H)
