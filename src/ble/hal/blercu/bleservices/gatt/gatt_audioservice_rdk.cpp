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
//  gatt_audioservice_rdk.cpp
//

#include "gatt_audioservice_rdk.h"
#include "gatt_audiopipe.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"

#include "ctrlm_log_ble.h"
#include "ctrlm_hal_ble.h"

#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <string>
#include <algorithm>
#include <utility>

// The duration of audio in a single packet is 96 bytes * 2 samples/byte / 16 samples/msec * 1000 usec/msec = 12000 usec
#define AUDIO_PACKET_DURATION_USEC (12000)

#define AUDIO_FRAME_SIZE           (100)

#define AUDIO_SEQ_NUM_MAX          (0xFF)

// Audio Mode Control characteristic value holds up to 5 (HID keycode, audio mode) byte pairs
#define AUDIO_MODE_CONTROL_MAX_SIZE (CTRLM_HAL_BLE_AUDIO_MODE_CONTROL_MAX_KEYS * 2)

// Built-in voice key mode defaults, used when the "voice_key_mode" config array is empty
#define VOICE_KEY_MODE_LEGACY_DEFAULT_MODEL     "XRAB"

using namespace std;

static uint32_t uint32FromLittleEndian(const vector<uint8_t> &value)
{
    return (uint32_t(value[0])) | (uint32_t(value[1]) << 8) | (uint32_t(value[2]) << 16) | (uint32_t(value[3]) << 24);
}

static vector<uint8_t> uint32ToLittleEndian(uint32_t value)
{
    return vector<uint8_t>({ uint8_t(value), uint8_t(value >> 8), uint8_t(value >> 16), uint8_t(value >> 24) });
}

const BleUuid GattAudioServiceRdk::m_serviceUuid(BleUuid::RdkVoice);

GattAudioServiceRdk::GattAudioServiceRdk(GMainLoop* mainLoop, const std::string &modelName, const std::vector<ConfigVoiceKeyMode> &voiceKeyModes)
    : GattAudioService(AUDIO_FRAME_SIZE, 5, AUDIO_PACKET_DURATION_USEC, AUDIO_SEQ_NUM_MAX, mainLoop)
    , m_modelName(modelName)
    , m_voiceKeyModes(voiceKeyModes)
{
}

GattAudioServiceRdk::~GattAudioServiceRdk()
{
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.

 */
BleUuid GattAudioServiceRdk::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    Starts the service by sending a initial request for the audio to stop
    streaming.  When a reply is received we signal that this service is now
    ready.
 */
bool GattAudioServiceRdk::start(const shared_ptr<const BleGattService> &gattService)
{
    // sanity check the supplied info is valid
    if (!gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
        XLOGD_WARN("invalid voice gatt service info");
        return false;
    }

    // get the dbus proxies to the audio characteristics
    if (!getAudioGainCharacteristic(gattService) ||
        !getAudioControlCharacteristic(gattService) ||
        !getAudioDataCharacteristic(gattService) ||
        !getAudioCodecsCharacteristic(gattService)) {
        XLOGD_WARN("failed to get one or more gatt characteristics");
        return false;
    }
    
    if(getAudioInfoCharacteristic(gattService)) {
        updateFrameCountSupport(true);
    }

    // the audio mode characteristics are optional, older RCUs only support press and hold
    getOptionalCharacteristic(gattService, BleUuid::AudioModeSupport,         m_audioModeSupportCharacteristic);
    getOptionalCharacteristic(gattService, BleUuid::AudioModeControl,         m_audioModeCtrlCharacteristic);
    getOptionalCharacteristic(gattService, BleUuid::PressAndReleaseTimeout,   m_parTimeoutCharacteristic);
    getOptionalCharacteristic(gattService, BleUuid::PressAndReleaseThreshold, m_parThresholdCharacteristic);

    requestGainLevel();
    requestAudioCodecs();

    // clear values cached from a previous connection, they are re-read below
    m_audioModeSupport         = 0;
    m_pressAndReleaseTimeout   = 0;
    m_pressAndReleaseThreshold = 0;
    {
        std::unique_lock<std::mutex> guard(m_audioModeControlMutex);
        m_audioModeControl.clear();
        m_audioModeControlValid = false;
    }

    // read the audio mode characteristics in sequence so that all values are known when the audio mode control
    // value is received and compared against the configured voice key modes
    requestUint32Characteristic(m_audioModeSupportCharacteristic, "audio mode support", m_audioModeSupport, m_audioModeSupportChangedSlots, [this]() {
        requestUint32Characteristic(m_parThresholdCharacteristic, "press and release threshold", m_pressAndReleaseThreshold, m_pressAndReleaseThresholdChangedSlots, [this]() {
            requestUint32Characteristic(m_parTimeoutCharacteristic, "press and release timeout", m_pressAndReleaseTimeout, m_pressAndReleaseTimeoutChangedSlots, [this]() {
                requestAudioModeControl([this]() {
                    applyVoiceKeyModeConfig();
                });
            });
        });
    });

    return(GattAudioService::start(gattService));
}

// -----------------------------------------------------------------------------
/*!
    Stops the service

 */
void GattAudioServiceRdk::stop()
{
    GattAudioService::stop();
}

void GattAudioServiceRdk::onEnteredIdle() {
    stateMachineCancelDelayedEvents(RetryStartNotifyEvent);
    stateMachineCancelDelayedEvents(GattErrorEvent);

    if (m_audioDataCharacteristic) {
        XLOGD_INFO("Disabling notifications for m_audioDataCharacteristic");
        m_audioDataCharacteristic->disableNotifications();
    }
    if(m_audioInfoCharacteristic) {
        XLOGD_INFO("Disabling notifications for m_audioInfoCharacteristic");
        m_audioInfoCharacteristic->disableNotifications();
        m_audioInfoCharacteristic.reset();
    }
    m_audioGainCharacteristic.reset();
    m_audioCtrlCharacteristic.reset();
    m_audioDataCharacteristic.reset();
    m_audioModeSupportCharacteristic.reset();
    m_audioModeCtrlCharacteristic.reset();
    m_parTimeoutCharacteristic.reset();
    m_parThresholdCharacteristic.reset();

    GattAudioService::onEnteredIdle();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called upon entry to the 'enable notifications' state, this is
    where we ask the system to set the CCCD and start funneling notifications to
    the app.

 */
void GattAudioServiceRdk::onEnteredEnableNotificationsState()
{
    auto replyHandlerData = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to enable audio data notifications due to <%s>", reply->errorMessage().c_str());
                
                // notifications are required for audio streaming, so if this fails we need to retry in a couple of seconds
                stateMachineCancelDelayedEvents(RetryStartNotifyEvent);
                stateMachinePostDelayedEvent(RetryStartNotifyEvent, 2000);
            } else {
                // notifications enabled so post an event to the state machine
                stateMachinePostEvent(NotificationsEnabledEvent);
            }
        };

    auto replyHandlerInfo = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {
                // if this happens, we won't get audio info, but the stream can continue
                XLOGD_WARN("failed to enable audio info notifications due to <%s>", reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("successfully enabled audio info notifications");
            }
        };

    if (m_audioInfoCharacteristic && !m_audioInfoCharacteristic->notificationsEnabled()) {
        m_audioInfoCharacteristic->enableNotifications(
            Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
                std::bind(&GattAudioServiceRdk::onAudioInfoNotification, this, std::placeholders::_1)),
            PendingReply<>(getIsAlivePtr(), replyHandlerInfo));
    }

    if (m_audioDataCharacteristic->notificationsEnabled()) {
        // notifications already enabled so post an event to the state machine
        stateMachinePostEvent(NotificationsEnabledEvent);
    } else {
        m_audioDataCharacteristic->enableNotifications(
                Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
                    std::bind(&GattAudioServiceRdk::onAudioDataNotification, this, std::placeholders::_1)), 
                PendingReply<>(getIsAlivePtr(), replyHandlerData));
    }

    GattAudioService::onEnteredEnableNotificationsState();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the start streaming state, here we need to perform the
    GATT write to enable the audio streaming on the RCU.

 */
void GattAudioServiceRdk::onEnteredStartStreamingState()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to write audio control characteristic due to <%s>", reply->errorMessage().c_str());

                setLastError(StreamingError::InternalError);
                stateMachinePostEvent(GattErrorEvent);

            } else {
                stateMachinePostEvent(StreamingStartedEvent);
            }
        };


    if (!m_audioDataCharacteristic->notificationsEnabled()) {
        XLOGD_ERROR("audio data notifications not enabled, cannot continue with audio stream");
        setLastError(StreamingError::InternalError);
        stateMachineCancelDelayedEvents(GattErrorEvent);
        stateMachinePostDelayedEvent(GattErrorEvent, 10);   // needs to be delayed to avoid re-entrancy issues with the state machine
    } else {
        // the first byte is the codec to use, the second byte is to enable voice
        const vector<uint8_t> value({ 0x01, 0x01 });
        m_audioCtrlCharacteristic->writeValueWithoutResponse(value, PendingReply<>(getIsAlivePtr(), replyHandler));
    }
    GattAudioService::onEnteredStartStreamingState();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called when the user requests to stop streaming or the output
    pipe has been closed and therefore we've entered the 'stop streaming' state.
    In this state we just send a request to the RCU to stop streaming.

 */
void GattAudioServiceRdk::onEnteredStopStreamingState()
{

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to write audio control characteristic due to <%s>", reply->errorMessage().c_str());

                setLastError(StreamingError::InternalError);
                stateMachinePostEvent(GattErrorEvent);

            } else {
                stateMachinePostEvent(StreamingStoppedEvent);
            }
        };


    // the first byte is the codec to use, the second byte is to enable voice
    const vector<uint8_t> value({ 0x01, 0x00 });

    // send a write request to write the control characteristic
    m_audioCtrlCharacteristic->writeValueWithoutResponse(value, PendingReply<>(getIsAlivePtr(), replyHandler));

    GattAudioService::onEnteredStopStreamingState();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a notification is received from the Audio Data characteristic.

 */
void GattAudioServiceRdk::onAudioDataNotification(const vector<uint8_t> &value)
{
    // all notifications from the audio pipe should be 20 bytes in size, any
    // other size is an error
    if (value.size() != 20) {
        XLOGD_WARN("audio data notification not 20 bytes in size (%d bytes)", value.size());
        return;
    }
    
    GattAudioService::onAudioDataNotification(value);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a notification is received from the Audio Data characteristic.

 */
void GattAudioServiceRdk::onAudioInfoNotification(const vector<uint8_t> &value)
{
    // all notifications from the audio info descriptor should be 6 bytes in size, any other size is an error
    if (value.size() != 6) {
        XLOGD_WARN("audio info notification not 6 bytes in size (%d bytes)", value.size());
        return;
    }
    uint16_t audioFrameCount = value[0] | (value[1] << 8);
    uint32_t audioDurationMs = value[2] | (value[3] << 8) | (value[4] << 16) | (value[5] << 24);
    XLOGD_INFO("audio info notification: audioFrameCount <%u>, audioDurationMs <%u>", audioFrameCount, audioDurationMs);

    GattAudioService::onAudioInfoReceived(audioFrameCount, audioDurationMs);
}

// -----------------------------------------------------------------------------
/*!
    \overload
 
    Examines the header of each frame to check for errors.

 */
void GattAudioServiceRdk::validateFrame(const uint8_t *frame, uint32_t frameCount)
{
    const uint8_t sequenceNumber = frame[0];
    const uint8_t stepIndex = frame[1];
    const int16_t prevValue = (int16_t(frame[2]) << 0) | (int16_t(frame[3]) << 8);

    XLOGD_DEBUG("frame: [%3hhu] <%3hhu,0x%04x> %02x %02x %02x ...", sequenceNumber, stepIndex, uint16_t(prevValue), frame[4], frame[5], frame[6]);

    updateSequenceNumber(sequenceNumber, frameCount);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for the Audio Codecs
    characteristic, returns \c true on success and \c false on failure.

 */
bool GattAudioServiceRdk::getAudioCodecsCharacteristic(const shared_ptr<const BleGattService> &gattService)
{
    // don't re-create if we already have valid proxies
    if (m_audioCodecsCharacteristic && m_audioCodecsCharacteristic->isValid()) {
        return true;
    }

    // get the chararacteristic for the audio control
    m_audioCodecsCharacteristic = gattService->characteristic(BleUuid::AudioCodecs);
    if (!m_audioCodecsCharacteristic || !m_audioCodecsCharacteristic->isValid()) {
        XLOGD_WARN("failed to get audio codecs characteristic");
        return false;
    }

    // set the timeout to two slave latencies, rather than the full 30 seconds
    m_audioCodecsCharacteristic->setTimeout(11000);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for the Audio Gain
    characteristic, returns \c true on success and \c false on failure.

 */
bool GattAudioServiceRdk::getAudioGainCharacteristic(const shared_ptr<const BleGattService> &gattService)
{
    // don't re-create if we already have valid proxies
    if (m_audioGainCharacteristic && m_audioGainCharacteristic->isValid()) {
        return true;
    }

    // get the chararacteristic for the audio control
    m_audioGainCharacteristic = gattService->characteristic(BleUuid::AudioGain);
    if (!m_audioGainCharacteristic || !m_audioGainCharacteristic->isValid()) {
        XLOGD_WARN("failed to get audio gain characteristic");
        return false;
    }

    // set the timeout to two slave latencies, rather than the full 30 seconds
    m_audioGainCharacteristic->setTimeout(11000);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for the Audio Control
    characteristic, returns \c true on success and \c false on failure.

 */
bool GattAudioServiceRdk::getAudioControlCharacteristic(const shared_ptr<const BleGattService> &gattService)
{
    // don't re-create if we already have valid proxies
    if (m_audioCtrlCharacteristic && m_audioCtrlCharacteristic->isValid()){
        return true;
    }

    // get the chararacteristic for the audio control
    m_audioCtrlCharacteristic = gattService->characteristic(BleUuid::AudioControl);
    if (!m_audioCtrlCharacteristic || !m_audioCtrlCharacteristic->isValid()) {
        XLOGD_WARN("failed to get audio control characteristic");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for the Audio Data
    characteristic, returns \c true on success and \c false on failure.

 */
bool GattAudioServiceRdk::getAudioDataCharacteristic(const shared_ptr<const BleGattService> &gattService)
{
    // don't re-create if we already have valid proxies
    if (m_audioDataCharacteristic && m_audioDataCharacteristic->isValid()){
        return true;
    }

    // get the chararacteristic for the audio data
    m_audioDataCharacteristic = gattService->characteristic(BleUuid::AudioData);
    if (!m_audioDataCharacteristic || !m_audioDataCharacteristic->isValid()) {
        XLOGD_WARN("failed to get audio data characteristic");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for the Audio Info
    characteristic, returns \c true on success and \c false on failure.

 */
bool GattAudioServiceRdk::getAudioInfoCharacteristic(const shared_ptr<const BleGattService> &gattService)
{
    // don't re-create if we already have valid proxies
    if (m_audioInfoCharacteristic && m_audioInfoCharacteristic->isValid()){
        return true;
    }

    // get the chararacteristic for the audio data
    m_audioInfoCharacteristic = gattService->characteristic(BleUuid::AudioInfo);
    if (!m_audioInfoCharacteristic || !m_audioInfoCharacteristic->isValid()) {
        XLOGD_WARN("failed to get audio info characteristic");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to create a proxy to the GATT interface for an optional
    characteristic, returns \c true on success and \c false if the RCU doesn't
    have the characteristic.

 */
bool GattAudioServiceRdk::getOptionalCharacteristic(const shared_ptr<const BleGattService> &gattService,
                                                    BleUuid::CustomRdkCharacteristicType uuid,
                                                    shared_ptr<BleGattCharacteristic> &characteristic)
{
    // don't re-create if we already have valid proxies
    if (characteristic && characteristic->isValid()) {
        return true;
    }

    const BleUuid charUuid(uuid);
    characteristic = gattService->characteristic(charUuid);
    if (!characteristic || !characteristic->isValid()) {
        XLOGD_INFO("optional characteristic <%s> not supported by RCU", charUuid.name().c_str());
        characteristic.reset();
        return false;
    }

    // set the timeout to two slave latencies, rather than the full 30 seconds
    characteristic->setTimeout(11000);

    XLOGD_INFO("optional characteristic <%s> found, flags <0x%04X>", charUuid.name().c_str(), characteristic->flags());

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the current gain level.

 */
void GattAudioServiceRdk::requestGainLevel()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to get gain level due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    m_gainLevel = value[0];
                    XLOGD_DEBUG("Successfully read from RCU gain level = %u", m_gainLevel);
                    m_gainLevelChangedSlots.invoke(m_gainLevel);
                } else {
                    XLOGD_ERROR("gain value received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to the bluez daemon to read the characteristic
    m_audioGainCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the current audio codec.

 */
void GattAudioServiceRdk::requestAudioCodecs()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to get audio codec due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == sizeof(m_audioCodecs)) {
                    memcpy(&m_audioCodecs, value.data(), sizeof(m_audioCodecs));
                    XLOGD_DEBUG("Successfully read from RCU audio codecs bit mask = 0x%X", m_audioCodecs);
                    m_audioCodecsChangedSlots.invoke(m_audioCodecs);
                } else {
                    XLOGD_ERROR("audio codec received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to the bluez daemon to read the characteristic
    m_audioCodecsCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to read a 32-bit little endian characteristic value.  On
    success the value is cached in \a value and \a changedSlots are invoked.
    Does nothing if the characteristic isn't supported by the RCU.  If supplied,
    \a onComplete is called once the read has finished, whether or not it
    succeeded, or immediately if the characteristic isn't supported.

 */
void GattAudioServiceRdk::requestUint32Characteristic(const shared_ptr<BleGattCharacteristic> &characteristic,
                                                      const char *name, uint32_t &value, Slots<uint32_t> &changedSlots,
                                                      const std::function<void()> &onComplete)
{
    if (!characteristic || !characteristic->isValid()) {
        if (onComplete) {
            onComplete();
        }
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [name, &value, &changedSlots, onComplete](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to read %s due to <%s>", name, reply->errorMessage().c_str());
            } else {
                const std::vector<uint8_t> result = reply->result();

                if (result.size() == sizeof(uint32_t)) {
                    value = uint32FromLittleEndian(result);
                    XLOGD_INFO("Successfully read from RCU %s = %u (0x%X)", name, value, value);
                    changedSlots.invoke(value);
                } else {
                    XLOGD_ERROR("%s received has invalid length (%u bytes)", name, (unsigned int)result.size());
                }
            }
            if (onComplete) {
                onComplete();
            }
        };

    characteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to read the Audio Mode Control characteristic, which
    contains up to 5 pairs of (HID keycode, audio mode) bytes.  Does nothing if
    the characteristic isn't supported by the RCU.  If supplied, \a onComplete
    is called once the read has finished, whether or not it succeeded.

 */
void GattAudioServiceRdk::requestAudioModeControl(const std::function<void()> &onComplete)
{
    if (!m_audioModeCtrlCharacteristic || !m_audioModeCtrlCharacteristic->isValid()) {
        if (onComplete) {
            onComplete();
        }
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this, onComplete](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to read audio mode control due to <%s>", reply->errorMessage().c_str());
            } else {
                const std::vector<uint8_t> value = reply->result();

                if ((value.size() > AUDIO_MODE_CONTROL_MAX_SIZE) || (value.size() % 2 != 0)) {
                    XLOGD_ERROR("audio mode control received has invalid length (%u bytes)", (unsigned int)value.size());
                } else {
                    std::string str;
                    char buf[16];
                    for (size_t i = 0; i + 1 < value.size(); i += 2) {
                        snprintf(buf, sizeof(buf), "%s0x%02X:%u", str.empty() ? "" : ", ", value[i], value[i + 1]);
                        str += buf;
                    }
                    XLOGD_INFO("Successfully read from RCU audio mode control (keycode:mode) = <%s>", str.c_str());

                    {
                        std::unique_lock<std::mutex> guard(m_audioModeControlMutex);
                        m_audioModeControl = value;
                        m_audioModeControlValid = true;
                    }
                    m_audioModeControlChangedSlots.invoke(value);
                }
            }
            if (onComplete) {
                onComplete();
            }
        };

    m_audioModeCtrlCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \overload

    The gain level is currently not used in a real production box, however
    just in case for debugging we get the gain level as a blocking operation.

    \note This may block for the slave latency period, which is 5 seconds.

 */
uint8_t GattAudioServiceRdk::gainLevel() const
{
    return m_gainLevel;
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Audio Codecs bit mask listing the codecs supported by the remote
 */
uint32_t GattAudioServiceRdk::audioCodecs() const
{
    return m_audioCodecs;
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Audio format used by the remote for the specified encoding
 */
bool GattAudioServiceRdk::audioFormat(Encoding encoding, AudioFormat &format) const
{
    if(encoding == Encoding::ADPCM_FRAME) {
        format.setFrameInfo(AUDIO_FRAME_SIZE, 4);
        format.setHeaderInfoAdpcm(1, 2, 3, 0, 0, 0, 0xFF);
        format.setPressAndHoldSupport(true);
        return(true);
    } else if(encoding == Encoding::PCM16) {
        format.setFrameInfo(AUDIO_FRAME_SIZE, 0);
        format.setPressAndHoldSupport(true);
        return(true);
    }
    XLOGD_ERROR("Invalid encoding type");
    return(false);
}

// -----------------------------------------------------------------------------
/*!
    \overload

    The gain level is currently not used in a real production box, however
    just in case for debugging we get the gain level as a blocking operation.

    \note This may block for the slave latency period, which is 5 seconds.

 */
void GattAudioServiceRdk::setGainLevel(uint8_t level)
{
    // check if the service is running, if not give up
    if (!m_audioGainCharacteristic || stateMachineIsIdle()) {
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("failed to write audio gain level due to <%s>", reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("successfully wrote audio gain level, reading new value");
                requestGainLevel();
            }
        };


    // send a request to the bluez daemon to read the characteristic
    const vector<uint8_t> value(1, level);
    XLOGD_INFO("sending gain level = %u", level);
    m_audioGainCharacteristic->writeValue(value, PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Audio Mode Support bit mask listing the audio modes supported by the remote,
    0 if the remote doesn't have the characteristic.
 */
uint32_t GattAudioServiceRdk::audioModeSupport() const
{
    return m_audioModeSupport;
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Audio Mode Control value, made up of (HID keycode, audio mode) byte pairs.
    Empty if the remote doesn't have the characteristic.
 */
vector<uint8_t> GattAudioServiceRdk::audioModeControl() const
{
    std::unique_lock<std::mutex> guard(m_audioModeControlMutex);
    return m_audioModeControl;
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Press and Release timeout in milliseconds, 0 if the remote doesn't have the
    characteristic.
 */
uint32_t GattAudioServiceRdk::pressAndReleaseTimeout() const
{
    return m_pressAndReleaseTimeout;
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Press and Release threshold in milliseconds, 0 if the remote doesn't have the
    characteristic.
 */
uint32_t GattAudioServiceRdk::pressAndReleaseThreshold() const
{
    return m_pressAndReleaseThreshold;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Writes the Audio Mode Control characteristic.  \a value must be made up of
    (HID keycode, audio mode) byte pairs, up to 5 pairs.  On success the value
    is read back from the RCU and the changed slots are invoked.
 */
void GattAudioServiceRdk::writeAudioModeControl(const vector<uint8_t> &value, PendingReply<> &&reply)
{
    if (!m_audioModeCtrlCharacteristic || !m_audioModeCtrlCharacteristic->isValid() || stateMachineIsIdle()) {
        reply.setError("Audio mode control characteristic is not valid, check that the remote firmware version supports this feature.");
        reply.finish();
        return;
    }
    if (value.empty() || (value.size() > AUDIO_MODE_CONTROL_MAX_SIZE) || (value.size() % 2 != 0)) {
        reply.setError("Invalid audio mode control value length");
        reply.finish();
        return;
    }

    auto replyPtr = make_shared<PendingReply<>>(std::move(reply));

    // lambda invoked when the request returns
    auto replyHandler = [this, replyPtr](PendingReply<> *writeReply)
        {
            if (writeReply->isError()) {
                XLOGD_ERROR("failed to write audio mode control due to <%s>", writeReply->errorMessage().c_str());
                replyPtr->setError(writeReply->errorMessage());
            } else {
                XLOGD_INFO("successfully wrote audio mode control, reading new value");
                requestAudioModeControl();
            }
            replyPtr->finish();
        };

    std::string hex;
    char buf[4];
    for (uint8_t byte : value) {
        snprintf(buf, sizeof(buf), "%s%02X", hex.empty() ? "" : " ", byte);
        hex += buf;
    }
    XLOGD_INFO("sending audio mode control (%u bytes) <%s>", (unsigned int)value.size(), hex.c_str());
    m_audioModeCtrlCharacteristic->writeValue(value, PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Writes the Press and Release Timeout characteristic (milliseconds).
 */
void GattAudioServiceRdk::writePressAndReleaseTimeout(uint32_t timeoutMs, PendingReply<> &&reply)
{
    writeUint32Characteristic(m_parTimeoutCharacteristic, "press and release timeout", timeoutMs,
                              m_pressAndReleaseTimeout, m_pressAndReleaseTimeoutChangedSlots, std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    \overload
    Writes the Press and Release Threshold characteristic (milliseconds).
 */
void GattAudioServiceRdk::writePressAndReleaseThreshold(uint32_t thresholdMs, PendingReply<> &&reply)
{
    writeUint32Characteristic(m_parThresholdCharacteristic, "press and release threshold", thresholdMs,
                              m_pressAndReleaseThreshold, m_pressAndReleaseThresholdChangedSlots, std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Writes a 32-bit little endian value to \a characteristic.  On success the
    value is read back from the RCU, which updates \a value and invokes
    \a changedSlots.
 */
void GattAudioServiceRdk::writeUint32Characteristic(const shared_ptr<BleGattCharacteristic> &characteristic,
                                                    const char *name, uint32_t newValue,
                                                    uint32_t &value, Slots<uint32_t> &changedSlots,
                                                    PendingReply<> &&reply)
{
    if (!characteristic || !characteristic->isValid() || stateMachineIsIdle()) {
        reply.setError(std::string(name) + " characteristic is not valid, check that the remote firmware version supports this feature.");
        reply.finish();
        return;
    }

    auto replyPtr = make_shared<PendingReply<>>(std::move(reply));

    // lambda invoked when the request returns
    auto replyHandler = [this, replyPtr, characteristic, name, &value, &changedSlots](PendingReply<> *writeReply)
        {
            if (writeReply->isError()) {
                XLOGD_ERROR("failed to write %s due to <%s>", name, writeReply->errorMessage().c_str());
                replyPtr->setError(writeReply->errorMessage());
            } else {
                XLOGD_INFO("successfully wrote %s, reading new value", name);
                requestUint32Characteristic(characteristic, name, value, changedSlots);
            }
            replyPtr->finish();
        };

    XLOGD_INFO("sending %s = %u", name, newValue);
    characteristic->writeValue(uint32ToLittleEndian(newValue), PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called once per connection after the audio mode characteristics have been
    read.  Compares the voice key audio modes reported by the RCU against the
    desired configuration and writes the configuration to the RCU if they
    differ.

    The desired configuration comes from the "voice_key_mode" config array.  If
    that is empty, the RCU model default is applied to all voice keys reported
    by the RCU: "legacy" for XRAB, "hold" otherwise.
 */
void GattAudioServiceRdk::applyVoiceKeyModeConfig()
{
    // older RCUs don't support audio modes, they only use press and hold
    if (!m_audioModeCtrlCharacteristic || !m_audioModeCtrlCharacteristic->isValid()) {
        return;
    }

    vector<uint8_t> current;
    {
        std::unique_lock<std::mutex> guard(m_audioModeControlMutex);
        if (!m_audioModeControlValid) {
            XLOGD_WARN("audio mode control value not read from RCU, can't apply voice key modes");
            return;
        }
        current = m_audioModeControl;
    }

    vector<pair<uint8_t, uint8_t>> currentKeys;
    for (size_t i = 0; i + 1 < current.size(); i += 2) {
        if (current[i] != 0 && current[i] != CTRLM_HAL_BLE_AUDIO_MODE_CONTROL_UNUSED_KEY) {
            currentKeys.emplace_back(current[i], current[i + 1]);
        }
    }

    vector<pair<uint8_t, uint8_t>> desiredKeys;
    bool     thresholdPresent = false;
    uint32_t thresholdMs      = 0;
    bool     timeoutPresent   = false;
    uint32_t timeoutMs        = 0;
    const char *source;

    if (!m_voiceKeyModes.empty()) {
        source = "config";
        for (const auto &keyMode : m_voiceKeyModes) {
            desiredKeys.emplace_back(keyMode.hidKey, keyMode.audioMode);

            // the threshold and timeout are global RCU settings, so the first value found is used
            if (keyMode.thresholdPresent) {
                if (!thresholdPresent) {
                    thresholdPresent = true;
                    thresholdMs      = keyMode.thresholdMs;
                } else if (thresholdMs != keyMode.thresholdMs) {
                    XLOGD_WARN("conflicting voice key mode thresholds, using <%u ms>", thresholdMs);
                }
            }
            if (keyMode.timeoutPresent) {
                if (!timeoutPresent) {
                    timeoutPresent = true;
                    timeoutMs      = keyMode.timeoutMs;
                } else if (timeoutMs != keyMode.timeoutMs) {
                    XLOGD_WARN("conflicting voice key mode timeouts, using <%u ms>", timeoutMs);
                }
            }
        }
    } else {
        source = "model default";
        uint8_t mode = CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_HOLD;
        if (m_modelName == VOICE_KEY_MODE_LEGACY_DEFAULT_MODEL) {
            mode = CTRLM_HAL_BLE_AUDIO_MODE_LEGACY;
        }
        if (currentKeys.empty()) {
            XLOGD_WARN("RCU model <%s> doesn't report any voice keys in audio mode control, can't apply default voice key mode", m_modelName.c_str());
            return;
        }
        for (const auto &key : currentKeys) {
            desiredKeys.emplace_back(key.first, mode);
        }
    }

    // make sure the RCU supports the requested modes.  Press and hold is always supported.
    const uint32_t supported = m_audioModeSupport | CTRLM_HAL_BLE_AUDIO_MODE_SUPPORT_PRESS_AND_HOLD;
    for (const auto &key : desiredKeys) {
        uint32_t required = 0;
        switch (key.second) {
            case CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_HOLD:
                required = CTRLM_HAL_BLE_AUDIO_MODE_SUPPORT_PRESS_AND_HOLD;
                break;
            case CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_AND_HOLD_THRESHOLD:
            case CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_TIMEOUT:
                required = CTRLM_HAL_BLE_AUDIO_MODE_SUPPORT_PRESS_AND_RELEASE_TIMEOUT;
                break;
            case CTRLM_HAL_BLE_AUDIO_MODE_PRESS_AND_RELEASE_CONTINUOUS:
                required = CTRLM_HAL_BLE_AUDIO_MODE_SUPPORT_PRESS_AND_RELEASE_CONTINUOUS;
                break;
            case CTRLM_HAL_BLE_AUDIO_MODE_VOICE_DISABLED:
            case CTRLM_HAL_BLE_AUDIO_MODE_LEGACY:     // existing voice behavior, doesn't have a support bit
            default:
                break;
        }
        if ((supported & required) != required) {
            XLOGD_ERROR("RCU model <%s> audio mode support <0x%X> doesn't support mode <%u> for key <0x%02X>, not applying %s voice key modes",
                        m_modelName.c_str(), m_audioModeSupport, key.second, key.first, source);
            return;
        }
    }

    if (thresholdPresent && !m_parThresholdCharacteristic) {
        XLOGD_WARN("RCU doesn't have the press and release threshold characteristic, threshold <%u ms> not applied", thresholdMs);
        thresholdPresent = false;
    }
    if (timeoutPresent && !m_parTimeoutCharacteristic) {
        XLOGD_WARN("RCU doesn't have the press and release timeout characteristic, timeout <%u ms> not applied", timeoutMs);
        timeoutPresent = false;
    }

    vector<pair<uint8_t, uint8_t>> currentSorted(currentKeys);
    vector<pair<uint8_t, uint8_t>> desiredSorted(desiredKeys);
    std::sort(currentSorted.begin(), currentSorted.end());
    std::sort(desiredSorted.begin(), desiredSorted.end());

    const bool controlMatch   = (currentSorted == desiredSorted);
    const bool thresholdMatch = !thresholdPresent || (thresholdMs == m_pressAndReleaseThreshold);
    const bool timeoutMatch   = !timeoutPresent   || (timeoutMs   == m_pressAndReleaseTimeout);

    if (controlMatch && thresholdMatch && timeoutMatch) {
        XLOGD_INFO("RCU voice key modes match the %s", source);
        return;
    }

    // write the threshold and timeout before the modes, so the RCU never uses a new mode with an old threshold/timeout
    auto steps = make_shared<std::deque<VoiceKeyModeWriteStep>>();
    if (!thresholdMatch) {
        XLOGD_INFO("RCU press and release threshold <%u ms> doesn't match the %s <%u ms>", m_pressAndReleaseThreshold, source, thresholdMs);
        steps->push_back({ "press and release threshold", [this, thresholdMs](PendingReply<> &&reply) {
            writePressAndReleaseThreshold(thresholdMs, std::move(reply));
        }});
    }
    if (!timeoutMatch) {
        XLOGD_INFO("RCU press and release timeout <%u ms> doesn't match the %s <%u ms>", m_pressAndReleaseTimeout, source, timeoutMs);
        steps->push_back({ "press and release timeout", [this, timeoutMs](PendingReply<> &&reply) {
            writePressAndReleaseTimeout(timeoutMs, std::move(reply));
        }});
    }
    if (!controlMatch) {
        // Only the (keycode, audio mode) pairs for the configured keys are sent, unused entries are not padded
        vector<uint8_t> value;
        std::string str;
        char buf[16];
        for (size_t i = 0; i < desiredKeys.size() && i < CTRLM_HAL_BLE_AUDIO_MODE_CONTROL_MAX_KEYS; i++) {
            value.push_back(desiredKeys[i].first);
            value.push_back(desiredKeys[i].second);
            snprintf(buf, sizeof(buf), "%s0x%02X:%u", str.empty() ? "" : ", ", desiredKeys[i].first, desiredKeys[i].second);
            str += buf;
        }
        XLOGD_INFO("RCU audio mode control doesn't match the %s (keycode:mode) <%s>", source, str.c_str());
        steps->push_back({ "audio mode control", [this, value](PendingReply<> &&reply) {
            writeAudioModeControl(value, std::move(reply));
        }});
    }

    runVoiceKeyModeWrites(steps);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Performs the voice key mode writes in \a steps one at a time, stopping at the
    first failure.
 */
void GattAudioServiceRdk::runVoiceKeyModeWrites(const shared_ptr<std::deque<VoiceKeyModeWriteStep>> &steps)
{
    if (steps->empty()) {
        XLOGD_INFO("successfully applied voice key modes to RCU");
        return;
    }

    VoiceKeyModeWriteStep step = std::move(steps->front());
    steps->pop_front();

    const char *name = step.name;
    auto replyHandler = [this, steps, name](PendingReply<> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to apply voice key modes to RCU, %s write failed due to <%s>", name, reply->errorMessage().c_str());
                return;
            }
            runVoiceKeyModeWrites(steps);
        };

    step.write(PendingReply<>(getIsAlivePtr(), replyHandler));
}
