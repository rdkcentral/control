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
//  gatt_upgradeservice.h
//

#ifndef GATT_UPGRADESERVICE_H
#define GATT_UPGRADESERVICE_H

#include "blercu/bleservices/blercuupgradeservice.h"
#include "utils/bleuuid.h"

#include "utils/statemachine.h"
#include "utils/fwimagefile.h"


class BleGattService;
class BleGattCharacteristic;
class BleGattDescriptor;


class GattUpgradeService : public BleRcuUpgradeService
{
public:
    GattUpgradeService(GMainLoop *mainLoop = NULL);
    ~GattUpgradeService() final;

public:
    static BleUuid uuid();

public:
    bool isReady() const;

public:
    bool start(const std::shared_ptr<const BleGattService> &gattService);
    void stop();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }
    inline void addUpgradeCompleteSlot(const Slot<> &func)
    {
        m_upgradeCompleteSlots.addSlot(func);
    }

protected:
    Slots<> m_readySlots;
    Slots<> m_upgradeCompleteSlots;

public:
    void startUpgrade(const std::string &fwFile, PendingReply<> &&reply) override;
    void cancelUpgrade(PendingReply<> &&reply) override;

    bool upgrading() const override;
    int progress() const override;

    void disablePacketNotifications();

private:
    enum State {
        InitialState,
        SendingSuperState,
            SendingWriteRequestState,
            SendingDataState,
        ErroredState,
        FinishedState
    };

    void init();

    enum SetupFlag {
        EnabledNotifications = 0x01,
        ReadWindowSize = 0x02,
        VerifiedDeviceModel = 0x04,
    };

// private slots:
    void onStateEntry(int state);

    void onEnteredInitialState();
    void onEnteredSendWriteRequestState();
    void onEnteredSendingDataState();
    void onEnteredErroredState();
    void onEnteredFinishedState();
    void onPacketNotification(const std::vector<uint8_t> &value);

public:
    bool onTimeout();

private:

    void enablePacketNotifications();
    void readControlPoint();
    void readWindowSize();

    void setSetupFlag(SetupFlag flag);

    void doPacketWrite(const std::vector<uint8_t> &value);

    void sendWRQ();
    void sendDATA();

    void onACKPacket(const std::vector<uint8_t> &data);
    void onERRORPacket(const std::vector<uint8_t> &data);

private:
    std::shared_ptr<bool> m_isAlive;
    bool m_ready;

    std::shared_ptr<BleGattCharacteristic> m_controlCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_packetCharacteristic;
    std::shared_ptr<BleGattDescriptor> m_windowSizeDescriptor;

    uint16_t m_setupFlags;

    int m_progress;

    int m_windowSize;

    std::shared_ptr<FwImageFile> m_fwFile;

    std::shared_ptr< PendingReply<> > m_startPromise;

    unsigned int m_timeoutTimer;

    StateMachine m_stateMachine;

private:
    int m_lastAckBlockId;

    int m_timeoutCounter;

    std::string m_lastError;

private:
    static const Event::Type CancelledEvent         = Event::Type(Event::User + 1);
    static const Event::Type TimeoutErrorEvent      = Event::Type(Event::User + 2);
    static const Event::Type EnableNotifyErrorEvent = Event::Type(Event::User + 3);
    static const Event::Type WriteErrorEvent        = Event::Type(Event::User + 4);
    static const Event::Type ReadErrorEvent         = Event::Type(Event::User + 5);
    static const Event::Type PacketErrorEvent       = Event::Type(Event::User + 6);
    static const Event::Type StopServiceEvent       = Event::Type(Event::User + 7);

    static const Event::Type FinishedSetupEvent     = Event::Type(Event::User + 8);
    static const Event::Type PacketAckEvent         = Event::Type(Event::User + 9);
    static const Event::Type CompleteEvent          = Event::Type(Event::User + 10);

};


#endif // !defined(GATT_UPGRADESERVICE_H)


