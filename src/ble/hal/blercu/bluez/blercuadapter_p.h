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
//  blercuadapter_p.h
//

#ifndef BLUEZ_BLERCUADAPTER_P_H
#define BLUEZ_BLERCUADAPTER_P_H

#include "../blercuadapter.h"
#include "utils/statemachine.h"
#include "utils/hcisocket.h"
#include "dbus/dbusservicewatcher.h"
#include "dbus/dbusobjectmanager.h"
#include "configsettings/configmodelsettings.h"
#include "utils/pendingreply.h"

#include <memory>
#include <regex>

// class BleRcuNotifier;
class BleRcuDeviceBluez;
class BleRcuServicesFactory;
class ConfigSettings;

class DBusObjectManagerInterface;
class BluezAdapterInterface;


class BleRcuAdapterBluez : public BleRcuAdapter
{

public:
    BleRcuAdapterBluez(const std::shared_ptr<const ConfigSettings> &config,
                       const std::shared_ptr<BleRcuServicesFactory> &servicesFactory,
                       const GDBusConnection *bluezBusConn,
                       GMainLoop *mainLoop = NULL);
    ~BleRcuAdapterBluez() final;

    std::string findAdapter(int timeout = 2000);
    
private:
    void initStateMachine();

    bool attachAdapter(const std::string &adapterPath);
    bool setAdapterDiscoveryFilter();

    void getRegisteredDevices();

public:
    bool isValid() const override;
    bool isAvailable() const override;
    bool isPowered() const override;

    bool isDiscovering() const override;
    bool startDiscovery() override;
    bool stopDiscovery() override;

    bool isPairable() const override;
    bool enablePairable(unsigned int timeout) override;
    bool disablePairable() override;

    void reconnectAllDevices() override;

    std::set<BleAddress> pairedDevices() const override;

    std::map<BleAddress, std::string> deviceNames() const override;

    std::shared_ptr<BleRcuDevice> getDevice(const BleAddress &address) const override;

    bool isDevicePaired(const BleAddress &address) const override;
    bool isDeviceConnected(const BleAddress &address) const override;

    bool addDevice(const BleAddress &address) override;
    bool removeDevice(const BleAddress &address) override;

    bool setConnectionParams(BleAddress address, double minInterval, double maxInterval,
                            int32_t latency, int32_t supervisionTimeout) override;

    void onBluezServiceRegistered(const std::string &serviceName);
    void onBluezServiceUnregistered(const std::string &serviceName);

    void onAdapterPowerChanged(bool powered);
    void onAdapterDiscoveringChanged(bool discovering);
    void onAdapterPairableChanged(bool pairable);

    void onBluezInterfacesAdded(const std::string &objectPath,
                                const DBusInterfaceList &interfacesAndProperties);
    void onBluezInterfacesRemoved(const std::string &objectPath,
                                  const std::vector<std::string> &interfaces);

    void onDeviceNameChanged(const BleAddress &address, const std::string &name);
    void onDevicePairingError(const BleAddress &address, const std::string &error);
    void onDevicePairedChanged(const BleAddress &address, bool paired);
    void onDeviceReadyChanged(const BleAddress &address, bool ready);

    void onStartDiscoveryReply(PendingReply<> *reply);
    void onStopDiscoveryReply(PendingReply<> *reply);
    void onRemoveDeviceReply(PendingReply<> *reply);
    void onPowerOnReply(PendingReply<> *reply);

    void onStateEntry(int state);
    void onStateExit(int state);


private:

    void onDeviceAdded(const std::string &objectPath,
                       const DBusPropertiesMap &properties);
    void onDeviceRemoved(const std::string &objectPath);

    void onPowerCycleAdapter();
    void onDisconnectReconnectDevice(const BleAddress &device);

private:
    std::shared_ptr<bool> m_isAlive;

    const std::shared_ptr<BleRcuServicesFactory> m_servicesFactory;
    const GDBusConnection *m_bluezDBusConn;

    const std::string m_bluezService;
    std::shared_ptr<DBusServiceWatcher> m_bluezServiceWatcher;
    std::shared_ptr<DBusObjectManagerInterface> m_bluezObjectMgr;

    BleAddress m_address;
    std::string m_adapterObjectPath;
    std::shared_ptr<BluezAdapterInterface> m_adapterProxy;

    std::map<BleAddress, std::shared_ptr<BleRcuDeviceBluez>> m_devices;

    std::shared_ptr<HciSocket> m_hciSocket;

public:
    bool m_discovering;
    bool m_pairable;

    int m_discoveryRequests;
    enum { StartDiscovery, StopDiscovery } m_discoveryRequested;
    unsigned int m_discoveryWatchdogID;
    unsigned int m_discoveryWatchdogTimeout;

private:
    static std::vector<std::regex> getSupportedPairingNames(const std::vector<ConfigModelSettings> &details);

    const std::vector<std::regex> m_supportedPairingNames;

private:
    enum State {
        ServiceUnavailableState,
        ServiceAvailableSuperState,
            AdapterUnavailableState,
            AdapterAvailableSuperState,
                AdapterPoweredOffState,
                AdapterPoweredOnState,
        ShutdownState
    };

    StateMachine m_stateMachine;

    int64_t m_retryEventId;

    static const Event::Type ServiceRetryEvent         = Event::Type(Event::User + 1);
    static const Event::Type ServiceAvailableEvent     = Event::Type(Event::User + 2);
    static const Event::Type ServiceUnavailableEvent   = Event::Type(Event::User + 3);

    static const Event::Type AdapterRetryAttachEvent   = Event::Type(Event::User + 4);
    static const Event::Type AdapterAvailableEvent     = Event::Type(Event::User + 5);
    static const Event::Type AdapterUnavailableEvent   = Event::Type(Event::User + 6);

    static const Event::Type AdapterRetryPowerOnEvent  = Event::Type(Event::User + 7);
    static const Event::Type AdapterPoweredOnEvent     = Event::Type(Event::User + 8);
    static const Event::Type AdapterPoweredOffEvent    = Event::Type(Event::User + 9);

    static const Event::Type ShutdownEvent             = Event::Type(Event::User + 10);

    void onEnteredServiceUnavailableState();
    void onExitedServiceAvailableSuperState();
    void onEnteredAdapterUnavailableState();
    void onExitedAdapterAvailableSuperState();
    void onEnteredAdapterPoweredOffState();
    void onEnteredAdapterPoweredOnState();
    void onExitedAdapterPoweredOnState();

};

#endif // !defined(BLUEZ_BLERCUADAPTER_P_H)
