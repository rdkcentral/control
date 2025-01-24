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
//  gatt_batteryservice.cpp
//

#include "gatt_batteryservice.h"
#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"
#include "ctrlm_log_ble.h"

#include <algorithm>

using namespace std;


const BleUuid GattBatteryService::m_serviceUuid(BleUuid::BatteryService);
const BleUuid GattBatteryService::m_batteryLevelCharUuid(BleUuid::BatteryLevel);


GattBatteryService::GattBatteryService(GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_batteryLevel(-1)
{
    // create the basic statemachine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattBatteryService::~GattBatteryService()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.

 */
BleUuid GattBatteryService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Vendor daemons send 255 for when the battery level is unknown / invalid,
    whereas we use -1 for that case, this function handles the conversion.
 */
unsigned int GattBatteryService::sanitiseBatteryLevel(uint8_t level) const
{
    return std::max((uint8_t)0, std::min(level, (uint8_t)100));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattBatteryService::init()
{
    m_stateMachine.setObjectName(string("GattBatteryService"));

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, string("Idle"));
    m_stateMachine.addState(StartNotifyState, string("StartNotify"));

    m_stateMachine.addState(RunningSuperState, string("RunningSuperState"));
    m_stateMachine.addState(RunningSuperState, RunningState, string("Running"));
    m_stateMachine.addState(RunningSuperState, RereadBatteryLevelState, string("RereadBatteryLevel"));


    // add the transitions:      From State     ->  Event                   ->  To State
    m_stateMachine.addTransition(IdleState,         StartServiceRequestEvent,   StartNotifyState);

    m_stateMachine.addTransition(StartNotifyState,  RetryStartNotifyEvent,      StartNotifyState);
    m_stateMachine.addTransition(StartNotifyState,  StopServiceRequestEvent,    IdleState);
    m_stateMachine.addTransition(StartNotifyState,  StartedNotifingEvent,       RunningState);

    m_stateMachine.addTransition(RunningSuperState, StopServiceRequestEvent,    IdleState);
    m_stateMachine.addTransition(RunningSuperState, RereadBatteryLevelEvent,    RereadBatteryLevelState);


    // connect to the state entry signal
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattBatteryService::onEnteredState, this, std::placeholders::_1)));
            

    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    \reimp

    Starts the service by creating a dbus proxy connection to the gatt
    service exposed by the bluez daemon.

    \a connection is the dbus connection to use to connected to dbus and
    the \a gattService contains the info on the dbus paths to the service
    and it's child characteristics and descriptors.

 */
bool GattBatteryService::start(const shared_ptr<const BleGattService> &gattService)
{

    // create the bluez dbus proxy to the characteristic
    if (!m_battLevelCharacteristic || !m_battLevelCharacteristic->isValid()) {

        // sanity check the supplied info is valid
        if (!gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
            XLOGD_WARN("invalid battery gatt service info");
            return false;
        }

        // get the chararacteristic for the actual battery level
        m_battLevelCharacteristic = gattService->characteristic(m_batteryLevelCharUuid);
        if (!m_battLevelCharacteristic || !m_battLevelCharacteristic->isValid()) {
            XLOGD_WARN("failed to get battery level characteristic");
            return false;
        }
    }

    // check we're not already started
    if (m_stateMachine.state() != IdleState) {
        XLOGD_WARN("battery service already started");
        return true;
    }

    m_stateMachine.postEvent(StartServiceRequestEvent);
    return true;
}

// -----------------------------------------------------------------------------
/*!

 */
void GattBatteryService::stop()
{
    m_stateMachine.cancelDelayedEvents(RereadBatteryLevelEvent);
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattBatteryService::onEnteredState(int state)
{
    if (state == IdleState) {
        if (m_battLevelCharacteristic) {
            XLOGD_INFO("Disabling notifications for m_battLevelCharacteristic");
            m_battLevelCharacteristic->disableNotifications();
        }
        m_battLevelCharacteristic.reset();

    } else if (state == StartNotifyState) {
        requestStartNotify();

    } else if (state == RunningState) {
        requestBatteryLevel();

        // entered running state so emit the ready signal
        m_readySlots.invoke();

    } else if (state == RereadBatteryLevelState) {
        XLOGD_INFO("Previous battery level read failed, rereading now...");
        requestBatteryLevel();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.StartNotify() to enable
    notifications for changes to the characteristic value.

 */
void GattBatteryService::requestStartNotify()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                // this is bad if this happens as we won't get updates to the battery level, 
                // so we install a timer to retry enabling notifications in a couple of seconds time
                XLOGD_ERROR("failed to enable battery level notifications due to <%s>", reply->errorMessage().c_str());

                m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
            } else {
                // notifications enabled so post an event to the state machine
                m_stateMachine.postEvent(StartedNotifingEvent);
            }
        };

    m_battLevelCharacteristic->enableNotifications(
            Slot<const std::vector<uint8_t> &>(m_isAlive,
                std::bind(&GattBatteryService::onBatteryLevelChanged, this, std::placeholders::_1)), 
            PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the current battery level.

 */
void GattBatteryService::requestBatteryLevel()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to get battery level due to <%s>", reply->errorMessage().c_str());
                m_stateMachine.postDelayedEvent(RereadBatteryLevelEvent, 60 * 1000);
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    XLOGD_INFO("successfully read battery level = %u%%", value[0]);
                    onBatteryLevelChanged(value);
                }
            }
        };

    m_battLevelCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}
// -----------------------------------------------------------------------------
/*!
    \internal

    Internal slot called when a notification from the remote device is sent
    due to a battery level change.
 */
void GattBatteryService::onBatteryLevelChanged(const std::vector<uint8_t> &newValue)
{
    // sanity check the data received
    if (newValue.size() != 1) {
        XLOGD_ERROR("battery value received has invalid length (%d bytes)", newValue.size());
        return;
    }

    XLOGD_INFO("battery level changed to %u%%", newValue[0]);

    // clamp the reply between 0 and 100
    unsigned int level = sanitiseBatteryLevel(newValue[0]);

    // store the battery level and emit a signal if it's changed
    if (level != m_batteryLevel) {
        m_batteryLevel = level;
        m_levelChangedSlots.invoke(m_batteryLevel);
    }
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
bool GattBatteryService::isReady() const
{
    return (m_stateMachine.state() == RunningState);
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
int GattBatteryService::level() const
{
    return m_batteryLevel;
}
