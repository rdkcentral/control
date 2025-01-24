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
//  pendingreply.h
//

#ifndef PENDINGREPLY_H
#define PENDINGREPLY_H


#include <string>
#include <functional>
#include <memory>

class PendingReplyBase
{

public:
    PendingReplyBase()
        : m_name(std::string())
        , m_errorMessage(std::string())
        , m_finished(std::make_shared<bool>(false))
    { }
    virtual ~PendingReplyBase() = default;

public:
    inline void setName(const std::string &name)
    {
        m_name = name;
    }
    std::string getName() const
    {
        return m_name;
    }

    inline bool isError() const
    {
        return !m_errorMessage.empty();
    }
    inline void setError(const std::string &message)
    {
        m_errorMessage = message;
    }
    std::string errorMessage() const
    {
        return m_errorMessage;
    }
    bool isFinished() const
    {
        return *m_finished;
    }


protected:
    std::string m_name;
    std::string m_errorMessage;

    std::shared_ptr<bool> m_finished;
};


template <typename T = void>
class PendingReply final : public PendingReplyBase
{
public:
    PendingReply()
        : m_callbackValid(nullptr)
    { }
    PendingReply(const std::shared_ptr<bool> &callbackValid, std::function<void(PendingReply<T> *)> callback)
        : m_callbackValid(callbackValid)
        , m_callback(std::move(callback))
    { }
    ~PendingReply()
    { }

public:
    inline void finish()
    {
        *m_finished = true;
        if (m_callbackValid && *m_callbackValid) {
            m_callback(this);
        }
    }

    inline void setResult(const T &result)
    {
        m_result = result;
    }

    inline T result() const
    {
        return m_result;
    }


private:
    T m_result{};

    std::shared_ptr<bool> m_callbackValid;
    std::function<void(PendingReply<T> *)> m_callback;
};


template <>
class PendingReply<void> final : public PendingReplyBase
{
public:
    PendingReply()
        : m_callbackValid(nullptr)
    { }
    PendingReply(const std::shared_ptr<bool> &callbackValid, std::function<void(PendingReply<void> *)> callback)
        : m_callbackValid(callbackValid)
        , m_callback(std::move(callback))
    { }
    ~PendingReply()
    { }

public:
    inline void finish()
    {
        *m_finished = true;
        if (m_callbackValid && *m_callbackValid) {
            m_callback(this);
        }
    }

private:
    std::shared_ptr<bool> m_callbackValid;
    std::function<void(PendingReply<void> *)> m_callback;
};


#endif //PENDINGREPLY_H
