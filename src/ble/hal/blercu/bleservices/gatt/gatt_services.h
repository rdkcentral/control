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
//  gatt_services.h
//

#ifndef GATT_SERVICES_H
#define GATT_SERVICES_H

#include "blercu/bleservices/blercuservices.h"
#include "utils/bleaddress.h"
#include "utils/statemachine.h"
#include "utils/bleuuid.h"
#include "configsettings/configsettings.h"


class BleGattProfile;

class GattAudioService;
class GattDeviceInfoService;
class GattBatteryService;
class GattFindMeService;
class GattInfraredService;
class GattUpgradeService;
class GattRemoteControlService;



class GattServices : public BleRcuServices
{
public:
    GattServices(const BleAddress &address,
                 const std::shared_ptr<BleGattProfile> &gattProfile,
                 const ConfigModelSettings &settings,
                 GMainLoop *mainLoop = NULL);
    ~GattServices();

public:
    bool isValid() const override;
    bool isReady() const override;

    bool start() override;
    void stop() override;

    std::shared_ptr<BleRcuAudioService> audioService() const override;
    std::shared_ptr<BleRcuDeviceInfoService> deviceInfoService() const override;
    std::shared_ptr<BleRcuBatteryService> batteryService() const override;
    std::shared_ptr<BleRcuFindMeService> findMeService() const override;
    std::shared_ptr<BleRcuInfraredService> infraredService() const override;
    std::shared_ptr<BleRcuUpgradeService> upgradeService() const override;
    std::shared_ptr<BleRcuRemoteControlService> remoteControlService() const override;

private:
    template <typename T>
    bool startService(const std::shared_ptr<T> &service, Event::Type readyEvent, bool &serviceNotUsed);

    void onEnteredState(int state);

    void onGattProfileUpdated();

private:
    enum State {
        IdleState,
        GettingGattServicesState,
        ResolvedServicesSuperState,
            StartingDeviceInfoServiceState,
            StartingBatteryServiceState,
            StartingFindMeServiceState,
            StartingAudioServiceState,
            StartingInfraredServiceState,
            StartingTouchServiceState,
            StartingUpgradeServiceState,
            StartingRemoteControlServiceState,
            ReadyState,
        StoppingState
    };

    void init();

    void onEnteredResolvingGattServicesState();
    void onEnteredGetGattServicesState();

    void isServiceSupported(const BleUuid &uuid, bool &required, bool &optional);

private:
    std::shared_ptr<bool> m_isAlive;
    const BleAddress m_address;
    const std::shared_ptr<BleGattProfile> m_gattProfile;
    
    std::set<std::string> m_servicesRequired;
    std::set<std::string> m_servicesOptional;
    
    StateMachine m_stateMachine;

    // Standard BLE services
    std::shared_ptr<GattDeviceInfoService> m_deviceInfoService;
    std::shared_ptr<GattBatteryService> m_batteryService;
    std::shared_ptr<GattFindMeService> m_findMeService;

    // Custom BLE services
    std::vector<std::shared_ptr<GattAudioService>> m_audioServices;
    std::size_t m_audioServiceIndex = 0;

    std::vector<std::shared_ptr<GattInfraredService>> m_infraredServices;
    std::size_t m_infraredServiceIndex = 0;

    std::vector<std::shared_ptr<GattUpgradeService>> m_upgradeServices;
    std::size_t m_upgradeServiceIndex = 0;

    std::shared_ptr<GattRemoteControlService> m_remoteControlService;

private:
    static const Event::Type StartServicesRequestEvent      = Event::Type(Event::User + 1);
    static const Event::Type StopServicesRequestEvent       = Event::Type(Event::User + 2);
    static const Event::Type GotGattServicesEvent           = Event::Type(Event::User + 3);

    static const Event::Type DeviceInfoServiceReadyEvent    = Event::Type(Event::User + 4);
    static const Event::Type BatteryServiceReadyEvent       = Event::Type(Event::User + 5);
    static const Event::Type FindMeServiceReadyEvent        = Event::Type(Event::User + 6);
    static const Event::Type AudioServiceReadyEvent         = Event::Type(Event::User + 7);
    static const Event::Type InfraredServiceReadyEvent      = Event::Type(Event::User + 8);
    static const Event::Type UpgradeServiceReadyEvent       = Event::Type(Event::User + 9);
    static const Event::Type RemoteControlServiceReadyEvent = Event::Type(Event::User + 10);
    static const Event::Type ServicesStoppedEvent           = Event::Type(Event::User + 11);
};


#endif // GATT_SERVICES_H
