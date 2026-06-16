#ifndef GATT_MFVVOICESERVICE_H
#define GATT_MFVVOICESERVICE_H

#include "blercu/bleservices/blercumfvvoiceservice.h"
#include "utils/bleuuid.h"
#include "utils/statemachine.h"

#include <memory>

class BleGattService;
class BleGattCharacteristic;

class GattMfvVoiceService : public BleRcuMfvVoiceService
{
public:
    GattMfvVoiceService(GMainLoop *mainLoop = NULL);
    ~GattMfvVoiceService() final;

public:
    static BleUuid uuid();

public:
    bool isReady() const;

    DetectionType detectionType() const override;
    DetectionData detectionData() const override;
    ModelVersion wakeWordModelVersion() const override;
    bool privacyEnabled() const override;
    std::vector<uint8_t> modelConfiguration() const override;
    uint8_t capabilities() const override;
    StreamStatsRaw streamStats() const override;

    void writePrivacy(bool enabled, PendingReply<> &&reply) override;
    void writeModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply) override;

public:
    bool start(const std::shared_ptr<const BleGattService> &gattService);
    void stop();

// signals:
    inline void addReadySlot(const Slot<> &func)
    {
        m_readySlots.addSlot(func);
    }

private:
    enum State {
        IdleState,
        RunningState,
        ReadInitialValuesState,
        EnableNotificationsState,
    };

    void init();

    void onEnteredState(int state);

    // Initial reads
    void requestCapabilities();
    void requestModelVersion();
    void requestPrivacy();
    void requestModelConfig();

    // Enable notifications
    void requestStartSessionStartNotify();
    void requestStartDetectionDataNotify();
    void requestStartPrivacyNotify();

    // Notification handlers
    void onSessionStartChanged(const std::vector<uint8_t> &newValue);
    void onDetectionDataChanged(const std::vector<uint8_t> &newValue);
    void onPrivacyChanged(const std::vector<uint8_t> &newValue);

    // Write reply handlers
    void onWritePrivacyReply(PendingReply<> *reply);
    void onWriteModelConfigReply(PendingReply<> *reply);

private:
    std::shared_ptr<bool> m_isAlive;

    StateMachine m_stateMachine;

    DetectionType m_detectionType;
    DetectionData m_detectionData;
    ModelVersion m_modelVersion;
    bool m_privacyEnabled;
    std::vector<uint8_t> m_modelConfigurationData;
    uint8_t m_capabilities;
    StreamStatsRaw m_streamStats;

    int m_initialReadsRemaining;

    std::shared_ptr<BleGattCharacteristic> m_sessionStartCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_detectionDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_modelVersionCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_privacyCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_modelConfigCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_capabilitiesCharacteristic;

    std::shared_ptr<PendingReply<>> m_promiseResults;

private:
    static const BleUuid m_serviceUuid;
    static const BleUuid m_sessionStartCharUuid;
    static const BleUuid m_detectionDataCharUuid;
    static const BleUuid m_modelVersionCharUuid;
    static const BleUuid m_privacyCharUuid;
    static const BleUuid m_modelConfigCharUuid;
    static const BleUuid m_capabilitiesCharUuid;

private:
    static const Event::Type StartServiceRequestEvent       = Event::Type(Event::User + 1);
    static const Event::Type StopServiceRequestEvent        = Event::Type(Event::User + 2);
    static const Event::Type InitialValuesRetrievedEvent    = Event::Type(Event::User + 3);
    static const Event::Type RetryStartNotifyEvent          = Event::Type(Event::User + 4);
};

#endif // !defined(GATT_MFVVOICESERVICE_H)
