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

#include "compositeIn.hpp"

class CompositeInputImplMock : public device::CompositeInputImpl {
public:
    virtual ~CompositeInputImplMock() = default;

    MOCK_METHOD(uint8_t, getNumberOfInputs, (), (const, override));
    MOCK_METHOD(bool, isPortConnected, (int8_t Port), (const, override));
    MOCK_METHOD(void, selectPort, (int8_t Port), (const, override));
	MOCK_METHOD(void, scaleVideo, (int32_t x, int32_t y, int32_t width, int32_t height), (const, override));
   
};
