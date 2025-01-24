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
//  gatt_remotecontrolservice.cpp
//

#include "gatt_remotecontrolservice.h"
#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"
#include "blercu/blercuerror.h"

#include "ctrlm_log_ble.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_ble.h"
#include "ctrlm_utils.h"
#include "ble/ctrlm_ble_utils.h"


const BleUuid GattRemoteControlService::m_serviceUuid(BleUuid::RdkRemoteControl);
const BleUuid GattRemoteControlService::m_unpairReasonCharUuid(BleUuid::UnpairReason);
const BleUuid GattRemoteControlService::m_rebootReasonCharUuid(BleUuid::RebootReason);
const BleUuid GattRemoteControlService::m_rcuActionCharUuid(BleUuid::RcuAction);
const BleUuid GattRemoteControlService::m_lastKeypressCharUuid(BleUuid::LastKeypress);
const BleUuid GattRemoteControlService::m_advConfigCharUuid(BleUuid::AdvertisingConfig);
const BleUuid GattRemoteControlService::m_advConfigCustomListCharUuid(BleUuid::AdvertisingConfigCustomList);
const BleUuid GattRemoteControlService::m_assertReportCharUuid(BleUuid::AssertReport);

using namespace std;


GattRemoteControlService::GattRemoteControlService(GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_unpairReason(0xFF)
    , m_rebootReason(0xFF)
    , m_rcuAction(0xFF)
    , m_lastKeypress(0xFF)
    , m_advConfig(0xFF)
    , m_advConfig_toWrite(0xFF)
{
    // create the basic statemachine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattRemoteControlService::~GattRemoteControlService()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.

 */
BleUuid GattRemoteControlService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattRemoteControlService::init()
{
    m_stateMachine.setObjectName("GattRemoteControlService");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(RunningState, "Running");
    m_stateMachine.addState(RunningState, RetrieveInitialValuesState, "RetrieveInitialValuesState");
    m_stateMachine.addState(RunningState, EnableNotificationsState,   "EnableNotificationsState");

    // add the transitions:      From State                  -> Event                     ->  To State
    m_stateMachine.addTransition(IdleState,                  StartServiceRequestEvent,    RetrieveInitialValuesState);

    m_stateMachine.addTransition(RetrieveInitialValuesState, InitialValuesRetrievedEvent, EnableNotificationsState);
    m_stateMachine.addTransition(EnableNotificationsState,   RetryStartNotifyEvent,       EnableNotificationsState);


    m_stateMachine.addTransition(RunningState,               StopServiceRequestEvent,    IdleState);


    // connect to the state entry signal
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattRemoteControlService::onEnteredState, this, std::placeholders::_1)));


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
bool GattRemoteControlService::start(const shared_ptr<const BleGattService> &gattService)
{
    // sanity check the supplied info is valid
    if (!gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
        XLOGD_WARN("invalid remote control gatt service info");
        return false;
    }
    // create the bluez dbus proxy to the characteristics
    if (!m_lastKeypressCharacteristic || !m_lastKeypressCharacteristic->isValid()) {
        m_lastKeypressCharacteristic = gattService->characteristic(m_lastKeypressCharUuid);
        if (!m_lastKeypressCharacteristic || !m_lastKeypressCharacteristic->isValid()) {
            XLOGD_WARN("Failed to get last keypress characteristic, check that remote firmware supports this feature.  Continuing anyway...");
        }
    }

    if (!m_advConfigCharacteristic || !m_advConfigCharacteristic->isValid()) {
        m_advConfigCharacteristic = gattService->characteristic(m_advConfigCharUuid);
        if (!m_advConfigCharacteristic || !m_advConfigCharacteristic->isValid()) {
            XLOGD_WARN("Failed to get advertising config characteristic, check that remote firmware supports this feature.  Continuing anyway...");
        }
    }
    if (!m_advConfigCustomListCharacteristic || !m_advConfigCustomListCharacteristic->isValid()) {
        m_advConfigCustomListCharacteristic = gattService->characteristic(m_advConfigCustomListCharUuid);
        if (!m_advConfigCustomListCharacteristic || !m_advConfigCustomListCharacteristic->isValid()) {
            XLOGD_WARN("Failed to get advertising config custom list characteristic, check that remote firmware supports this feature.  Continuing anyway...");
        }
    }
    if (!m_unpairReasonCharacteristic || !m_unpairReasonCharacteristic->isValid()) {
        m_unpairReasonCharacteristic = gattService->characteristic(m_unpairReasonCharUuid);
        if (!m_unpairReasonCharacteristic || !m_unpairReasonCharacteristic->isValid()) {
            XLOGD_WARN("failed to get unpair reason characteristic");
            return false;
        }
    }
    if (!m_rebootReasonCharacteristic || !m_rebootReasonCharacteristic->isValid()) {
        m_rebootReasonCharacteristic = gattService->characteristic(m_rebootReasonCharUuid);
        if (!m_rebootReasonCharacteristic || !m_rebootReasonCharacteristic->isValid()) {
            XLOGD_WARN("failed to get reboot reason characteristic");
            return false;
        }
    }
    if (!m_rcuActionCharacteristic || !m_rcuActionCharacteristic->isValid()) {
        m_rcuActionCharacteristic = gattService->characteristic(m_rcuActionCharUuid);
        if (!m_rcuActionCharacteristic || !m_rcuActionCharacteristic->isValid()) {
            XLOGD_WARN("failed to get RCU action characteristic");
            return false;
        }
    }
    if (!m_assertReportCharacteristic || !m_assertReportCharacteristic->isValid()) {
        m_assertReportCharacteristic = gattService->characteristic(m_assertReportCharUuid);
        if (!m_assertReportCharacteristic || !m_assertReportCharacteristic->isValid()) {
            XLOGD_WARN("failed to get optional Assert Reporting characteristic, continuing anyway...");
        }
    }

    // check we're not already started
    if (m_stateMachine.state() != IdleState) {
        XLOGD_WARN("remote control service already started");
        return true;
    }

    m_stateMachine.postEvent(StartServiceRequestEvent);
    return true;
}

// -----------------------------------------------------------------------------
/*!

 */
void GattRemoteControlService::stop()
{
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
void GattRemoteControlService::onEnteredState(int state)
{
    if (state == IdleState) {
        m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);

        if (m_unpairReasonCharacteristic) {
            XLOGD_INFO("Disabling notifications for m_unpairReasonCharacteristic");
            m_unpairReasonCharacteristic->disableNotifications();
        }
        if (m_rebootReasonCharacteristic) {
            XLOGD_INFO("Disabling notifications for m_rebootReasonCharacteristic");
            m_rebootReasonCharacteristic->disableNotifications();
        }

        m_lastKeypressCharacteristic.reset();
        m_advConfigCharacteristic.reset();
        m_advConfigCustomListCharacteristic.reset();
        m_unpairReasonCharacteristic.reset();
        m_rebootReasonCharacteristic.reset();
        m_rcuActionCharacteristic.reset();
        m_assertReportCharacteristic.reset();


    } else if (state == RetrieveInitialValuesState) {
        
        requestLastKeypress();  // request this value first so that it gets evented up as soon as possible
        requestAdvConfig();
        requestAdvConfigCustomList();
        requestUnpairReason();
        requestRebootReason();

        m_readySlots.invoke();

    } else if (state == EnableNotificationsState) {
        requestStartUnpairNotify();
        requestStartRebootNotify();
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.StartNotify() to enable
    notifications for changes to the characteristic value.

 */
void GattRemoteControlService::requestStartUnpairNotify()
{
    // lambda called if an error occurs enabling the notifications
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {
                // this is bad if this happens as we won't get updates, so we install a timer to 
                // retry enabling notifications in a couple of seconds time
                XLOGD_ERROR("failed to enable unpair reason characteristic notifications due to <%s>", 
                        reply->errorMessage().c_str());

                m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
                m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
            } else {
                XLOGD_INFO("request to start notifications on Unpair characteristic succeeded");
            }
        };

    m_unpairReasonCharacteristic->enableNotifications(
            Slot<const std::vector<uint8_t> &>(m_isAlive,
                std::bind(&GattRemoteControlService::onUnpairReasonChanged, this, std::placeholders::_1)), 
            PendingReply<>(m_isAlive, replyHandler));
}

void GattRemoteControlService::requestStartRebootNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
        {
            // check for errors
            if (reply->isError()) {
                // this is bad if this happens as we won't get updates, so we install a timer to 
                // retry enabling notifications in a couple of seconds time
                XLOGD_ERROR("failed to enable reboot reason characteristic notifications due to <%s>", 
                        reply->errorMessage().c_str());

                m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
                m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
            } else {
                XLOGD_INFO("request to start notifications on Reboot Reason characteristic succeeded");
            }
        };

    m_rebootReasonCharacteristic->enableNotifications(
            Slot<const std::vector<uint8_t> &>(m_isAlive,
                std::bind(&GattRemoteControlService::onRebootReasonChanged, this, std::placeholders::_1)), 
            PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    Write RCU Action characteristic.

    This method will fail and return \c false if the service is not started or
    it is already signalling, i.e. should only be called it \a state() returns
    \a RemoteControlService::Ready.

 */
void GattRemoteControlService::sendRcuAction(uint8_t action, PendingReply<> &&reply)
{
    // check the service is ready
    if (!isReady()) {
        reply.setError("Service is not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_promiseResults) {
        reply.setError("Request already in progress");
        reply.finish();
    }


    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply_)
        {
            // check for errors
            if (reply_->isError()) {

                XLOGD_ERROR("failed to send RCU action due to <%s>", reply_->errorMessage().c_str());

                // signal the client that we failed
                if (m_promiseResults) {
                    m_promiseResults->setError(reply_->errorMessage());
                    m_promiseResults->finish();
                    m_promiseResults.reset();
                }
            } else {

                // there should be a valid pending results object, if not something has gone wrong
                if (!m_promiseResults) {
                    XLOGD_ERROR("received a dbus reply message with no matching pending operation");
                    return;
                }

                XLOGD_DEBUG("RCU action written successfully");

                // non-error so complete the pending operation and post a message to the caller
                m_promiseResults->finish();
                m_promiseResults.reset();
            }
        };


    // set the new level
    m_rcuAction = action;
    const vector<uint8_t> value(1, m_rcuAction);
    XLOGD_WARN("sending RCU Action = %d (%s)", m_rcuAction, 
            ctrlm_ble_rcu_action_str((ctrlm_ble_RcuAction_t)m_rcuAction));

    m_promiseResults = make_shared<PendingReply<>>(std::move(reply));

    // send a request to the vendor daemon write the characteristic, and connect the reply to a listener
    m_rcuActionCharacteristic->writeValue(value, PendingReply<>(m_isAlive, replyHandler));
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the unpair reason.

 */
void GattRemoteControlService::requestUnpairReason()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to get unpair reason due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    m_unpairReason = value[0];
                    XLOGD_INFO("Initial unpair reason is %u (%s)", m_unpairReason, 
                            ctrlm_ble_unpair_reason_str((ctrlm_ble_RcuUnpairReason_t)m_unpairReason));
                } else {
                    XLOGD_ERROR("Unpair reason received has invalid length (%d bytes)", value.size());
                }
            }
        };


    // send a request to the bluez daemon to read the characteristic
    m_unpairReasonCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the reboot reason.

 */
void GattRemoteControlService::requestRebootReason()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to get reboot reason due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    m_rebootReason = value[0];
                    XLOGD_INFO("Initial reboot reason is %u (%s)", m_rebootReason, 
                            ctrlm_ble_reboot_reason_str((ctrlm_ble_RcuRebootReason_t)m_rebootReason));

                    if (m_rebootReason == CTRLM_BLE_RCU_REBOOT_REASON_ASSERT) {
                        // Reboot reason is "assert", so read the assert report characteristic
                        requestAssertReport();
                    }
                } else {
                    XLOGD_ERROR("Reboot reason received has invalid length (%d bytes)", value.size());
                }
            }
            m_stateMachine.postEvent(InitialValuesRetrievedEvent);
        };


    // send a request to the bluez daemon to read the characteristic
    m_rebootReasonCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the assert data.

 */
void GattRemoteControlService::requestAssertReport()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("failed to get RCU assert report due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == CTRLM_RCU_ASSERT_REPORT_MAX_SIZE) {
                    string assertStr(value.begin(), value.end()); 
                    XLOGD_INFO("Initial RCU assert report is <%s>", assertStr.c_str());
                } else {
                    XLOGD_ERROR("RCU assert report has invalid length (%d bytes)", value.size());
                }
            }
        };


    if (m_assertReportCharacteristic) {
        // send a request to the bluez daemon to read the characteristic
        m_assertReportCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the last key press.

 */
void GattRemoteControlService::requestLastKeypress()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("Failed to read last key press due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    m_lastKeypress = value[0];
                    XLOGD_INFO("Successfully read last key press characteristic, value = <0x%X>, emitting signal...", m_lastKeypress);
                    m_lastKeypressChangedSlots.invoke(m_lastKeypress);
                } else {
                    XLOGD_ERROR("Last key press received has invalid length (%d bytes)", value.size());
                }
            }
        };


    if (m_lastKeypressCharacteristic && m_lastKeypressCharacteristic->isValid()) {
        
        // send a request to the bluez daemon to read the characteristic
        m_lastKeypressCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));

    } else {
        XLOGD_ERROR("Last key press characteristic is not valid, check that the remote firmware version supports this feature.");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the advertising config.

 */
void GattRemoteControlService::requestAdvConfig()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("Failed to read advertising config due to <%s>", reply->errorMessage().c_str());
            } else {
                std::vector<uint8_t> value;
                value = reply->result();
                
                if (value.size() == 1) {
                    m_advConfig = value[0];
                    XLOGD_INFO("Successfully read advertising config characteristic, value = <0x%X>, emitting signal...", m_advConfig);
                    m_advConfigChangedSlots.invoke(m_advConfig);
                } else {
                    XLOGD_ERROR("Advertising config received has invalid length (%d bytes)", value.size());
                }
            }
        };


    if (m_advConfigCharacteristic && m_advConfigCharacteristic->isValid()) {
        
        // send a request to the bluez daemon to read the characteristic
        m_advConfigCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));

    } else {
        XLOGD_ERROR("Advertising config characteristic is not valid, check that the remote firmware version supports this feature.");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a request to org.bluez.GattCharacteristic1.Value() to get the value
    propery of the characteristic which contains the advertising config.

 */
void GattRemoteControlService::requestAdvConfigCustomList()
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("Failed to read custom advertising config due to <%s>", reply->errorMessage().c_str());
            } else {
                m_advConfigCustomList = reply->result();
                XLOGD_INFO("Successfully read advertising config custom list characteristic");
                
                m_advConfigCustomListChangedSlots.invoke(m_advConfigCustomList);
            }
        };


    if (m_advConfigCustomListCharacteristic && m_advConfigCustomListCharacteristic->isValid()) {
        
        // send a request to the bluez daemon to read the characteristic
        m_advConfigCustomListCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));

    } else {
        XLOGD_ERROR("Advertising config custom list characteristic is not valid, check that the remote firmware version supports this feature.");
    }
}



// -----------------------------------------------------------------------------
/*!
    This method will fail and return \c false if the service is not started or
    it is already signalling, i.e. should only be called it \a state() returns
    \a RemoteControlService::Ready.
 */
void GattRemoteControlService::writeAdvertisingConfig(uint8_t config, const vector<uint8_t> &customList, PendingReply<> &&reply)
{
    // check the service is ready
    if (!isReady()) {
        reply.setError("Service is not ready");
        reply.finish();
        return;
    }

    // check we don't already have an outstanding pending call
    if (m_promiseResults) {
        reply.setError("Request already in progress");
        reply.finish();
        return;
    }

    if (!m_advConfigCharacteristic || !m_advConfigCharacteristic->isValid() ||
            !m_advConfigCustomListCharacteristic || !m_advConfigCustomListCharacteristic->isValid()) {
        reply.setError("Advertising config characteristic is not valid, check that the remote firmware version supports this feature.");
        reply.finish();

        XLOGD_ERROR("m_advConfigCharacteristic is not valid");
        return;
    }

    m_advConfig_toWrite = config;

    // create a new pending result object to notify the caller when the request is complete
    m_promiseResults = make_shared<PendingReply<>>(std::move(reply));

    if (!customList.empty()) {

        // write the custom list first to avoid the following scenario:
        //   Write the config first, then something goes wrong and the custom list itself doesn't get set.
        //   Now the remote is using whatever was stored in custom list characteristic before

        XLOGD_INFO("Writing custom adv config list...");
        m_advConfigCustomListCharacteristic->writeValue(customList, PendingReply<>(m_isAlive, 
                std::bind(&GattRemoteControlService::onWriteAdvConfigCustomListReply, this, std::placeholders::_1)));

    } else {
        // custom list is empty, so just write the advertising config value
        const vector<uint8_t> value(1, m_advConfig_toWrite);
        XLOGD_INFO("Writing advertising config = %u (%s)", m_advConfig_toWrite, 
                ctrlm_rcu_wakeup_config_str((ctrlm_rcu_wakeup_config_t)m_advConfig_toWrite));

        m_advConfigCharacteristic->writeValue(value, PendingReply<>(m_isAlive, 
                std::bind(&GattRemoteControlService::onWriteAdvConfigReply, this, std::placeholders::_1)));
    }
}


void GattRemoteControlService::onWriteAdvConfigCustomListReply(PendingReply<> *reply)
{            
    // check for errors (only for logging)
    if (reply->isError()) {

        XLOGD_ERROR("failed to write custom config due to <%s>", reply->errorMessage().c_str());

        // signal the client that we failed
        if (m_promiseResults) {
            m_promiseResults->setError(reply->errorMessage());
            m_promiseResults->finish();
            m_promiseResults.reset();
        }

    } else {

        XLOGD_INFO("Custom config list written successfully");

        // there should be a valid pending results object, if not something has gone wrong
        if (!m_promiseResults) {
            XLOGD_ERROR("received a dbus reply message with no matching pending operation");
            return;
        }

        if (!m_advConfigCharacteristic || !m_advConfigCharacteristic->isValid()) {

            m_promiseResults->setError("Advertising config characteristic is not valid, check that the remote firmware version supports this feature.");
            m_promiseResults->finish();
            m_promiseResults.reset();

            XLOGD_ERROR("m_advConfigCharacteristic is not valid");
            return;
        }

            
        // now write the advertising config value
        const vector<uint8_t> value(1, m_advConfig_toWrite);
        XLOGD_INFO("sending advertising config = %u (%s)", m_advConfig_toWrite, 
                ctrlm_rcu_wakeup_config_str((ctrlm_rcu_wakeup_config_t)m_advConfig_toWrite));
    
        m_advConfigCharacteristic->writeValue(value, PendingReply<>(m_isAlive, 
                std::bind(&GattRemoteControlService::onWriteAdvConfigReply, this, std::placeholders::_1)));
    }
}

void GattRemoteControlService::onWriteAdvConfigReply(PendingReply<> *reply)
{
    // check for errors (only for logging)
    if (reply->isError()) {
        
        XLOGD_ERROR("Failed to write advertising config due to <%s>", reply->errorMessage().c_str());

        // signal the client that we failed
        if (m_promiseResults) {
            m_promiseResults->setError(reply->errorMessage());
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    } else {

        XLOGD_INFO("Advertising config written successfully");
        requestAdvConfig();
        requestAdvConfigCustomList();

        // there should be a valid pending results object, if not something has gone wrong
        if (!m_promiseResults) {
            XLOGD_ERROR("received a dbus reply message with no matching pending operation");
            return;
        }

        // non-error so complete the pending operation and post a message to the caller
        m_promiseResults->finish();
        m_promiseResults.reset();

    }
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Internal slot called when a notification from the remote device is sent
    due to unpair reason change.
 */
void GattRemoteControlService::onUnpairReasonChanged(const vector<uint8_t> &newValue)
{
    // sanity check the data received
    if (newValue.size() != 1) {
        XLOGD_ERROR("Unpair reason received has invalid length (%d bytes)", newValue.size());
    } else {
        m_unpairReason = newValue[0];
        XLOGD_INFO("unpair reason changed to %u (%s)", m_unpairReason, 
                ctrlm_ble_unpair_reason_str((ctrlm_ble_RcuUnpairReason_t)m_unpairReason));
        m_unpairReasonChangedSlots.invoke(m_unpairReason);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Internal slot called when a notification from the remote device is sent
    due to reboot reason change.
 */
void GattRemoteControlService::onRebootReasonChanged(const vector<uint8_t> &newValue)
{
    // sanity check the data received
    if (newValue.size() != 1) {
        XLOGD_ERROR("Reboot reason received has invalid length (%d bytes)", newValue.size());
    } else {
        m_rebootReason = newValue[0];
        XLOGD_INFO("reboot reason changed to %u (%s)", m_rebootReason, 
                ctrlm_ble_reboot_reason_str((ctrlm_ble_RcuRebootReason_t)m_rebootReason));

        if (m_rebootReason == CTRLM_BLE_RCU_REBOOT_REASON_ASSERT && m_assertReportCharacteristic) {

            // lambda invoked when the request returns
            auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
                {
                    string assertStr = "";
                    // check for errors
                    if (reply->isError()) {
                        XLOGD_ERROR("failed to get RCU assert report due to <%s>", reply->errorMessage().c_str());
                    } else {
                        std::vector<uint8_t> value;
                        value = reply->result();
                        
                        if (value.size() == CTRLM_RCU_ASSERT_REPORT_MAX_SIZE) {
                            string _assertStr(value.begin(), value.end());
                            assertStr = std::move(_assertStr);
                        } else {
                            XLOGD_ERROR("RCU assert report has invalid length (%d bytes)", value.size());
                        }
                    }
                    m_rebootReasonChangedSlots.invoke(m_rebootReason, std::move(assertStr));
                };

            // Reboot reason is "assert", so read the assert report characteristic
            m_assertReportCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));

        } else {

            m_rebootReasonChangedSlots.invoke(m_rebootReason, string());
        }
    }
}


// -----------------------------------------------------------------------------
/*!
    \overload

 */
bool GattRemoteControlService::isReady() const
{
    return (m_stateMachine.inState(RunningState));
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
uint8_t GattRemoteControlService::unpairReason() const
{
    return m_unpairReason;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
uint8_t GattRemoteControlService::rebootReason() const
{
    return m_rebootReason;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
uint8_t GattRemoteControlService::lastKeypress() const
{
    return m_lastKeypress;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
uint8_t GattRemoteControlService::advConfig() const
{
    return m_advConfig;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
vector<uint8_t> GattRemoteControlService::advConfigCustomList() const
{
    return m_advConfigCustomList;
}
