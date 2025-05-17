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
#include "ctrlm.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_iarm_powermanager.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "pwrMgr.h"
#include "ctrlm_tr181.h"

// Keep state since we do not want to service calls on termination
static ctrlm_ipc_iarm_powermanager_t *instance = NULL;
static volatile int g_running = 0;
static ctrlm_power_state_t ctrlm_iarm_power_state_map(IARM_Bus_PowerState_t iarm_power_state);

ctrlm_ipc_iarm_powermanager_t::ctrlm_ipc_iarm_powermanager_t() {
   XLOGD_INFO("IARM PowerManager Implementation");
}

ctrlm_ipc_iarm_powermanager_t::~ctrlm_ipc_iarm_powermanager_t() {
}

void ctrlm_ipc_iarm_powermanager_t::running_set(bool running) {
    g_running = running;
}

void ctrlm_ipc_iarm_powermanager_t::get_power_state(ctrlm_power_state_t &power_state) {
   IARM_Bus_PWRMgr_GetPowerState_Param_t param;

   power_state = CTRLM_POWER_STATE_ON;

   if(IARM_RESULT_SUCCESS != IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetPowerState, (void *)&param, sizeof(param))) {
      XLOGD_WARN("IARM bus failed to read power state, defaulting to <%s>", ctrlm_power_state_str(power_state));
   } else {
      power_state = ctrlm_iarm_power_state_map(param.curState);
      #ifdef NETWORKED_STANDBY_MODE_ENABLED
      //If ctrlm restarts with system STANDBY state, set to ON, will receive a DEEP_SLEEP or ON message shortly
      if(power_state == CTRLM_POWER_STATE_STANDBY) {
         power_state = CTRLM_POWER_STATE_ON;
      }
      #endif
      XLOGD_INFO("power state is : <%s>", ctrlm_power_state_str(power_state));
   }

   return;
}

#ifdef NETWORKED_STANDBY_MODE_ENABLED
void ctrlm_ipc_iarm_powermanager_t::get_networked_standby_mode(bool &networked_standby_mode) {
   IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {0};
   IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetNetworkStandbyMode, (void *)&param, sizeof(param));

   networked_standby_mode = false;

   if (res != IARM_RESULT_SUCCESS) {
      XLOGD_ERROR("IARM query for network standby mode failed, default to NO");
      return;
   }

   networked_standby_mode = param.bStandbyMode ? true : false;

   return;
}

void ctrlm_ipc_iarm_powermanager_t::get_wakeup_reason_voice(bool &wakeup_reason_voice) {
   DeepSleep_WakeupReason_t wakeup_reason;
   IARM_Result_t res;
   bool pwrmgr2 = false;

   wakeup_reason_voice = false;

   if(CTRLM_TR181_RESULT_SUCCESS != ctrlm_tr181_bool_get(CTRLM_RT181_POWER_RFC_PWRMGR2, &pwrmgr2)) {
      XLOGD_INFO("failed to determine Power Manager revision, defaulting to 1");
   }

   res = IARM_Bus_Call(pwrmgr2 ? IARM_BUS_PWRMGR_NAME : IARM_BUS_DEEPSLEEPMGR_NAME, IARM_BUS_DEEPSLEEPMGR_API_GetLastWakeupReason, (void*)&wakeup_reason, sizeof(wakeup_reason));
   if(res != IARM_RESULT_SUCCESS) {
      XLOGD_ERROR("IARM query for wakeup reason failed, returning false!");
      return;
   }

   XLOGD_INFO("wakeup_reason <%s>", ctrlm_wakeup_reason_str(wakeup_reason));
   
   wakeup_reason_voice = (wakeup_reason == DEEPSLEEP_WAKEUPREASON_VOICE) ? true: false;

   return;
}
#endif

#if CTRLM_HAL_RF4CE_API_VERSION >= 10 && !defined(CTRLM_DPI_CONTROL_NOT_SUPPORTED)
IARM_Result_t ctrlm_iarm_powermanager_event_handler_power_pre_change(void* pArgs)
{
   const IARM_Bus_CommonAPI_PowerPreChange_Param_t* pParams = (const IARM_Bus_CommonAPI_PowerPreChange_Param_t*) pArgs;
   if(0 == g_atomic_int_get(&g_running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(pArgs == NULL) {
      XLOGD_ERROR("Invalid argument");
      return IARM_RESULT_INVALID_PARAM;
   }

   ctrlm_main_queue_power_state_change_t *msg = (ctrlm_main_queue_power_state_change_t *)g_malloc(sizeof(ctrlm_main_queue_power_state_change_t));
   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return IARM_RESULT_OOM;
   }

   msg->header.type = CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE;
   msg->new_state = ctrlm_iarm_power_state_map(pParams->newState);
   XLOGD_DEBUG("Power State mapped to <%s>", ctrlm_power_state_str(msg->new_state));
   ctrlm_main_queue_msg_push(msg);

   return IARM_RESULT_SUCCESS;
}
#endif

ctrlm_ipc_iarm_powermanager_t *ctrlm_ipc_iarm_powermanager_create() {
   if(instance == NULL) {
      instance = new ctrlm_ipc_iarm_powermanager_t();
   }
   return(instance);
}

ctrlm_power_state_t ctrlm_iarm_power_state_map(IARM_Bus_PowerState_t iarm_power_state) {

   switch(iarm_power_state) {
      case IARM_BUS_PWRMGR_POWERSTATE_ON:                  return CTRLM_POWER_STATE_ON;
      case IARM_BUS_PWRMGR_POWERSTATE_STANDBY:
      case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP: return CTRLM_POWER_STATE_STANDBY;
      case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP:
      case IARM_BUS_PWRMGR_POWERSTATE_OFF:                 return CTRLM_POWER_STATE_DEEP_SLEEP;
      default:                                             return CTRLM_POWER_STATE_ON;
   }
}
