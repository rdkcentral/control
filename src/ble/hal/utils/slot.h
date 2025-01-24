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
//  slot.h
//

#ifndef SLOT_H
#define SLOT_H

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class SlotBase
{

public:
    SlotBase()
        : m_callbackValid(nullptr)
    { }
    SlotBase(const std::shared_ptr<bool> &callbackValid)
        : m_callbackValid(callbackValid)
    { }
    virtual ~SlotBase()
    { }

public:
    inline void setName(const std::string &name)
    {
        m_name = name;
    }
    inline std::string getName() const
    {
        return m_name;
    }
    inline bool isCallbackValid() const
    {
        if (m_callbackValid) {
            return *m_callbackValid;
        }
        return false;
    }

protected:
    std::string m_name;
    std::shared_ptr<bool> m_callbackValid;
};


template<typename... Args>
class Slot final : public SlotBase
{
public:
    Slot() = default;
    Slot(const std::shared_ptr<bool> &callbackValid, std::function<void(Args...)> callback)
        : SlotBase(callbackValid)
        , m_callback(std::move(callback))
    { }
    ~Slot() = default;

public:
    inline void invokeCallback(Args... args) const
    {
        if (m_callbackValid && *m_callbackValid) {
            m_callback(std::move(args)...);
        }
    }

private:
    std::function<void(Args...)> m_callback;
};

template<typename... Args>
class Slots
{
public:
    Slots() = default;
    ~Slots()
    { }

    Slots(const Slots<Args...>& other)
        : m_slots(other.m_slots)
    { }

public:
    inline void clear()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_slots.clear();
    }

    inline void invoke(Args... args)
    {
        //First, delete any invalid callbacks
        m_lock.lock();
        for (auto it = m_slots.begin(); it != m_slots.end();) {
            if (it->isCallbackValid()) {
                ++it;
            } else {
                it = m_slots.erase(it);
            }
        }

        // make a local copy of slots inside mutex.
        // I'm doing this because a slot callback could invoke other slots or functions
        // that add more slots.  This could be a mutex deadlock risk.
        const std::vector<Slot<Args...>> slots = m_slots;
        m_lock.unlock();

        for(const auto &slot : slots) {
            slot.invokeCallback(args...);
        }
    }

    inline void addSlot(const Slot<Args...> &slot)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_slots.push_back(slot);
    }

private:
    std::vector<Slot<Args...>> m_slots;
    std::mutex m_lock;
};

#endif //SLOT_H
