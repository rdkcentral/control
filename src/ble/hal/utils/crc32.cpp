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
//  crc32.cpp
//

#include "crc32.h"


using namespace std;


// -----------------------------------------------------------------------------
/*!
    Constructs an object that can be used to create a CRC32 hash from data.

 */
Crc32::Crc32()
{
    m_crc = crc32(0L, Z_NULL, 0);
}

// -----------------------------------------------------------------------------
/*!
    Destroys the object.

 */
Crc32::~Crc32()
{
}

// -----------------------------------------------------------------------------
/*!
    Adds the first \a length chars of \a data to the CRC32 hash.

 */
void Crc32::addData(const uint8_t *data, int length)
{
    if (length <= 0) {
        return;
    }

    m_crc = crc32(m_crc, (const Bytef *)&data[0], length);
}


// -----------------------------------------------------------------------------
/*!
    Reads the data from the open QIODevice \a device until it ends and hashes it.
    Returns true if reading was successful.

 */
bool Crc32::addData(int fd)
{
    uint8_t buffer[1024];
    ssize_t length;

    m_crc = crc32(0L, Z_NULL, 0);


    while ((length = read(fd, (void*)buffer, sizeof(buffer))) > 0) {
        addData(buffer, int(length));
    }

    return length == 0;
}

// -----------------------------------------------------------------------------
/*!
    Resets the object.

 */
void Crc32::reset()
{
    m_crc = crc32(0L, Z_NULL, 0);
}

// -----------------------------------------------------------------------------
/*!
    Returns the final hash value.

 */
uint32_t Crc32::result() const
{
    return (uint32_t)m_crc;
}
