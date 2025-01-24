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
//  gatt_deviceinfoservice.cpp
//

#include "gatt_deviceinfoservice.h"
#include "blercu/blegattservice.h"
#include "blercu/blegattcharacteristic.h"
#include "blercu/blercuerror.h"

#include "ctrlm_log_ble.h"


using namespace std;

const BleUuid GattDeviceInfoService::m_serviceUuid(BleUuid::DeviceInformation);


// -----------------------------------------------------------------------------
/*!
    \class GattDeviceInfoService
    \brief

 */



// this table describes what command to send in each state and also the next
// state to move too plus the handler to call to process the reply message sent
// by the device through the tunnel
const map<GattDeviceInfoService::InfoField, GattDeviceInfoService::StateHandler> GattDeviceInfoService::m_stateHandler = {

    //  flag                   characteristic uuid              characteristic handler
    {   ManufacturerName,    { BleUuid::ManufacturerNameString, &GattDeviceInfoService::setManufacturerName  }   },
    {   ModelNumber,         { BleUuid::ModelNumberString,      &GattDeviceInfoService::setModelNumber       }   },
    {   SerialNumber,        { BleUuid::SerialNumberString,     &GattDeviceInfoService::setSerialNumber      }   },
    {   HardwareRevision,    { BleUuid::HardwareRevisionString, &GattDeviceInfoService::setHardwareRevision  }   },
    {   FirmwareVersion,     { BleUuid::FirmwareRevisionString, &GattDeviceInfoService::setFirmwareVersion   }   },
    {   SoftwareVersion,     { BleUuid::SoftwareRevisionString, &GattDeviceInfoService::setSoftwareVersion   }   },
    {   SystemId,            { BleUuid::SystemID,               &GattDeviceInfoService::setSystemId          }   },
    {   PnPId,               { BleUuid::PnPID,                  &GattDeviceInfoService::setPnPId             }   },

};


// -----------------------------------------------------------------------------
/*!
    Constructs the device info service which queries the info over the bluez
    GATT interface.

 */
GattDeviceInfoService::GattDeviceInfoService(GMainLoop* mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_forceRefresh(false)
    , m_infoFlags(0)
    , m_systemId(0)
    , m_vendorIdSource(Invalid)
    , m_vendorId(0)
    , m_productId(0)
    , m_productVersion(0)
{
    // setup the basic statemachine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

GattDeviceInfoService::~GattDeviceInfoService()
{
    *m_isAlive = false;
    stop();
}

// -----------------------------------------------------------------------------
/*!
    Returns the gatt uuid of this service.

 */
BleUuid GattDeviceInfoService::uuid()
{
    return m_serviceUuid;
}

// -----------------------------------------------------------------------------
/*!
    \internal
 
    Initailises the state machine used internally by the class.

 */
void GattDeviceInfoService::init()
{
    m_stateMachine.setObjectName("GattDeviceInfoService");

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, "Idle");
    m_stateMachine.addState(InitialisingState, "Initialising");
    m_stateMachine.addState(RunningState, "Running");
    m_stateMachine.addState(StoppedState, "Stopped");


    // add the transitions:      From State        ->   Event                               ->  To State
    m_stateMachine.addTransition(IdleState,             StartServiceRequestEvent,               InitialisingState);
    m_stateMachine.addTransition(IdleState,             StartServiceForceRefreshRequestEvent,   InitialisingState);

    m_stateMachine.addTransition(InitialisingState,     StopServiceRequestEvent,                IdleState);
    m_stateMachine.addTransition(InitialisingState,     InitialisedEvent,                       RunningState);

    m_stateMachine.addTransition(RunningState,          StopServiceRequestEvent,                StoppedState);
    m_stateMachine.addTransition(StoppedState,          StartServiceRequestEvent,               RunningState);
    m_stateMachine.addTransition(StoppedState,          StartServiceForceRefreshRequestEvent,   InitialisingState);


    // connect to the state entry signal
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattDeviceInfoService::onEnteredState, this, std::placeholders::_1)));
    m_stateMachine.addExitedHandler(Slot<int>(m_isAlive, std::bind(&GattDeviceInfoService::onExitedState, this, std::placeholders::_1)));


    // set the initial state of the state machine and start it
    m_stateMachine.setInitialState(IdleState);
    m_stateMachine.start();
}


// -----------------------------------------------------------------------------
/*!
    Starts the service by setting the initial state and sending off the first
    gatt characteristic read requests.  When the service has finished it's setup,
    a \a ready() signal will be emitted.


 */
bool GattDeviceInfoService::start(const shared_ptr<const BleGattService> &gattService)
{
    // check we're not already started
    if (!m_stateMachine.inState( { IdleState, StoppedState } )) {
        XLOGD_WARN("service is already started");
        return true;
    }

    // unlike the other services, the device information only contains static
    // data, so we don't create and store multiple dbus proxies to the actual
    // characteristics, instead we just send one off requests to read the
    // values
    m_gattService = gattService;

    // if the force flag is set then start or restart the service forcing it
    // to rescan the device info (this is typically set after f/w upgrade)
    if (m_forceRefresh) {
        m_forceRefresh = false;
        m_stateMachine.postEvent(StartServiceForceRefreshRequestEvent);

    } else {
        // just an ordinary service start
        m_stateMachine.postEvent(StartServiceRequestEvent);
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!


 */
void GattDeviceInfoService::stop()
{
    m_gattService.reset();
    m_stateMachine.postEvent(StopServiceRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    Slot typically called after the completion of a firmware upgrade, it causes
    the device info fields to be re-read the next time the service is started.

 */
void GattDeviceInfoService::forceRefresh()
{
    m_forceRefresh = true;
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattDeviceInfoService::onEnteredState(int state)
{
    switch (state) {
        case InitialisingState:
            // clear the bitmask of received fields and go and request them all in one big lump
            m_infoFlags = 0;
            sendCharacteristicReadRequest(ManufacturerName);
            sendCharacteristicReadRequest(ModelNumber);
            sendCharacteristicReadRequest(SerialNumber);
            sendCharacteristicReadRequest(HardwareRevision);
            sendCharacteristicReadRequest(FirmwareVersion);
            sendCharacteristicReadRequest(SoftwareVersion);
            sendCharacteristicReadRequest(PnPId);
            sendCharacteristicReadRequest(SystemId);
            break;

        case RunningState:
            m_readySlots.invoke();
            break;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattDeviceInfoService::onExitedState(int state)
{
    // on exiting the initialising state we should have all the required
    // device info fields, so at this point log a milestone message with all
    // the details
    if (state == InitialisingState) {
        XLOGD_INFO("bluetooth rcu device info [ %s / %s / hw:%s / fw:%s / sw:%s ]",
                 m_manufacturerName.c_str(), m_modelNumber.c_str(),
                 m_hardwareRevision.c_str(), m_firmwareVersion.c_str(),
                 m_softwareVersion.c_str());
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal


 */
void GattDeviceInfoService::sendCharacteristicReadRequest(InfoField field)
{
    // sanity checks
    if (m_stateHandler.end() == m_stateHandler.find(field)) {
        XLOGD_ERROR("trying to send command for unknown info field %d", field);
        return;
    }

    if (!m_gattService || !m_gattService->isValid()) {
        XLOGD_ERROR("gatt service info is not valid");
        return;
    }


    // get the uuid of characteristic whos value we want to retrieve
    const BleUuid &uuid = m_stateHandler.at(field).uuid;

    // try and get the dbus object path to the characteristic
    shared_ptr<BleGattCharacteristic> characteristic = m_gattService->characteristic(uuid);
    if (!characteristic || !characteristic->isValid()) {

        // systemID is optional so don't log an error if not present
        if (uuid != BleUuid(BleUuid::SystemID)) {
            XLOGD_WARN("missing or invalid gatt characteristic with uuid %s, skipping device info characteristic",
                       uuid.toString().c_str());
        }
        return;
    }

    // lambda invoked when the request returns
    auto replyHandler = [this, field](PendingReply<std::vector<uint8_t>> *reply)
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                onCharacteristicReadError(reply->errorMessage(), field);
            } else {
                onCharacteristicReadSuccess(reply->result(), field);
            }
        };

    // request a read on the characteristic
    characteristic->readValue(PendingReply<std::vector<uint8_t>>(m_isAlive, replyHandler));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the system replies after successifully reading the
    characteristic value from the remote device.

 */
void GattDeviceInfoService::onCharacteristicReadSuccess(const std::vector<uint8_t> &value,
                                                        InfoField field)
{
    // check we're in a state where a command is expected
    if (m_stateHandler.end() == m_stateHandler.find(field)) {
        XLOGD_WARN("received gatt char reply we weren't expecting for field %d - ignoring the reply", field);
        return;
    }

    // if we have a processor for the field, call it
    const StateHandler &handler = m_stateHandler.at(field);
    if (handler.handler != nullptr) {
        (this->*(handler.handler))(value);
    }


    // add the field to our bitmask of received fields, if we've got them all
    // then can signal we're initialised
    m_infoFlags |= field;


    // check if we now have all the required fields
    static const uint16_t requiredFields = ManufacturerName
                                         | ModelNumber
                                         | SerialNumber
                                         | HardwareRevision
                                         | FirmwareVersion
                                         | SoftwareVersion
                                         | PnPId;

    if ((requiredFields & m_infoFlags) == requiredFields) {
        m_stateMachine.postEvent(InitialisedEvent);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the system replies after failing to read the characteristic
    value from the remote device.

 */
void GattDeviceInfoService::onCharacteristicReadError(const string &message,
                                                      InfoField field)
{
    // check we're in a state where a command is expected
    if (m_stateHandler.end() == m_stateHandler.find(field)) {
        XLOGD_WARN("received gatt char reply we weren't expecting for field %d - ignoring the reply", field);
        return;
    }

    const StateHandler &handler = m_stateHandler.at(field);
    XLOGD_WARN("failed to read value for characteristic with uuid %s due to %s",
            handler.uuid.toString().c_str(), message.c_str());
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c manufacturer_name_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
 */
void GattDeviceInfoService::setManufacturerName(const std::vector<uint8_t> &value)
{
    const string name(value.begin(), value.end());

    if (name != m_manufacturerName) {
        m_manufacturerName = name;
        XLOGD_INFO("manufacturer name: %s", m_manufacturerName.c_str());

        m_manufacturerNameChangedSlots.invoke(m_manufacturerName);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c model_number_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.model_number_string.xml
 */
void GattDeviceInfoService::setModelNumber(const std::vector<uint8_t> &value)
{
    const string model(value.begin(), value.end());

    if (model != m_modelNumber) {
        m_modelNumber = model;
        XLOGD_INFO("model number: %s", m_modelNumber.c_str());

        m_modelNumberChangedSlots.invoke(m_modelNumber);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c serial_number_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.serial_number_string.xml
 */
void GattDeviceInfoService::setSerialNumber(const std::vector<uint8_t> &value)
{
    const string serial(value.begin(), value.end());

    if (serial != m_serialNumber) {
        m_serialNumber = serial;
        XLOGD_INFO("serial number: %s", m_serialNumber.c_str());

        m_serialNumberChangedSlots.invoke(m_serialNumber);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c hardware_revision_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hardware_revision_string.xml
 */
void GattDeviceInfoService::setHardwareRevision(const std::vector<uint8_t> &value)
{
    const string hwVersion(value.begin(), value.end());

    if (hwVersion != m_hardwareRevision) {
        m_hardwareRevision = hwVersion;
        XLOGD_INFO("hardware revision: %s", m_hardwareRevision.c_str());

        m_hardwareRevisionChangedSlots.invoke(m_hardwareRevision);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c firmware_revision_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.firmware_revision_string.xml
 */
void GattDeviceInfoService::setFirmwareVersion(const std::vector<uint8_t> &value)
{
    const string fwVersion(value.begin(), value.end());

    if (fwVersion != m_firmwareVersion) {

        m_firmwareVersion = fwVersion;
        XLOGD_INFO("firmware version: %s", m_firmwareVersion.c_str());

        m_firmwareVersionChangedSlots.invoke(m_firmwareVersion);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c software_revision_string
    characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.software_revision_string.xml
 */
void GattDeviceInfoService::setSoftwareVersion(const std::vector<uint8_t> &value)
{
    const string swVersion(value.begin(), value.end());

    if (swVersion != m_softwareVersion) {

        m_softwareVersion = swVersion;
        XLOGD_INFO("software version: %s", m_softwareVersion.c_str());

        m_softwareVersionChangedSlots.invoke(m_softwareVersion);
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c system_id characteristic.

    \see https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.system_id.xml
 */
void GattDeviceInfoService::setSystemId(const std::vector<uint8_t> &value)
{
    // sanity check that the received data is 64-bit / 8 bytes
    if (value.size() < 8) {
        XLOGD_ERROR("received invalid length for system id (%d bytes)", value.size());
        return;
    }

    m_systemId = (((uint64_t)value[0]) << 56) |
                 (((uint64_t)value[1]) << 48) |
                 (((uint64_t)value[2]) << 40) |
                 (((uint64_t)value[3]) << 32) |
                 (((uint64_t)value[4]) << 24) |
                 (((uint64_t)value[5]) << 16) |
                 (((uint64_t)value[6]) <<  8) |
                 (((uint64_t)value[7]) <<  0);

    XLOGD_INFO("system id: 0x%016llx", m_systemId);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when we receive a reply from the GATT \c pnp_id characteristic.

    The value of PnP is the same as in the bluetooth LE DIS profile;
    https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml

 */
void GattDeviceInfoService::setPnPId(const std::vector<uint8_t> &value)
{
    // sanity check that the received data is at least 7 bytes
    if (value.size() < 7) {
        XLOGD_ERROR("received invalid length for pnp id (%d bytes)", value.size());
        return;
    }

    // store the pnp data
    m_vendorIdSource =  (uint8_t)value[0];
    m_vendorId =        (((uint16_t)value[1]) << 0) | (((uint16_t)value[2]) << 8);
    m_productId =       (((uint16_t)value[3]) << 0) | (((uint16_t)value[4]) << 8);
    m_productVersion =  (((uint16_t)value[5]) << 0) | (((uint16_t)value[6]) << 8);

    XLOGD_INFO("pnp id (%s) 0x%04x:0x%04x:0x%04x",
          (m_vendorIdSource == Bluetooth) ? "bluetooth" :
          (m_vendorIdSource == USB)       ? "usb" : "?",
          m_vendorId, m_productId, m_productVersion);
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns \c true if the service is ready and all the info fields have been
    populated.

 */
bool GattDeviceInfoService::isReady() const
{
    return m_stateMachine.inState(RunningState);
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the manufacturer name string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::manufacturerName() const
{
    return m_manufacturerName;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the model number string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::modelNumber() const
{
    return m_modelNumber;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the serial number string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::serialNumber() const
{
    return m_serialNumber;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the hardware revision string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::hardwareRevision() const
{
    return m_hardwareRevision;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the firmware version string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::firmwareVersion() const
{
    return m_firmwareVersion;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the software version string, this is only valid if the service is
    in the ready state.

 */
string GattDeviceInfoService::softwareVersion() const
{
    return m_softwareVersion;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the system id value, this is only valid if the service is in the
    ready state.

 */
uint64_t GattDeviceInfoService::systemId() const
{
    return m_systemId;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the PnP vendor id source, i.e. if the vendor id is registered as a
    usb or bluetooth id. This is only valid if the service is in the ready state.

 */
BleRcuDeviceInfoService::PnPVendorSource GattDeviceInfoService::pnpVendorIdSource() const
{
    switch (m_vendorIdSource) {
        case 0x01:     return PnPVendorSource::Bluetooth;
        case 0x02:     return PnPVendorSource::USB;
        default:       return PnPVendorSource::Invalid;
    }
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the PnP vendor id, this is only valid if the service is in the ready
    state.

 */
uint16_t GattDeviceInfoService::pnpVendorId() const
{
    return m_vendorId;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the PnP product id, this is only valid if the service is in the
    ready state.

 */
uint16_t GattDeviceInfoService::pnpProductId() const
{
    return m_productId;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns the PnP product version, this is only valid if the service is in the
    ready state.

 */
uint16_t GattDeviceInfoService::pnpProductVersion() const
{
    return m_productVersion;
}

