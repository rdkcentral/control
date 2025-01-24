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
//  blegattprofile.cpp
//

#include "blegattprofile_p.h"
#include "blegattservice_p.h"
#include "blegattcharacteristic_p.h"
#include "blegattdescriptor_p.h"

#include "interfaces/bluezadapterinterface.h"
#include "interfaces/bluezgattserviceinterface.h"
#include "interfaces/bluezgattcharacteristicinterface.h"
#include "interfaces/bluezgattdescriptorinterface.h"

#include "dbus/dbusobjectmanager.h"
#include "ctrlm_log_ble.h"

using namespace std;

static gboolean notifyUpdateCompleted(gpointer user_data);


BleGattProfileBluez::BleGattProfileBluez(const GDBusConnection *bluezDBusConn,
                                         const std::string &bluezDBusPath)
    : m_isAlive(make_shared<bool>(true))
    , m_dbusConn(bluezDBusConn)
    , m_dbusPath(bluezDBusPath)
    // , m_bluezVersion(5, 48)
    , m_valid(false)
{
    m_valid = true;
}

BleGattProfileBluez::~BleGattProfileBluez()
{
    *m_isAlive = false;
}

// -----------------------------------------------------------------------------
/*!
    \overload


 */
bool BleGattProfileBluez::isValid() const
{
    return m_valid;
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Returns \c true if the profile doesn't contain any services.

 */
bool BleGattProfileBluez::isEmpty() const
{
    return m_services.empty();
}

// -----------------------------------------------------------------------------
/*!
    \overload

    Called from the GattServices class when it is restarting services and wants
    the profile.  However on Android we always keep the profile in sync so there
    is no need to do anything here, except clients expect that the updateComplete
    signal is emitted once done.

 */
void BleGattProfileBluez::updateProfile()
{
    // clear the old data first, will also clean-up any slot receiver details
    // that haven't yet been called
    m_services.clear();

    DBusObjectManagerInterface bluezObjectMgr("org.bluez", "/", m_dbusConn);

    bluezObjectMgr.GetManagedObjects(PendingReply<DBusManagedObjectList>(m_isAlive,
            std::bind(&BleGattProfileBluez::onGetObjectsReply, this, std::placeholders::_1)));
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when a reply (or timeout error) is received to our query to get the
    objects in the bluez dbus tree.

 */
void BleGattProfileBluez::onGetObjectsReply(PendingReply<DBusManagedObjectList> *reply)
{
    // check for an error and log it
    if (reply->isError()) {
        XLOGD_ERROR("failed to get bluez object list, error <%s>", reply->errorMessage().c_str());
        return;
    }

    const string adapterInterfaceName = BluezAdapterInterface::staticInterfaceName();

    const DBusManagedObjectList &objects = reply->result();
    const string bluezDevicePathStr = m_dbusPath;

    // we are looking for objects that have the following interfaces
    //   org.bluez.GattService1
    //   org.bluez.GattCharacteristic1
    //   org.bluez.GattDescriptor1
    //
    // we initial scan everything into a flat list, then we populate the tree;
    // services first, then characteristics and finally descriptors

    vector< std::shared_ptr<BleGattCharacteristicBluez> > characteristics;
    vector< std::shared_ptr<BleGattDescriptorBluez> > descriptors;

    characteristics.reserve(50);
    descriptors.reserve(50);

    DBusManagedObjectList::const_iterator object = objects.begin();
    for (; object != objects.end(); ++object) {

        // get the object path and interfaces
        const string &path = object->first;
        const DBusInterfaceList &interfaces = object->second;

        // if the object contains the "org.bluez.Adapter1" interface then
        // we read it so that we can get the version of bluez, this is needed
        // later when using some of the GATT APIs
        // if (interfaces.contains(adapterInterfaceName))
        // updateBluezVersion(interfaces[adapterInterfaceName]);

        // check the object path is under the one we are looking for, i.e. the
        // object belongs to this rcu device
        if (path.find(bluezDevicePathStr) == std::string::npos) {
            continue;
        }

        DBusInterfaceList::const_iterator interface = interfaces.begin();
        for (; interface != interfaces.end(); ++interface) {

            // get the interface name and properties
            const string &name = interface->first;
            const DBusPropertiesMap &properties = interface->second;

            // check if a service
            if (name == BluezGattServiceInterface::staticInterfaceName()) {

                shared_ptr<BleGattServiceBluez> service = make_shared<BleGattServiceBluez>(m_dbusConn, path, properties);
                if (service && service->isValid()) {
                    m_services.insert(pair<BleUuid,shared_ptr<BleGattServiceBluez>>(service->uuid(), service));
                } else {
                    XLOGD_ERROR("failed to create BleGattServiceBluez object");
                }

            // check is a characteristic
            } else if (name == BluezGattCharacteristicInterface::staticInterfaceName()) {

                shared_ptr<BleGattCharacteristicBluez> characteristic = 
                        make_shared<BleGattCharacteristicBluez>(m_dbusConn, path, properties);
                if (characteristic && characteristic->isValid()) {
                    characteristics.push_back(characteristic);
                } else {
                    XLOGD_ERROR("failed to create BleGattCharacteristicBluez object");
                }

            // check if a descriptor
            } else if (name == BluezGattDescriptorInterface::staticInterfaceName()) {

                shared_ptr<BleGattDescriptorBluez> descriptor =
                        make_shared<BleGattDescriptorBluez>(m_dbusConn, path, properties);
                if (descriptor && descriptor->isValid()) {
                    descriptors.push_back(descriptor);
                } else {
                    XLOGD_ERROR("failed to create BleGattDescriptorBluez object");
                }
            }
        }
    }


    // add the descriptors to their parent characteristic
    for (const shared_ptr<BleGattDescriptorBluez> &descriptor : descriptors) {

        // get the path to the parent characteristic in string form
        const string &parentPath = descriptor->m_characteristicPath;

        // find the parent characteristic
        bool found = false;
        for (shared_ptr<BleGattCharacteristicBluez> &characteristic : characteristics) {
            if (parentPath == characteristic->m_path) {
                descriptor->m_characteristic = characteristic;
                characteristic->addDescriptor(descriptor);
                found = true;
                break;
            }
        }

        // check we found the parent characteristic, if not something has gone wrong in bluez
        if (!found) {
            XLOGD_WARN("failed to find parent gatt characteristic for descriptor %s@%s",
                       descriptor->m_uuid.toString().c_str(), descriptor->m_path.c_str());
        }
    }


    // and then add the characteristics to their parent services
    for (const shared_ptr<BleGattCharacteristicBluez> &characteristic : characteristics) {

        // for each characteristic update the bluez version so the correct
        // version of the dbus API is used
        // characteristic->setBluezVersion(m_bluezVersion);

        // get the path to the parent characteristic in string form
        const string &parentPath = characteristic->m_servicePath;

        // find the parent service
        bool found = false;
        for (auto &service : m_services) {
            if (parentPath == service.second->m_path) {
                characteristic->m_service = service.second;
                service.second->addCharacteristic(characteristic);
                found = true;
                break;
            }
        }

        // check we found the parent service, if not something has gone wrong in bluez
        if (!found) {
            XLOGD_WARN("failed to find parent gatt service for characteristic %s@%s",
                       characteristic->m_uuid.toString().c_str(), characteristic->m_path.c_str());
        }
    }

    // (we shouldn't have but as a final sanity) check that the services we have all list 
    // their device as the one we're targeting. We remove any that don't and log an error
    multimap<BleUuid, shared_ptr<BleGattServiceBluez>>::iterator it = m_services.begin();
    while (it != m_services.end()) {

        const shared_ptr<BleGattServiceBluez> &service = it->second;

        if (service->m_devicePath != m_dbusPath) {
            XLOGD_WARN("service with uuid %s@%s unexpectedly does not belong to the target device",
                       service->m_uuid.toString().c_str(), service->m_devicePath.c_str());
            it = m_services.erase(it);
        } else {
            ++it;
        }
    }

    dumpGattTree();

    // finally finished so notify the original caller that all objects are fetched, 
    // use a singleShot timer so the event is triggered from the main event loop
    g_timeout_add(0, notifyUpdateCompleted, this);
    
}

static gboolean notifyUpdateCompleted(gpointer user_data)
{
    BleGattProfileBluez *profile = (BleGattProfileBluez*)user_data;
    if (profile == nullptr) {
        XLOGD_ERROR("user_data is NULL!!!!!!!!!!");
    } else {
        profile->m_updateCompletedSlots.invoke();
    }
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Scans the dbus properties of the org.bluez.Adapter1 interface for the
    "Modalias" property and then reads the value to determine the version
    of bluez we are using.

    This is needed to determine the API used for some of the GATT operations;
    the API changed between 5.47 and 5.48 but the interface version number
    didn't ... hence need to know which version we're talking to.

 */
#if 0
void BleGattProfileBluez::updateBluezVersion(const DBusPropertiesMap &properties)
{
    const QVariant modalias = properties.value(stringLiteral("Modalias"));
    if (modalias.isNull() || !modalias.canConvert<string>()) {
        qWarning("failed to get 'Modalias' property");
        return;
    }

    const string strModalias = modalias.toString();

    char type[64];
    int vendor, product, version;

    int fields = sscanf(qPrintable(strModalias), "%63[^:]:v%04Xp%04Xd%04X",
                        type, &vendor, &product, &version);
    if (fields != 4) {
        qWarning("failed to parse 'Modalias' property value '%s'",
                 qPrintable(strModalias));

    } else if ((vendor != 0x1d6b) || (product != 0x0246)) {
        qWarning("invalid vendor (0x%04x) or product (0x%04x) 'Modalias' value",
                 vendor, product);

    } else if ((version >> 8) != 5) {
        qWarning("unexpected 'Modalias' major version number (0x%04x)",
                 version);

    } else {
        m_bluezVersion = QVersionNumber((version >> 8), (version & 0xff));
        qDebug("found bluez version '%s'", qPrintable(m_bluezVersion.toString()));
    }
}
#endif
// -----------------------------------------------------------------------------
/*!
    Debugging function that dumps out details on the services, characteristics
    & descriptors found in the last update.

 */
void BleGattProfileBluez::dumpGattTree()
{
    if (xlog_level_get(XLOG_MODULE_ID) <= XLOG_LEVEL_DEBUG) {

        for (auto const &service : m_services) {
        
            XLOGD_DEBUG("+-- Service: %s",service.second->uuid().toString().c_str());
            XLOGD_DEBUG(".   +-- Path: %s", service.second->m_path.c_str());
            XLOGD_DEBUG(".   +-- Primary: %s", service.second->primary() ? "true" : "false");
            XLOGD_DEBUG(".   +-- InstanceId: 0x%x", service.second->instanceId());

            for (auto const &characteristic : service.second->m_characteristics) {

                XLOGD_DEBUG(".   +-- Characteristic: %s", characteristic.second->uuid().toString().c_str());
                XLOGD_DEBUG(".   .   +-- Path: %s", characteristic.second->m_path.c_str());
                XLOGD_DEBUG(".   .   +-- Flags: 0x%X", characteristic.second->flags());
                XLOGD_DEBUG(".   .   +-- InstanceId: 0x%x", characteristic.second->instanceId());

                for (auto const &descriptor : characteristic.second->m_descriptors) {

                    XLOGD_DEBUG(".   .   +-- Descriptor: %s", descriptor.second->uuid().toString().c_str());
                    XLOGD_DEBUG(".   .   .   +-- Path: %s", descriptor.second->m_path.c_str());
                    XLOGD_DEBUG(".   .   .   +-- Flags: 0x%X", descriptor.second->flags());
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
/*!


 */
std::vector< std::shared_ptr<BleGattService> > BleGattProfileBluez::services() const
{
    vector< shared_ptr<BleGattService> > services;
    services.reserve(m_services.size());

    for (auto const &service : m_services)
        services.push_back(service.second);

    return services;
}

// -----------------------------------------------------------------------------
/*!


 */
std::vector< std::shared_ptr<BleGattService> > BleGattProfileBluez::services(const BleUuid &serviceUuid) const
{
    vector< shared_ptr<BleGattService> > services;
    services.reserve(m_services.size());

    for (auto const &service : m_services) {
        if (service.first == serviceUuid)
            services.push_back(static_pointer_cast<BleGattService>(service.second));
    }

    return services;
}

// -----------------------------------------------------------------------------
/*!


 */
shared_ptr<BleGattService> BleGattProfileBluez::service(const BleUuid &serviceUuid) const
{
    auto it = m_services.find(serviceUuid);
    if (it != m_services.end()) {
        return static_pointer_cast<BleGattService>(it->second);
    } else {
        return nullptr;
    }
}

