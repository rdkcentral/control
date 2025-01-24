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
//  blegattnotifypipe.h
//

#ifndef BLEGATTNOTIFYPIPE_H
#define BLEGATTNOTIFYPIPE_H

#include <utils/slot.h>

#include <semaphore.h>
#include <cstdint>
#include <memory>
#include <pthread.h>

#include "bleuuid.h"

typedef struct {
   const char *   name;
   pthread_t      id;
   bool           running;
} ctrlm_thread_t;


class BleGattNotifyPipe
{
public:
    BleGattNotifyPipe(int notfiyPipeFd, uint16_t mtu, BleUuid uuid);
    ~BleGattNotifyPipe();

public:
    bool isValid() const;
    
    void shutdown();

// signals:
    inline void addNotificationSlot(const Slot<const std::vector<uint8_t> &> &func)
    {
        m_notificationSlots.addSlot(func);
    }
    inline void addClosedSlot(const Slot<> &func)
    {
        m_closedSlots.addSlot(func);
    }

    Slots<const std::vector<uint8_t> &> m_notificationSlots;
    Slots<> m_closedSlots;


    bool onActivated();

public:
    std::shared_ptr<bool> m_isAlive;

    int m_pipeFd;
    int m_exitEventFds[2] = {-1,-1};

    size_t m_bufferSize;
    uint8_t *m_buffer;
    BleUuid m_uuid;


    // xr_mq_t m_notifyThreadMsgQ;
    sem_t m_notifyThreadSem;
    ctrlm_thread_t m_notifyThread;
};


#endif // !defined(BLEGATTNOTIFYPIPE_H)
