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
//  futureaggregator.cpp
//

#include "futureaggregator.h"

#include "ctrlm_log_ble.h"

using namespace std;

// -----------------------------------------------------------------------------
/*!
    \class FutureAggregator
    \brief Object that wraps one or more Future objects and returns another
    Future that will only be signaled when all internal Futures complete.

    This is useful if you have a bunch of parallel operations and you only want
    to know when all of them have completed.

    If an error is signal on any of the Futures then the error is stored
    internally and will be signaled once all the futures have completed.
    Only the first error is stored and returned, other errors are discarded.
    If an error occured on any of the internal Futures then the aggregator
    is also considered to be errored.

 */


// -----------------------------------------------------------------------------
/*!
    Constructs the FutureAggregator watching the supplied \a futures list or
    Future objects.

  */
FutureAggregator::FutureAggregator(PendingReply<> &&reply)
    : m_isAlive(make_shared<bool>(true))
    , m_promise(make_shared<PendingReply<>>(std::move(reply)))
{
}


// -----------------------------------------------------------------------------
/*!
    Destructs the FutureAggregator, this does NOT wait for internal futures to
    finish before returning.

    If any internal Future is still running and the aggregator was holding the
    last handle to it then an error will be logged.

 */
FutureAggregator::~FutureAggregator()
{
    *m_isAlive = false;

    // debugging - expect if a promise was created then we have signaled it's completion
    if (m_promise) {
        XLOGD_INFO("destroying incomplete promise");
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Connects to the signals of all the futures aggregated by this object.

 */
PendingReply<> FutureAggregator::connectReply()
{
    PendingReply<> reply(m_isAlive, std::bind(&FutureAggregator::onFutureFinished, this, std::placeholders::_1));
    m_futures.push_back(reply);
    return reply;
}


// -----------------------------------------------------------------------------
/*!
    Returns \c true if no futures have been added to the aggregator.

 */
bool FutureAggregator::isEmpty() const
{
    return m_futures.empty();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if all the futures have finished.

 */
bool FutureAggregator::isFinished() const
{
    // iterate through the list of futures, if they've all finished then
    // emit our finished signal
    unsigned int finishedCount = 0;

    for (const PendingReply<> &future : m_futures) {
        if (future.isFinished()) {
            finishedCount++;
        }
    }
    return (finishedCount == m_futures.size());
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if one or more of the futures is still running.

 */
bool FutureAggregator::isRunning() const
{
    return !isFinished();
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if one or more of the futures has finished with an error.

 */
bool FutureAggregator::isError() const
{
    return !m_errorMessage.empty();
}

void FutureAggregator::setError(std::string message)
{
    m_errorMessage = std::move(message);
}

void FutureAggregator::finish()
{
    onAllFuturesFinished();
}


// -----------------------------------------------------------------------------
/*!
    Returns the message of the first error that occured on a future.  If no
    error occurred then an empty string is returned.

 */
string FutureAggregator::errorMessage() const
{
    return m_errorMessage;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when one of our futures has finished, when this happens we check if
    all the futures have finished and if so emit our finished signal.
    
    If there is an error, we store the error if we don't already have an error.
    This means we only store the first error message received

 */
void FutureAggregator::onFutureFinished(PendingReply<> *reply)
{
    if (reply->isError() && !this->isError()) {
        m_errorMessage = reply->errorMessage();
    }

    // check if all the futures have now finished, if so emit a finished signal from the event queue
    if (isFinished()) {
        onAllFuturesFinished();
    }
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Called when one of our futures has finished, when this happens we check if
    all the futures have no finished and if so emit our finished signal.

 */
void FutureAggregator::onAllFuturesFinished()
{
    XLOGD_DEBUG("all futures finished");

    if (m_promise) {
        if (this->isError()) {
            m_promise->setError(m_errorMessage);
        }
        m_promise->finish();
        m_promise.reset();
    }
}
