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
//  blercuerror.h
//

#ifndef BLERCUERROR_H
#define BLERCUERROR_H

#include <string>
#include <utility>

class BleRcuError
{
public:
    enum ErrorType {
        NoError = 0,
        General,
        Rejected,
        Busy,
        IoDevice,
        InvalidArg,
        FileNotFound,
        BadFormat,
        InvalidHardware,
        NotImplemented,
        TimedOut,

        // don't use this one!
        LastErrorType = BadFormat
    };

    BleRcuError();
    BleRcuError(ErrorType error);
    BleRcuError(ErrorType error, const std::string &message);
    ~BleRcuError() = default;
    
    BleRcuError(const BleRcuError &other);
    BleRcuError(BleRcuError &&other);

    BleRcuError &operator=(const BleRcuError &other);
    BleRcuError &operator=(BleRcuError &&other);

    explicit operator bool() const noexcept { return (m_code != NoError); }
    bool     operator !()    const noexcept { return (m_code == NoError); }

    void assign(ErrorType error, const std::string &message = std::string());

public:
    void swap(BleRcuError &other)
    {
        std::swap(m_valid,   other.m_valid);
        std::swap(m_code,    other.m_code);
        std::swap(m_message, other.m_message);
    }

    ErrorType type() const;
    std::string name() const;
    std::string message() const;
    bool isValid() const;

    static std::string errorString(ErrorType error);

private:
    bool m_valid;
    ErrorType m_code;
    std::string m_message;
};

#endif // !defined(BLERCUERROR_H)
