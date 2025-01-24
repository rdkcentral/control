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
//  blergattdescriptor.h
//

#ifndef BLEGATTDESCRIPTOR_H
#define BLEGATTDESCRIPTOR_H


#include "utils/bleuuid.h"
#include "utils/pendingreply.h"

#include <vector>
#include <memory>

class BleGattCharacteristic;


class BleGattDescriptor
{
protected:
    BleGattDescriptor() = default;

public:
    virtual ~BleGattDescriptor() = default;

public:
    enum Flag {
        Read = 0x001,
        Write = 0x002,
        EncryptRead = 0x004,
        EncryptWrite = 0x008,
        EncryptAuthenticatedRead = 0x010,
        EncryptAuthenticatedWrite = 0x020
    };

public:
    virtual bool isValid() const = 0;
    virtual BleUuid uuid() const = 0;
    virtual uint16_t flags() const = 0;

    virtual void setCacheable(bool cacheable) = 0;
    virtual bool cacheable() const = 0;

    virtual std::vector<uint8_t> readValueSync(std::string &errorMessage) = 0;
    virtual void readValue(PendingReply<std::vector<uint8_t>> &&reply) = 0;
    virtual void writeValue(const std::vector<uint8_t> &value, PendingReply<> &&reply) = 0;

    virtual int timeout() = 0;
    virtual void setTimeout(int timeout) = 0;

};


#endif // !defined(BLEGATTDESCRIPTOR_H)
