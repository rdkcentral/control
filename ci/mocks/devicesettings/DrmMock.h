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

#include <gmock/gmock.h>
#include <list>

class DRMMock : public drmImpl {
public:
    virtual ~DRMMock() = default;

    MOCK_METHOD(drmModeEncoderPtr, drmModeGetEncoder, (int fd, uint32_t encoder_id), (override));
    MOCK_METHOD(void, drmModeFreeEncoder, (drmModeEncoderPtr* encoder), (override));
    MOCK_METHOD(drmModeConnectorPtr, drmModeGetConnector, (int fd, uint32_t connectorId), (override));
    MOCK_METHOD(drmModeCrtcPtr, drmModeGetCrtc, (int fd, uint32_t crtcId), (override));
    MOCK_METHOD(drmModeResPtr, drmModeGetResources, (int fd), (override));
    MOCK_METHOD(void, drmModeFreeConnector, (drmModeConnectorPtr ptr), (override));
    MOCK_METHOD(void, drmModeFreeCrtc, (drmModeCrtcPtr ptr), (override));
    MOCK_METHOD(void, drmModeFreeResources, (drmModeResPtr ptr), (override));
    MOCK_METHOD(drmModePropertyPtr, drmModeGetProperty, (int fd, uint32_t propertyId), (override));
    MOCK_METHOD(void, drmModeFreeProperty, (drmModePropertyPtr ptr), (override));
    MOCK_METHOD(drmModePlaneResPtr, drmModeGetPlaneResources, (int fd), (override));
    MOCK_METHOD(drmModePlanePtr, drmModeGetPlane, (int fd, uint32_t plane_id), (override));
    MOCK_METHOD(drmModeObjectPropertiesPtr, drmModeObjectGetProperties, (int fd, uint32_t object_id, uint32_t object_type), (override));
    MOCK_METHOD(void, drmModeFreeObjectProperties, (drmModeObjectPropertiesPtr ptr), (override));
    MOCK_METHOD(void, drmModeFreePlane, (drmModePlanePtr ptr), (override));
    MOCK_METHOD(void, drmModeFreePlaneResources, (drmModePlaneResPtr ptr), (override));
    MOCK_METHOD(int, drmSetClientCap, (int fd, uint64_t capability, uint64_t value), (override));
    MOCK_METHOD(void, drmModeFreeFB, (drmModeFBPtr ptr), (override));
    MOCK_METHOD(drmModeFBPtr, drmModeGetFB, (int fd, uint32_t bufferId), (override));
    MOCK_METHOD(void, drmModeFreeEncoder, (drmModeEncoderPtr ptr), (override));

};