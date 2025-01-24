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
//  filedescriptor.cpp
//

#include "filedescriptor.h"
#include "ctrlm_log_ble.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// -----------------------------------------------------------------------------
/*!
    \class FileDescriptor
    \brief Light wrapper around a file descriptor so it can be used safely with
    signals / slots

    Why do we need this?  Because we want to safely pass a file descriptor
    around using signal / slots and safely convert the value to / from a QVariant.

    Why not just use an integer? Because although it's obviously fine to pass
    an integer around and convert to / from QVariant there is no gaurantee that
    the descriptor is still valid when it's used by the slot.  This class uses
    \c dup(2) to ensure that if the object was created with a valid file
    descriptor in the first place then it and all copy constructed objects will
    have a valid file descriptor.

    \sa QDBusUnixFileDescriptor
 */


FileDescriptor::FileDescriptor()
    : m_fd(-1)
{
}

FileDescriptor::FileDescriptor(int fd)
    : m_fd(-1)
{
    if (fd >= 0) {
        m_fd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
        if (m_fd < 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to dup supplied fd: error = <%d>, <%s>", errsv, strerror(errsv));
        }
    }
}

FileDescriptor::FileDescriptor(const FileDescriptor &other)
    : m_fd(-1)
{
    if (other.m_fd >= 0) {
        m_fd = fcntl(other.m_fd, F_DUPFD_CLOEXEC, 3);
        if (m_fd < 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to dup supplied fd: error = <%d>, <%s>", errsv, strerror(errsv));
        }
    }
}

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other)
{
    if ((m_fd >= 0) && (::close(m_fd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close file descriptor: error = <%d>, <%s>", errsv, strerror(errsv));
    }

    m_fd = other.m_fd;
    other.m_fd = -1;

    return *this;
}

FileDescriptor &FileDescriptor::operator=(const FileDescriptor &other)
{
    if ((m_fd >= 0) && (::close(m_fd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close file descriptor: error = <%d>, <%s>", errsv, strerror(errsv));
    }

    m_fd = -1;

    if (other.m_fd >= 0) {
        m_fd = fcntl(other.m_fd, F_DUPFD_CLOEXEC, 3);
        if (m_fd < 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to dup supplied fd: error = <%d>, <%s>", errsv, strerror(errsv));
        }
    }

    return *this;
}

FileDescriptor::~FileDescriptor()
{
    reset();
}

bool FileDescriptor::isValid() const
{
    return (m_fd >= 0);
}

int FileDescriptor::fd() const
{
    return m_fd;
}

void FileDescriptor::reset()
{
    if ((m_fd >= 0) && (::close(m_fd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close file descriptor: error = <%d>, <%s>", errsv, strerror(errsv));
    }

    m_fd = -1;
}

void FileDescriptor::clear()
{
    reset();
}
