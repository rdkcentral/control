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
//  blercudevice_p.h
//

#ifndef BLUEZ_BLERCUDEVICE_P_H
#define BLUEZ_BLERCUDEVICE_P_H

#include "../blercudevice.h"
#include "utils/bleuuid.h"
#include "utils/statemachine.h"
#include "dbus/dbusvariant.h"

#include <gio/gio.h>
#include <string>
#include <memory>


class BleRcuServicesFactory;

class BleRcuServices;
class BleGattProfileBluez;

class BluezDeviceInterface;


class BleRcuDeviceBluez : public BleRcuDevice
{
public:
    enum State {
        IdleState,
        PairedState,
        ConnectedState,
        ResolvingServicesState,
        RecoverySuperState,
            RecoveryDisconnectingState,
            RecoveryReconnectingState,
        SetupSuperState,
            StartingServicesState,
            ReadyState
    };

public:
    BleRcuDeviceBluez(const BleAddress &bdaddr,
                      const std::string &name,
                      const GDBusConnection *bluezDBusConn,
                      const std::string &bluezDBusPath,
                      const std::shared_ptr<BleRcuServicesFactory> &servicesFactory,
                      GMainLoop* mainLoop = NULL);
    BleRcuDeviceBluez();
    ~BleRcuDeviceBluez() final;

public:
    bool isValid() const override;

    BleAddress address() const override;
    std::string name() const override;

    bool isConnected() const override;
    bool isPairing() const override;
    bool isPaired() const override;
    bool isReady() const override;

    double msecsSinceReady() const override;
    void shutdown() override;


public:
    std::string bluezObjectPath() const;

    void pair(int timeout);
    void cancelPairing();
    void connect();

    void block();
    void unblock();


public:
    std::shared_ptr<BleRcuAudioService> audioService() const override;
    std::shared_ptr<BleRcuBatteryService> batteryService() const override;
    std::shared_ptr<BleRcuDeviceInfoService> deviceInfoService() const override;
    std::shared_ptr<BleRcuFindMeService> findMeService() const override;
    std::shared_ptr<BleRcuInfraredService> infraredService() const override;
    std::shared_ptr<BleRcuUpgradeService> upgradeService() const override;
    std::shared_ptr<BleRcuRemoteControlService> remoteControlService() const override;

    void onDeviceConnectedChanged(bool connected);
    void onDevicePairedChanged(bool paired);
    void onDeviceNameChanged(const std::string &name);
    void onDeviceServicesResolvedChanged(bool resolved);

    bool init(const GDBusConnection *bluezDBusConn,
              const std::string &bluezDBusPath);

    // Device Info Service
    std::string firmwareRevision() const override;
    std::string hardwareRevision() const override;
    std::string softwareRevision() const override;
    std::string manufacturer() const override;
    std::string model() const override;
    std::string serialNumber() const override;
    void addManufacturerNameChangedSlot(const Slot<const std::string &> &func) override;
    void addModelNumberChangedSlot(const Slot<const std::string &> &func) override;
    void addSerialNumberChangedSlot(const Slot<const std::string &> &func) override;
    void addHardwareRevisionChangedSlot(const Slot<const std::string &> &func) override;
    void addFirmwareVersionChangedSlot(const Slot<const std::string &> &func) override;
    void addSoftwareVersionChangedSlot(const Slot<const std::string &> &func) override;

    // Battery Service
    uint8_t batteryLevel() const override;
    void addBatteryLevelChangedSlot(const Slot<int> &func) override;

    // FindMe Service
    void findMe(uint8_t level, PendingReply<> &&reply) const override;

    // Remote Control Service
    uint8_t unpairReason() const override;
    uint8_t rebootReason() const override;
    uint8_t lastKeypress() const override;
    uint8_t advConfig() const override;
    std::vector<uint8_t> advConfigCustomList() const override;
    void sendRcuAction(uint8_t action, PendingReply<> &&reply) override;
    void writeAdvertisingConfig(uint8_t config, const std::vector<uint8_t> &customList, PendingReply<> &&reply) override;
    void addUnpairReasonChangedSlot(const Slot<uint8_t> &func) override;
    void addRebootReasonChangedSlot(const Slot<uint8_t, std::string> &func) override;
    void addLastKeypressChangedSlot(const Slot<uint8_t> &func) override;
    void addAdvConfigChangedSlot(const Slot<uint8_t> &func) override;
    void addAdvConfigCustomListChangedSlot(const Slot<const std::vector<uint8_t> &> &func) override;

    // Voice Service
    uint8_t audioGainLevel() const override;
    void setAudioGainLevel(uint8_t value) override;
    uint32_t audioCodecs() const override;
    bool audioStreaming() const override;

    bool getAudioFormat(Encoding encoding, AudioFormat &format) const override;
    void startAudioStreaming(uint32_t encoding, PendingReply<int> &&reply, uint32_t durationMax = 0) override;
    void stopAudioStreaming(uint32_t audioDuration, PendingReply<> &&reply) override;
    void getAudioStatus(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets) override;
    bool getFirstAudioDataTime(ctrlm_timestamp_t &time) override;

    void addStreamingChangedSlot(const Slot<bool> &func) override;
    void addGainLevelChangedSlot(const Slot<uint8_t> &func) override;
    void addAudioCodecsChangedSlot(const Slot<uint32_t> &func) override;

    // IR Service
    int32_t irCode() const override;
    uint8_t irSupport() const override;
    void setIrControl(const uint8_t irControl, PendingReply<> &&reply) override;
    void programIrSignalWaveforms(const std::map<uint32_t, std::vector<uint8_t>> &irWaveforms,
                                  const uint8_t irControl, PendingReply<> &&reply) override;
    void eraseIrSignals(PendingReply<> &&reply) override;
    void emitIrSignal(uint32_t keyCode, PendingReply<> &&reply) override;
    void addCodeIdChangedSlot(const Slot<int32_t> &func) override;
    void addIrSupportChangedSlot(const Slot<uint8_t> &func) override;

    // Upgrade Service
    void startUpgrade(const std::string &fwFile, PendingReply<> &&reply) override;
    void cancelUpgrade(PendingReply<> &&reply) override;
    void addUpgradingChangedSlot(const Slot<bool> &func) override;
    void addUpgradeProgressChangedSlot(const Slot<int> &func) override;
    void addUpgradeErrorSlot(const Slot<std::string> &func) override;

private:
    void setupStateMachine();


    void onPairRequestReply(PendingReply<> *reply);
    void onCancelPairingRequestReply(PendingReply<> *reply);
    void onConnectRequestReply(PendingReply<> *reply);


private:
    void onEnteredState(int state);
    void onExitedState(int state);

    void onEnteredReadyState();
    void onExitedReadyState();

    void onExitedSetupSuperState();

    void onEnteredRecoveryDisconnectingState();
    void onEnteredRecoveryReconnectingState();

    void onEnteredResolvingServicesState();
    void onEnteredStartingServicesState();

    void onServicesReady();

private:
    std::shared_ptr<bool> m_isAlive;

public:
    std::shared_ptr<BluezDeviceInterface> m_deviceProxy;

private:
    std::shared_ptr<BleGattProfileBluez> m_gattProfile;
    std::shared_ptr<BleRcuServices> m_services;

    const std::string m_bluezObjectPath;
    const BleAddress m_address;

    std::string m_name;

    bool m_lastConnectedState;
    bool m_lastPairedState;
    bool m_lastServicesResolvedState;
    bool m_isPairing;

    GTimer* m_timeSinceReady;

    int m_recoveryAttempts;
    const int m_maxRecoveryAttempts;

    StateMachine m_stateMachine;


    static const Event::Type DeviceConnectedEvent        = Event::Type(Event::User + 1);
    static const Event::Type DeviceDisconnectedEvent     = Event::Type(Event::User + 2);
    static const Event::Type DevicePairedEvent           = Event::Type(Event::User + 3);
    static const Event::Type DeviceUnpairedEvent         = Event::Type(Event::User + 4);

    static const Event::Type ServicesResolvedEvent       = Event::Type(Event::User + 5);
    static const Event::Type ServicesNotResolvedEvent    = Event::Type(Event::User + 6);
    static const Event::Type ServicesStartedEvent        = Event::Type(Event::User + 7);

    static const Event::Type ServicesResolveTimeoutEvent = Event::Type(Event::User + 8);
};


#endif // !defined(BLUEZ_BLERCUDEVICE_P_H)
