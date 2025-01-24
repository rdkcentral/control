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
//  blercucontroller_p.h
//

#ifndef BLERCUCONTROLLER_P_H
#define BLERCUCONTROLLER_P_H

#include "blercucontroller.h"
#include "blercupairingstatemachine.h"
#include "blercuscannerstatemachine.h"

#include <string>

class ConfigSettings;

class BleRcuAdapter;
class BleRcuDevice;



class BleRcuControllerImpl final : public BleRcuController
{
public:
    BleRcuControllerImpl(const std::shared_ptr<const ConfigSettings> &config,
                         const std::shared_ptr<BleRcuAdapter> &manager);
    ~BleRcuControllerImpl() final;

public:

    bool isValid() const override;
    State state() const override;

    BleRcuError lastError() const override;

    bool isPairing() const override;
    int pairingCode() const override;

    bool startPairing(uint8_t filterByte, uint8_t pairingCode) override;
    bool startPairingMacHash(uint8_t filterByte, uint8_t macHash) override;

    bool cancelPairing() override;

    bool isScanning() const override;
    bool startScanning(int timeoutMs) override;
    bool cancelScanning() override;

    std::set<BleAddress> managedDevices() const override;
    std::shared_ptr<BleRcuDevice> managedDevice(const BleAddress &address) const override;

    bool unpairDevice(const BleAddress &address) const override;
    void shutdown() const override;

    void syncManagedDevices();
    void removeLastConnectedDevice();
    void onInitialized();
    
private:

// private slots:
    void onStartedPairing();
    void onFinishedPairing();
    void onFailedPairing();
    void onDevicePairingChanged(const BleAddress &address, bool paired);
    void onDeviceReadyChanged(const BleAddress &address, bool ready);

    void onStartedScanning();
    void onFinishedScanning();
    void onFailedScanning();
    void onFoundPairableDevice(const BleAddress &address, const std::string &name);

private:
    std::shared_ptr<bool> m_isAlive;
    
    const std::shared_ptr<const ConfigSettings> m_config;
    const std::shared_ptr<BleRcuAdapter> m_adapter;

    BleRcuPairingStateMachine m_pairingStateMachine;
    BleRcuScannerStateMachine m_scannerStateMachine;

    std::set<BleAddress> m_managedDevices;

    BleRcuError m_lastError;

    unsigned int m_maxManagedDevices;

    State m_state;

    bool m_ignoreScannerSignal;
};

#endif // !defined(BLERCUCONTROLLER_P_H)
