/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
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
*/

#ifndef _CTRLM_INPUT_EVENT_WRITER_H_
#define _CTRLM_INPUT_EVENT_WRITER_H_

#include <cstdint>
#include <string>
#include <map>
#include <linux/input.h>
#include <sys/stat.h>
#include "ctrlm_ipc_key_codes.h"
#include "ctrlm_ipc.h"

class linux_ui_code_values_t {
public:
    uint16_t key_code  = KEY_RESERVED;
    uint32_t scan_code = 0x0;
    uint16_t modifier  = KEY_RESERVED;

    linux_ui_code_values_t() = default;
    ~linux_ui_code_values_t() = default;

    linux_ui_code_values_t(uint16_t key, uint32_t scan, uint16_t mod) {
        key_code  = key;
        scan_code = scan;
        modifier  = mod;
    }
};

const std::map<ctrlm_key_code_t, linux_ui_code_values_t> ctrlm_key_to_linux_map =
{
    {CTRLM_KEY_CODE_OK,           linux_ui_code_values_t(KEY_ENTER,      0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_UP_ARROW,     linux_ui_code_values_t(KEY_UP,         0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_DOWN_ARROW,   linux_ui_code_values_t(KEY_DOWN,       0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_LEFT_ARROW,   linux_ui_code_values_t(KEY_LEFT,       0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_RIGHT_ARROW,  linux_ui_code_values_t(KEY_RIGHT,      0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_MENU,         linux_ui_code_values_t(KEY_HOME,       0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_0,      linux_ui_code_values_t(KEY_0,          0x00, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_1,      linux_ui_code_values_t(KEY_1,          0x01, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_2,      linux_ui_code_values_t(KEY_2,          0x02, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_3,      linux_ui_code_values_t(KEY_3,          0x03, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_4,      linux_ui_code_values_t(KEY_4,          0x04, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_5,      linux_ui_code_values_t(KEY_5,          0x05, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_6,      linux_ui_code_values_t(KEY_6,          0x06, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_7,      linux_ui_code_values_t(KEY_7,          0x07, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_8,      linux_ui_code_values_t(KEY_8,          0x08, KEY_RESERVED)},
    {CTRLM_KEY_CODE_DIGIT_9,      linux_ui_code_values_t(KEY_9,          0x09, KEY_RESERVED)},
    {CTRLM_KEY_CODE_CH_UP,        linux_ui_code_values_t(KEY_PAGEUP,     0x58, KEY_LEFTCTRL)},
    {CTRLM_KEY_CODE_CH_DOWN,      linux_ui_code_values_t(KEY_PAGEDOWN,   0x59, KEY_LEFTCTRL)},
    {CTRLM_KEY_CODE_LAST,         linux_ui_code_values_t(KEY_ESC,        0x0,  KEY_LEFTCTRL)},
    {CTRLM_KEY_CODE_INPUT_SELECT, linux_ui_code_values_t(KEY_F15,        0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_INFO,         linux_ui_code_values_t(KEY_F9,         0xcb, KEY_LEFTCTRL)},
    {CTRLM_KEY_CODE_VOL_UP,       linux_ui_code_values_t(KEY_KPPLUS,     0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_VOL_DOWN,     linux_ui_code_values_t(KEY_KPMINUS,    0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_MUTE,         linux_ui_code_values_t(KEY_KPASTERISK, 0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_POWER_ON,     linux_ui_code_values_t(KEY_F1,         0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_POWER_OFF,    linux_ui_code_values_t(KEY_F1,         0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_POWER_TOGGLE, linux_ui_code_values_t(KEY_F1,         0x0,  KEY_RESERVED)},
    {CTRLM_KEY_CODE_EXIT,         linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA exit
    {CTRLM_KEY_CODE_PAGE_UP,      linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA page up
    {CTRLM_KEY_CODE_PAGE_DOWN,    linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA page down
    {CTRLM_KEY_CODE_RECORD,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA record
    {CTRLM_KEY_CODE_REWIND,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA <<
    {CTRLM_KEY_CODE_FAST_FORWARD, linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA >>
    {CTRLM_KEY_CODE_GUIDE,        linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA guide
    {CTRLM_KEY_CODE_PLAY_PAUSE,   linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2/XRA play/pause
    {CTRLM_KEY_CODE_OCAP_B,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2 b
    {CTRLM_KEY_CODE_OCAP_C,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2 c
    {CTRLM_KEY_CODE_OCAP_D,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2 d
    {CTRLM_KEY_CODE_OCAP_A,       linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR15v2 a
    {CTRLM_KEY_CODE_ASTERISK,     linux_ui_code_values_t(KEY_RESERVED,   0x0,  KEY_RESERVED)}, // TODO XR16/XRA asterisk key
};

enum class key_stroke {
    UP     = 0,
    DOWN   = 1,
    REPEAT = 2,
    INVALID
};

const std::map<ctrlm_key_status_t, key_stroke> ev_key_value_map =
{
    {CTRLM_KEY_STATUS_DOWN,   key_stroke::DOWN},
    {CTRLM_KEY_STATUS_UP,     key_stroke::UP},
    {CTRLM_KEY_STATUS_REPEAT, key_stroke::REPEAT}
};

class ctrlm_input_event_writer {
private:
    bool initialized_ = false;
    int  fd_          = -1;

protected:
    bool write_event_internal(uint32_t scan_code, uint16_t key_code, key_stroke stroke);

public:
    ctrlm_input_event_writer() = default;
    ~ctrlm_input_event_writer() = default;

    bool     init(std::string uinput_name, uint32_t vendor, uint32_t product);
    void     shutdown(void);
    uint16_t write_event(ctrlm_key_code_t code, ctrlm_key_status_t status);
    bool     get_meta_data(struct stat &file_meta_data);
};

#endif
