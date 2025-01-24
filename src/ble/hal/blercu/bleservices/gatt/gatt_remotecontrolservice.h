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
//  gatt_remotecontrolservice.h
//

#ifndef GATT_REMOTECONTROLSERVICE_H
#define GATT_REMOTECONTROLSERVICE_H

#include "blercu/bleservices/blercuremotecontrolservice.h"
#include "utils/bleuuid.h"
#include "utils/statemachine.h"

#include <memory>


class BleGattService;
class BleGattCharacteristic;


class GattRemoteControlService : public BleRcuRemoteControlService
{
public:
    GattRemoteControlService(GMainLoop *mainLoop = NULL);
    ~GattRemoteControlService() final;

public:
    static BleUuid uuid();

public:
    bool isReady() const;

    uint8_t unpairReason() const override;
    uint8_t rebootReason() const override;
    uint8_t lastKeypress() const override;
    uint8_t advConfig() const override;
    std::vector<uint8_t> advConfigCustomList() const override;
    void sendRcuAction(uint8_t action, PendingReply<> &&reply) override;
    void writeAdvertisingConfig(uint8_t config, const std::vector<uint8_t> &customList, PendingReply<> &&reply) override;


public:
    bool start(const std::shared_ptr<const BleGattService> &gattService);
    void stop();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }
private:
    Slots<> m_readySlots;

private:
    enum State {
        IdleState,
        RunningState,
            RetrieveInitialValuesState,
            EnableNotificationsState,
    };

    void init();

// private slots:
    void onEnteredState(int state);
    void onUnpairReasonChanged(const std::vector<uint8_t> &newValue);
    void onRebootReasonChanged(const std::vector<uint8_t> &newValue);

private:
    void requestStartUnpairNotify();
    void requestStartRebootNotify();
    void requestUnpairReason();
    void requestRebootReason();
    void requestAssertReport();
    void requestLastKeypress();
    void requestAdvConfig();
    void requestAdvConfigCustomList();
    
    void onWriteAdvConfigReply(PendingReply<> *reply);
    void onWriteAdvConfigCustomListReply(PendingReply<> *reply);

private:
    std::shared_ptr<bool> m_isAlive;

    std::shared_ptr<BleGattCharacteristic> m_unpairReasonCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_rebootReasonCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_rcuActionCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_lastKeypressCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_advConfigCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_advConfigCustomListCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_assertReportCharacteristic;

    StateMachine m_stateMachine;

    uint8_t m_unpairReason;
    uint8_t m_rebootReason;
    uint8_t m_rcuAction;
    uint8_t m_lastKeypress;
    uint8_t m_advConfig;
    std::vector<uint8_t> m_advConfigCustomList;
    
    uint8_t m_advConfig_toWrite;

    std::shared_ptr<PendingReply<>> m_promiseResults;

private:
    static const BleUuid m_serviceUuid;
    static const BleUuid m_unpairReasonCharUuid;
    static const BleUuid m_rebootReasonCharUuid;
    static const BleUuid m_rcuActionCharUuid;
    static const BleUuid m_lastKeypressCharUuid;
    static const BleUuid m_advConfigCharUuid;
    static const BleUuid m_advConfigCustomListCharUuid;
    static const BleUuid m_assertReportCharUuid;

private:
    static const Event::Type StartServiceRequestEvent       = Event::Type(Event::User + 1);
    static const Event::Type StopServiceRequestEvent        = Event::Type(Event::User + 2);
    static const Event::Type InitialValuesRetrievedEvent    = Event::Type(Event::User + 3);
    static const Event::Type RetryStartNotifyEvent          = Event::Type(Event::User + 4);
};

#endif // !defined(GATT_RemoteControlService_H)
