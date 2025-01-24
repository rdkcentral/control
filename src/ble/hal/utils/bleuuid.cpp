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
//  bleuuid.cpp
//

#include "bleuuid.h"
#include "ctrlm_log_ble.h"

#include <ctype.h>
#include <cstring>
#include <mutex>
#include <map>
#include <set>

#ifdef CTRLM_BLE_SERVICES
#include "blercu/bleservices/gatt/gatt_external_services.h"
#endif

using namespace std;

static bool ble_uuid_names_init(void);

// Variable initialization to add the default uuid names to the map
static bool g_ble_uuid_names_initialized = ble_uuid_names_init();

// This function adds the names for the default uuid's to the map
bool ble_uuid_names_init(void)
{
    XLOGD_INFO("adding default uuid names");
    BleUuid(BleUuid::RdkVoice,                                std::string("RDK Voice"));
    BleUuid(BleUuid::RdkInfrared,                             std::string("RDK Infrared"));
    BleUuid(BleUuid::RdkFirmwareUpgrade,                      std::string("RDK Firmware Upgrade"));
    BleUuid(BleUuid::RdkRemoteControl,                        std::string("RDK Remote Control"));
    BleUuid(BleUuid::AudioCodecs,                             std::string("Audio Codecs"));
    BleUuid(BleUuid::AudioGain,                               std::string("Audio Gain"));
    BleUuid(BleUuid::AudioControl,                            std::string("Audio Control"));
    BleUuid(BleUuid::AudioData,                               std::string("Audio Data"));
    BleUuid(BleUuid::InfraredStandby,                         std::string("Infrared Standby"));
    BleUuid(BleUuid::InfraredCodeId,                          std::string("Infrared CodeId"));
    BleUuid(BleUuid::InfraredSignal,                          std::string("Infrared Signal"));
    BleUuid(BleUuid::EmitInfraredSignal,                      std::string("Emit Infrared Signal"));
    BleUuid(BleUuid::InfraredSupport,                         std::string("Infrared Support"));
    BleUuid(BleUuid::InfraredControl,                         std::string("Infrared Control"));
    BleUuid(BleUuid::FirmwareControlPoint,                    std::string("Firmware ControlPoint"));
    BleUuid(BleUuid::FirmwarePacket,                          std::string("Firmware Packet"));
    BleUuid(BleUuid::RebootReason,                            std::string("Reboot Reason"));
    BleUuid(BleUuid::UnpairReason,                            std::string("Unpair Reason"));
    BleUuid(BleUuid::RcuAction,                               std::string("RCU Action"));
    BleUuid(BleUuid::LastKeypress,                            std::string("Last Key Press"));
    BleUuid(BleUuid::AdvertisingConfig,                       std::string("Advertising Config"));
    BleUuid(BleUuid::AdvertisingConfigCustomList,             std::string("Advertising Config Custom List"));
    BleUuid(BleUuid::AssertReport,                            std::string("Assert Report"));
    BleUuid(BleUuid::InfraredSignalReference,                 std::string("Infrared Signal Reference"));
    BleUuid(BleUuid::InfraredSignalConfiguration,             std::string("Infrared Signal Configuration"));
    BleUuid(BleUuid::FirmwarePacketWindowSize,                std::string("Firmware Packet Window Size"));
    BleUuid(BleUuid::GenericAccess,                           std::string("Generic Access"));
    BleUuid(BleUuid::GenericAttribute,                        std::string("Generic Attribute"));
    BleUuid(BleUuid::ImmediateAlert,                          std::string("Immediate Alert"));
    BleUuid(BleUuid::DeviceInformation,                       std::string("Device Info"));
    BleUuid(BleUuid::BatteryService,                          std::string("Battery"));
    BleUuid(BleUuid::HumanInterfaceDevice,                    std::string("Human Interface Device"));
    BleUuid(BleUuid::LinkLoss,                                std::string("Link Loss"));
    BleUuid(BleUuid::TxPower,                                 std::string("TX Power"));
    BleUuid(BleUuid::ScanParameters,                          std::string("Scan Parameters"));
    BleUuid(BleUuid::ScanRefresh,                             std::string("Scan Refresh"));
    BleUuid(BleUuid::ScanIntervalWindow,                      std::string("Scan Interval Window"));
    BleUuid(BleUuid::DeviceName,                              std::string("Device Name"));
    BleUuid(BleUuid::Appearance,                              std::string("Appearance"));
    BleUuid(BleUuid::ServiceChanged,                          std::string("Service Changed"));
    BleUuid(BleUuid::AlertLevel,                              std::string("Alert Level"));
    BleUuid(BleUuid::BatteryLevel,                            std::string("Battery Level"));
    BleUuid(BleUuid::SystemID,                                std::string("System ID"));
    BleUuid(BleUuid::ModelNumberString,                       std::string("Model Number"));
    BleUuid(BleUuid::SerialNumberString,                      std::string("Serial Number"));
    BleUuid(BleUuid::FirmwareRevisionString,                  std::string("Firmware Revision"));
    BleUuid(BleUuid::HardwareRevisionString,                  std::string("Hardware Revision"));
    BleUuid(BleUuid::SoftwareRevisionString,                  std::string("Software Revision"));
    BleUuid(BleUuid::ManufacturerNameString,                  std::string("Manufacturer Name"));
    BleUuid(BleUuid::PnPID,                                   std::string("PnP ID"));
    BleUuid(BleUuid::BootKeyboardOutputReport,                std::string("Boot Keyboard Output Report"));
    BleUuid(BleUuid::BootMouseInputReport,                    std::string("Boot MouseInput Report"));
    BleUuid(BleUuid::HIDInformation,                          std::string("HID Information"));
    BleUuid(BleUuid::ReportMap,                               std::string("Report Map"));
    BleUuid(BleUuid::HIDControlPoint,                         std::string("HID Control Point"));
    BleUuid(BleUuid::Report,                                  std::string("Report"));
    BleUuid(BleUuid::ProtocolMode,                            std::string("Protocol Mode"));
    BleUuid(BleUuid::IEEERegulatatoryCertificationDataList,   std::string("IEEE Regulatory Certification Data List"));
    BleUuid(BleUuid::PeripheralPreferredConnectionParameters, std::string("Peripheral Preferred Connection Parameters"));
    BleUuid(BleUuid::ClientCharacteristicConfiguration,       std::string("Client Characteristic Configuration"));
    BleUuid(BleUuid::ReportReference,                         std::string("Report Reference"));
    #ifdef CTRLM_BLE_SERVICES
    ctrlm_ble_uuid_names_install();
    #endif

    return(true);
}

// Generate a 64-bit key from a 128-bit UUID for use in a map by XORing the upper and lower 64-bits
static uint64_t ble_uuid_to_key(uuid_t uuid)
{
    uint64_t *upper = (uint64_t *)&uuid[0];
    uint64_t *lower = (uint64_t *)&uuid[8];
    return(*upper ^ *lower);
}

// If name is not empty, set the name for this uuid. Return the name for this uuid.
static std::string ble_uuid_to_name(uuid_t uuid, const std::string &name = std::string())
{
    // Please note that the following variables are declared static to use a single instance for the process
    static std::map<uint64_t, std::string> g_ble_uuid_names; // Map of hash of uuid to name
    static std::mutex g_ble_uuid_names_mutex;                // Mutex to serialize access to the map

    std::unique_lock<std::mutex> guard(g_ble_uuid_names_mutex);
    if(!name.empty()) {
        #if 0
        char uuidChar[UUID_STR_LEN];
        uuid_unparse_lower(uuid, uuidChar);

        XLOGD_INFO("adding mapping from uuid <%s> to name <%s>", uuidChar, name.c_str());
        #endif

        uint64_t key = ble_uuid_to_key(uuid);
        
        g_ble_uuid_names[key] = name;

        return(name);
    }
    return(g_ble_uuid_names[ble_uuid_to_key(uuid)]);
}

// If addServiceName is true, add the name to the service list and return true. 
// If addServiceName is false, search the service list for the specified name and return true if found or false if not found.
static bool ble_service_name_list(const std::string &name, bool addServiceName)
{
    // Please note that the following variables are declared static to use a single instance for the process
    static std::set<std::string> g_ble_service_names; // Set of service names
    static std::mutex g_ble_service_names_mutex;      // Mutex to serialize access to the set

    std::unique_lock<std::mutex> guard(g_ble_service_names_mutex);

    if(addServiceName) {
        g_ble_service_names.insert(name);
        return true;
    }
    auto result = g_ble_service_names.find(name);
    if (result != g_ble_service_names.end()) {
        return true;
    }
    
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */

// The following is the base UUID for all standardised bluetooth APIs
//   {00000000-0000-1000-8000-00805F9B34FB}
#define STANDARD_BASE_UUID   "0000-1000-8000-00805f9b34fb"

// The following is the base UUID for custom defined RDK bluetooth APIs
//   {00000000-bdf0-407c-aaff-d09967f31acd}
#define CUSTOM_RDK_BASE_UUID    "bdf0-407c-aaff-d09967f31acd"


bool BleUuid::ConstructUuid(const uint32_t head, const char *base, uuid_t uuid, const std::string &name, bool isService)
{
    char buf[UUID_STR_LEN];
    snprintf(buf, sizeof(buf), "%08x-%s", head, base);

    if (uuid_parse(buf, uuid) < 0) {
        XLOGD_ERROR("failed to create uuid from <%s>", buf);
        return false;
    }

    m_name = ble_uuid_to_name(uuid, name);
    if(isService) { // Add the service name to the service list
        ble_service_name_list(m_name, true);
    }
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Constructs a new null Bluetooth UUID.

 */


BleUuid::BleUuid()
{
    uuid_clear(m_uuid);
}

BleUuid::BleUuid(ServiceType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, STANDARD_BASE_UUID, m_uuid, name, true)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(CharacteristicType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, STANDARD_BASE_UUID, m_uuid, name)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(DescriptorType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, STANDARD_BASE_UUID, m_uuid, name)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(CustomRdkServiceType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, CUSTOM_RDK_BASE_UUID, m_uuid, name, true)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(CustomRdkCharacteristicType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, CUSTOM_RDK_BASE_UUID, m_uuid, name)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(CustomRdkDescriptorType uuid, const std::string &name)
{
    if (!ConstructUuid(uuid, CUSTOM_RDK_BASE_UUID, m_uuid, name)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(uint16_t uuid, const std::string &name, bool isService)
{
    if (!ConstructUuid(uuid, STANDARD_BASE_UUID, m_uuid, name, isService)) {
        uuid_clear(m_uuid);
    }
}

BleUuid::BleUuid(uint32_t uuid, const std::string &name, bool isService)
{
    if (!ConstructUuid(uuid, STANDARD_BASE_UUID, m_uuid, name, isService)) {
        uuid_clear(m_uuid);
    }
}


BleUuid::BleUuid(const string &uuid, const std::string &name, bool isService)
{
    if (uuid_parse(uuid.c_str(), m_uuid) < 0) {
        XLOGD_ERROR("failed to parse UUID string: <%s>", uuid.c_str());
        uuid_clear(m_uuid);
    } else {
        m_name = ble_uuid_to_name(m_uuid, name);
        if(isService) { // Add the service name to the service list
            ble_service_name_list(m_name, true);
        }
    }
}

BleUuid::BleUuid(const BleUuid &other, bool isService)
{
    uuid_copy(m_uuid, other.m_uuid);
    m_name = ble_uuid_to_name(m_uuid);
    if(isService) { // Add the service name to the service list
        ble_service_name_list(m_name, true);
    }
}


// -----------------------------------------------------------------------------
/*!
    Destroys the Bluetooth UUID.
 */
BleUuid::~BleUuid()
{
    uuid_clear(m_uuid);
}

// -----------------------------------------------------------------------------
/*!
    Returns the name of the service / characteristic or descriptor that the
    uuid corresponds to if known, otherwise an empty string.

 */
string BleUuid::name() const
{
    return(m_name);
}

// -----------------------------------------------------------------------------
/*!
    Returns true if the service is known, otherwise false.

 */
bool BleUuid::doesServiceExist(const std::string &name) const
{
    return(ble_service_name_list(name, false));
}

// -----------------------------------------------------------------------------
/*!
    An override of the QUuid::toString() method that allows for returning a
    string with or without the curly braces around it.  Typically when used with
    BLE the UUID are displayed with braces.

 */
string BleUuid::toString(UuidFormat format) const
{
    char buf[100];
    char uuidChar[UUID_STR_LEN];
    uuid_unparse_lower(m_uuid, uuidChar);

    snprintf(buf, sizeof(buf), "%s%s%s [%s]",
        (format == WithCurlyBraces) ? "{" : "", uuidChar, (format == WithCurlyBraces) ? "}" : "",
        name().c_str());

    return(string(buf));
}

