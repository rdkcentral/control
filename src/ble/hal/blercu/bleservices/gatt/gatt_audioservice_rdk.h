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
#include "configsettings/configsettings.h"

#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>


class BleGattService;
class BleGattCharacteristic;

class GattAudioPipe;


class GattAudioServiceRdk : public GattAudioService
{
public:
    GattAudioServiceRdk(GMainLoop* mainLoop = NULL,
                        const std::string &modelName = std::string(),
                        const std::vector<ConfigVoiceKeyMode> &voiceKeyModes = std::vector<ConfigVoiceKeyMode>());
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

    uint32_t audioModeSupport() const override;
    std::vector<uint8_t> audioModeControl() const override;
    uint32_t pressAndReleaseTimeout() const override;
    uint32_t pressAndReleaseThreshold() const override;

    void writeAudioModeControl(const std::vector<uint8_t> &value, PendingReply<> &&reply) override;
    void writePressAndReleaseTimeout(uint32_t timeoutMs, PendingReply<> &&reply) override;
    void writePressAndReleaseThreshold(uint32_t thresholdMs, PendingReply<> &&reply) override;

private:

    bool getAudioCodecsCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioGainCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioControlCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioDataCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioInfoCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getOptionalCharacteristic(const std::shared_ptr<const BleGattService> &gattService,
                                   BleUuid::CustomRdkCharacteristicType uuid,
                                   std::shared_ptr<BleGattCharacteristic> &characteristic);

    void requestGainLevel();
    void requestAudioCodecs();
    void requestAudioModeControl(const std::function<void()> &onComplete = nullptr);
    void requestUint32Characteristic(const std::shared_ptr<BleGattCharacteristic> &characteristic,
                                     const char *name, uint32_t &value, Slots<uint32_t> &changedSlots,
                                     const std::function<void()> &onComplete = nullptr);

    void writeUint32Characteristic(const std::shared_ptr<BleGattCharacteristic> &characteristic,
                                   const char *name, uint32_t newValue,
                                   uint32_t &value, Slots<uint32_t> &changedSlots,
                                   PendingReply<> &&reply);

    struct VoiceKeyModeWriteStep {
        const char *name;
        std::function<void(PendingReply<> &&)> write;
    };
    void applyVoiceKeyModeConfig();
    void runVoiceKeyModeWrites(const std::shared_ptr<std::deque<VoiceKeyModeWriteStep>> &steps);

    static const BleUuid m_serviceUuid;

    const std::string                     m_modelName;
    const std::vector<ConfigVoiceKeyMode> m_voiceKeyModes;

    uint8_t  m_gainLevel = 0xFF;
    uint32_t m_audioCodecs = 0;
    uint32_t m_audioModeSupport = 0;
    std::vector<uint8_t> m_audioModeControl;
    bool                 m_audioModeControlValid = false;
    mutable std::mutex   m_audioModeControlMutex;
    uint32_t m_pressAndReleaseTimeout = 0;
    uint32_t m_pressAndReleaseThreshold = 0;

    std::shared_ptr<BleGattCharacteristic> m_audioGainCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCtrlCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioInfoCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCodecsCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioModeSupportCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioModeCtrlCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_parTimeoutCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_parThresholdCharacteristic;

};

#endif // !defined(GATT_AUDIOSERVICE_RDK_H)
