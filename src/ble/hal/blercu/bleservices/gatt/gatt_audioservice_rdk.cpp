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

#define AUDIO_FRAME_SIZE (100)

using namespace std;

const BleUuid GattAudioServiceRdk::m_serviceUuid(BleUuid::RdkVoice);

GattAudioServiceRdk::GattAudioServiceRdk(GMainLoop* mainLoop) : GattAudioService(AUDIO_FRAME_SIZE, 5, AUDIO_PACKET_DURATION_USEC, mainLoop)
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

    requestGainLevel();
    requestAudioCodecs();

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
    m_audioGainCharacteristic.reset();
    m_audioCtrlCharacteristic.reset();
    m_audioDataCharacteristic.reset();

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
    auto replyHandler = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {
                // this is bad if this happens as we won't get updates, so we install a timer to 
                // retry enabling notifications in a couple of seconds time
                XLOGD_ERROR("failed to enable audio notifications due to <%s>", 
                        reply->errorMessage().c_str());

                setLastError(StreamingError::InternalError);
                stateMachinePostEvent(GattErrorEvent);

            } else {
                // notifications enabled so post an event to the state machine
                stateMachinePostEvent(NotificationsEnabledEvent);
            }
        };

    if (m_audioDataCharacteristic->notificationsEnabled()) {
        
        // notifications already enabled so post an event to the state machine
        stateMachinePostEvent(NotificationsEnabledEvent);

    } else {
        m_audioDataCharacteristic->enableNotifications(
                Slot<const std::vector<uint8_t> &>(getIsAlivePtr(),
                    std::bind(&GattAudioServiceRdk::onAudioDataNotification, this, std::placeholders::_1)), 
                PendingReply<>(getIsAlivePtr(), replyHandler));
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
        format.setHeaderInfoAdpcm(1, 2, 3, 0, 0, 0xFF);
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
