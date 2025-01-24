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
//  blergattcharacteristic.h
//

#ifndef BLEGATTCHARACTERISTIC_H
#define BLEGATTCHARACTERISTIC_H


#include "utils/bleuuid.h"
#include "utils/slot.h"
#include "dbus/dbusabstractinterface.h"

#include <memory>
#include <vector>
#include <functional>

class BleGattService;
class BleGattDescriptor;

class BleGattCharacteristic
{
protected:
    BleGattCharacteristic() = default;

public:
    virtual ~BleGattCharacteristic() = default;

public:
    enum Flag {
        Broadcast                   = 0x0001,
        Read                        = 0x0002,
        WriteWithoutResponse        = 0x0004,
        Write                       = 0x0008,
        Notify                      = 0x0010,
        Indicate                    = 0x0020,
        AuthenticatedSignedWrites   = 0x0040,
        ReliableWrite               = 0x0080,
        WritableAuxiliaries         = 0x0100,
        EncryptRead                 = 0x0200,
        EncryptWrite                = 0x0400,
        EncryptAuthenticatedRead    = 0x0800,
        EncryptAuthenticatedWrite   = 0x1000
    };

public:
    virtual bool isValid() const = 0;
    virtual BleUuid uuid() const = 0;
    virtual int instanceId() const = 0;
    virtual uint16_t flags() const = 0;

    virtual void setCacheable(bool cacheable) = 0;
    virtual bool cacheable() const = 0;

    virtual std::vector< std::shared_ptr<BleGattDescriptor> > descriptors() const = 0;
    virtual std::shared_ptr<BleGattDescriptor> descriptor(BleUuid descUuid) const = 0;

    virtual void readValue(PendingReply<std::vector<uint8_t>> &&reply) = 0;
    virtual void writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply) = 0;
    virtual void writeValueWithoutResponse(const std::vector<uint8_t> &value, PendingReply<> &&reply) = 0;
    virtual bool notificationsEnabled() = 0;
    virtual void enableNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) = 0;
    virtual void enableDbusNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) = 0;
    virtual void enablePipeNotifications(const Slot<const std::vector<uint8_t> &> &notifyCB, PendingReply<> &&reply) = 0;
    virtual void disableNotifications() = 0;

    virtual std::vector<uint8_t> readValueSync(std::string &errorMessage) = 0;

    virtual int timeout() = 0;
    virtual void setTimeout(int timeout) = 0;
};


#endif // !defined(BLEGATTCHARACTERISTIC_H)
