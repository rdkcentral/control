/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

#include "ctrlm_thunder_helpers.h"
#include "ctrlm_thunder_log.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <array>
#include <utility>
#include <systemd/sd-bus.h>
#include <cstring>

#define SYSTEMD_DESTINATION ("org.freedesktop.systemd1")
#define SYSTEMD_PATH        ("/org/freedesktop/systemd1")
#define SYSTEMD_IFACE_MGR   ("org.freedesktop.systemd1.Manager")
#define SYSTEMD_IFACE_UNIT  ("org.freedesktop.systemd1.Unit")
#define SYSTEMD_IFACE_PROP  ("org.freedesktop.DBus.Properties")

bool Thunder::Helpers::is_systemd_process_active(const char *process) {
   bool ret              = false;
   int rc                = 0;
   sd_bus *bus           = NULL;
   sd_bus_message *reply = NULL;
   std::string unit_path_str, active_state_str, sub_state_str;

   do {
      if((rc = sd_bus_open_system(&bus)) < 0) {
         XLOGD_ERROR("failed to open sd_bus: %s", process);
         break;
      }

      if((rc = sd_bus_call_method(bus, SYSTEMD_DESTINATION, SYSTEMD_PATH, SYSTEMD_IFACE_MGR, "GetUnit", NULL, &reply, "s", process)) < 0) {
         XLOGD_ERROR("failed to call GetUnit: %s\n", process);
         break;
      }

      const char *unit_path;
      if((rc = sd_bus_message_read(reply, "o", &unit_path)) < 0) {
         XLOGD_ERROR("failed to read unit path from reply: %s", process);
         break;
      }
      unit_path_str = std::string(unit_path);

      if((rc = sd_bus_call_method(bus, SYSTEMD_DESTINATION, unit_path_str.c_str(), SYSTEMD_IFACE_PROP, "Get", NULL, &reply, "ss", SYSTEMD_IFACE_UNIT, "ActiveState")) < 0) {
         XLOGD_ERROR("failed to call property get for active state: %s", process);
         break;
      }

      const char *active_state;
      if((rc = sd_bus_message_read(reply, "v", "s", &active_state)) < 0) {
         XLOGD_ERROR("failed to read active state from reply: %s", process);
         break;
      }
      active_state_str = std::string(active_state);

      if((rc = sd_bus_call_method(bus, SYSTEMD_DESTINATION, unit_path_str.c_str(), SYSTEMD_IFACE_PROP, "Get", NULL, &reply, "ss", SYSTEMD_IFACE_UNIT, "SubState")) < 0) {
         XLOGD_ERROR("failed to call property get for active state: %s", process);
         break;
      }

      const char *sub_state;
      if((rc = sd_bus_message_read(reply, "v", "s", &sub_state)) < 0) {
         XLOGD_ERROR("failed to read sub state from reply: %s", process);
         break;
      }
      sub_state_str = std::string(sub_state);

      XLOGD_INFO("process <%s> active state <%s> sub state <%s>", process, active_state_str.c_str(), sub_state_str.c_str());
      if(active_state_str == "active" && sub_state_str == "running") {
         ret = true;
      }
   } while(0);

   // Cleanup
   if(reply != NULL) {
      sd_bus_message_unref(reply);
      reply = NULL;
   }
   if(bus != NULL) {
      sd_bus_unref(bus);
      bus = NULL;
   }

   return(ret);
}