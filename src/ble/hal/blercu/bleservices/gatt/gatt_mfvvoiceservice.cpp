#include "gatt_mfvvoiceservice.h"

#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"

#include "ctrlm_log_ble.h"

using namespace std;

const BleUuid GattMfvVoiceService::m_serviceUuid("0000ea08-bdf0-407c-aaff-d09967f31acd", "MFV Voice", true);
const BleUuid GattMfvVoiceService::m_sessionStartCharUuid("0000ea08-bdf0-407c-aaff-d09967f31acd", "Session Start");
const BleUuid GattMfvVoiceService::m_detectionDataCharUuid("0000ea09-bdf0-407c-aaff-d09967f31acd", "Detection Data");
const BleUuid GattMfvVoiceService::m_modelVersionCharUuid("0000ea0a-bdf0-407c-aaff-d09967f31acd", "Wake Word Model Version");
const BleUuid GattMfvVoiceService::m_privacyCharUuid("0000ea0b-bdf0-407c-aaff-d09967f31acd", "Privacy Settings");
const BleUuid GattMfvVoiceService::m_modelConfigCharUuid("0000ea0c-bdf0-407c-aaff-d09967f31acd", "Model Configuration");
const BleUuid GattMfvVoiceService::m_capabilitiesCharUuid("0000ea0d-bdf0-407c-aaff-d09967f31acd", "MFV Capabilities");

GattMfvVoiceService::GattMfvVoiceService(GMainLoop *mainLoop)
	: m_ready(false)
	, m_detectionType(FullPower)
	, m_detectionData({ 0, 0, 0 })
	, m_modelVersion({ 0, 0 })
	, m_privacyEnabled(false)
	, m_capabilities(0)
{
	(void)mainLoop;
}

GattMfvVoiceService::~GattMfvVoiceService()
{
	stop();
}

BleUuid GattMfvVoiceService::uuid()
{
	return m_serviceUuid;
}

bool GattMfvVoiceService::isReady() const
{
	return m_ready;
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
	return m_modelConfiguration;
}

uint8_t GattMfvVoiceService::capabilities() const
{
	return m_capabilities;
}

BleRcuMfvVoiceService::StreamStatsRaw GattMfvVoiceService::streamStats() const
{
	return m_streamStats;
}

bool GattMfvVoiceService::start(const shared_ptr<const BleGattService> &gattService)
{
	if (m_ready) {
		XLOGD_WARN("MFV voice service already started");
		return true;
	}

	if (!gattService || !gattService->isValid() || (gattService->uuid() != m_serviceUuid)) {
		XLOGD_WARN("invalid MFV voice gatt service info");
		return false;
	}

	m_sessionStartCharacteristic = gattService->characteristic(m_sessionStartCharUuid);
	m_detectionDataCharacteristic = gattService->characteristic(m_detectionDataCharUuid);
	m_modelVersionCharacteristic = gattService->characteristic(m_modelVersionCharUuid);
	m_privacyCharacteristic = gattService->characteristic(m_privacyCharUuid);
	m_modelConfigCharacteristic = gattService->characteristic(m_modelConfigCharUuid);
	m_capabilitiesCharacteristic = gattService->characteristic(m_capabilitiesCharUuid);

	if (!m_sessionStartCharacteristic || !m_sessionStartCharacteristic->isValid() ||
		!m_detectionDataCharacteristic || !m_detectionDataCharacteristic->isValid() ||
		!m_modelVersionCharacteristic || !m_modelVersionCharacteristic->isValid() ||
		!m_privacyCharacteristic || !m_privacyCharacteristic->isValid() ||
		!m_modelConfigCharacteristic || !m_modelConfigCharacteristic->isValid() ||
		!m_capabilitiesCharacteristic || !m_capabilitiesCharacteristic->isValid()) {
		XLOGD_ERROR("failed to discover one or more MFV characteristics");
		stop();
		return false;
	}

	m_ready = true;
	m_readySlots.invoke();
	return true;
}

void GattMfvVoiceService::stop()
{
	m_ready = false;
	m_sessionStartCharacteristic.reset();
	m_detectionDataCharacteristic.reset();
	m_modelVersionCharacteristic.reset();
	m_privacyCharacteristic.reset();
	m_modelConfigCharacteristic.reset();
	m_capabilitiesCharacteristic.reset();
}

void GattMfvVoiceService::writePrivacy(bool enabled, PendingReply<> &&reply)
{
	(void)enabled;
	reply.setError("MFV privacy write not implemented");
	reply.finish();
}

void GattMfvVoiceService::writeModelConfiguration(uint8_t sensitivity, uint8_t secondary, uint8_t aad, PendingReply<> &&reply)
{
	(void)sensitivity;
	(void)secondary;
	(void)aad;
	reply.setError("MFV model configuration write not implemented");
	reply.finish();

}