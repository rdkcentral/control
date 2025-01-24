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
//  gatt_upgradeservice.cpp
//

#include "gatt_upgradeservice.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"
#include "blercu/blegattdescriptor.h"

#include "ctrlm_log_ble.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


/// Possible packet opcodes
#define OPCODE_WRQ                    uint8_t(0x0 << 6)
#define OPCODE_DATA                   uint8_t(0x1 << 6)
#define OPCODE_ACK                    uint8_t(0x2 << 6)
#define OPCODE_ERROR                  uint8_t(0x3 << 6)

#define OPCODE_MASK                   uint8_t(0x3 << 6)

/// The maximum number of data bytes in a DATA packet
#define FIRMWARE_PACKET_MTU           18

using namespace std;

/// The structure representing the contents of the control point characteristic
struct ControlPoint {
    uint32_t deviceModelId;
    uint32_t firmwareVersion;
    uint32_t firmwareCrc32;
};

static gboolean timerEvent(gpointer user_data);

GattUpgradeService::GattUpgradeService(GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_ready(false)
    , m_setupFlags(0)
    , m_progress(-1)
    , m_windowSize(5)
    , m_timeoutTimer(0)
    , m_lastAckBlockId(-1)
    , m_timeoutCounter(0)
{
    // initialise the state machine for the upgrades
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattUpgradeService::~GattUpgradeService()
{
    *m_isAlive = false;

    // clean up the firmware file
    m_fwFile.reset();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sets up the state machine used for the firmware download.

 */
void GattUpgradeService::init()
{
    // set the name of the state machine for logging
    m_stateMachine.setObjectName("GattUpgradeService");

    // add all the states
    m_stateMachine.addState(InitialState, "Initial");

    m_stateMachine.addState(SendingSuperState, "SendingSuperState");
    m_stateMachine.addState(SendingSuperState, SendingWriteRequestState, "SendingWriteRequest");
    m_stateMachine.addState(SendingSuperState, SendingDataState, "SendingData");

    m_stateMachine.addState(ErroredState, "Errored");
    m_stateMachine.addState(FinishedState, "Finished");


    // add the transitions       From State           ->    Event              ->   To State
    m_stateMachine.addTransition(InitialState,              CancelledEvent,         FinishedState);
    m_stateMachine.addTransition(InitialState,              StopServiceEvent,       ErroredState);
    m_stateMachine.addTransition(InitialState,              EnableNotifyErrorEvent, ErroredState);
    m_stateMachine.addTransition(InitialState,              ReadErrorEvent,         ErroredState);
    m_stateMachine.addTransition(InitialState,              FinishedSetupEvent,     SendingWriteRequestState);

    m_stateMachine.addTransition(SendingSuperState,         CancelledEvent,         FinishedState);
    m_stateMachine.addTransition(SendingSuperState,         StopServiceEvent,       ErroredState);
    m_stateMachine.addTransition(SendingSuperState,         WriteErrorEvent,        ErroredState);
    m_stateMachine.addTransition(SendingSuperState,         PacketErrorEvent,       ErroredState);
    m_stateMachine.addTransition(SendingSuperState,         TimeoutErrorEvent,      ErroredState);
    m_stateMachine.addTransition(SendingWriteRequestState,  PacketAckEvent,         SendingDataState);
    m_stateMachine.addTransition(SendingDataState,          CompleteEvent,          FinishedState);

    m_stateMachine.addTransition(ErroredState,              CompleteEvent,          FinishedState);


    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattUpgradeService::onStateEntry, this, std::placeholders::_1)));


    // set the initial state
    m_stateMachine.setInitialState(InitialState);
    m_stateMachine.setFinalState(FinishedState);
}

// -----------------------------------------------------------------------------
/*!
    Returns the uuid of the upgrade service.

 */
BleUuid GattUpgradeService::uuid()
{
    return BleUuid(BleUuid::RdkFirmwareUpgrade);
}

// -----------------------------------------------------------------------------
/*!
    This service is always ready.

 */
bool GattUpgradeService::isReady() const
{
    return m_ready;
}

// -----------------------------------------------------------------------------
/*!
    Should be called whenever the bluez GATT profile is updated, this is used
    to update the internal dbus proxies to the GATT characteristics &
    descriptors.


 */
bool GattUpgradeService::start(const shared_ptr<const BleGattService> &gattService)
{
    // this service doesn't have a 'ready' state-machine, it's always ready if
    // started and not ready when stopped
    m_ready = true;


    // try and get the gatt characteristic for the OTA control point
    if (!m_controlCharacteristic || !m_controlCharacteristic->isValid()) {

        m_controlCharacteristic = gattService->characteristic(BleUuid::FirmwareControlPoint);
        if (!m_controlCharacteristic || !m_controlCharacteristic->isValid()) {
            XLOGD_WARN("failed get the f/w upgrade control point gatt proxy");
            m_controlCharacteristic.reset();
            return true;
        }
    }

    // try and get the gatt characteristic for the OTA packet
    if (!m_packetCharacteristic || !m_packetCharacteristic->isValid()) {

        m_packetCharacteristic = gattService->characteristic(BleUuid::FirmwarePacket);
        if (!m_packetCharacteristic || !m_packetCharacteristic->isValid()) {
            XLOGD_WARN("failed get the f/w upgrade packet gatt proxy");
            m_packetCharacteristic.reset();
            return true;
        }
    }

    // try and get the gatt window size descriptor, this is optional
    if (!m_windowSizeDescriptor || !m_windowSizeDescriptor->isValid()) {

        m_windowSizeDescriptor = m_packetCharacteristic->descriptor(BleUuid::FirmwarePacketWindowSize);
        if (!m_windowSizeDescriptor || !m_windowSizeDescriptor->isValid()) {
            // this descriptor is optional so don't log an error if it wasn't found
            m_windowSizeDescriptor.reset();
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!

 */
void GattUpgradeService::stop()
{
    // no longer ready
    m_ready = false;

    // if a download is in progress then it's cancelled
    if (m_stateMachine.isRunning()) {
        m_lastError = "Device disconnected";
        m_stateMachine.postEvent(StopServiceEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \reimp

    Attempts to starts the firmware update.

 */
void GattUpgradeService::startUpgrade(const std::string &path, PendingReply<> &&reply)
{
    // if state machine is running it means an upgrade is already in progress
    if (m_stateMachine.isRunning()) {
        reply.setError("Upgrade in progress");
        reply.finish();
        return;
    }


    // check we have the necessary characteristic proxes
    if (!m_packetCharacteristic || !m_controlCharacteristic) {
        reply.setError("Upgrade service not ready");
        reply.finish();
        return;
    }


    // reset the state (just in case it wasn't cleaned up correctly)
    m_fwFile.reset();

    m_fwFile = make_shared<FwImageFile>(path);

    if (!m_fwFile || !m_fwFile->isValid()) {
        reply.setError("Invalid file descriptor");
        reply.finish();
        return;
    }

    // set the initial progress
    m_progress = 0;

    // finally start the state machine
    m_stateMachine.start();

    // create a promise to signal success of failure once we get to a certain
    // place in the state-machine
    m_startPromise = make_shared< PendingReply<> >(std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    Attempts to stop / cancel a running firmware upgrade.

    This event may return without the upgrading being stopped, instead obvserve
    the finished() signal.

 */
void GattUpgradeService::cancelUpgrade(PendingReply<> &&reply)
{
    // if state machine is not running then no need to do anything
    if (!m_stateMachine.isRunning()) {
        reply.setError("Upgrade not running");
        reply.finish();
        return;
    }

    // post a cancel to the running state machine
    m_stateMachine.postEvent(CancelledEvent);

    // we've requested to cancel the upgrade so this is success
    reply.finish();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if we're currently in the process of updating the firmware
    image.

 */
bool GattUpgradeService::upgrading() const
{
    return m_stateMachine.isRunning();
}

// -----------------------------------------------------------------------------
/*!
    Returns the current progress of an upgrade as a percentage value from 0 to
    100.  If an upgrade is not in progress then -1 is returned.

 */
int GattUpgradeService::progress() const
{
    return m_progress;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to a new state or super state.
 */
void GattUpgradeService::onStateEntry(int state)
{
    switch (state) {
        case InitialState:
            onEnteredInitialState();
            break;
        case SendingWriteRequestState:
            onEnteredSendWriteRequestState();
            break;
        case SendingDataState:
            onEnteredSendingDataState();
            break;

        case ErroredState:
            onEnteredErroredState();
            break;
        case FinishedState:
            onEnteredFinishedState();
            break;

        default:
            break;
    }
}


// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattUpgradeService::onEnteredInitialState()
{
    // clear the setup flags
    m_setupFlags = 0;

    // all the following operations are async, they will set a flag in m_setupFlags
    // once complete or in case of an error they will post error events to the
    // state machine object


    // enable notifications from the packet characteristic
    enablePacketNotifications();

    // read the control point, used to verify the f/w image is indeed for the target RCU device
    readControlPoint();

    // check if a window size descriptor exists, if so try and read it's value
    if (!m_windowSizeDescriptor) {

        // no characteristic so just use the default value and pretend we
        // actually read the value and go with the default
        m_windowSize = 5;
        m_setupFlags |= ReadWindowSize;

    } else {

        // read the packet size descriptor
        readWindowSize();
    }

    // emit the started signal
    m_upgradingChangedSlots.invoke(true);
    m_progressChangedSlots.invoke(m_progress);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Request notifications to be enabled on the f/w packet characteristic. This
    is potentially an async operation; on success the EnabledNotifications flag
    will be set, on failure an EnableNotifyErrorEvent will be posted to
    the state-machine.

 */
void GattUpgradeService::enablePacketNotifications()
{
    if (!m_packetCharacteristic) {
        return;
    }


    auto replyHandler = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {

                XLOGD_ERROR("failed to enable OTA packet notifications due to <%s>", 
                        reply->errorMessage().c_str());
                
                m_lastError = reply->errorMessage();

                if (m_stateMachine.isRunning()) {
                    m_stateMachine.postEvent(EnableNotifyErrorEvent);
                }

            } else {
                // it's all good so update the flags
                setSetupFlag(EnabledNotifications);
            }
        };


    // connect to the notification signal for when the remote device
    // is sending ACK or ERROR packets back to us
    m_packetCharacteristic->enableNotifications(
            Slot<const std::vector<uint8_t> &>(m_isAlive,
                std::bind(&GattUpgradeService::onPacketNotification, this, std::placeholders::_1)), 
            PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Requests a read on the control pointer characteristic.  This is an async
    operation, when it completes it will set the VerifiedDeviceModel bit in
    the m_setupFlags field. On failure it will post a

 */
void GattUpgradeService::readControlPoint()
{
    if (!m_controlCharacteristic) {
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                
                XLOGD_ERROR("failed to read control point characteristic due to <%s>", 
                        reply->errorMessage().c_str());

                m_lastError = reply->errorMessage();

                if (m_stateMachine.isRunning()) {
                    m_stateMachine.postEvent(ReadErrorEvent);
                }

            } else {

                std::vector<uint8_t> value;
                value = reply->result();

                // check the value size and actual value
                if (value.size() != sizeof(ControlPoint)) {
                    
                    XLOGD_ERROR("invalid length of OTA control point");
                    m_lastError = "Invalid data length in OTA Control Point characteristic";

                    if (m_stateMachine.isRunning()) {
                        m_stateMachine.postEvent(ReadErrorEvent);
                    }

                } else {

                    ControlPoint ctrlPoint;
                    memcpy(&ctrlPoint, value.data(), sizeof(ControlPoint));

                    XLOGD_INFO("OTA control point data deviceModelId = 0x%X, firmwareVersion = 0x%X, firmwareCrc32 = 0x%X", 
                        ctrlPoint.deviceModelId, ctrlPoint.firmwareVersion, ctrlPoint.firmwareCrc32);

                    // it's all good so update the flags
                    setSetupFlag(VerifiedDeviceModel);
                }
            }
        };



    // perform the read request
    m_controlCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattUpgradeService::readWindowSize()
{
    if (!m_windowSizeDescriptor) {
        return;
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            if (reply->isError()) {
                
                XLOGD_ERROR("failed to read window size descriptor due to <%s>", 
                        reply->errorMessage().c_str());

                m_lastError = reply->errorMessage();

                if (m_stateMachine.isRunning()) {
                    m_stateMachine.postEvent(ReadErrorEvent);
                }

            } else {

                std::vector<uint8_t> value;
                value = reply->result();

                // check the value size and actual value
                if (value.size() != 1) {
                    
                    XLOGD_ERROR("invalid length of window size data");
                    m_lastError = "Invalid data length in OTA Packet Window Size descriptor";

                } else {

                    m_windowSize = value[0];
                    if (m_windowSize <= 0) {
                        XLOGD_ERROR("invalid window size value");
                        m_lastError = "Invalid OTA Packet Window Size descriptor value";

                    } else {
                        XLOGD_INFO("read window size of %u packets", m_windowSize);
                        setSetupFlag(ReadWindowSize);
                        return;
                    }
                }

                if (m_stateMachine.isRunning())
                    m_stateMachine.postEvent(ReadErrorEvent);
                }
        };


    // perform the read request
    m_windowSizeDescriptor->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when one of the setup phases is complete, when all flags are set then
     we signal the state machine that we're ready to go.

 */
void GattUpgradeService::setSetupFlag(SetupFlag flag)
{
    if (m_setupFlags & flag) {
        XLOGD_WARN("setup flag already set?");
    }

    m_setupFlags |= flag;

    if ( (m_setupFlags & EnabledNotifications) &&
         (m_setupFlags & ReadWindowSize) &&
         (m_setupFlags & VerifiedDeviceModel)) {

        m_stateMachine.postEvent(FinishedSetupEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the state were we send the first write request.

 */
void GattUpgradeService::onEnteredSendWriteRequestState()
{
    // reset the last ACK'ed block
    m_lastAckBlockId = -1;

    // send the initial write request
    sendWRQ();

    // reset the timeout counter
    m_timeoutCounter = 0;

    // write request has been sent, start the timeout timer as we expect an ACK back pretty snappy
    // (6000ms is chosen because it's slightly longer than the 5s slave latency)
    if (m_timeoutTimer > 0) {
        g_source_remove(m_timeoutTimer);
    }
    m_timeoutTimer = g_timeout_add(6000, timerEvent, this);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the state were we send the first data packet. This is
    where we complete the start promise as we're finally up and transferring
    data to the RCU.

 */
void GattUpgradeService::onEnteredSendingDataState()
{
    if (!m_startPromise) {
        XLOGD_ERROR("start promise already completed?");
        return;
    }

    m_startPromise->finish();
    m_startPromise.reset();
}


static gboolean disablePacketNotificationsMainLoop(gpointer user_data)
{
    GattUpgradeService *service = (GattUpgradeService*)user_data;    
    if (service == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
        return false;
    }
    service->disablePacketNotifications();

    return false;
}

void GattUpgradeService::disablePacketNotifications()
{
    if (m_packetCharacteristic) {
        m_packetCharacteristic->disableNotifications();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the errored state, this is just a transitional state
    used to emit an error() signal before moving on to the Finished state

 */
void GattUpgradeService::onEnteredErroredState()
{
    if (m_lastError.empty()) {
        m_lastError = "Unknown error";
    }

    // if there is still an outstanding promise it means we never completed
    // the initial phase, so release the promise now with an error
    if (m_startPromise) {
        m_startPromise->setError(m_lastError);
        m_startPromise->finish();
        m_startPromise.reset();

    } else {
        // no start promise so failed during data transfer, for this we should
        // emit an error signal
        m_errorChangedSlots.invoke(m_lastError);
    }

    m_lastError.clear();

    m_stateMachine.postEvent(CompleteEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the Finished state, here we just clean up any resources
    that may be left open.

 */
void GattUpgradeService::onEnteredFinishedState()
{
    // stop the timeout timer if running
    if (m_timeoutTimer > 0) {
        g_source_remove(m_timeoutTimer);
        m_timeoutTimer = 0;
    }

    // get the number of blocks in the f/w file
    const int fwBlockCount = static_cast<int>((m_fwFile->size() + (FIRMWARE_PACKET_MTU - 1)) / FIRMWARE_PACKET_MTU);

    // Disable notifications from the packet characteristic (we don't care
    // about the result of the operation FIXME).
    // Post this event in the main event loop, because this function could be called from
    // BleGattNotifyPipe NotifyThread, and when notifications are disabled that BleGattNotifyPipe
    // gets destroyed.  And we can't try to join with the NotifyThread while
    // within the NotifyThread. 
    g_timeout_add(0, disablePacketNotificationsMainLoop, this);

    // it's possible we never completed the start promise as the user could
    // have cancelled, if this happens we complete with an error
    if (m_startPromise) {
        m_startPromise->setError("Upgrade cancelled");
        m_startPromise->finish();
        m_startPromise.reset();
    }

    // emit the upgrade complete signal if we managed to send some data to
    // the RCU ... this is a workaround for an issue with the UEI RCU where
    // they don't ack the final block of data and therefore we think the
    // transfer has failed but it probably succeeded
    if ((fwBlockCount > m_windowSize) && (m_lastAckBlockId >= (fwBlockCount - m_windowSize))) {
        m_upgradeCompleteSlots.invoke();
    }

    // emit the finished signal
    m_upgradingChangedSlots.invoke(false);

    // reset the progress and emit a progress changed signal
    m_progress = -1;
    // emit progressChanged(m_progress);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Requests a write of \a value to the packet characteristic.

 */
void GattUpgradeService::doPacketWrite(const vector<uint8_t> &value)
{
    if (!m_packetCharacteristic) {
        return;
    }

    // lambda called if an error occurs writing to the packet characteristic,
    // just use to log an error message, the timeout(s) will handle the retry
    auto replyHandler = [this](PendingReply<> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("failed to write to OTA packet char due to <%s>", 
                        reply->errorMessage().c_str());
            }
        };


    // send the write without response request
    m_packetCharacteristic->writeValueWithoutResponse(value, PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a WRQ packet to the remote BLE device over the packet gatt characteristic.

 */
void GattUpgradeService::sendWRQ()
{
    // construct the WRQ packet
    struct {
        uint8_t opCode;
        uint8_t reserved;
        uint32_t length;
        uint32_t version;
        uint32_t crc32;
    } __attribute__((packed)) writePacket;

    uint32_t fwVersion = m_fwFile->version();

    writePacket.opCode = OPCODE_WRQ;
    writePacket.reserved = 0x00;
    writePacket.length = m_fwFile->size();
    writePacket.version = fwVersion;
    writePacket.crc32 = m_fwFile->crc32();

	XLOGD_DEBUG("sending WRQ packet (length:0x%08x version:0x%08x crc32:0x%08x)",
	       writePacket.length, writePacket.version, writePacket.crc32);

    const char* buffer = reinterpret_cast<const char*>(&writePacket);
    vector<uint8_t> value(buffer, buffer + sizeof(writePacket));

    // send the WRQ packet
    doPacketWrite(value);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends the next window of firmware upgrade packets.

    If any packets were queued but not written to the pipe then they are
    discarded before new packets are pushed into the q

 */
void GattUpgradeService::sendDATA()
{
    int16_t blockId = static_cast<int16_t>(m_lastAckBlockId + 1);

    // (re)seek to the right block
    if (m_fwFile == nullptr || !m_fwFile->seek((blockId - 1) * FIRMWARE_PACKET_MTU)) {
        XLOGD_WARN("failed to seek to location of block %hu", blockId);

        m_lastError = "Failed seeking to correct place in firmware file";
        m_stateMachine.postEvent(WriteErrorEvent);
        return;
    }

    // buffer for storing DATA packets
    struct {
        uint8_t header[2];
        uint8_t body[18];
    } __attribute__((packed)) packet;

    // fire off a bunch of DATA packets for the next window
    for (int i = 0; i < m_windowSize; i++) {

        // read up to 18 bytes of data
        if (m_fwFile == nullptr) {
            XLOGD_WARN("firmware file closed, was the upgrade cancelled?");

            m_lastError = "unable to read firmware file, its been closed";
            m_stateMachine.postEvent(WriteErrorEvent);
            return;
        }

        int64_t rd = m_fwFile->readFile(packet.body, FIRMWARE_PACKET_MTU);
        if ((rd != FIRMWARE_PACKET_MTU) && !m_fwFile->atEnd()) {
            XLOGD_WARN("read too few bytes but not at end of file?");
        }

        // set the block id
        packet.header[0] = OPCODE_DATA | uint8_t((blockId >> 8) & 0x3f);
        packet.header[1] = uint8_t(blockId & 0xff);

        // send / queue the data packet
        const char* buffer = reinterpret_cast<const char*>(&packet);
        vector<uint8_t> value(buffer, buffer + static_cast<int>(2 + rd));

        // do the packet write
        doPacketWrite(value);

        // increment the block number
        blockId++;

        // break out if we've sent a packet smaller than 18 bytes
        if (rd < FIRMWARE_PACKET_MTU) {
            break;
        }
    }
}

static gboolean timerEvent(gpointer user_data)
{
    GattUpgradeService *us = (GattUpgradeService*)user_data;
    if (us) {
        return us->onTimeout();
    }
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the timeout timer expires, this is enable when either a WRQ or
    DATA packet is written and we expect a reply.

 */
bool GattUpgradeService::onTimeout()
{
    // sanity check the state machine is running
    if (!m_stateMachine.isRunning()) {
        return false;
    }

    XLOGD_INFO("f/w upgrade timed-out in state %d", m_stateMachine.state());

    // check if we've timed-out to many times
    if (m_timeoutCounter++ > 3) {
        XLOGD_WARN("timeout counter exceeded in state %d", m_stateMachine.state());

        // simply inject the timeout event into the state machine if running
        m_lastError = BLE_RCU_UPGRADE_SERVICE_ERR_TIMEOUT_STR;
        m_stateMachine.postEvent(TimeoutErrorEvent);
        return false;
    }

    // re-send the data based on current state
    if (m_stateMachine.inState(SendingWriteRequestState)) {

        // send the initial write request
        sendWRQ();

    } else if (m_stateMachine.inState(SendingDataState)) {

        // send the next window of DATA packets
        sendDATA();

    }

    // return true to restart the timer
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a notification packet from the PACKET characteristic.
    The packet will either contain an ACK or ERROR packet.

 */
void GattUpgradeService::onPacketNotification(const vector<uint8_t> &value)
{
    // every notification should be 2 bytes in size
    if (value.size() != 2) {
        XLOGD_WARN("length of notification packet is not 2 bytes (actual %d)", value.size());
        return;
    }

    XLOGD_DEBUG("received packet notification 0x %02X %02X", value[0], value[1]);

    // got a two byte packet, check the opcode (warning the following code
    // can trigger a state machine transition)
    switch (value[0] & OPCODE_MASK) {
        case OPCODE_ACK:
            onACKPacket(value);
            break;
        case OPCODE_ERROR:
            onERRORPacket(value);
            break;
        default:
            XLOGD_WARN("unexpected notification opcode 0x%02hhx", value[0] & OPCODE_MASK);
            break;
    }

}


// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an ACK packet has been received.

 */
void GattUpgradeService::onACKPacket(const vector<uint8_t> &data)
{
    // get the block id of the ack
    int blockId = (int16_t(data[0] & 0x3f) << 8)
                | (int16_t(data[1] & 0xff) << 0);

    XLOGD_DEBUG("received ACK %u", blockId);

    // if not in the sending super state ignore the ACK
    if (!m_stateMachine.isRunning() ||
        !m_stateMachine.inState(SendingSuperState)) {
        
        XLOGD_WARN("received ACK %u in wrong state", blockId);
        return;
    }

    // reset the timeout counter
    m_timeoutCounter = 0;

    const int fwDataSize = static_cast<int>(m_fwFile->size());

    // check if the ACK is for the last block
    if ((blockId * FIRMWARE_PACKET_MTU) > fwDataSize) {

        // stop the timeout
        if (m_timeoutTimer > 0) {
            g_source_remove(m_timeoutTimer);
            m_timeoutTimer = 0;
        }

        // set progress at 100% and emit the final progress change
        m_progress = 100;
        m_progressChangedSlots.invoke(m_progress);

        // emit an upgrade complete signal (used to notify other services,
        // notably device info, that an upgrade has taken place)
        m_upgradeCompleteSlots.invoke();

        m_stateMachine.postEvent(CompleteEvent);

    } else if (blockId > m_lastAckBlockId) {

        // update the confirmation of the last block ACKed
        m_lastAckBlockId = blockId;

        // if this is the first ack then post a message to the state machine
        // so we move into the sending data state
        if (blockId == 0)
            m_stateMachine.postEvent(PacketAckEvent);

        // emit a signal for the progress update
        int progress = (blockId * FIRMWARE_PACKET_MTU * 100) / fwDataSize;
        if (progress != m_progress) {
            m_progress = progress;
            m_progressChangedSlots.invoke(progress);
        }

        // send the next window of packets
        sendDATA();

        // This function gets called asynchronously from a thread that monitors notifications from bluez.
        // So this can occur after the operation was cancelled and the state machine finished.  
        // Only restart timer if state machine is running.
        // TODO: if we get too many errors of 'GLib-CRITICAL **: Source ID <id> was not found', which
        // are largely benign, we could use delayed state machine events for this timeout timer instead.
        if (m_stateMachine.isRunning()) {
            // (re)start the timeout timer
            if (m_timeoutTimer > 0) {
                g_source_remove(m_timeoutTimer);
            }
            m_timeoutTimer = g_timeout_add(6000, timerEvent, this);
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattUpgradeService::onERRORPacket(const vector<uint8_t> &data)
{
    // sanity check the bits that should be zero are zero
    if (data[0] != OPCODE_ERROR) {
        XLOGD_WARN("malformed error packet");
    }

    XLOGD_WARN("received ERROR 0x%02hhx", data[1]);

    // if not in the sending super state ignore the ERROR
    if (!m_stateMachine.isRunning() ||
        !m_stateMachine.inState(SendingSuperState)) {
    
        return;
    }

    // set the error string based on the code
    switch (data[1]) {
        case 0x01:
            m_lastError = "CRC mismatch error from RCU";
            break;
        case 0x02:
            m_lastError = "Invalid size error from RCU";
            break;
        case 0x03:
            m_lastError = "Size mismatch error from RCU";
            break;
        case 0x04:
            m_lastError = "Battery too low";
            break;
        case 0x05:
            m_lastError = "Invalid opcode error from RCU";
            break;
        case 0x06:
            m_lastError = "Internal error from RCU";
            break;
        case 0x07:
            m_lastError = "Invalid hash error from RCU";
            break;
        default:
            m_lastError = "Received unknown error from RCU";
            break;
    }

    m_stateMachine.postEvent(PacketErrorEvent);
}
