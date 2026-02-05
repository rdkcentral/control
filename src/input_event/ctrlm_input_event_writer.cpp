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

#include "ctrlm_input_event_writer.h"
#include "ctrlm_ipc_key_codes.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "safec_lib.h"
#include <linux/uinput.h>
#include <sys/time.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sstream>

bool ctrlm_input_event_writer::init(std::string uinput_name, uint32_t vendor, uint32_t product) {
    if (initialized_) {
        return true;
    }

    XLOGD_INFO("Initializing a user input device for %s...", uinput_name.c_str());
    int fd = open("/dev/uinput", O_WRONLY|O_SYNC);
    if (fd == -1) {
        int errsv = errno;
        XLOGD_ERROR("Open failed with errno %d (%s)", errsv, std::strerror(errsv));
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_EVBIT, EV_MSC);
    ioctl(fd, UI_SET_MSCBIT, MSC_SCAN);
    for (auto const &entry : ctrlm_key_to_linux_map) {
        if (entry.second.key_code != KEY_RESERVED) {
            ioctl(fd, UI_SET_KEYBIT, entry.second.key_code);
        }
    }

    struct uinput_setup usetup = {};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = vendor;
    usetup.id.product = product;
    errno_t safec_rc = strcpy_s(usetup.name, sizeof(usetup.name), uinput_name.c_str());
    ERR_CHK(safec_rc);

    int err = ioctl(fd, UI_DEV_SETUP, &usetup);
    if (err == -1) {
        int errsv = errno;
        XLOGD_ERROR("UI_DEV_SETUP failed with errno %d (%s)", errsv, std::strerror(errsv));
        close(fd);
        return false;
    }

    err = ioctl(fd, UI_DEV_CREATE);
    if (err == -1) {
        int errsv = errno;
        XLOGD_ERROR("UI_DEV_CREATE failed with errno %d (%s)", errsv, std::strerror(errsv));
        close(fd);
        return false;
    }

    char sysfs_name[16] = {0};
    err = ioctl(fd, UI_GET_SYSNAME(sizeof(sysfs_name)), sysfs_name);
    if (err == -1) {
        int errsv = errno;
        XLOGD_ERROR("UI_GET_SYSNAME failed with errno %d (%s)", errsv, std::strerror(errsv));
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
        return false;
    }
    sysfs_name_ = sysfs_name;

    fd_ = fd;
    initialized_ = true;

    return true;
}

void ctrlm_input_event_writer::shutdown(void) {
    if (!initialized_) {
        return;
    }
    ioctl(fd_, UI_DEV_DESTROY);
    close(fd_);
    fd_ = -1;
    initialized_ = false;
}

bool ctrlm_input_event_writer::write_event_internal(uint32_t scan_code, uint16_t key_code, key_stroke stroke) {
    int errsv = 0;
    struct input_event event = {};
    gettimeofday(&event.time, NULL);

    event.type  = EV_MSC;
    event.code  = MSC_SCAN;
    event.value = scan_code;

    if (write(fd_, &event, sizeof(event)) != sizeof(event)) {
        errsv = errno;
        XLOGD_ERROR("Write failed with errno %d (%s)", errsv, std::strerror(errsv));
        return false;
    }

    event.type  = EV_KEY;
    event.code  = key_code;
    event.value = static_cast<uint8_t>(stroke);

    if (write(fd_, &event, sizeof(event)) != sizeof(event)) {
        errsv = errno;
        XLOGD_ERROR("Write failed with errno %d (%s)", errsv, std::strerror(errsv));
        return false;
    }

    event.type  = EV_SYN;
    event.code  = SYN_REPORT;
    event.value = 0;

    if (write(fd_, &event, sizeof(event)) != sizeof(event)) {
        errsv = errno;
        XLOGD_ERROR("Write failed with errno %d (%s)", errsv, std::strerror(errsv));
        return false;
    }

    return true;
}

uint16_t ctrlm_input_event_writer::write_event(ctrlm_key_code_t code, ctrlm_key_status_t status) {
    if (!initialized_) {
        XLOGD_ERROR("User input device is not yet initialized!");
        return KEY_RESERVED;
    }

    if (ctrlm_key_to_linux_map.find(code) == ctrlm_key_to_linux_map.end()) {
        XLOGD_ERROR("Code <%d, %s> not found in mapping", code, ctrlm_key_code_str(code));
        return KEY_RESERVED;
    }

    if (ev_key_value_map.find(status) == ev_key_value_map.end()) {
        XLOGD_ERROR("Key status <%d> not found in mapping", status);
        return KEY_RESERVED;
    }

    linux_ui_code_values_t param = ctrlm_key_to_linux_map.at(code);

    if (param.key_code == KEY_RESERVED) {
        XLOGD_WARN("Code <%d, %s> is currently not mapped - skipping", code, ctrlm_key_code_str(code));
        return param.key_code;
    }

    if (!write_event_internal(param.scan_code, param.key_code, ev_key_value_map.at(status))) {
        return KEY_RESERVED;
    }

    return param.key_code;
}

bool ctrlm_input_event_writer::get_meta_data(struct stat &file_meta_data) {
    std::ostringstream oss;
    oss << "/sys/devices/virtual/input/" << sysfs_name_;
    std::string dir_path = oss.str();
    XLOGD_DEBUG("virtual input path = %s", dir_path.c_str());

    DIR *dir = opendir(dir_path.c_str());
    if (dir == nullptr) {
        int errsv = errno;
        XLOGD_ERROR("Failed to open virtual input device dir at path <%s>: error = <%d>, <%s>",
              dir_path.c_str(), errsv, strerror(errsv));
        return false;
    }

    struct dirent *entry;
    std::string stat_path = "/dev/input/";
    bool event_node_found = false;

    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find("event") == 0) {
            event_node_found = true;
            stat_path += filename;
            break;
        }
    }
    closedir(dir);

    XLOGD_DEBUG("dev input event path = %s", stat_path.c_str());
    if (!event_node_found) {
        XLOGD_ERROR("no event node found in virtual device dir <%s>", dir_path.c_str());
        return false;
    }

    int ret = stat(stat_path.c_str(), &file_meta_data);
    if (ret == -1) {
        int errsv = errno;
        XLOGD_ERROR("stat() failed: error = <%d>, <%s>", errsv, std::strerror(errsv));
        return false;
    }
    return true;
}
