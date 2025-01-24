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

    void validateFrame(const uint8_t *frame, uint32_t frameCount) override;

    uint32_t audioCodecs() const override;
    bool audioFormat(Encoding encoding, AudioFormat &format) const override;

    uint8_t gainLevel() const override;
    void setGainLevel(uint8_t level) override;

private:

    bool getAudioCodecsCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioGainCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioControlCharacteristic(const std::shared_ptr<const BleGattService> &gattService);
    bool getAudioDataCharacteristic(const std::shared_ptr<const BleGattService> &gattService);

    void requestGainLevel();
    void requestAudioCodecs();

    static const BleUuid m_serviceUuid;

    uint8_t  m_gainLevel = 0xFF;
    uint32_t m_audioCodecs = 0;

    std::shared_ptr<BleGattCharacteristic> m_audioGainCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCtrlCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioDataCharacteristic;
    std::shared_ptr<BleGattCharacteristic> m_audioCodecsCharacteristic;

};

#endif // !defined(GATT_AUDIOSERVICE_RDK_H)
