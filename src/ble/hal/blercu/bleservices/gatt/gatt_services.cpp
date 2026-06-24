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
//  gatt_services.cpp
//

#include "gatt_services.h"

#include "gatt_audioservice_rdk.h"
#include "gatt_batteryservice.h"
#include "gatt_deviceinfoservice.h"
#include "gatt_findmeservice.h"
#include "gatt_infraredservice.h"
#include "gatt_upgradeservice.h"
#include "gatt_remotecontrolservice.h"

#include "blercu/blegattprofile.h"
#include "blercu/blegattservice.h"

#include "ctrlm_log_ble.h"

#ifdef CTRLM_BLE_SERVICES
#include "gatt_external_services.h"
#endif

using namespace std;

// -----------------------------------------------------------------------------
/*!
    Constructs an \l{GattServices} object which will connect to the vendor
    daemon and attempt to register the device with the supplied
    \a bluezDeviceObjPath.


 */
GattServices::GattServices(const BleAddress &address,
                           const shared_ptr<BleGattProfile> &gattProfile,
                           const ConfigModelSettings &settings,
                           GMainLoop *mainLoop)
    : m_isAlive(make_shared<bool>(true))
    , m_address(address)
    , m_gattProfile(gattProfile)
    , m_servicesRequired(settings.servicesRequired())
    , m_servicesOptional(settings.servicesOptional())
    , m_deviceInfoService(make_shared<GattDeviceInfoService>(mainLoop))
    , m_batteryService(make_shared<GattBatteryService>(mainLoop))
    , m_findMeService(make_shared<GattFindMeService>(mainLoop))
    , m_remoteControlService(make_shared<GattRemoteControlService>(mainLoop))
{
    
    // Add the built-in services to the list of services available
    m_audioServices.push_back(make_shared<GattAudioServiceRdk>(mainLoop));
    m_infraredServices.push_back(make_shared<GattInfraredService>(settings, m_deviceInfoService, mainLoop));
    m_upgradeServices.push_back(make_shared<GattUpgradeService>(mainLoop));

    #ifdef CTRLM_BLE_SERVICES
    ctrlm_ble_gatt_services_install(mainLoop, settings, m_deviceInfoService, m_audioServices, m_infraredServices, m_upgradeServices);
    #endif

    // connect to the gatt profile update completed event
    gattProfile->addUpdateCompletedHandler(Slot<>(m_isAlive, std::bind(&GattServices::onGattProfileUpdated, this)));

    for (const auto &upgradeService : m_upgradeServices) {
        // connect to the upgradeFinished signal of the upgrade service to the device info 
        // service so that the device info service re-queries it's cached values after an upgrade
        upgradeService->addUpgradeCompleteSlot(m_deviceInfoService->getForceRefreshSlot());
    }

    // setup and start the state machine
    m_stateMachine.setGMainLoop(mainLoop);
    init();
}

// -----------------------------------------------------------------------------
/*!
    Disposes of this object.

 */
GattServices::~GattServices()
{
    *m_isAlive = false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Configures and starts the state machine
 */
void GattServices::init()
{
    m_stateMachine.setObjectName(string("GattServices"));

    // add all the states and the super state groupings
    m_stateMachine.addState(IdleState, string("Idle"));
    m_stateMachine.addState(GettingGattServicesState, string("GettingGattServicesState"));

    m_stateMachine.addState(ResolvedServicesSuperState, string("ResolvedServicesSuperState"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingDeviceInfoServiceState, string("StartingDeviceInfoService"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingBatteryServiceState, string("StartingBatteryService"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingFindMeServiceState, string("StartingFindMeService"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingAudioServiceState, string("StartingAudioService"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingInfraredServiceState, string("StartingInfraredService"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingUpgradeServiceState, string("StartingUpgradeServiceState"));
    m_stateMachine.addState(ResolvedServicesSuperState, StartingRemoteControlServiceState, string("StartingRemoteControlServiceState"));
    m_stateMachine.addState(ResolvedServicesSuperState, ReadyState, string("Ready"));

    m_stateMachine.addState(StoppingState, string("Stopping"));


    // set the initial state of the state machine
    m_stateMachine.setInitialState(IdleState);


    // add the transitions:      From State                    ->   Event                       ->  To State
    m_stateMachine.addTransition(IdleState,                         StartServicesRequestEvent,      GettingGattServicesState);
    m_stateMachine.addTransition(GettingGattServicesState,          StopServicesRequestEvent,       IdleState);
    
    // Need to start RemoteControl service first so that we read the last keypress characterisitic as soon as possible
    m_stateMachine.addTransition(GettingGattServicesState,          GotGattServicesEvent,           StartingRemoteControlServiceState);

    m_stateMachine.addTransition(StartingRemoteControlServiceState, RemoteControlServiceReadyEvent, StartingDeviceInfoServiceState);
    m_stateMachine.addTransition(StartingDeviceInfoServiceState,    DeviceInfoServiceReadyEvent,    StartingBatteryServiceState);

    m_stateMachine.addTransition(StartingBatteryServiceState,       BatteryServiceReadyEvent,       StartingFindMeServiceState);
    m_stateMachine.addTransition(StartingFindMeServiceState,        FindMeServiceReadyEvent,        StartingAudioServiceState);

    m_stateMachine.addTransition(StartingAudioServiceState,         AudioServiceReadyEvent,         StartingInfraredServiceState);

    m_stateMachine.addTransition(StartingInfraredServiceState,      InfraredServiceReadyEvent,      StartingUpgradeServiceState);
    m_stateMachine.addTransition(StartingUpgradeServiceState,       UpgradeServiceReadyEvent,       ReadyState);

    m_stateMachine.addTransition(ResolvedServicesSuperState,        StopServicesRequestEvent,       StoppingState);
    m_stateMachine.addTransition(StoppingState,                     ServicesStoppedEvent,           IdleState);


    // connect to the state entry and exit signals
    m_stateMachine.addEnteredHandler(Slot<int>(m_isAlive, std::bind(&GattServices::onEnteredState, this, std::placeholders::_1)));

    // start the state machine
    m_stateMachine.start();
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Starts all the services for the remote device.

 */
bool GattServices::start()
{
    XLOGD_INFO("starting services");

    // start the state machine
    m_stateMachine.postEvent(StartServicesRequestEvent);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \overload


 */
void GattServices::stop()
{
    XLOGD_INFO("stopping services");

    m_stateMachine.postEvent(StopServicesRequestEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Private slot called on entry to a new state.
 */
void GattServices::onEnteredState(int state)
{
    bool serviceOk = true;
    bool serviceNotUsed = false;
    switch (state) {
        case IdleState:
            break;

        case GettingGattServicesState:
            onEnteredGetGattServicesState();
            break;

        case StartingDeviceInfoServiceState:
            serviceOk = startService(m_deviceInfoService, DeviceInfoServiceReadyEvent, serviceNotUsed);
            break;

        case StartingBatteryServiceState:
            serviceOk = startService(m_batteryService, BatteryServiceReadyEvent, serviceNotUsed);
            break;

        case StartingFindMeServiceState:
            serviceOk = startService(m_findMeService, FindMeServiceReadyEvent, serviceNotUsed);
            break;

        case StartingAudioServiceState:
            for (const auto &audioService : m_audioServices) {
                serviceNotUsed = false;
                serviceOk = startService(audioService, AudioServiceReadyEvent, serviceNotUsed);
                if (serviceOk && !serviceNotUsed) {
                    break;
                }
                m_audioServiceIndex++;
            }
            if(m_audioServiceIndex >= m_audioServices.size()) {
                m_audioServiceIndex = 0;
            }
            break;

        case StartingInfraredServiceState:
            for (const auto &infraredService : m_infraredServices) {
                serviceNotUsed = false;
                serviceOk = startService(infraredService, InfraredServiceReadyEvent, serviceNotUsed);
                if (serviceOk && !serviceNotUsed) {
                    break;
                }
                m_infraredServiceIndex++;
            }
            if(m_infraredServiceIndex >= m_infraredServices.size()) {
                m_infraredServiceIndex = 0;
            }
            break;

        case StartingUpgradeServiceState:
            for (const auto &upgradeService : m_upgradeServices) {
                serviceNotUsed = false;
                serviceOk = startService(upgradeService, UpgradeServiceReadyEvent, serviceNotUsed);
                if (serviceOk && !serviceNotUsed) {
                    break;
                }
                m_upgradeServiceIndex++;
            }
            if(m_upgradeServiceIndex >= m_upgradeServices.size()) {
                m_upgradeServiceIndex = 0;
            }
            break;

        case StartingRemoteControlServiceState:
            serviceOk = startService(m_remoteControlService, RemoteControlServiceReadyEvent, serviceNotUsed);
            break;

        case ReadyState:
            m_readySlots.invoke();
            break;

        case StoppingState:
            m_remoteControlService->stop();
            m_deviceInfoService->stop();
            m_batteryService->stop();
            m_findMeService->stop();
            for (const auto &audioService : m_audioServices) {
                audioService->stop();
            }
            for (const auto &infraredService : m_infraredServices) {
                infraredService->stop();
            }
            for (const auto &upgradeService : m_upgradeServices) {
                upgradeService->stop();
            }

            m_stateMachine.postEvent(ServicesStoppedEvent);
            break;

        default:
            break;
    }
    if(!serviceOk) { // TODO take some action based on the service that failed
       XLOGD_ERROR("failed to start gatt service");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called on entry to the 'GetGattServices' state, this means that bluez has
    told us that it's resolved the gatt services and we just need to get
    all the dbus object paths to the gatt services / characteristics /
    descriptors.

 */
void GattServices::onEnteredGetGattServicesState()
{
    // request an update of all the gatt details from bluez / android, this will
    // emit the update signal when done which will trigger onGattProfileUpdated()
    m_gattProfile->updateProfile();
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the GattBluezProfile object has gathered all the gatt data
    from bluez and we're ready to proceed to try and setup the services.

 */
void GattServices::onGattProfileUpdated()
{
    m_stateMachine.postEvent(GotGattServicesEvent);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Determines if a service is supported by the remote control model as a required
    service, optional service or not supported at all.

 */
void GattServices::isServiceSupported(const BleUuid &uuid, bool &required, bool &optional) {
    // Look up the list of supported services by remote control model
    std::string serviceName = uuid.name();
    if (serviceName.empty()) { // Service is not found in the uuid to service map
        required = false;
        optional = false;
    } else {
        auto resultRequired = m_servicesRequired.find(serviceName);
        if (resultRequired != m_servicesRequired.end()) {
            required = true;
        }
        auto resultOptional = m_servicesOptional.find(serviceName);
        if (resultOptional != m_servicesOptional.end()) {
            optional = true;
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

 */
template <typename T>
bool GattServices::startService(const shared_ptr<T> &service, Event::Type readyEvent, bool &serviceNotUsed)
{
    // get the service uuid and use it to try and find a matching bluez service
    // in the profile
    if (!service->isReady()) {
        bool required = false;
        bool optional = false;
        isServiceSupported(service->uuid(), required, optional);

        if (!required && !optional) {
            XLOGD_INFO("unused gatt service <%s>", service->uuid().toString().c_str());
            m_stateMachine.postEvent(readyEvent);
            serviceNotUsed = true;
            return(true);
        }

        shared_ptr<BleGattService> gattService = m_gattProfile->service(service->uuid());
        if (!gattService || !gattService->isValid()) {
            if (optional) {
                XLOGD_WARN("failed to find optional gatt service <%s>", service->uuid().toString().c_str());
                m_stateMachine.postEvent(readyEvent);
                return(true);
            }
            XLOGD_ERROR("failed to find required gatt service <%s>", service->uuid().toString().c_str());
            return(false);
        }

        // try and start the service
        if (!service->start(gattService)) {
            XLOGD_ERROR("failed to start gatt service <%s>", service->uuid().toString().c_str());
            return(false);
        }
    }

    // check if it's ready now
    if (service->isReady()) {
        m_stateMachine.postEvent(readyEvent);
        return(true);
    }

    // otherwise install a functor to deliver the 'ready' event to the
    // state machine when it becomes ready
    std::function<void()> functor = [this,readyEvent]() {
        m_stateMachine.postEvent(readyEvent);
    };
    service->addReadySlot(Slot<>(m_isAlive, std::move(functor)));
    return(true);
}

// -----------------------------------------------------------------------------
/*!
    \reimp

 */
bool GattServices::isValid() const
{
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
bool GattServices::isReady() const
{
    return m_stateMachine.inState(ReadyState);
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
shared_ptr<BleRcuAudioService> GattServices::audioService() const
{
    if(m_audioServiceIndex >= m_audioServices.size()) {
        XLOGD_ERROR("gatt audio service index out of range <%u> max <%u>", m_audioServiceIndex, m_audioServices.size());
        return nullptr;
    }
    return m_audioServices[m_audioServiceIndex];
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
shared_ptr<BleRcuDeviceInfoService> GattServices::deviceInfoService() const
{
    return m_deviceInfoService;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
shared_ptr<BleRcuBatteryService> GattServices::batteryService() const
{
    return m_batteryService;
}
// -----------------------------------------------------------------------------
/*!
    \overload

 */
shared_ptr<BleRcuFindMeService> GattServices::findMeService() const
{
    return m_findMeService;
}

// -----------------------------------------------------------------------------
/*!
    \overload

 */
shared_ptr<BleRcuInfraredService> GattServices::infraredService() const
{
    if(m_infraredServiceIndex >= m_infraredServices.size()) {
        XLOGD_ERROR("gatt infrared service index out of range <%u> max <%u>", m_infraredServiceIndex, m_infraredServices.size());
        return nullptr;
    }
    return m_infraredServices[m_infraredServiceIndex];
}

// -----------------------------------------------------------------------------
/*!
    \overload

     Returns a shared pointer to the upgrade service for this device.

 */
shared_ptr<BleRcuUpgradeService> GattServices::upgradeService() const
{
    if(m_upgradeServiceIndex >= m_upgradeServices.size()) {
        XLOGD_ERROR("gatt upgrade service index out of range <%u> max <%u>", m_upgradeServiceIndex, m_upgradeServices.size());
        return nullptr;
    }
    return m_upgradeServices[m_upgradeServiceIndex];
}

// -----------------------------------------------------------------------------
/*!
    \overload

     Returns a shared pointer to the remote control service for this device.

 */
shared_ptr<BleRcuRemoteControlService> GattServices::remoteControlService() const
{
    return m_remoteControlService;
}
