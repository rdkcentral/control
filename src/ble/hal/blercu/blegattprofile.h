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
//  blegattprofile.h
//

#ifndef BLEGATTPROFILE_H
#define BLEGATTPROFILE_H

#include "utils/bleuuid.h"
#include "utils/slot.h"

#include <vector>
#include <memory>
#include <functional>


class BleGattService;


class BleGattProfile
{
protected:
    BleGattProfile() = default;

public:
    virtual ~BleGattProfile() = default;

public:
    virtual bool isValid() const = 0;
    virtual bool isEmpty() const = 0;

    virtual void updateProfile() = 0;

    virtual std::vector< std::shared_ptr<BleGattService> > services() const = 0;
    virtual std::vector< std::shared_ptr<BleGattService> > services(const BleUuid &serviceUuid) const = 0;
    virtual std::shared_ptr<BleGattService> service(const BleUuid &serviceUuid) const = 0;

    inline void addUpdateCompletedHandler(const Slot<> &func)
    {
        m_updateCompletedSlots.addSlot(func);
    }

    Slots<> m_updateCompletedSlots;
};


#endif // !defined(BLEGATTPROFILE_H)
