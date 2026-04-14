/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/

#pragma once

typedef enum {
    T2ERROR_SUCCESS,
    T2ERROR_FAILURE,
    T2ERROR_INVALID_PROFILE,
    T2ERROR_PROFILE_NOT_FOUND,
    T2ERROR_PROFILE_NOT_SET,
    T2ERROR_MAX_PROFILES_REACHED,
    T2ERROR_MEMALLOC_FAILED,
    T2ERROR_INVALID_ARGS,
    T2ERROR_INTERNAL_ERROR
} T2ERROR;

class TelemetryApiImpl {
public:
    virtual ~TelemetryApiImpl() = default;

    virtual void t2_init(char* component) = 0;
    virtual void t2_uninit(void) = 0;
    virtual T2ERROR t2_event_s(const char* marker, const char* value) = 0;
    virtual T2ERROR t2_event_d(const char* marker, int value) = 0;
};

class TelemetryApi {
protected:
    static TelemetryApiImpl* impl;
public:
    TelemetryApi();
    TelemetryApi(const TelemetryApi &obj) = delete;
    static void setImpl(TelemetryApiImpl* newImpl);
    static void t2_init(char* component);
    static void t2_uninit(void);
    static T2ERROR t2_event_s(const char* marker, const char* value);
    static T2ERROR t2_event_d(const char* marker, int value);
};

extern void (*t2_init)(char*);
extern void (*t2_uninit)(void);
extern  T2ERROR (*t2_event_s)(const char*,const char*);
extern  T2ERROR (*t2_event_d)(const char*,int);

