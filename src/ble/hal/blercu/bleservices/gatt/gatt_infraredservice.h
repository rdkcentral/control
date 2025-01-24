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
//  gatt_infraredservice.h
//

#ifndef GATT_INFRAREDSERVICE_H
#define GATT_INFRAREDSERVICE_H

#include "blercu/bleservices/blercuinfraredservice.h"
#include "blercu/blercuerror.h"

#include "utils/bleuuid.h"
#include "utils/statemachine.h"
#include "utils/futureaggregator.h"

#include "configsettings/configsettings.h"

#include "gatt_deviceinfoservice.h"
#include "gatt_infraredsignal.h"



class BleGattService;
class BleGattCharacteristic;

class GattInfraredSignal;


class GattInfraredService : public BleRcuInfraredService
{

public:
    explicit GattInfraredService(const ConfigModelSettings &settings,
                                 const std::shared_ptr<const GattDeviceInfoService> &deviceInfo,
                                 GMainLoop* mainLoop = NULL);
    ~GattInfraredService() final;

public:
    static BleUuid uuid();

public:
    bool isReady() const;

public:
    bool start(const std::shared_ptr<BleGattService> &gattService);
    void stop();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }

protected:
    Slots<> m_readySlots;

public:
    int32_t codeId() const override;
    uint8_t irSupport() const override;

    void setIrControl(const uint8_t irControl, PendingReply<> &&reply) override;
    void eraseIrSignals(PendingReply<> &&reply) override;
    void programIrSignalWaveforms(const std::map<uint32_t, std::vector<uint8_t>> &irWaveforms,
                                  const uint8_t irControl,
                                  PendingReply<> &&reply) override;

    void emitIrSignal(uint32_t keyCode, PendingReply<> &&reply) override;


private:
    enum State {
        IdleState,
        StartingSuperState,
            SetStandbyModeState,
            GetIrSupportState,
            GetCodeIdState,
            GetIrSignalsState,
        RunningState,
    };

    void init();

    void requestIrSupport();
    void requestCodeId();

// private slots:
    void onEnteredState(int state);
    void onIrSignalReady();

    void onEnteredIdleState();
    void onEnteredSetStandbyModeState();
    void onEnteredGetIrSignalsState();

private:
    void getSignalCharacteristics(const std::shared_ptr<BleGattService> &gattService);

    uint8_t keyCodeToGattValue(GattInfraredSignal::Key keyCode) const;

    void writeCodeIdValue(int32_t codeId, PendingReply<> &&reply);

private:
    std::shared_ptr<bool> m_isAlive;
    GMainLoop *m_GMainLoop;
    
    const std::shared_ptr<const GattDeviceInfoService> m_deviceInfo;

    enum StandbyMode {
        StandbyModeB,
        StandbyModeC
    };

    StandbyMode m_irStandbyMode;

    std::shared_ptr<BleGattCharacteristic> m_standbyModeCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_codeIdCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_emitIrCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_irSupportCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_irControlCharacteristic;

    std::vector< std::shared_ptr<GattInfraredSignal> > m_irSignals;

    StateMachine m_stateMachine;
    
    int32_t m_codeId;

    uint8_t m_irSupport;

    std::shared_ptr<FutureAggregator> m_outstandingOperation;


private:
    static const BleUuid m_serviceUuid;

private:
    static const Event::Type StartServiceRequestEvent = Event::Type(Event::User + 1);
    static const Event::Type StopServiceRequestEvent  = Event::Type(Event::User + 2);

    static const Event::Type SetIrStandbyModeEvent    = Event::Type(Event::User + 3);
    static const Event::Type IrSignalsReadyEvent      = Event::Type(Event::User + 4);

    
};

#endif // !defined(GATT_INFRAREDSERVICE_H)
