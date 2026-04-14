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


class MotionDetectionImplMock : public MotionDetectionImpl {
public:

        MotionDetectionImplMock():MotionDetectionImpl()
        {
                ON_CALL(*this, MOTION_DETECTION_Platform_Init())
                        .WillByDefault(::testing::Return(MOTION_DETECTION_RESULT_SUCCESS));

                ON_CALL(*this, MOTION_DETECTION_Platform_Term())
                        .WillByDefault(::testing::Return(MOTION_DETECTION_RESULT_SUCCESS));
        }

    virtual ~MotionDetectionImplMock() = default;

    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_Platform_Init, (), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_Platform_Term, (), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_RegisterEventCallback, (MOTION_DETECTION_OnMotionEventCallback motionEvent), (override));

    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_GetMotionDetectors, (MOTION_DETECTION_CurrentSensorSettings_t* motionDetectors), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_ArmMotionDetector, (MOTION_DETECTION_Mode_t mode, std::string index), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_DisarmMotionDetector, (std::string index), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_IsMotionDetectorArmed, (std::string index, bool* armState), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_SetNoMotionPeriod, (std::string index, int period), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_GetNoMotionPeriod, (std::string index, unsigned int* period), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_SetSensitivity, (std::string index, std::string sensitivity, int inferredMode), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_GetSensitivity, (std::string index, char** sensitivity, int* currentMode), (override));
 MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_SetActivePeriod, (std::string index, MOTION_DETECTION_TimeRange_t timeSet), (override));
    MOCK_METHOD(MOTION_DETECTION_Result_t, MOTION_DETECTION_GetActivePeriod, (MOTION_DETECTION_TimeRange_t* timeSet), (override));
};
