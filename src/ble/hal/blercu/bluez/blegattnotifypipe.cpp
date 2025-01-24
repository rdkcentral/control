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
//  blegattnotifypipe.cpp
//

#include "blegattnotifypipe.h"
#include "ctrlm_log_ble.h"


#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <algorithm>


using namespace std;

#define FD_SIGNAL(x) (x[1])
#define FD_RECV(x) (x[0])


void *NotifyThread(void *data);

static bool ThreadCreate(ctrlm_thread_t *thread, void *(*start_routine)(void *), void *arg, pthread_attr_t *attr = NULL)
{
    thread->running = false;
    if (0 != pthread_create(&thread->id, attr, start_routine, arg)) {
        XLOGD_ERROR("unable to launch thread <%s>", thread->name == NULL ? "" : thread->name);
        return (false);
    }

    if (thread->name != NULL) {
        char name_max[16];
        snprintf(name_max, sizeof(name_max), "%s", thread->name);
        if (pthread_setname_np(thread->id, name_max) != 0) {
            XLOGD_WARN("pthread_setname_np() failed to set name of thread to <%s>", name_max);
        }
    }

    thread->running = true;
    return (true);
}

static bool ThreadJoin(ctrlm_thread_t *thread, uint32_t timeout_secs)
{
    if (!thread->running) {
        XLOGD_WARN("Thread <%s> not running.", thread->name);
        return (true);
    }

    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);
    end_time.tv_sec += timeout_secs;

    int ret;
    if (0 != (ret = pthread_timedjoin_np(thread->id, NULL, &end_time))) {
        XLOGD_ERROR("Thread <%s> join timed-out, not waiting any longer for it to exit, ret = %d, %s",
             thread->name, ret, ret == EDEADLK ? "EDEADLK" : ret == ETIMEDOUT ? "ETIMEDOUT" : "UNKNOWN");
        return false;
    }

    XLOGD_DEBUG("Thread <%s> join successful.", thread->name);
    thread->running = false;
    return (true);
}

static void ClearEventFd(int &fd) {
    if(fd > -1) {
        uint64_t d;
        if((read(fd, &d, sizeof(uint64_t))) != sizeof(uint64_t)) {
            XLOGD_ERROR("failed to clear eventfd");
        }
    } else {
        XLOGD_ERROR("invalid fd");
    }
}

static void SignalEventFd(int &fd) {
    if(fd > -1) {
        uint64_t d = 1;
        if((write(fd, &d, sizeof(uint64_t))) != sizeof(uint64_t)) {
            XLOGD_ERROR("failed to signal eventfd");
        }
    } else {
        XLOGD_ERROR("invalid fd");
    }
}
// -----------------------------------------------------------------------------
/*!
    Constructs a BleGattNotifyPipe object wrapping the supplied \a notifyPipeFd
    descriptor.  The \a mtu value describes the maximum transfer size for
    each notification.

    The class dup's the supplied descriptor so it should / can be closed after
    construction.

 */
BleGattNotifyPipe::BleGattNotifyPipe(int notifyPipeFd, uint16_t mtu, BleUuid uuid)
    : m_isAlive(make_shared<bool>(true))
    , m_pipeFd(-1)
    , m_bufferSize(23)
    , m_buffer(nullptr)
    , m_uuid(std::move(uuid))
{
    m_notifyThread.name = "";
    m_notifyThread.id = 0;
    m_notifyThread.running = false;

    // sanity check the input notify pipe
    if (notifyPipeFd < 0) {
        XLOGD_ERROR("invalid notify pipe fd");
        return;
    }

    // dup the supplied fd
    m_pipeFd = fcntl(notifyPipeFd, F_DUPFD_CLOEXEC, 3);
    if (m_pipeFd < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to dup bluez notify pipe: error = <%d>, <%s>", errsv, strerror(errsv));
        return;
    }

    // open eventfd to signal to the thread to exit
    if (pipe(m_exitEventFds) == -1) {
        int errsv = errno;
        XLOGD_ERROR("failed to open exit eventfd, error = <%d>, <%s>", errsv, strerror(errsv));
        return;
    }

    // put in non-blocking mode
    int flags = fcntl(m_pipeFd, F_GETFL);
    if (flags < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to F_GETFL: error = <%d>, <%s>", errsv, strerror(errsv));
        return;
    } else if (!(flags & O_NONBLOCK)) {
        int ret = fcntl(m_pipeFd, F_SETFL, flags | O_NONBLOCK);
        if (ret < 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to put in non-blocking mode: error = <%d>, <%s>", errsv, strerror(errsv));
        }
    }


    // allocate a buffer for each individual notification
    m_bufferSize = mtu;
    if (m_bufferSize < 1) {
        XLOGD_ERROR("invalid mtu size, defaulting to 23");
        m_bufferSize = 23;
    }
    if (m_bufferSize > PIPE_BUF) {
        XLOGD_ERROR("mtu size is larger than atomic pipe buffer size");
        m_bufferSize = PIPE_BUF;
    }
    m_buffer = new uint8_t[m_bufferSize];


    m_notifyThread.name = "ble_notify";
    sem_init(&m_notifyThreadSem, 0, 0);
    XLOGD_INFO("Launching thread <%s> for %s...", m_notifyThread.name, m_uuid.toString().c_str());
    if (false == ThreadCreate(&m_notifyThread, NotifyThread, this)) {
        sem_destroy(&m_notifyThreadSem);
    } else  {
        // Block until initialization is complete or a timeout occurs
        sem_wait(&m_notifyThreadSem);
        sem_destroy(&m_notifyThreadSem);
    }
}

// -----------------------------------------------------------------------------
/*!

 */
BleGattNotifyPipe::~BleGattNotifyPipe()
{
    *m_isAlive = false;

    shutdown();

    if (m_buffer) {
        delete [] m_buffer;
    }
}

// -----------------------------------------------------------------------------
/*!

 */
void BleGattNotifyPipe::shutdown()
{
    if (m_notifyThread.running) {
        if(FD_SIGNAL(m_exitEventFds) > -1) {
            SignalEventFd(FD_SIGNAL(m_exitEventFds));
        }
        ThreadJoin(&m_notifyThread, 2);
    }
    if (FD_SIGNAL(m_exitEventFds) > -1) {
        close(FD_SIGNAL(m_exitEventFds));
    }
    if (FD_RECV(m_exitEventFds) > -1) {
        close(FD_RECV(m_exitEventFds));
    }

    if ((m_pipeFd >= 0) && (::close(m_pipeFd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close notification pipe fd: error = <%d>, <%s>", errsv, strerror(errsv));
    }
    m_pipeFd = -1;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the notification pipe is valid.

 */
bool BleGattNotifyPipe::isValid() const
{
    return (m_pipeFd >= 0);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when there is data available to be read from the input pipe.

 */
bool BleGattNotifyPipe::onActivated()
{
    // read as much as we can from the pipe
    while (m_pipeFd >= 0) {

        // note that bluez sensibly uses the O_DIRECT flag for the pipe so that
        // the data in the pipe is packetised, meaning we must read in 20 byte
        // chunks, and we should only get 20 bytes
        ssize_t rd = TEMP_FAILURE_RETRY(::read(m_pipeFd, m_buffer, m_bufferSize));
        if (rd < 0) {

            // check if the pipe is empty, if not the error is valid
            if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
                int errsv = errno;
                XLOGD_ERROR("failed to read from pipe: error = <%d>, <%s>", errsv, strerror(errsv));
            }

            break;

        } else if (rd == 0) {
            // a read of zero bytes means the remote end of the pipe has been
            // closed, this usually just means that the RCU has disconnected,
            // this will stop the recording,
            XLOGD_INFO("remote end of notification pipe closed for %s", m_uuid.toString().c_str());

            if (::close(m_pipeFd) != 0) {
                int errsv = errno;
                XLOGD_ERROR("failed to close pipe fd: error = <%d>, <%s>", errsv, strerror(errsv));
            }
            m_pipeFd = -1;
            
            m_closedSlots.invoke();

            return false;

        } else {
            std::vector<uint8_t> vec(m_buffer, m_buffer + rd);

            // emit a notification signal
            m_notificationSlots.invoke(vec);
        }
    }
    return true;
}

void *NotifyThread(void *data)
{
    BleGattNotifyPipe *notifyPipe = (BleGattNotifyPipe *)data;
    shared_ptr<bool> isAlive = notifyPipe->m_isAlive;

    fd_set rfds;
    int nfds = -1;
    bool running = true;

    // Unblock the caller that launched this thread
    sem_post(&notifyPipe->m_notifyThreadSem);

    XLOGD_INFO("Enter main loop for bluez notification pipe (%d) for %s", 
            notifyPipe->m_pipeFd, notifyPipe->m_uuid.toString().c_str());
    do {
        // Needs to be reinitialized before each call to select() because select() will modify these variables
        FD_ZERO(&rfds);
        FD_SET(FD_RECV(notifyPipe->m_exitEventFds), &rfds);
        nfds = FD_RECV(notifyPipe->m_exitEventFds);

        FD_SET(notifyPipe->m_pipeFd,  &rfds);
        nfds = std::max(nfds, notifyPipe->m_pipeFd);
        nfds++;

        int ret = select(nfds, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            int errsv = errno;
            XLOGD_DEBUG("select() failed: error = <%d>, <%s>", errsv, strerror(errsv));
            continue;
        }

        if (*isAlive && FD_ISSET(FD_RECV(notifyPipe->m_exitEventFds), &rfds)) {
            XLOGD_DEBUG("received exit signal");
            ClearEventFd(FD_RECV(notifyPipe->m_exitEventFds));
            running = false;
            break;
        }

        if (*isAlive && FD_ISSET(notifyPipe->m_pipeFd, &rfds)) {
            running = notifyPipe->onActivated();
        }

    } while (running && *isAlive);

    if (*isAlive == false) {
        XLOGD_ERROR("BleGattNotifyPipe object has been destroyed before thread exited.  Suspect something went wrong, exiting...");
    
    } else {
        notifyPipe->m_notifyThread.running = false;

        if (!running) {
            XLOGD_INFO("BLE notification pipe thread exited gracefully.");
        } else {
            XLOGD_ERROR("BLE notification pipe thread exited unexpectedly, suspect an error occurred...");
        }
    }

    return NULL;
}
