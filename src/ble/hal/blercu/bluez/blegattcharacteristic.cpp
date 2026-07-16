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
//  blegattcharacteristic.cpp
//

#include "blegattcharacteristic_p.h"
#include "blegattdescriptor_p.h"
#include "blegattnotifypipe.h"
// #include "blegatthelpers.h"
#include "blercu/blercuerror.h"


#include <algorithm>

using namespace std;

// -----------------------------------------------------------------------------
/*!
    \internal

    Constructs a new object parsing the details of the GATT characteristic from
    the dictionary received from bluez over dbus.

    The following is an example of the \a properties we get over dbus when a
    service is found

    \startcode

        dict entry(
            string "org.bluez.GattCharacteristic1"
            array [
                dict entry(
                    string "UUID"
                    variant                   string "00020001-bdf0-407c-aaff-d09967f31acd"
                )
                dict entry(
                    string "Service"
                    variant                   object path "/org/bluez/hci0/dev_1C_A2_B1_BE_EF_02/service003d"
                )
                dict entry(
                    string "Value"
                        variant                   array [
                        ]
                )
                dict entry(
                    string "Flags"
                    variant                   array [
                        string "read"
                        string "write"
                    ]
                )
            ]
        )

    \endcode
 */
BleGattCharacteristicBluez::BleGattCharacteristicBluez(const GDBusConnection *conn,
                                                       const std::string &path,
                                                       const DBusPropertiesMap &properties)
    : BleGattCharacteristicBluezProxy(conn, path)
    , m_isAlive(make_shared<bool>(true))
    , m_valid(false)
    , m_flags(0)
    , m_instanceId(0)
    , m_notifyEnabled(false)
{
    // get the uuid of the characteristic
    string uuid_str;
    DBusPropertiesMap::const_iterator property = properties.find("UUID");
    if ((property == properties.end()) || !property->second.toString(uuid_str)) {
        XLOGD_WARN("invalid uuid property of gatt characteristic <%s>", m_path.c_str());
        return;
    }
    m_uuid = BleUuid(uuid_str);

    // get the parent service object path (used for constructing the tree)
    property = properties.find("Service");
    if ((property == properties.end()) || !property->second.toObjectPath(m_servicePath)) {
        XLOGD_WARN("failed to get the service path of the characteristic with uuid: %s", m_uuid.toString().c_str());
    }

    // the flags are a string array
    std::vector<std::string> flagsList;
    property = properties.find("Flags");
    if ((property == properties.end()) || !property->second.toStringList(flagsList)) {
        XLOGD_WARN("invalid flags of gatt characteristic with uuid: %s", m_uuid.toString().c_str());
    } else {

        // possible flags
        static const map<string, uint16_t> flagsMap = {
            { string("broadcast"),                   Broadcast },
            { string("read"),                        Read },
            { string("write-without-response"),      WriteWithoutResponse },
            { string("write"),                       Write },
            { string("notify"),                      Notify },
            { string("indicate"),                    Indicate },
            { string("authenticated-signed-writes"), AuthenticatedSignedWrites },
            { string("reliable-write"),              ReliableWrite },
            { string("writable-auxiliaries"),        WritableAuxiliaries },
            { string("encrypt-read"),                EncryptRead },
            { string("encrypt-write"),               EncryptWrite },
            { string("encrypt-authenticated-read"),  EncryptAuthenticatedRead },
            { string("encrypt-authenticated-write"), EncryptAuthenticatedWrite },
        };

        // parse all the flags
        for (string &flag : flagsList) {
              std::transform(flag.begin(), flag.end(), flag.begin(), ::tolower);
            map<string, uint16_t>::const_iterator it = flagsMap.find(flag);
            if (it == flagsMap.end()) {
                XLOGD_WARN("unknown flag (%s) for gatt characteristic %s", flag.c_str(), m_uuid.toString().c_str());
            } else {
                m_flags |= it->second;
            }
        }
    }

    // the instance id is used to distinguish between different instances of the
    // same characteristic, for bluez we use the dbus path id, ie. a typical dbus
    // path would be '/org/bluez/hci0/dev_D4_B8_FF_11_76_EE/service0043/char004c' we
    // trim off the last bit and parse the 004c as the instance id
      size_t found = path.find_last_of("/");
    string serviceId = path.substr(found+1);
    if (sscanf(serviceId.c_str(), "char%04x", &m_instanceId) != 1) {
        XLOGD_WARN("failed to parse characteristic '%s' to get the instance id", serviceId.c_str());
        m_instanceId = -1;
    }

    m_valid = true;
}

// -----------------------------------------------------------------------------
/*!

 */
BleGattCharacteristicBluez::~BleGattCharacteristicBluez()
{
    *m_isAlive = false;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattCharacteristic::isValid() const

    Returns \c true if the object was successifully created from the java / JNI
    class object.

 */
bool BleGattCharacteristicBluez::isValid() const
{
    return m_valid;
}


// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattCharacteristic::timeout()

    Returns the current value of the timeout in milliseconds. \c -1 means the
    default timeout (usually 25 seconds).

 */
int BleGattCharacteristicBluez::timeout()
{
    auto proxy = getProxy();
    
    if (proxy && proxy->isValid()) {
        return proxy->timeout();
    } else {
        return -1;
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattCharacteristic::setTimeout(int timeout)

    Sets the timeout in milliseconds for all future DBus calls to timeout. \c -1
    means the default timeout (usually 25 seconds).

 */
void BleGattCharacteristicBluez::setTimeout(int timeout)
{
    auto proxy = getProxy();
    
    if (proxy && proxy->isValid()) {
        if (timeout < 0) {
            proxy->setTimeout(-1);
        } else {
            proxy->setTimeout(std::max(1000, std::min(timeout, 60000)));
        }
    }
}

// -----------------------------------------------------------------------------
/*!
    \fn BleUuid BleGattCharacteristic::uuid() const

    Returns the UUID of the characteristic.  If the characteristic is not valid
    the returned value is undefined.

 */
BleUuid BleGattCharacteristicBluez::uuid() const
{
    return m_uuid;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleUuid BleGattCharacteristic::instanceId() const

    Returns the instance ID for this characteristic. If a remote device offers
    multiple characteristics with the same UUID, the instance ID is used to
    distinguish between characteristics.

 */
int BleGattCharacteristicBluez::instanceId() const
{
    return m_instanceId;
}

// -----------------------------------------------------------------------------
/*!
    \fn BleGattCharacteristic::Flags BleGattCharacteristic::flags() const

    Returns the properties and permission flags for the characteristic. If the
    characteristic is not valid the returned value is undefined.

 */
uint16_t BleGattCharacteristicBluez::flags() const
{
    return m_flags;
}

// -----------------------------------------------------------------------------
/*!
    \fn vector< shared_ptr<BleGattDescriptor> > BleGattCharacteristic::descriptors() const

    Returns a list of all the descriptors attached to this characteristic.

 */
vector< shared_ptr<BleGattDescriptor> > BleGattCharacteristicBluez::descriptors() const
{
    vector< shared_ptr<BleGattDescriptor> > descriptors;
    descriptors.reserve(m_descriptors.size());

    for (auto const &descriptor : m_descriptors) {
        descriptors.push_back(descriptor.second);
    }

    return descriptors;
}

// -----------------------------------------------------------------------------
/*!
    \fn shared_ptr<BleGattDescriptor> BleGattCharacteristic::descriptor(BleUuid descUuid) const

    Returns the shared pointer to the descriptor with the given UUID. If no
    descriptor exists then a null shared pointer is returned.


 */
shared_ptr<BleGattDescriptor> BleGattCharacteristicBluez::descriptor(BleUuid descUuid) const
{
      auto it = m_descriptors.find(descUuid);
    if (it != m_descriptors.end()) {
        return static_pointer_cast<BleGattDescriptor>(it->second);
    } else {
        return nullptr;
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Adds the descriptor to the characteristic, basically just inserts it into the
    internal multimap.

 */
void BleGattCharacteristicBluez::addDescriptor(const shared_ptr<BleGattDescriptorBluez> &descriptor)
{
    m_descriptors[descriptor->uuid()] = descriptor;
}

// -----------------------------------------------------------------------------
/*!
    \fn void BleGattCharacteristic::setCacheable(bool cacheable)

    Sets whether this characteristic is \a cacheable; cacheable characteristics
    store the last read / written value and on a new read won't actually send a
    request to the remote device, instead will return the last read / written
    value.

    By default the cacheable property is \c false.

 */
void BleGattCharacteristicBluez::setCacheable(bool cacheable)
{
    XLOGD_WARN("cacheable not yet implemented for characteristic");
}

// -----------------------------------------------------------------------------
/*!
    \fn bool BleGattDescriptor::cacheable() const

    Returns the current cacheable property value.

    \see BleGattDescriptor::setCacheable()
 */
bool BleGattCharacteristicBluez::cacheable() const
{
    XLOGD_WARN("cacheable not yet implemented for characteristic");
    return false;
}



std::vector<uint8_t> BleGattCharacteristicBluez::readValueSync(std::string &errorMessage) 
{
    std::vector<uint8_t> ret;
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        errorMessage = "no proxy connection";
        XLOGD_ERROR("failed: <%s>", errorMessage.c_str());
        return ret;
    }
    return proxy->ReadValueSync();
}


// -----------------------------------------------------------------------------
/*!
    \fn  BleGattCharacteristic::readValue()

    Requests a read on the characteristic. This is an async operation, the
    result is returned in the PendingReply callback.

 */
void BleGattCharacteristicBluez::readValue(PendingReply<std::vector<uint8_t>> &&reply)
{
    std::vector<uint8_t> ret;
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }
    proxy->ReadValue(std::move(reply));
}


// -----------------------------------------------------------------------------
/*!
    \fn Future<void> BleGattCharacteristic::writeValue(const QByteArray &value)

    Requests a write on the characteristic. This is an async operation, the
    result is returned in the Future object.

 */
void BleGattCharacteristicBluez::writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply)
{
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }
    proxy->WriteValue(value, std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    \fn Future<void> BleGattCharacteristic::writeValueWithoutResponse(const QByteArray &value)

    Requests a writeWithoutResponse (aka WRITE_CMD) on the characteristic. This
    is an async operation, the result is returned in the Future object.

 */
void BleGattCharacteristicBluez::writeValueWithoutResponse(const std::vector<uint8_t> &value, 
                                                           PendingReply<> &&reply)
{
    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }

    // send the read request and put the reply on in future
    proxy->WriteValueWithouResponse(value, std::move(reply));
}


bool BleGattCharacteristicBluez::notificationsEnabled()
{
    return m_notifyEnabled;
}


void BleGattCharacteristicBluez::disableNotifications()
{
    // sanity check notifications are supported
    if (!(m_flags & Notify)) {
        XLOGD_ERROR("notifications not supported for %s", m_uuid.toString().c_str());
        return;
    }

    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        XLOGD_ERROR("no proxy connection for %s", m_uuid.toString().c_str());
        return;
    }

    // check if notifications are already disabled
    if (!m_notifyEnabled) {
        return;
    }

    if (m_notifyPipe) {
        // If we used AcquireNotify to get a pipe we just need to close it.
        m_notifyPipe->shutdown();
        m_notifyPipe.reset();
        XLOGD_INFO("closed notification pipe for %s", m_uuid.toString().c_str());
    } else {
        // if using dbus notifications, send StopNotify message to bluez
        proxy->StopNotifySync();
    }
    
    m_notifyEnabled = false;
}


void BleGattCharacteristicBluez::enableNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, 
                                                         PendingReply<> &&reply)
{
    // use pipe notifications by default because of the following:
    // DBUS notifications have been seen in practice to send a notification when
    // simply reading the value of a characteristic.  This can be problematic for 
    // characteristics that we only want to be notified of changes to.
    enablePipeNotifications(notifyCB, std::move(reply));
}

// -----------------------------------------------------------------------------
/*!
    \fn Future<void> BleGattCharacteristic::enableDbusNotifications(bool enable)

    Request notifications be enabled or disabled on the characteristic.

    Bluez sets the CCCD value in the descriptor to tell the remote device to
    send notifications automatically.

    This uses DBUS notifications, which in practice will send a notification when
    simply reading the value of a characteristic.  This can be problematic for 
    characteristics that we only want to be notified of changes to.

 */
void BleGattCharacteristicBluez::enableDbusNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, 
                                                         PendingReply<> &&reply)
{
    // sanity check notifications are supported
    if (!(m_flags & Notify)) {
        reply.setError("notifications not supported for this characteristic");
        reply.finish();
        return;
    }

    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }

    // check if notifications are already enabled
    if (m_notifyEnabled) {
        XLOGD_WARN("notifications already enabled for %s, not enabling again...", m_uuid.toString().c_str());
        reply.finish();
        return;
    }

    proxy->StartNotify(notifyCB, std::move(reply));
    m_notifyEnabled = true;
}


// -----------------------------------------------------------------------------
/*!
    \fn Future<void> BleGattCharacteristic::enablePipeNotifications(bool enable)

    Request notifications be enabled or disabled on the characteristic.

    Bluez sets the CCCD value in the descriptor to tell the remote device to
    send notifications automatically.

 */
void BleGattCharacteristicBluez::enablePipeNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB,
                                                        PendingReply<> &&reply)
{
    // sanity check notifications are supported
    if (!(m_flags & Notify)) {
        reply.setError("notifications not supported for this characteristic");
        reply.finish();
        return;
    }

    auto proxy = getProxy();
    
    if (!proxy || !proxy->isValid()) {
        reply.setError("no proxy connection");
        reply.finish();
        return;
    }

    // check if notifications are already enabled / disabled
    if (m_notifyEnabled) {
        XLOGD_WARN("notifications already enabled for %s, not enabling again...", m_uuid.toString().c_str());
        reply.finish();
        return;
    }


    // lambda invoked when the request returns
    auto convertVariant = [this, notifyCB, reply](PendingReply<DBusVariant> *dbusReply) mutable
        {
            reply.setName(dbusReply->getName());
            if (dbusReply->isError()) {
                reply.setError(dbusReply->errorMessage());
                reply.finish();
                return;
            }

            int32_t fdIndex, pipeFd;
            uint16_t mtu;
            g_variant_get(dbusReply->result().getGVariant(), "(hq)", &fdIndex, &mtu);

            if (!dbusReply->result().getFd(fdIndex, pipeFd)) {

                // signal the client that we failed
                reply.setError("invalid notify pipe fd from bluez");
                reply.finish();
                return;
            }

            if (mtu < 23) {
                close(pipeFd);

                XLOGD_ERROR("invalid MTU size on the notify pipe (%hd bytes)", mtu);
                reply.setError("Invalid MTU size from bluez");
                reply.finish();
                return;
            }

            if (m_notifyPipe) {
                m_notifyPipe.reset();
            }

            // wrap the file descriptor in a new GATT notification pipe object
            m_notifyPipe = make_shared<BleGattNotifyPipe>(pipeFd, mtu, m_uuid);

            //BleGattNotifyPipe dups the pipe, so close the original here.
            close(pipeFd);

            if (!m_notifyPipe || !m_notifyPipe->isValid()) {
                m_notifyPipe.reset();

                // signal the client that we failed
                reply.setError("failed to create BleGattNotifyPipe object");
                reply.finish();
                return;
            }

            m_notifyEnabled = true;

            m_notifyPipe->addNotificationSlot(notifyCB);

            m_notifyPipe->addClosedSlot(Slot<>(m_isAlive,
                    std::bind(&BleGattCharacteristicBluez::onNotifyPipeClosed, this)));

            reply.finish();
        };

    // request a notification pipe from bluez
    proxy->AcquireNotify(PendingReply<DBusVariant>(m_isAlive, convertVariant));
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the notification pipe is closed, bluez does this if the remote
    device disconnects.

 */
void BleGattCharacteristicBluez::onNotifyPipeClosed()
{
    // Nothing to be done here.  This callback is invoked from inside the 
    // m_notifyPipe object itself, which will get cleaned up later
}
