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
//  fwimagefile.h
//

#ifndef FWIMAGEFILE_H
#define FWIMAGEFILE_H

#include <string>

class FwImageFile
{
public:
    FwImageFile(const std::string &filePath);
    ~FwImageFile();

    //disable copy and move constructors
    FwImageFile(const FwImageFile &) = delete;
    FwImageFile(FwImageFile &&) = delete;
    FwImageFile &operator=(FwImageFile &&other) = delete;
    FwImageFile &operator=(const FwImageFile &other) = delete;

public:
    bool isValid() const;

    std::string errorString() const;

    uint8_t manufacturerId() const;
    uint32_t hwVersion() const;
    uint32_t version() const;

    uint32_t crc32() const;

public:
    bool atEnd() const;
    // int64_t pos() const;
    bool seek(int64_t pos);

    uint32_t size() const;

    int64_t readFile(void *data, int64_t len);

private:
    bool checkFile();

private:
    bool m_valid;
    int m_fd;

    std::string m_path;
    std::string m_error;

    uint32_t m_hardwareVersion = 0;
    uint32_t m_firmwareVersion = 0;
    uint32_t m_firmwareSize = 0;
    uint32_t m_firmwareCrc = 0;
    
    bool m_eof;
};

#endif // !defined(FWIMAGEFILE_H)
