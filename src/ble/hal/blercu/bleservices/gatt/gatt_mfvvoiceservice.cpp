#include "gatt_mfvvoiceservice.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"

#include "ctrlm_log_ble.h"

using namespace std;

const BleUuid GattMfvVoiceService::m_serviceUuid(BleUuid::MfvVoice);
const BleUuid GattMfvVoiceService::m_sessionStartCharUuid(BleUuid::SessionStart);
const BleUuid GattMfvVoiceService::m_detectionDataCharUuid(BleUuid::DetectionData);
const BleUuid GattMfvVoiceService::m_modelVersionCharUuid(BleUuid::WakeWordModelVersion);
const BleUuid GattMfvVoiceService::m_privacyCharUuid(BleUuid::PrivacySettings);
const BleUuid GattMfvVoiceService::m_modelConfigCharUuid(BleUuid::ModelConfiguration);
const BleUuid GattMfvVoiceService::m_capabilitiesCharUuid(BleUuid::MfvCapabilities);


// -----------------------------------------------------------------------------
/*!
    Constructs the MFV Voice GATT service.
 */
GattMfvVoiceService::GattMfvVoiceService(GMainLoop *mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_detectionType(FullPower)
    , m_detectionData({ 0, 0, 0 })
    , m_modelVersion({ 0, 0 })
    , m_privacyEnabled(false)
    , m_capabilities(0)
    , m_initialReadsRemaining(0)
{
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattMfvVoiceService::~GattMfvVoiceService()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the constant gatt service uuid.
 */
BleUuid GattMfvVoiceService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattMfvVoiceService::init()
{
    m_stateMachine.setObjectName(string("GattMfvVoiceService"));

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, string("Idle"));
    m_stateMachine.addState(RunningState, string("Running"));
    m_stateMachine.addState(RunningState, ReadInitialValuesState, string("ReadInitialValues"));
    m_stateMachine.addState(RunningState, EnableNotificationsState, string("EnableNotifications"));

    // add the transitions:      From State                ->  Event                       ->  To State
    m_stateMachine.addTransition(IdleState,                    StartServiceRequestEvent,       ReadInitialValuesState);
    m_stateMachine.addTransition(ReadInitialValuesState,       InitialValuesRetrievedEvent,    EnableNotificationsState);
    m_stateMachine.addTransition(EnableNotificationsState,     RetryStartNotifyEvent,          EnableNotificationsState);
    m_stateMachine.addTransition(RunningState,                 StopServiceRequestEvent,        IdleState);

    // connect to the state entry signal
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattMfvVoiceService::onEnteredState, this, std::placeholders::_1)));

    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    \reimp
 */
bool GattMfvVoiceService::isReady() const
{
    return m_stateMachine.inState(EnableNotificationsState);
}

BleRcuMfvVoiceService::DetectionType GattMfvVoiceService::detectionType() const
{
    return m_detectionType;
}

BleRcuMfvVoiceService::DetectionData GattMfvVoiceService::detectionData() const
{
    return m_detectionData;
}

BleRcuMfvVoiceService::ModelVersion GattMfvVoiceService::wakeWordModelVersion() const
{
    return m_modelVersion;
}

bool GattMfvVoiceService::privacyEnabled() const
{
    return m_privacyEnabled;
}

vector<uint8_t> GattMfvVoiceService::modelConfiguration() const
{
    return m_modelConfigurationData;
}

uint8_t GattMfvVoiceService::capabilities() const
{
    return m_capabilities;
}

BleRcuMfvVoiceService::StreamStatsRaw GattMfvVoiceService::streamStats() const
{
    return m_streamStats;
}


// -----------------------------------------------------------------------------
/*!
    \reimp

    Starts the service by discovering characteristics from the GATT profile.
 */
bool GattMfvVoiceService::start(const shared_ptr<const BleGattService> &gattService)
{
    // sanity check the supplied info is valid
    if (!gattService || !gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
        XLOGD_WARN("invalid MFV voice gatt service info");
        return false;
    }

    // discover characteristics (each is required)
    if (!m_sessionStartCharacteristic || !m_sessionStartCharacteristic->isValid()) {
        m_sessionStartCharacteristic = gattService->characteristic(m_sessionStartCharUuid);
        if (!m_sessionStartCharacteristic || !m_sessionStartCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Session Start characteristic");
            return false;
        }
    }
    if (!m_detectionDataCharacteristic || !m_detectionDataCharacteristic->isValid()) {
        m_detectionDataCharacteristic = gattService->characteristic(m_detectionDataCharUuid);
        if (!m_detectionDataCharacteristic || !m_detectionDataCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Detection Data characteristic");
            return false;
        }
    }
    if (!m_modelVersionCharacteristic || !m_modelVersionCharacteristic->isValid()) {
        m_modelVersionCharacteristic = gattService->characteristic(m_modelVersionCharUuid);
        if (!m_modelVersionCharacteristic || !m_modelVersionCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Wake Word Model Version characteristic");
            return false;
        }
    }
    if (!m_privacyCharacteristic || !m_privacyCharacteristic->isValid()) {
        m_privacyCharacteristic = gattService->characteristic(m_privacyCharUuid);
        if (!m_privacyCharacteristic || !m_privacyCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Privacy Settings characteristic");
            return false;
        }
    }
    if (!m_modelConfigCharacteristic || !m_modelConfigCharacteristic->isValid()) {
        m_modelConfigCharacteristic = gattService->characteristic(m_modelConfigCharUuid);
        if (!m_modelConfigCharacteristic || !m_modelConfigCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Model Configuration characteristic");
            return false;
        }
    }
    if (!m_capabilitiesCharacteristic || !m_capabilitiesCharacteristic->isValid()) {
        m_capabilitiesCharacteristic = gattService->characteristic(m_capabilitiesCharUuid);
        if (!m_capabilitiesCharacteristic || !m_capabilitiesCharacteristic->isValid()) {
            XLOGD_ERROR("failed to get MFV Capabilities characteristic");
            return false;
        }
    }

    // check we're not already started
    if (m_stateMachine.state() != IdleState) {
        XLOGD_WARN("MFV voice service already started");
        return true;
    }

    m_stateMachine.postEvent(StartServiceRequestEvent);
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Stops the service and resets all characteristics.
 */
void GattMfvVoiceService::stop()
{
    m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called on entry to a new state.
 */
void GattMfvVoiceService::onEnteredState(int state)
{
    if (state == IdleState) {
        m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);

        if (m_sessionStartCharacteristic) {
            m_sessionStartCharacteristic->disableNotifications();
        }
        if (m_detectionDataCharacteristic) {
            m_detectionDataCharacteristic->disableNotifications();
        }
        if (m_privacyCharacteristic) {
            m_privacyCharacteristic->disableNotifications();
        }

        m_sessionStartCharacteristic.reset();
        m_detectionDataCharacteristic.reset();
        m_modelVersionCharacteristic.reset();
        m_privacyCharacteristic.reset();
        m_modelConfigCharacteristic.reset();
        m_capabilitiesCharacteristic.reset();

    } else if (state == ReadInitialValuesState) {
        // Read all readable characteristics. Track completion with a counter;
        // the last read to finish will post the InitialValuesRetrievedEvent.
        m_initialReadsRemaining = 4;
        requestCapabilities();
        requestModelVersion();
        requestPrivacy();
        requestModelConfig();

    } else if (state == EnableNotificationsState) {
        requestStartSessionStartNotify();
        requestStartDetectionDataNotify();
        requestStartPrivacyNotify();

        // Emit the ready signal so GattServices knows the service is up
        m_readySlots.invoke();
    }
}


// =============================================================================
// Initial value reads
// =============================================================================

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the MFV Capabilities characteristic (1 byte bitmask).
 */
void GattMfvVoiceService::requestCapabilities()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Capabilities due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 1) {
                m_capabilities = value[0];
                XLOGD_INFO("MFV Capabilities = 0x%02X (midfield=%d privacy_ctrl=%d soww_eoww=%d aad_ctrl=%d)",
                    m_capabilities,
                    !!(m_capabilities & MidfieldVoiceCapable),
                    !!(m_capabilities & SoftwarePrivacyControl),
                    !!(m_capabilities & SowwEowwTimingAvailable),
                    !!(m_capabilities & AadSensitivityControlAvailable));
            } else {
                XLOGD_ERROR("MFV Capabilities has invalid length (%zu bytes, expected 1)", value.size());
            }
        }
        if (--m_initialReadsRemaining <= 0) {
            m_stateMachine.postEvent(InitialValuesRetrievedEvent);
        }
    };

    m_capabilitiesCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Wake Word Model Version characteristic (2 bytes: major, minor).
 */
void GattMfvVoiceService::requestModelVersion()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Model Version due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 2) {
                m_modelVersion.major = value[0];
                m_modelVersion.minor = value[1];
                XLOGD_INFO("MFV Wake Word Model Version = %u.%u", m_modelVersion.major, m_modelVersion.minor);
            } else {
                XLOGD_ERROR("MFV Model Version has invalid length (%zu bytes, expected 2)", value.size());
            }
        }
        if (--m_initialReadsRemaining <= 0) {
            m_stateMachine.postEvent(InitialValuesRetrievedEvent);
        }
    };

    m_modelVersionCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Privacy Settings characteristic (1 byte boolean).
 */
void GattMfvVoiceService::requestPrivacy()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Privacy due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 1) {
                m_privacyEnabled = (value[0] != 0);
                XLOGD_INFO("MFV Privacy = %s", m_privacyEnabled ? "enabled" : "disabled");
            } else {
                XLOGD_ERROR("MFV Privacy has invalid length (%zu bytes, expected 1)", value.size());
            }
        }
        if (--m_initialReadsRemaining <= 0) {
            m_stateMachine.postEvent(InitialValuesRetrievedEvent);
        }
    };

    m_privacyCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Reads the Model Configuration characteristic (3 bytes: sensitivity,
    secondary sensitivity, AAD sensitivity).
 */
void GattMfvVoiceService::requestModelConfig()
{
    auto replyHandler = [this](PendingReply<std::vector<uint8_t>> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to read MFV Model Config due to <%s>", reply->errorMessage().c_str());
        } else {
            std::vector<uint8_t> value = reply->result();
            if (value.size() == 3) {
                m_modelConfigurationData = value;
                XLOGD_INFO("MFV Model Config: sensitivity=%u secondary=%u aad=%u",
                    value[0], value[1], value[2]);
            } else {
                XLOGD_ERROR("MFV Model Config has invalid length (%zu bytes, expected 3)", value.size());
            }
        }
        if (--m_initialReadsRemaining <= 0) {
            m_stateMachine.postEvent(InitialValuesRetrievedEvent);
        }
    };

    m_modelConfigCharacteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}


// =============================================================================
// Notification subscriptions
// =============================================================================

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Session Start characteristic (0xEA08).
    On notification: 1 byte detection type flags.
 */
void GattMfvVoiceService::requestStartSessionStartNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable Session Start notifications due to <%s>",
                reply->errorMessage().c_str());
            m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
            m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
        } else {
            XLOGD_DEBUG("Session Start notifications enabled successfully");
        }
    };

    m_sessionStartCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(m_isAlive,
            std::bind(&GattMfvVoiceService::onSessionStartChanged, this, std::placeholders::_1)),
        PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Detection Data characteristic (0xEA09).
    On notification: 6 bytes (start_lo, start_hi, end_lo, end_hi, conf_lo, conf_hi).
 */
void GattMfvVoiceService::requestStartDetectionDataNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable Detection Data notifications due to <%s>",
                reply->errorMessage().c_str());
            m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
            m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
        } else {
            XLOGD_DEBUG("Detection Data notifications enabled successfully");
        }
    };

    m_detectionDataCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(m_isAlive,
            std::bind(&GattMfvVoiceService::onDetectionDataChanged, this, std::placeholders::_1)),
        PendingReply<>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Enables notifications on the Privacy Settings characteristic (0xEA0B).
    On notification: 1 byte boolean (1 = enabled, 0 = disabled).
 */
void GattMfvVoiceService::requestStartPrivacyNotify()
{
    auto replyHandler = [this](PendingReply<> *reply)
    {
        if (reply->isError()) {
            XLOGD_ERROR("failed to enable Privacy notifications due to <%s>",
                reply->errorMessage().c_str());
            m_stateMachine.cancelDelayedEvents(RetryStartNotifyEvent);
            m_stateMachine.postDelayedEvent(RetryStartNotifyEvent, 2000);
        } else {
            XLOGD_DEBUG("Privacy notifications enabled successfully");
        }
    };

    m_privacyCharacteristic->enableNotifications(
        Slot<const std::vector<uint8_t> &>(m_isAlive,
            std::bind(&GattMfvVoiceService::onPrivacyChanged, this, std::placeholders::_1)),
        PendingReply<>(m_isAlive, replyHandler));
}


// =============================================================================
// Notification handlers (payload parsers)
// =============================================================================

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Session Start notification is received from the RCU.
    Payload: 1 byte detection type (0x01=FullPower, 0x02=AAD, 0x03=BelowThreshold).
 */
void GattMfvVoiceService::onSessionStartChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 1) {
        XLOGD_ERROR("Session Start notification has invalid length (%zu bytes, expected 1)", newValue.size());
        return;
    }

    uint8_t raw = newValue[0];
    if (raw < 0x01 || raw > 0x03) {
        XLOGD_WARN("Session Start notification has unknown detection type 0x%02X", raw);
    }

    m_detectionType = static_cast<DetectionType>(raw);
    XLOGD_INFO("MFV Session Start: detection type = 0x%02X", raw);

    m_detectionTypeChangedSlots.invoke(m_detectionType);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Detection Data notification is received from the RCU.
    Payload: 6 bytes, little-endian.
      Bytes 0-1: start of wake word (uint16_t)
      Bytes 2-3: end of wake word (uint16_t)
      Bytes 4-5: confidence score x10 (uint16_t)
 */
void GattMfvVoiceService::onDetectionDataChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 6) {
        XLOGD_ERROR("Detection Data notification has invalid length (%zu bytes, expected 6)", newValue.size());
        return;
    }

    m_detectionData.start          = static_cast<uint16_t>(newValue[0]) | (static_cast<uint16_t>(newValue[1]) << 8);
    m_detectionData.end            = static_cast<uint16_t>(newValue[2]) | (static_cast<uint16_t>(newValue[3]) << 8);
    m_detectionData.confidence_x10 = static_cast<uint16_t>(newValue[4]) | (static_cast<uint16_t>(newValue[5]) << 8);

    XLOGD_INFO("MFV Detection Data: start=%u end=%u confidence=%.1f%%",
        m_detectionData.start, m_detectionData.end,
        m_detectionData.confidence_x10 / 10.0);

    m_detectionDataChangedSlots.invoke(m_detectionData);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a Privacy Settings notification is received from the RCU.
    Payload: 1 byte boolean (1 = enabled, 0 = disabled).
 */
void GattMfvVoiceService::onPrivacyChanged(const std::vector<uint8_t> &newValue)
{
    if (newValue.size() != 1) {
        XLOGD_ERROR("Privacy notification has invalid length (%zu bytes, expected 1)", newValue.size());
        return;
    }

    m_privacyEnabled = (newValue[0] != 0);
    XLOGD_INFO("MFV Privacy changed: %s", m_privacyEnabled ? "enabled" : "disabled");

    m_privacyChangedSlots.invoke(m_privacyEnabled);
}


// =============================================================================
// Write handlers
// =============================================================================

// -----------------------------------------------------------------------------
/*!
    \reimp

    Writes the Privacy Settings characteristic.
    Payload: 1 byte (1 = enable, 0 = disable).
 */
void GattMfvVoiceService::writePrivacy(bool enabled, PendingReply<> &&reply)
{
    if (!isReady()) {
        reply.setError("MFV service is not ready");
        reply.finish();
        return;
    }

    if (m_promiseResults) {
        reply.setError("MFV write already in progress");
        reply.finish();
        return;
    }

    if (!m_privacyCharacteristic || !m_privacyCharacteristic->isValid()) {
        reply.setError("MFV Privacy characteristic is not valid");
        reply.finish();
        return;
    }

    m_promiseResults = make_shared<PendingReply<>>(std::move(reply));

    const vector<uint8_t> value(1, enabled ? 0x01 : 0x00);
    XLOGD_INFO("Writing MFV Privacy = %s", enabled ? "enabled" : "disabled");

    m_privacyCharacteristic->writeValue(value, PendingReply<>(m_isAlive,
        std::bind(&GattMfvVoiceService::onWritePrivacyReply, this, std::placeholders::_1)));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Callback for the Privacy write completion.
 */
void GattMfvVoiceService::onWritePrivacyReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        XLOGD_ERROR("failed to write MFV Privacy due to <%s>", reply->errorMessage().c_str());
        if (m_promiseResults) {
            m_promiseResults->setError(reply->errorMessage());
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    } else {
        XLOGD_INFO("MFV Privacy written successfully");
        if (m_promiseResults) {
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \reimp

    Writes the Model Configuration characteristic.
    Payload: 3 bytes (sensitivity, secondary sensitivity, AAD sensitivity).
    AAD sensitivity must be in [60..95] in 5 dB increments.
 */
void GattMfvVoiceService::writeModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply)
{
    if (!isReady()) {
        reply.setError("MFV service is not ready");
        reply.finish();
        return;
    }

    if (m_promiseResults) {
        reply.setError("MFV write already in progress");
        reply.finish();
        return;
    }

    if (!m_modelConfigCharacteristic || !m_modelConfigCharacteristic->isValid()) {
        reply.setError("MFV Model Configuration characteristic is not valid");
        reply.finish();
        return;
    }

    // Validate AAD sensitivity range per spec: [60..95] in 5 dB increments
    if (aad != 0 && (aad < 60 || aad > 95 || (aad % 5) != 0)) {
        XLOGD_ERROR("MFV AAD sensitivity %u is out of valid range [60..95 step 5]", aad);
        reply.setError("AAD sensitivity out of valid range [60..95 step 5]");
        reply.finish();
        return;
    }

    m_promiseResults = make_shared<PendingReply<>>(std::move(reply));

    const vector<uint8_t> value = { sensitivity, secondary, aad };
    XLOGD_INFO("Writing MFV Model Config: sensitivity=%u secondary=%u aad=%u",
        sensitivity, secondary, aad);

    m_modelConfigCharacteristic->writeValue(value, PendingReply<>(m_isAlive,
        std::bind(&GattMfvVoiceService::onWriteModelConfigReply, this, std::placeholders::_1)));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Callback for the Model Configuration write completion.
 */
void GattMfvVoiceService::onWriteModelConfigReply(PendingReply<> *reply)
{
    if (reply->isError()) {
        XLOGD_ERROR("failed to write MFV Model Config due to <%s>", reply->errorMessage().c_str());
        if (m_promiseResults) {
            m_promiseResults->setError(reply->errorMessage());
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    } else {
        XLOGD_INFO("MFV Model Config written successfully");
        if (m_promiseResults) {
            m_promiseResults->finish();
            m_promiseResults.reset();
        }
    }
}
