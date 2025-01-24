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
//  filedescriptor.h
//

#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H


class FileDescriptor
{
public:
    FileDescriptor();
    explicit FileDescriptor(int fd);
    FileDescriptor(const FileDescriptor &other);
    FileDescriptor &operator=(FileDescriptor &&other);
    FileDescriptor &operator=(const FileDescriptor &other);
    ~FileDescriptor();

public:
    bool isValid() const;
    int fd() const;

    void reset();
    void clear();

private:
    int m_fd;
};


#endif // !defined(FILEDESCRIPTOR_H)
