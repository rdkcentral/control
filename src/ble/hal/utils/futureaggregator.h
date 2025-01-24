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
//  futureaggregator.h
//

#ifndef FUTUREAGGREGATOR_H
#define FUTUREAGGREGATOR_H

#include "pendingreply.h"

#include <vector>

class FutureAggregator
{

public:
    explicit FutureAggregator(PendingReply<> &&reply);
    ~FutureAggregator();

public:
    PendingReply<> connectReply();

    bool isEmpty() const;

    bool isFinished() const;
    bool isRunning() const;
    bool isError() const;

    void setError(std::string message);

    void finish();

    std::string errorMessage() const;


private:
    void onFutureFinished(PendingReply<> *reply);

    void onAllFuturesFinished();

private:
    std::shared_ptr<bool> m_isAlive;

    std::vector< PendingReply<> > m_futures;

    std::string m_errorMessage;

    std::shared_ptr< PendingReply<> > m_promise;
};



#endif // !defined(FUTUREAGGREGATOR_H)
