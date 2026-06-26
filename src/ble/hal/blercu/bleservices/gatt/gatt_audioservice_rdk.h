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
//  gatt_audioservice.h
//

#ifndef GATT_AUDIOSERVICE_RDK_H
#define GATT_AUDIOSERVICE_RDK_H

#include "blercu/bleservices/blercuaudioservice.h"
#include "gatt_audioservice.h"
#include "blercu/blercuerror.h"
#include "utils/bleuuid.h"


class BleGattService;
class BleGattCharacteristic;

class GattAudioPipe;


class GattAudioServiceRdk : public GattAudioService
{
public:
    GattAudioServiceRdk(GMainLoop* mainLoop = NULL);
    ~GattAudioServiceRdk() final;

    BleUuid uuid() override;

    bool start(const std::shared_ptr<const BleGattService> &gattService) override;
    void stop() override;
    void onEnteredIdle() override;
    void onEnteredEnableNotificationsState() override;
    void onEnteredStartStreamingState() override;
    void onEnteredStopStreamingState() override;
    void onAudioDataNotification(const std::vector<uint8_t> &value) override;
    void onAudioInfoNotification(const std::vector<uint8_t> &value);

    void validateFrame(const uint8_t *frame, uint32_t frameCount) override;

    uint32_t audioCodecs() const override;
    bool audioFormat(Encoding encoding, AudioFormat &format) const override;

    uint8_t gainLevel() const override;
    void setGainLevel(uint8_t level) override;

    // MFV accessors
    DetectionType mfvDetectionType() const override;
    DetectionData mfvDetectionData() const override;
    ModelVersion mfvModelVersion() const override;
    bool mfvPrivacyEnabled() const override;
    std::vector<uint8_t> mfvModelConfiguration() const override;
    uint8_t mfvCapabilities() const override;
    StreamStatsRaw mfvStreamStats() const override;

    void writeMfvPrivacy(bool enabled, PendingReply<> &&reply) override;
    void writeMfvModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply) override;

private:

    bool getAudioCodecsCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioGainCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioControlCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioDataCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioInfoCharacteristic(const std::shared_ptr<const BleGattService> &gattService);

    // MFV characteristic discovery
    bool getMfvCharacteristics(const std::shared_ptr<const BleGattService> &gattService);

    // MFV initial reads
    void requestMfvCapabilities();
    void requestMfvModelVersion();
    void requestMfvPrivacy();
    void requestMfvModelConfig();
    bool areMfvInitialReadsComplete() const;
    void onMfvInitialReadComplete();

    // MFV notification enable
    void requestStartMfvSessionStartNotify();
    void requestStartMfvDetectionDataNotify();
    void requestStartMfvPrivacyNotify();

    // MFV notification handlers
    void onMfvSessionStartChanged(const std::vector<uint8_t> &newValue);
    void onMfvDetectionDataChanged(const std::vector<uint8_t> &newValue);
    void onMfvPrivacyChanged(const std::vector<uint8_t> &newValue);

    // MFV write handlers
    void onWriteMfvPrivacyReply(PendingReply<> *reply);
    void onWriteMfvModelConfigReply(PendingReply<> *reply);

    void requestGainLevel();
    void requestAudioCodecs();

    static const BleUuid m_serviceUuid;

    uint8_t  m_gainLevel = 0xFF;
    uint32_t m_audioCodecs = 0;

    std::shared_ptr<BleGattCharacteristic> m_audioGainCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCtrlCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioInfoCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCodecsCharacteristic;

    // MFV characteristics and data
    bool m_mfvSupported = false;
    int  m_mfvInitialReadsRemaining = 0;

    DetectionType m_mfvDetectionType = FullPower;
    DetectionData m_mfvDetectionData;
    ModelVersion  m_mfvModelVersionData;
    bool          m_mfvPrivacyEnabled = false;
    std::vector<uint8_t> m_mfvModelConfigurationData;
    uint8_t       m_mfvCapabilitiesValue = 0;
    StreamStatsRaw m_mfvStreamStatsData;

    std::shared_ptr<PendingReply<>> m_mfvPromiseResults;

    std::shared_ptr<BleGattCharacteristic> m_mfvSessionStartCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_mfvDetectionDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_mfvModelVersionCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_mfvPrivacyCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_mfvModelConfigCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_mfvCapabilitiesCharacteristic;

    static const BleUuid m_mfvSessionStartCharUuid;
    static const BleUuid m_mfvDetectionDataCharUuid;
    static const BleUuid m_mfvModelVersionCharUuid;
    static const BleUuid m_mfvPrivacyCharUuid;
    static const BleUuid m_mfvModelConfigCharUuid;
    static const BleUuid m_mfvCapabilitiesCharUuid;

};

#endif // !defined(GATT_AUDIOSERVICE_RDK_H)
