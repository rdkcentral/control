/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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
#ifndef __CTRLM_THUNDER_PLUGIN_CEC_COMMON_H__
#define __CTRLM_THUNDER_PLUGIN_CEC_COMMON_H__
namespace Thunder {
namespace CEC {

class cec_device_t {
public:
    cec_device_t() {
        this->port            = -1;
        this->logical_address = 0xFF;
        this->vendor_id       = 0xFFFF;
        this->osd             = "";
    }

public:
    int         port;
    uint8_t     logical_address;
    std::string osd;
    uint16_t    vendor_id;
};
};
};
#endif