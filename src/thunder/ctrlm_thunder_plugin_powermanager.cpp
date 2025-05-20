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
#include "ctrlm_thunder_plugin_powermanager.h"
#include "ctrlm_thunder_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace PowerManager;
using namespace WPEFramework;

static ctrlm_thunder_plugin_powermanager_t *instance = NULL;

static ctrlm_power_state_t ctrlm_thunder_power_state_map(const string power_string);
static void _on_power_state_changed(ctrlm_thunder_plugin_powermanager_t *plugin, JsonObject params);

ctrlm_thunder_plugin_powermanager_t::ctrlm_thunder_plugin_powermanager_t() : ctrlm_thunder_plugin_t("PowerManager", "org.rdk.PowerManager", 1) {
   sem_init(&this->semaphore, 0, 1);
   this->registered_events = false;
}

ctrlm_thunder_plugin_powermanager_t::~ctrlm_thunder_plugin_powermanager_t() {
   free(instance);
}

ctrlm_thunder_plugin_powermanager_t *ctrlm_thunder_plugin_powermanager_t::get_instance() {
   if(instance == NULL) {
      instance = new ctrlm_thunder_plugin_powermanager_t;
   }

   return instance;
}

void ctrlm_thunder_plugin_powermanager_t::on_initial_activation() {
   Thunder::Plugin::ctrlm_thunder_plugin_t::on_initial_activation();
}

void ctrlm_thunder_plugin_powermanager_t::on_activation_change(Thunder::plugin_state_t state) {
   Thunder::Plugin::ctrlm_thunder_plugin_t::on_activation_change(state);
}

/* root@pioneer-uhd:~# curl --request POST --url http://127.0.0.1:9998/jsonrpc --header 'Content-Type: application/json' --data '{ "jsonrpc": "2.0", "id": 1234567890, "method": "org.rdk.PowerManager.1.getPowerState", "params": {} }'
 * {"jsonrpc":"2.0","id":1234567890,"result":{"currentState":"ON","previousState":"DEEP_SLEEP"}}
 * */
void ctrlm_thunder_plugin_powermanager_t::get_power_state(ctrlm_power_state_t &power_state) {
   JsonObject params, response;
   params = {};
   power_state = CTRLM_POWER_STATE_INVALID;

   sem_wait(&this->semaphore);
   if(this->call_plugin("getPowerState", (void *)&params, (void *)&response)) {
      std::string power_string;
      const char *query_string = "currentState";

      if(response.HasLabel(query_string)) {
      string power_string = response[query_string].String();
      power_state = ctrlm_thunder_power_state_map(power_string);
      XLOGD_DEBUG("power_state %s", ctrlm_power_state_str(power_state));
      } else {
         std::string response_string;

         response.ToString(response_string);
         XLOGD_ERROR("power state response malformed: <%s>", response_string.c_str());
      }
   }
   sem_post(&this->semaphore);
}


#ifdef NETWORKED_STANDBY_MODE_ENABLED
/* root@pioneer-uhd:~# curl --request POST --url http://127.0.0.1:9998/jsonrpc --header 'Content-Type: application/json' --data '{ "jsonrpc": "2.0", "id": 1234567890, "method": "org.rdk.PowerManager.1.getNetworkStandbyMode", "params": {} }'
{"jsonrpc":"2.0","id":1234567890,"result":true} */
void ctrlm_thunder_plugin_powermanager_t::get_networked_standby_mode(bool &networked_standby_mode) {
   JsonObject params, response;
   params = {};

   sem_wait(&this->semaphore);   
   if(this->call_plugin("getNetworkStandbyMode", (void *)&params, (void *)&response)) {
      networked_standby_mode = response["result"].Boolean();
      XLOGD_DEBUG("networked_standby_mode is %s", networked_standby_mode?"TRUE":"FALSE");
   } else {
     XLOGD_ERROR("getNetworkedStandbyMode call failed");
   }
   sem_post(&this->semaphore);
}

/* root@pioneer-uhd:~# curl --request POST --url http://127.0.0.1:9998/jsonrpc --header 'Content-Type: application/json' --data '{ "jsonrpc": "2.0", "id": 1234567890, "method": "org.rdk.PowerManager.1.getLastWakeupReason", "params": {} }'
{"jsonrpc":"2.0","id":1234567890,"result":"COLDBOOT"} */
void ctrlm_thunder_plugin_powermanager_t::get_wakeup_reason_voice(bool &wakeup_reason_voice) {
   JsonObject params, response;
   params = {};

   sem_wait(&this->semaphore);   
   if(this->call_plugin("getLastWakeupReason", (void *)&params, (void *)&response)) {
      wakeup_reason_voice = (0 == strncmp(response["result"].String().c_str(), "VOICE", 5));
      XLOGD_DEBUG("voice_wakeup is %s", wakeup_reason_voice?"TRUE":"FALSE");
   } else {
      XLOGD_ERROR("getLastWakeupReason call failed");
   }
   sem_post(&this->semaphore);}
#endif

void ctrlm_thunder_plugin_powermanager_t::on_power_state_changed(const ctrlm_power_state_t &current_state, const ctrlm_power_state_t &new_state) {
   ctrlm_main_queue_power_state_change_t *msg = (ctrlm_main_queue_power_state_change_t *)g_malloc(sizeof(ctrlm_main_queue_power_state_change_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   XLOGD_DEBUG("current state %s, new state %s", ctrlm_power_state_str(current_state), ctrlm_power_state_str(new_state));
   msg->header.type = CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE;
   msg->new_state = new_state;
   ctrlm_main_queue_msg_push(msg);
}


bool ctrlm_thunder_plugin_powermanager_t::register_events() {
   bool ret = this->registered_events;
   
   if(ret == false) {
       auto clientObject = (JSONRPC::LinkType<Core::JSON::IElement>*)this->plugin_client;
       if(clientObject) {
           ret = true;
           uint32_t thunderRet = clientObject->Subscribe<JsonObject>(CALL_TIMEOUT, _T("onPowerModeChanged"), _on_power_state_changed, this);
           if(thunderRet != Core::ERROR_NONE) {
               XLOGD_ERROR("Thunder subscribe failed <on_power_state_changed>");
               ret = false;
           }

           if(ret) {
               this->registered_events = true;
           }
       } else {
         XLOGD_INFO("clientObject is null");
       }
   }
   Thunder::Plugin::ctrlm_thunder_plugin_t::register_events();

   return(ret);
}

static void _on_power_state_changed(ctrlm_thunder_plugin_powermanager_t *plugin, JsonObject params) {
   ctrlm_power_state_t new_state = CTRLM_POWER_STATE_INVALID;
   ctrlm_power_state_t current_state = CTRLM_POWER_STATE_INVALID;

   string power_string;
   string params_string;
   const char *query_string;

   query_string = "newState";
   if(!params.HasLabel(query_string)) {
      params.ToString(params_string);
   } else {
      power_string = params[query_string].String();
      new_state = ctrlm_thunder_power_state_map(power_string);
   }

   query_string = "currentState";
   if(!params.HasLabel(query_string)) {
      params.ToString(params_string);      
   } else {
      power_string = params[query_string].String();
      current_state = ctrlm_thunder_power_state_map(power_string);
   }

   if(new_state == CTRLM_POWER_STATE_INVALID || current_state == CTRLM_POWER_STATE_INVALID) {
      XLOGD_ERROR("power state params malformed: <%s>", params_string.c_str());
   } else {
      plugin->on_power_state_changed(current_state, new_state);
   }
}

static ctrlm_power_state_t ctrlm_thunder_power_state_map(const string power_string) {
   if("ON" == power_string) {
      return CTRLM_POWER_STATE_ON;
   } else if("OFF" == power_string) {
      return CTRLM_POWER_STATE_DEEP_SLEEP;
   } else if("STANDBY" == power_string) {
      return CTRLM_POWER_STATE_STANDBY;
   } else if("DEEP_SLEEP" == power_string) {
      return CTRLM_POWER_STATE_DEEP_SLEEP;
   } else if("LIGHT_SLEEP" == power_string) {
      return CTRLM_POWER_STATE_STANDBY;
   }
   return CTRLM_POWER_STATE_INVALID;
}
