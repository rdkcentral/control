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
//  blercuerror.cpp
//

#include "blercuerror.h"
#include <cstdarg>

using namespace std;

BleRcuError::BleRcuError()
    : m_valid(false)
    , m_code(NoError)
{
}

BleRcuError::BleRcuError(ErrorType error)
    : m_valid(true)
    , m_code(error)
{
}

BleRcuError::BleRcuError(ErrorType error, const string &message)
    : m_valid(true)
    , m_code(error)
    , m_message(message)
{
}

BleRcuError::BleRcuError(const BleRcuError &other)
    : m_valid(other.m_valid)
    , m_code(other.m_code)
    , m_message(other.m_message)
{
}

BleRcuError::BleRcuError(BleRcuError &&other)
    : m_valid(other.m_valid)
    , m_code(other.m_code)
    , m_message(std::move(other.m_message))
{
}

BleRcuError &BleRcuError::operator=(const BleRcuError &other)
{
    m_valid = other.m_valid;
    m_code = other.m_code;
    m_message = other.m_message;
    return *this;
}

BleRcuError &BleRcuError::operator=(BleRcuError &&other)
{
    swap(other);
    return *this;
}

void BleRcuError::assign(ErrorType error, const string &message)
{
    m_valid = true;
    m_code = error;
    m_message = message;
}

BleRcuError::ErrorType BleRcuError::type() const
{
    return m_code;
}

string BleRcuError::name() const
{
    return errorString(m_code);
}

string BleRcuError::message() const
{
    return m_message;
}

bool BleRcuError::isValid() const
{
    return m_valid;
}

string BleRcuError::errorString(ErrorType error)
{
    switch (error) {
        case NoError:
            return "com.ble.Error.None";
        case General:
            return "com.ble.Error.Failed";
        case Rejected:
            return "com.ble.Error.Rejected";
        case Busy:
            return "com.ble.Error.Busy";
        case IoDevice:
            return "com.ble.Error.IODevice";
        case InvalidArg:
            return "com.ble.Error.InvalidArgument";
        case FileNotFound:
            return "com.ble.Error.FileNotFound";
        case BadFormat:
            return "com.ble.Error.BadFormat";
        case InvalidHardware:
            return "com.ble.Error.InvalidHardware";
        case NotImplemented:
            return "com.ble.Error.NotImplemented";
        case TimedOut:
            return "com.ble.Error.TimedOut";
        default:
            return "com.ble.Error.Unknown";
    }
}
