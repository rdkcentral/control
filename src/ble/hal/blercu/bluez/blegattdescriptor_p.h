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
//  blegattdescriptor_p.h
//

#ifndef BLUEZ_BLEGATTDESCRIPTOR_P_H
#define BLUEZ_BLEGATTDESCRIPTOR_P_H

#include "../blegattdescriptor.h"
#include "dbus/dbusvariant.h"
#include "interfaces/bluezgattdescriptorinterface.h"


class BluezGattDescriptorInterface;

class BleGattDescriptorBluezProxy
{
public:
    BleGattDescriptorBluezProxy(const GDBusConnection *conn,
                                const std::string &path)
        : m_path(path)
        , m_connection(conn)
    { }
    virtual ~BleGattDescriptorBluezProxy() = default;


    // -----------------------------------------------------------------------------
    /*!
        \fn std::shared_ptr<BluezGattDescriptorInterface> getProxy()

        Initializes the proxy interface to bluez GATT if it hasn't been already and returns
        it.  
        
        This can take some time so it shouldn't be done in the constructor.  
        Instead, this function should be called before each time the 
        proxy is used to ensure that its initialized only when its actually needed.

        Returns \c true if the proxy is already valid or if it was successifully created
    */
    inline std::shared_ptr<BluezGattDescriptorInterface> getProxy()
    {
        if (!m_proxy || !m_proxy->isValid()) {
            // create a dbus proxy for the descriptor object
            m_proxy = std::make_shared<BluezGattDescriptorInterface>("org.bluez", m_path, m_connection);
        }
        return m_proxy;
    }

    const std::string m_path;

private:
    const GDBusConnection *m_connection;
    std::shared_ptr<BluezGattDescriptorInterface> m_proxy;
};


class BleGattDescriptorBluez : public BleGattDescriptor, public BleGattDescriptorBluezProxy {

public:
    BleGattDescriptorBluez(const GDBusConnection *conn,
                           const std::string &path,
                           const DBusPropertiesMap &properties);

    ~BleGattDescriptorBluez() final;

public:
    bool isValid() const override;
    BleUuid uuid() const override;
    uint16_t flags() const override;

    void setCacheable(bool cacheable) override;
    bool cacheable() const override;

    std::vector<uint8_t> readValueSync(std::string &errorMessage) override;
    void readValue(PendingReply<std::vector<uint8_t>> &&reply) override;
    void writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply) override;

    int timeout() override;
    void setTimeout(int timeout) override;


private:
    friend class BleGattProfileBluez;

    std::shared_ptr<bool> m_isAlive;

    std::weak_ptr<BleGattCharacteristic> m_characteristic;
    std::string m_characteristicPath;

    bool m_valid;
    uint16_t m_flags;
    BleUuid m_uuid;

    bool m_cacheable;
    std::vector<uint8_t> m_lastValue;
};


#endif // !defined(BLUEZ_BLEGATTDESCRIPTOR_P_H)
