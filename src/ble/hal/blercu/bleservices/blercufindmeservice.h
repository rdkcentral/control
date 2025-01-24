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
//  blercufindmeservice.h
//

#ifndef BLERCUFINDMESERVICE_H
#define BLERCUFINDMESERVICE_H

#include "utils/pendingreply.h"


class BleRcuFindMeService
{
protected:
    BleRcuFindMeService() = default;

public:
    virtual ~BleRcuFindMeService() = default;

public:
    enum State {
        BeepingOff = 0,
        BeepingMid = 1,
        BeepingHigh = 2
    };

    virtual State state() const = 0;

    enum Level {
        Mid = 1,
        High = 2
    };

    virtual void startBeeping(Level level, PendingReply<> &&reply) = 0;
    virtual void stopBeeping(PendingReply<> &&reply) = 0;
};


#endif // !defined(BLERCUFINDMESERVICE_H)
