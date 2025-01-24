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
//  gatt_deviceinfoservice.h
//

#ifndef GATT_DEVICEINFOSERVICE_H
#define GATT_DEVICEINFOSERVICE_H

#include "blercu/bleservices/blercudeviceinfoservice.h"
#include "utils/statemachine.h"
#include "utils/bleaddress.h"
#include "utils/bleuuid.h"

#include <memory>
#include <map>


class BleGattProfile;
class BleGattService;

class GattDeviceInfoService : public BleRcuDeviceInfoService
{
public:
    GattDeviceInfoService(GMainLoop *mainLoop = NULL);
    ~GattDeviceInfoService();

public:
    static BleUuid uuid();

    bool isReady() const;
    
// signals:
public:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }
private:
    Slots<> m_readySlots;

public:
    bool start(const std::shared_ptr<const BleGattService> &gattService);
    void stop();

    void forceRefresh();
    
    inline Slot<> getForceRefreshSlot()
    {
        return Slot<>(m_isAlive, std::bind(&GattDeviceInfoService::forceRefresh, this));
    }
    
public:
    std::string manufacturerName() const override;
    std::string modelNumber() const override;
    std::string serialNumber() const override;
    std::string hardwareRevision() const override;
    std::string firmwareVersion() const override;
    std::string softwareVersion() const override;
    uint64_t systemId() const override;

public:
    PnPVendorSource pnpVendorIdSource() const override;
    uint16_t pnpVendorId() const override;
    uint16_t pnpProductId() const override;
    uint16_t pnpProductVersion() const override;

private:
    enum State {
        IdleState,
        InitialisingState,
        RunningState,
        StoppedState
    };

    void init();

public:
    enum InfoField {
        ManufacturerName  = (0x1 << 0),
        ModelNumber       = (0x1 << 1),
        SerialNumber      = (0x1 << 2),
        HardwareRevision  = (0x1 << 3),
        FirmwareVersion   = (0x1 << 4),
        SoftwareVersion   = (0x1 << 5),
        SystemId          = (0x1 << 6),
        PnPId             = (0x1 << 7),
    };

private:
    void onEnteredState(int state);
    void onExitedState(int state);

    void sendCharacteristicReadRequest(InfoField field);
    void onCharacteristicReadSuccess(const std::vector<uint8_t> &data, InfoField field);
    void onCharacteristicReadError(const std::string &message, InfoField field);

private:
    void setManufacturerName(const std::vector<uint8_t> &value);
    void setModelNumber(const std::vector<uint8_t> &value);
    void setSerialNumber(const std::vector<uint8_t> &value);
    void setHardwareRevision(const std::vector<uint8_t> &value);
    void setFirmwareVersion(const std::vector<uint8_t> &value);
    void setSoftwareVersion(const std::vector<uint8_t> &value);
    void setSystemId(const std::vector<uint8_t> &value);
    void setPnPId(const std::vector<uint8_t> &value);

private:
    std::shared_ptr<bool> m_isAlive;
    
    bool m_forceRefresh;

    std::shared_ptr<const BleGattService> m_gattService;

    StateMachine m_stateMachine;

    struct StateHandler {
        BleUuid uuid;
        void (GattDeviceInfoService::*handler)(const std::vector<uint8_t> &value);
    };

    static const std::map<InfoField, StateHandler> m_stateHandler;

    uint16_t m_infoFlags;

private:
    std::string m_manufacturerName;
    std::string m_modelNumber;
    std::string m_serialNumber;
    std::string m_hardwareRevision;
    std::string m_firmwareVersion;
    std::string m_softwareVersion;
    uint64_t m_systemId;

    uint8_t m_vendorIdSource;
    uint16_t m_vendorId;
    uint16_t m_productId;
    uint16_t m_productVersion;

private:
    static const BleUuid m_serviceUuid;

private:
    static const Event::Type StartServiceRequestEvent               = Event::Type(Event::User + 1);
    static const Event::Type StartServiceForceRefreshRequestEvent   = Event::Type(Event::User + 2);
    static const Event::Type StopServiceRequestEvent                = Event::Type(Event::User + 3);
    static const Event::Type InitialisedEvent                       = Event::Type(Event::User + 4);

};

#endif // !defined(GATT_DEVICEINFOSERVICE_H)
