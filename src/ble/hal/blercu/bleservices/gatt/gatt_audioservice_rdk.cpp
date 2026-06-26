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

#include <unistd.h>
#include <errno.h>

// The duration of audio in a single packet is 96 bytes * 2 samples/byte / 16 samples/msec * 1000 usec/msec = 12000 usec
#define AUDIO_PACKET_DURATION_USEC (12000)

#define AUDIO_FRAME_SIZE           (100)
#define MFV_INITIAL_READ_COUNT     (4)

#define AUDIO_SEQ_NUM_MAX          (0xFF)

using namespace std;

const BleUuid GattAudioServiceRdk::m_serviceUuid(BleUuid::RdkVoice);

const BleUuid GattAudioServiceRdk::m_mfvSessionStartCharUuid(BleUuid::SessionStart);
const BleUuid GattAudioServiceRdk::m_mfvDetectionDataCharUuid(BleUuid::DetectionData);
const BleUuid GattAudioServiceRdk::m_mfvModelVersionCharUuid(BleUuid::WakeWordModelVersion);
const BleUuid GattAudioServiceRdk::m_mfvPrivacyCharUuid(BleUuid::PrivacySettings);
const BleUuid GattAudioServiceRdk::m_mfvModelConfigCharUuid(BleUuid::ModelConfiguration);
const BleUuid GattAudioServiceRdk::m_mfvCapabilitiesCharUuid(BleUuid::MfvCapabilities);

GattAudioServiceRdk::GattAudioServiceRdk(GMainLoop* mainLoop) : GattAudioService(AUDIO_FRAME_SIZE, 5, AUDIO_PACKET_DURATION_USEC, AUDIO_SEQ_NUM_MAX, mainLoop)
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

    requestGainLevel();
    requestAudioCodecs();

    // Discover MFV characteristics (optional - not all remotes support MFV)
    if (getMfvCharacteristics(gattService)) {
        m_mfvSupported = true;
        m_mfvInitialReadsRemaining = MFV_INITIAL_READ_COUNT;
        requestMfvCapabilities();
        requestMfvModelVersion();
        requestMfvPrivacy();
        requestMfvModelConfig();
    }

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

    // Clean up MFV characteristics
    if (m_mfvSessionStartCharacteristic) {
        m_mfvSessionStartCharacteristic->disableNotifications();
        m_mfvSessionStartCharacteristic.reset();
    }
    if (m_mfvDetectionDataCharacteristic) {
        m_mfvDetectionDataCharacteristic->disableNotifications();
        m_mfvDetectionDataCharacteristic.reset();
    }
    if (m_mfvPrivacyCharacteristic) {
        m_mfvPrivacyCharacteristic->disableNotifications();
        m_mfvPrivacyCharacteristic.reset();
    }
    m_mfvModelVersionCharacteristic.reset();
    m_mfvModelConfigCharacteristic.reset();
    m_mfvCapabilitiesCharacteristic.reset();
    m_mfvSupported = false;
    m_mfvInitialReadsRemaining = 0;

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
                // this is bad if this happens as we won't get updates, so we install a timer to 
                // retry enabling notifications in a couple of seconds time
                XLOGD_ERROR("failed to enable audio data notifications due to <%s>", 
                        reply->errorMessage().c_str());

                setLastError(StreamingError::InternalError);
                stateMachinePostEvent(GattErrorEvent);

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

    // Enable MFV notifications here if initial reads have already completed,
    // otherwise onMfvInitialReadComplete() will enable them when they finish.
    if (areMfvInitialReadsComplete()) {
        requestStartMfvSessionStartNotify();
        requestStartMfvDetectionDataNotify();
        requestStartMfvPrivacyNotify();
    }
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
                XLOGD_ERROR("failed to write audio control characteristic due to <%s>", 
                        reply->errorMessage().c_str());

                setLastError(StreamingError::InternalError);
                stateMachinePostEvent(GattErrorEvent);

            } else {
                stateMachinePostEvent(StreamingStartedEvent);
            }
        };


    // the first byte is the codec to use, the second byte is to enable voice
    const vector<uint8_t> value({ 0x01, 0x01 });

    // send a write request to write the control characteristic
    m_audioCtrlCharacteristic->writeValueWithoutResponse(value, PendingReply<>(getIsAlivePtr(), replyHandler));

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
                XLOGD_ERROR("failed to write audio control characteristic due to <%s>", 
                        reply->errorMessage().c_str());

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
                    XLOGD_INFO("Successfully read from RCU gain level = %u", m_gainLevel);
                    m_gainLevelChangedSlots.invoke(m_gainLevel);
                } else {
                    XLOGD_ERROR("gain value received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to the bluez daemon to read the characteristic
    m_audioGainCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

bool GattAudioServiceRdk::areMfvInitialReadsComplete() const
{
    return m_mfvSupported && (m_mfvInitialReadsRemaining == 0);
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
                    XLOGD_INFO("Successfully read from RCU audio codecs bit mask = 0x%X", m_audioCodecs);
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


// =============================================================================
// MFV implementation
// =============================================================================

// -----------------------------------------------------------------------------
/*!
    \internal

    Attempts to discover all MFV characteristics from the RDK Voice GATT service.
    Returns true if all were found, false otherwise.
 */
bool GattAudioServiceRdk::getMfvCharacteristics(const shared_ptr<const BleGattService> &gattService)
{
    m_mfvSessionStartCharacteristic = gattService->characteristic(m_mfvSessionStartCharUuid);
    if (!m_mfvSessionStartCharacteristic || !m_mfvSessionStartCharacteristic->isValid()) {
        XLOGD_INFO("MFV Session Start characteristic not found (MFV not supported)");
        return false;
    }

    m_mfvDetectionDataCharacteristic = gattService->characteristic(m_mfvDetectionDataCharUuid);
    if (!m_mfvDetectionDataCharacteristic || !m_mfvDetectionDataCharacteristic->isValid()) {
        XLOGD_INFO("MFV Detection Data characteristic not found");
        return false;
    }

    m_mfvModelVersionCharacteristic = gattService->characteristic(m_mfvModelVersionCharUuid);
    if (!m_mfvModelVersionCharacteristic || !m_mfvModelVersionCharacteristic->isValid()) {
        XLOGD_INFO("MFV Wake Word Model Version characteristic not found");
        return false;
    }

    m_mfvPrivacyCharacteristic = gattService->characteristic(m_mfvPrivacyCharUuid);
    if (!m_mfvPrivacyCharacteristic || !m_mfvPrivacyCharacteristic->isValid()) {
        XLOGD_INFO("MFV Privacy Settings characteristic not found");
        return false;
    }

    m_mfvModelConfigCharacteristic = gattService->characteristic(m_mfvModelConfigCharUuid);
    if (!m_mfvModelConfigCharacteristic || !m_mfvModelConfigCharacteristic->isValid()) {
        XLOGD_INFO("MFV Model Configuration characteristic not found");
        return false;
    }

    m_mfvCapabilitiesCharacteristic = gattService->characteristic(m_mfvCapabilitiesCharUuid);
    if (!m_mfvCapabilitiesCharacteristic || !m_mfvCapabilitiesCharacteristic->isValid()) {
        XLOGD_INFO("MFV Capabilities characteristic not found");
        return false;
    }

    XLOGD_INFO("All MFV characteristics discovered successfully");
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the last MFV initial read completes. Enables MFV notifications
    and fires the changed slots with initial values.
 */
void GattAudioServiceRdk::onMfvInitialReadComplete()
{
    // Publish initial values via changed slots
    m_mfvCapabilitiesChangedSlots.invoke(m_mfvCapabilitiesValue);
    m_mfvPrivacyChangedSlots.invoke(m_mfvPrivacyEnabled);

    // Enable notifications now only if the audio state machine has already passed
    // through EnableNotificationsState (i.e. the service is already ready/streaming).
    // If not, onEnteredEnableNotificationsState() will handle it when the time comes.
    if (!stateMachineIsIdle()) {
        requestStartMfvSessionStartNotify();
        requestStartMfvDetectionDataNotify();
        requestStartMfvPrivacyNotify();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the MFV Capabilities characteristic (1 byte bitmask).
 */
void GattAudioServiceRdk::requestMfvCapabilities()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Capabilities due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 1) {
                m_mfvCapabilitiesValue = value[0];
                XLOGD_INFO("MFV Capabilities = 0x%02X (midfield=%d privacy_ctrl=%d soww_eoww=%d aad_ctrl=%d)",
                    m_mfvCapabilitiesValue,
                    !!(m_mfvCapabilitiesValue & MidfieldVoiceCapable),
                    !!(m_mfvCapabilitiesValue & SoftwarePrivacyControl),
                    !!(m_mfvCapabilitiesValue & SowwEowwTimingAvailable),
                    !!(m_mfvCapabilitiesValue & AadSensitivityControlAvailable));
            } else {
                XLOGD_ERROR("MFV Capabilities has invalid length (%zu bytes, expected 1)", value.size());
            }
        }
        if (--m_mfvInitialReadsRemaining <= 0) {
            onMfvInitialReadComplete();
        }
    };

    m_mfvCapabilitiesCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Wake Word Model Version characteristic (2 bytes: major, minor).
 */
void GattAudioServiceRdk::requestMfvModelVersion()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Model Version due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 2) {
                m_mfvModelVersionData.major = value[0];
                m_mfvModelVersionData.minor = value[1];
                XLOGD_INFO("MFV Wake Word Model Version = %u.%u", m_mfvModelVersionData.major, m_mfvModelVersionData.minor);
            } else {
                XLOGD_ERROR("MFV Model Version has invalid length (%zu bytes, expected 2)", value.size());
            }
        }
        if (--m_mfvInitialReadsRemaining <= 0) {
            onMfvInitialReadComplete();
        }
    };

    m_mfvModelVersionCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Privacy Settings characteristic (1 byte boolean).
 */
void GattAudioServiceRdk::requestMfvPrivacy()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Privacy due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 1) {
                m_mfvPrivacyEnabled = (value[0] != 0);
                XLOGD_INFO("MFV Privacy = %s", m_mfvPrivacyEnabled ? "enabled" : "disabled");
            } else {
                XLOGD_ERROR("MFV Privacy has invalid length (%zu bytes, expected 1)", value.size());
            }
        }
        if (--m_mfvInitialReadsRemaining <= 0) {
            onMfvInitialReadComplete();
        }
    };

    m_mfvPrivacyCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Model Configuration characteristic (3 bytes: sensitivity,
    secondary sensitivity, AAD sensitivity).
 */
void GattAudioServiceRdk::requestMfvModelConfig()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Model Config due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 3) {
                m_mfvModelConfigurationData = value;
                XLOGD_INFO("MFV Model Config: sensitivity=%u secondary=%u aad=%u",
                    value[0], value[1], value[2]);
            } else {
                XLOGD_ERROR("MFV Model Config has invalid length (%zu bytes, expected 3)", value.size());
            }
        }
        if (--m_mfvInitialReadsRemaining <= 0) {
            onMfvInitialReadComplete();
        }
    };

    m_mfvModelConfigCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Session Start characteristic (0xEA08).
 */
void GattAudioServiceRdk::requestStartMfvSessionStartNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable MFV Session Start notifications due to <%s>",
                reply->errorMessage().c_str());
        } else {
            XLOGD_DEBUG("MFV Session Start notifications enabled successfully");
        }
    };

    m_mfvSessionStartCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
            std::bind(&GattAudioServiceRdk::onMfvSessionStartChanged, this, std::placeholders::_1)),
        PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Detection Data characteristic (0xEA09).
 */
void GattAudioServiceRdk::requestStartMfvDetectionDataNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable MFV Detection Data notifications due to <%s>",
                reply->errorMessage().c_str());
        } else {
            XLOGD_DEBUG("MFV Detection Data notifications enabled successfully");
        }
    };

    m_mfvDetectionDataCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
            std::bind(&GattAudioServiceRdk::onMfvDetectionDataChanged, this, std::placeholders::_1)),
        PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Privacy Settings characteristic (0xEA0B).
 */
void GattAudioServiceRdk::requestStartMfvPrivacyNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable MFV Privacy notifications due to <%s>",
                reply->errorMessage().c_str());
        } else {
            XLOGD_DEBUG("MFV Privacy notifications enabled successfully");
        }
    };

    m_mfvPrivacyCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
            std::bind(&GattAudioServiceRdk::onMfvPrivacyChanged, this, std::placeholders::_1)),
        PendingReply<>(getIsAlivePtr(), replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Session Start notification is received from the RCU.
    Payload: 1 byte detection type (0x01=FullPower, 0x02=AAD, 0x03=BelowThreshold).
 */
void GattAudioServiceRdk::onMfvSessionStartChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 1) {
        XLOGD_ERROR("MFV Session Start notification has invalid length (%zu bytes, expected 1)", newValue.size());
        return;
    }

    uint8_t raw = newValue[0];
    if (raw < 0x01 || raw > 0x03) {
        XLOGD_WARN("MFV Session Start notification has unknown detection type 0x%02X", raw);
    }

    m_mfvDetectionType = static_cast<DetectionType>(raw);
    XLOGD_INFO("MFV Session Start: detection type = 0x%02X", raw);

    m_mfvDetectionTypeChangedSlots.invoke(m_mfvDetectionType);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Detection Data notification is received from the RCU.
    Payload: 6 bytes, little-endian (start, end, confidence).
 */
void GattAudioServiceRdk::onMfvDetectionDataChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 6) {
        XLOGD_ERROR("MFV Detection Data notification has invalid length (%zu bytes, expected 6)", newValue.size());
        return;
    }

    m_mfvDetectionData.start      = static_cast<uint16_t>(newValue[0]) | (static_cast<uint16_t>(newValue[1]) << 8);
    m_mfvDetectionData.end        = static_cast<uint16_t>(newValue[2]) | (static_cast<uint16_t>(newValue[3]) << 8);
    m_mfvDetectionData.confidence = static_cast<uint16_t>(newValue[4]) | (static_cast<uint16_t>(newValue[5]) << 8);

    XLOGD_INFO("MFV Detection Data: start=%u end=%u confidence=%.1f%%",
        m_mfvDetectionData.start, m_mfvDetectionData.end,
        m_mfvDetectionData.confidence / 10.0);

    m_mfvDetectionDataChangedSlots.invoke(m_mfvDetectionData);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Privacy Settings notification is received from the RCU.
    Payload: 1 byte boolean (1 = enabled, 0 = disabled).
 */
void GattAudioServiceRdk::onMfvPrivacyChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 1) {
        XLOGD_ERROR("MFV Privacy notification has invalid length (%zu bytes, expected 1)", newValue.size());
        return;
    }

    m_mfvPrivacyEnabled = (newValue[0] != 0);
    XLOGD_INFO("MFV Privacy changed: %s", m_mfvPrivacyEnabled ? "enabled" : "disabled");

    m_mfvPrivacyChangedSlots.invoke(m_mfvPrivacyEnabled);
}

// =============================================================================
// MFV accessors
// =============================================================================

BleRcuAudioService::DetectionType GattAudioServiceRdk::mfvDetectionType() const
{
    return m_mfvDetectionType;
}

BleRcuAudioService::DetectionData GattAudioServiceRdk::mfvDetectionData() const
{
    return m_mfvDetectionData;
}

BleRcuAudioService::ModelVersion GattAudioServiceRdk::mfvModelVersion() const
{
    return m_mfvModelVersionData;
}

bool GattAudioServiceRdk::mfvPrivacyEnabled() const
{
    return m_mfvPrivacyEnabled;
}

std::vector<uint8_t> GattAudioServiceRdk::mfvModelConfiguration() const
{
    return m_mfvModelConfigurationData;
}

uint8_t GattAudioServiceRdk::mfvCapabilities() const
{
    return m_mfvCapabilitiesValue;
}

BleRcuAudioService::StreamStatsRaw GattAudioServiceRdk::mfvStreamStats() const
{
    return m_mfvStreamStatsData;
}

// =============================================================================
// MFV write handlers
// =============================================================================

void GattAudioServiceRdk::writeMfvPrivacy(bool enabled, PendingReply<> &&reply)
{
    if (!m_mfvSupported) {
        reply.setError("MFV not supported on this remote");
        reply.finish();
        return;
    }

    if (m_mfvPromiseResults) {
        reply.setError("MFV write already in progress");
        reply.finish();
        return;
    }

    if (!m_mfvPrivacyCharacteristic || !m_mfvPrivacyCharacteristic->isValid()) {
        reply.setError("MFV Privacy characteristic is not valid");
        reply.finish();
        return;
    }

    m_mfvPromiseResults = make_shared<PendingReply<>>(std::move(reply));

    const vector<uint8_t> value(1, enabled ? 0x01 : 0x00);
    XLOGD_INFO("Writing MFV Privacy = %s", enabled ? "enabled" : "disabled");

    m_mfvPrivacyCharacteristic->writeValue(value, PendingReply<>(getIsAlivePtr(),
        std::bind(&GattAudioServiceRdk::onWriteMfvPrivacyReply, this, std::placeholders::_1)));
}

void GattAudioServiceRdk::onWriteMfvPrivacyReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        XLOGD_ERROR("failed to write MFV Privacy due to <%s>", reply->errorMessage().c_str());
        if (m_mfvPromiseResults) {
            m_mfvPromiseResults->setError(reply->errorMessage());
            m_mfvPromiseResults->finish();
            m_mfvPromiseResults.reset();
        }
    } else {
        XLOGD_INFO("MFV Privacy written successfully");
        if (m_mfvPromiseResults) {
            m_mfvPromiseResults->finish();
            m_mfvPromiseResults.reset();
        }
    }
}

void GattAudioServiceRdk::writeMfvModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply)
{
    if (!m_mfvSupported) {
        reply.setError("MFV not supported on this remote");
        reply.finish();
        return;
    }

    if (m_mfvPromiseResults) {
        reply.setError("MFV write already in progress");
        reply.finish();
        return;
    }

    if (!m_mfvModelConfigCharacteristic || !m_mfvModelConfigCharacteristic->isValid()) {
        reply.setError("MFV Model Configuration characteristic is not valid");
        reply.finish();
        return;
    }

    if (aad != 0 && (aad < 60 || aad > 95 || (aad % 5) != 0)) {
        XLOGD_ERROR("MFV AAD sensitivity %u is out of valid range [60..95 step 5]", aad);
        reply.setError("AAD sensitivity out of valid range [60..95 step 5]");
        reply.finish();
        return;
    }

    m_mfvPromiseResults = make_shared<PendingReply<>>(std::move(reply));

    const vector<uint8_t> value = { sensitivity, secondary, aad };
    XLOGD_INFO("Writing MFV Model Config: sensitivity=%u secondary=%u aad=%u",
        sensitivity, secondary, aad);

    m_mfvModelConfigCharacteristic->writeValue(value, PendingReply<>(getIsAlivePtr(),
        std::bind(&GattAudioServiceRdk::onWriteMfvModelConfigReply, this, std::placeholders::_1)));
}

void GattAudioServiceRdk::onWriteMfvModelConfigReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        XLOGD_ERROR("failed to write MFV Model Config due to <%s>", reply->errorMessage().c_str());
        if (m_mfvPromiseResults) {
            m_mfvPromiseResults->setError(reply->errorMessage());
            m_mfvPromiseResults->finish();
            m_mfvPromiseResults.reset();
        }
    } else {
        XLOGD_INFO("MFV Model Config written successfully");
        if (m_mfvPromiseResults) {
            m_mfvPromiseResults->finish();
            m_mfvPromiseResults.reset();
        }
    }
}
