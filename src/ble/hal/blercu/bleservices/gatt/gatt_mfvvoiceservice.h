#ifndef GATT_MFVVOICESERVICE_H
#define GATT_MFVVOICESERVICE_H

#include "blercu/bleservices/blercumfvvoiceservice.h"
#include "utils/bleuuid.h"

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

private:
    bool m_ready;

    DetectionType m_detectionType;
    DetectionData m_detectionData;
    ModelVersion m_modelVersion;
    bool m_privacyEnabled;
    std::vector<uint8_t> m_modelConfiguration;
    uint8_t m_capabilities;
    StreamStatsRaw m_streamStats;

    std::shared_ptr<BleGattCharacteristic> m_sessionStartCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_detectionDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_modelVersionCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_privacyCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_modelConfigCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_capabilitiesCharacteristic;

private:
    static const BleUuid m_serviceUuid;
    static const BleUuid m_sessionStartCharUuid;
    static const BleUuid m_detectionDataCharUuid;
    static const BleUuid m_modelVersionCharUuid;
    static const BleUuid m_privacyCharUuid;
    static const BleUuid m_modelConfigCharUuid;
    static const BleUuid m_capabilitiesCharUuid;
};

#endif // !defined(GATT_MFVVOICESERVICE_H)