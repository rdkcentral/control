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
//  fwimagefile.cpp
//

#include "fwimagefile.h"

#include "crc32.h"
#include "ctrlm_log_ble.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>


using namespace std;

// -----------------------------------------------------------------------------
/*!
    \class FwImageFile
    \brief Wraps a QIODevice object that contains a f/w image file.

    Basic utility object to abstract away some of the details of a f/w image
    file and to perform the integrity checks on said files.

 */



/// The format of the expect firmware file header
struct FwFileHeader {
    uint32_t hwIdent;
    uint32_t fwImageLength;
    uint32_t fwImageVersion;
    uint32_t fwImageCrc32;
};

/// The maximum number of data bytes in a DATA packet
#define FIRMWARE_PACKET_MTU           18


// -----------------------------------------------------------------------------
/*!
    Constructs a FwImageFile object by attempting to open the file at the given
    path.  Use isValid() to determine if the file could be opened and is a
    valid f/w image file.


 */
FwImageFile::FwImageFile(const string &filePath)
    : m_valid(false)
    , m_fd(-1)
    , m_eof(false)
{
    m_path = filePath;

    int fd = open(filePath.c_str(), O_CLOEXEC|O_RDONLY);
    if (fd < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to open fw file @ '%s', error <%s>", 
                filePath.c_str(), strerror(errsv));
        m_error = strerror(errsv);
        return;
    }
    m_fd = fd;

    // check the file header / contents
    m_valid = checkFile();
    if (!m_valid) {
        close(m_fd);
        m_fd = -1;
    }
}

FwImageFile::~FwImageFile()
{
    if ((m_fd >= 0) && (::close(m_fd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close file descriptor: error = <%d>, <%s>", errsv, strerror(errsv));
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Checks the open file has the correct header and the crc32 checksum matches.

 */
bool FwImageFile::checkFile()
{
    if (m_fd < 0) {
        m_error = "Failed to read f/w file";
        XLOGD_ERROR("%s", m_error.c_str()); 
        return false;
    }


    // get the size of the file, we may potential have a problem if the size
    // of the file would mean that the block ids would wrap, for now we ignore that
    
    // std::ifstream fp(m_path, std::ios::binary | std::ios::ate);

    // const uintmax_t fileSize = filesystem::file_size(fp);

    // if (fileSize > (0x3fff * FIRMWARE_PACKET_MTU)) {
    //     m_error = "Firmware file is too large";
    //     XLOGD_ERROR("%s", m_error.c_str());
    //     return false;
    // }
    // if (fileSize <= uintmax_t(sizeof(FwFileHeader))) {
    //     m_error = "Firmware file is empty";
    //     XLOGD_ERROR("%s", m_error.c_str()); 
    //     return false;
    // }


    // read the file header and verify the crc matches
    FwFileHeader header;

    ssize_t ret = read(m_fd, (void*)&header, sizeof(header));
    if (ret < 0) {
        int errsv = errno;
        XLOGD_ERROR("Error reading header: error = <%d>, <%s>", errsv, strerror(errsv));
        m_error = "Error reading header of firmware file";
        return false;
    }

    if (ret != sizeof(FwFileHeader)) {
        m_error = "Firmware file header read size incorrect";
        XLOGD_ERROR("%s", m_error.c_str());
        return false;
    }

    m_hardwareVersion = header.hwIdent;
    m_firmwareVersion = header.fwImageVersion;
    m_firmwareSize = header.fwImageLength;
    m_firmwareCrc = header.fwImageCrc32;


    // check the length in the header matches
    // if (m_firmwareSize != (fileSize - sizeof(FwFileHeader))) {
    //     m_error = "Firmware file header length error";
    //     XLOGD_ERROR("%s", m_error.c_str());
    //     return false;
    // }

    // calculate the crc over the rest of the file after the header
    Crc32 fileCrc;
    fileCrc.addData(m_fd);

    if (m_firmwareCrc != fileCrc.result()) {

        m_error = "Firmware file header crc error";
        XLOGD_ERROR("%s, m_firmwareCrc = 0x%X, fileCrc = 0x%X", m_error.c_str(), m_firmwareCrc, fileCrc.result());
        return false;
    }


    // now need to rewind the file to the point right after the header
    if (lseek(m_fd, sizeof(FwFileHeader), SEEK_SET) < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to rewind the file to the point right after the header, error <%s>", strerror(errsv));
        m_error = strerror(errsv);
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the file is valid and the f/w header checks out with
    the contents of the file; otherwise \c false.

 */
bool FwImageFile::isValid() const
{
    return m_valid;
}

// -----------------------------------------------------------------------------
/*!
    Returns a human-readable description of the last device error that occurred.

 */
string FwImageFile::errorString() const
{
    return m_error;
}

// -----------------------------------------------------------------------------
/*!
    Returns the manufacturer id byte this firmware image file is targeting.

    If the firmware file is not valid this method returns an undefined value.
 */
uint8_t FwImageFile::manufacturerId() const
{
    if (!m_valid) {
        return 0x00;
    }

    return (uint8_t)((m_hardwareVersion >> 24) & 0xff);
}

// -----------------------------------------------------------------------------
/*!
    Returns the hardware version this firmware image file is targeting.

    If the firmware file is not valid then a null version is returned.
 */
uint32_t FwImageFile::hwVersion() const
{
    if (!m_valid) {
        return 0;
    }

    return m_hardwareVersion & 0xffffff;
}

// -----------------------------------------------------------------------------
/*!
    Returns the version of the firmware in the image file.

    If the firmware file is not valid then a null version is returned.
 */
uint32_t FwImageFile::version() const
{
    if (!m_valid) {
        return 0;
    }

    return m_firmwareVersion;
}

// -----------------------------------------------------------------------------
/*!
    Returns the CRC32 checksum of the f/w data.

    If the firmware file is not valid then the return value is undefined.
 */
uint32_t FwImageFile::crc32() const
{
    if (!m_valid) {
        return 0;
    }

    return m_firmwareCrc;
}

// -----------------------------------------------------------------------------
/*!
    Returns the current position within the f/w file.  This is excluding the
    header, i.e. a position of 0 means we're at the start of the firmware image
    data.

 */
// int64_t FwImageFile::pos() const
// {
//     if (!m_valid) {
//         return -1;
//     }

//     return m_file->pos() - sizeof(FwFileHeader);
// }

// -----------------------------------------------------------------------------
/*!
    Returns \c true if the end of the file has been reached; otherwise returns
    \c false.

 */
bool FwImageFile::atEnd() const
{
    if (!m_valid) {
        return true;
    }

    return m_eof;
}

// -----------------------------------------------------------------------------
/*!
    Seeks to a position within the image data segment, ie. seeking to postion
    0 will be at the first byte in the f/w data image, not the first byte in
    the file.

 */
bool FwImageFile::seek(int64_t pos)
{
    if (!m_valid) {
        return false;
    }
    if (pos < 0) {
        return false;
    }


    if (lseek(m_fd, sizeof(FwFileHeader) + pos, SEEK_SET) < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to seek the file to offset %llu, error <%s>", 
                sizeof(FwFileHeader) + pos, strerror(errsv));
        m_error = strerror(errsv);
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    Returns the size of the f/w image data, not the size of the file.  I.e. this
    is the size of the image to transfer to the RCU.

 */
uint32_t FwImageFile::size() const
{
    if (!m_valid) {
        return -1;
    }

    return m_firmwareSize;
}

// -----------------------------------------------------------------------------
/*!
    Reads data from the firmware data segment starting at the current position.

 */
int64_t FwImageFile::readFile(void *data, int64_t len)
{
    if (!m_valid) {
        return -1;
    }

    ssize_t ret = read(m_fd, (void*)data, len);
    if (ret < 0) {
        int errsv = errno;
        XLOGD_ERROR("Error reading from file: error = <%d>, <%s>", errsv, strerror(errsv));
        m_error = "Error reading from file";
    } else if (ret == 0) {
        m_eof = true;
    }

    return ret;
}

