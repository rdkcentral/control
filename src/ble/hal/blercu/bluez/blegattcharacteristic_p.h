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
//  blegattcharacteristic_p.h
//

#ifndef BLUEZ_BLEGATTCHARACTERISTIC_P_H
#define BLUEZ_BLEGATTCHARACTERISTIC_P_H

#include "../blegattcharacteristic.h"
#include "interfaces/bluezgattcharacteristicinterface.h"


class BleGattNotifyPipe;
class BleGattDescriptorBluez;
class BluezGattCharacteristicInterface;

class BleGattCharacteristicBluezProxy
{
public:
    BleGattCharacteristicBluezProxy(const GDBusConnection *conn,
                                    const std::string &path)
        : m_path(path)
        , m_connection(conn)
    { }
    virtual ~BleGattCharacteristicBluezProxy() = default;


    // -----------------------------------------------------------------------------
    /*!
        \fn std::shared_ptr<BluezGattCharacteristicInterface> getProxy()

        Initializes the proxy interface to bluez GATT if it hasn't been already and returns
        it.  
        
        This can take some time so it shouldn't be done in the constructor.  
        Instead, this function should be called before each time the 
        proxy is used to ensure that its initialized only when its actually needed.

        Returns \c true if the proxy is already valid or if it was successifully created
    */
    inline std::shared_ptr<BluezGattCharacteristicInterface> getProxy()
    {
        if (!m_proxy || !m_proxy->isValid()) {
            // create a dbus proxy for the characteristic object
            m_proxy = std::make_shared<BluezGattCharacteristicInterface>("org.bluez", m_path, m_connection);
        }
        return m_proxy;
    }

    const std::string m_path;

private:
    const GDBusConnection *m_connection;
    std::shared_ptr<BluezGattCharacteristicInterface> m_proxy;
};


class BleGattCharacteristicBluez : public BleGattCharacteristic, public BleGattCharacteristicBluezProxy
{

public:
    BleGattCharacteristicBluez(const GDBusConnection *conn,
                               const std::string &path,
                               const DBusPropertiesMap &properties);
    ~BleGattCharacteristicBluez() final;

public:
    bool isValid() const override;
    BleUuid uuid() const override;
    int instanceId() const override;
    uint16_t flags() const override;

    void setCacheable(bool cacheable) override;
    bool cacheable() const override;


    std::vector< std::shared_ptr<BleGattDescriptor> > descriptors() const override;
    std::shared_ptr<BleGattDescriptor> descriptor(BleUuid descUuid) const override;

    void readValue(PendingReply<std::vector<uint8_t>> &&reply) override;
    void writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply) override;
    void writeValueWithoutResponse(const std::vector<uint8_t> &value, PendingReply<> &&reply) override;

    bool notificationsEnabled() override;
    void enableNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) override;
    void enableDbusNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) override;
    void enablePipeNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) override;
    void disableNotifications() override;


    std::vector<uint8_t> readValueSync(std::string &errorMessage) override;

    int timeout() override;
    void setTimeout(int timeout) override;


// private slots:
    void onNotifyPipeClosed();

private:
    friend class BleGattProfileBluez;

    void addDescriptor(const std::shared_ptr<BleGattDescriptorBluez> &descriptor);

private:
    std::shared_ptr<bool> m_isAlive;

    std::weak_ptr<BleGattService> m_service;
    std::string m_servicePath;

    bool m_valid;
    uint16_t m_flags;
    BleUuid m_uuid;
    int m_instanceId;

    bool m_notifyEnabled;
    std::shared_ptr<BleGattNotifyPipe> m_notifyPipe;

    std::map<BleUuid, std::shared_ptr<BleGattDescriptorBluez>> m_descriptors;
};



#endif // !defined(BLUEZ_BLEGATTCHARACTERISTIC_P_H)
