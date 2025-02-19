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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "libIBus.h"
#include "libIBusDaemon.h"
#ifdef USE_DEPRECATED_IRMGR
#include "irMgr.h"
#endif
#include "pwrMgr.h"
#include "sysMgr.h"
#ifdef ENABLE_DEEP_SLEEP
#include "deepSleepMgr.h"
#endif
#include "comcastIrKeyCodes.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_ipc.h"
#include "ctrlm_network.h"
#include "ctrlm_tr181.h"
#include "ctrlm_utils.h"
#include "dsMgr.h"
#include "dsRpc.h"

static IARM_Result_t ctrlm_main_iarm_call_status_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_network_status_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_property_set(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_property_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_discovery_config_set(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_autobind_config_set(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_precommission_config_set(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_controller_unbind(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_ir_remote_usage_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_last_key_info_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_control_service_set_values(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_control_service_get_values(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_control_service_can_find_my_remote(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_control_service_start_pairing_mode(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_control_service_end_pairing_mode(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_pairing_metrics_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_voice_session_begin(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_voice_session_end(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_start_pair_with_code(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_chip_status_get(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_audio_capture_start(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_audio_capture_stop(void *arg);
static IARM_Result_t ctrlm_main_iarm_call_power_state_change(void *arg);
#if CTRLM_HAL_RF4CE_API_VERSION >= 10  && !defined(CTRLM_DPI_CONTROL_NOT_SUPPORTED)
static IARM_Result_t ctrlm_event_handler_power_pre_change(void* pArgs);
#endif

typedef struct {
   const char *   name;
   IARM_BusCall_t handler;
} ctrlm_iarm_call_t;

// Keep state since we do not want to service calls on termination
static volatile int running = 0;

// Array to hold the IARM Calls that will be registered by Control Manager
ctrlm_iarm_call_t ctrlm_iarm_calls[] = {
   {CTRLM_MAIN_IARM_CALL_STATUS_GET,                         ctrlm_main_iarm_call_status_get                         },
   {CTRLM_MAIN_IARM_CALL_NETWORK_STATUS_GET,                 ctrlm_main_iarm_call_network_status_get                 },
   {CTRLM_MAIN_IARM_CALL_PROPERTY_SET,                       ctrlm_main_iarm_call_property_set                       },
   {CTRLM_MAIN_IARM_CALL_PROPERTY_GET,                       ctrlm_main_iarm_call_property_get                       },
   {CTRLM_MAIN_IARM_CALL_DISCOVERY_CONFIG_SET,               ctrlm_main_iarm_call_discovery_config_set               },
   {CTRLM_MAIN_IARM_CALL_AUTOBIND_CONFIG_SET,                ctrlm_main_iarm_call_autobind_config_set                },
   {CTRLM_MAIN_IARM_CALL_PRECOMMISSION_CONFIG_SET,           ctrlm_main_iarm_call_precommission_config_set           },
   {CTRLM_MAIN_IARM_CALL_CONTROLLER_UNBIND,                  ctrlm_main_iarm_call_controller_unbind                  },
   {CTRLM_MAIN_IARM_CALL_IR_REMOTE_USAGE_GET,                ctrlm_main_iarm_call_ir_remote_usage_get                },
   {CTRLM_MAIN_IARM_CALL_LAST_KEY_INFO_GET,                  ctrlm_main_iarm_call_last_key_info_get                  },
   {CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_SET_VALUES,         ctrlm_main_iarm_call_control_service_set_values         },
   {CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_GET_VALUES,         ctrlm_main_iarm_call_control_service_get_values         },
   {CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_CAN_FIND_MY_REMOTE, ctrlm_main_iarm_call_control_service_can_find_my_remote },
   {CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_START_PAIRING_MODE, ctrlm_main_iarm_call_control_service_start_pairing_mode },
   {CTRLM_MAIN_IARM_CALL_CONTROL_SERVICE_END_PAIRING_MODE,   ctrlm_main_iarm_call_control_service_end_pairing_mode   },
   {CTRLM_MAIN_IARM_CALL_PAIRING_METRICS_GET,                ctrlm_main_iarm_call_pairing_metrics_get                },
   {CTRLM_VOICE_IARM_CALL_SESSION_BEGIN,                     ctrlm_main_iarm_call_voice_session_begin                },
   {CTRLM_VOICE_IARM_CALL_SESSION_END,                       ctrlm_main_iarm_call_voice_session_end                  },
   {CTRLM_MAIN_IARM_CALL_START_PAIR_WITH_CODE,               ctrlm_main_iarm_call_start_pair_with_code               },
   {CTRLM_MAIN_IARM_CALL_CHIP_STATUS_GET,                    ctrlm_main_iarm_call_chip_status_get                    },
   {CTRLM_MAIN_IARM_CALL_AUDIO_CAPTURE_START,                ctrlm_main_iarm_call_audio_capture_start                },
   {CTRLM_MAIN_IARM_CALL_AUDIO_CAPTURE_STOP,                 ctrlm_main_iarm_call_audio_capture_stop                 },
   {CTRLM_MAIN_IARM_CALL_POWER_STATE_CHANGE,                 ctrlm_main_iarm_call_power_state_change                 },
#if CTRLM_HAL_RF4CE_API_VERSION >= 10 && !defined(CTRLM_DPI_CONTROL_NOT_SUPPORTED)
   {IARM_BUS_COMMON_API_PowerPreChange,                      ctrlm_event_handler_power_pre_change                    },
#endif
};


gboolean ctrlm_main_iarm_init(void) {
   IARM_Result_t result;
   guint index;

   // Register calls that can be invoked by IARM bus clients
   for(index = 0; index < sizeof(ctrlm_iarm_calls)/sizeof(ctrlm_iarm_call_t); index++) {
      result = IARM_Bus_RegisterCall(ctrlm_iarm_calls[index].name, ctrlm_iarm_calls[index].handler);
      if(IARM_RESULT_SUCCESS != result) {
         XLOGD_FATAL("Unable to register method %s!", ctrlm_iarm_calls[index].name);
         return(false);
      }
   }

   // Change to running state so we can accept calls
   g_atomic_int_set(&running, 1);

   return(true);
}

void ctrlm_main_iarm_terminate(void) {
   //IARM_Result_t result;
   //guint index;

   // Change to stopped or terminated state, so we do not accept new calls
   g_atomic_int_set(&running, 0);

#ifdef USE_DEPRECATED_IRMGR
   // IARM Events that we are listening to from other processes
   IARM_Bus_RemoveEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_IRKEY, ctrlm_event_handler_ir);
   IARM_Bus_RemoveEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_CONTROL, ctrlm_event_handler_ir);
#endif

   // Unregister calls that can be invoked by IARM bus clients
   //for(index = 0; index < sizeof(ctrlm_iarm_calls)/sizeof(ctrlm_iarm_call_t); index++) {
   //   result = IARM_Bus_UnRegisterCall(ctrlm_iarm_calls[index].name, ctrlm_iarm_calls[index].handler);
   //   if(IARM_RESULT_SUCCESS != result) {
   //      XLOGD_FATAL("Unable to unregister method %s!", ctrlm_iarm_calls[index].name);
   //   }
   //}
}

IARM_Result_t ctrlm_main_iarm_call_status_get(void *arg) {
   ctrlm_main_iarm_call_status_t *status = (ctrlm_main_iarm_call_status_t *) arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(status->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", status->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      status->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");

   if(!ctrlm_main_iarm_call_status_get(status)) {
      status->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_ir_remote_usage_get(void *arg) {
   ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage = (ctrlm_main_iarm_call_ir_remote_usage_t *) arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(ir_remote_usage == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(ir_remote_usage->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", ir_remote_usage->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      ir_remote_usage->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");

   if(!ctrlm_main_iarm_call_ir_remote_usage_get(ir_remote_usage)) {
      ir_remote_usage->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_last_key_info_get(void *arg) {
   ctrlm_main_iarm_call_last_key_info_t *last_key_info = (ctrlm_main_iarm_call_last_key_info_t *) arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(last_key_info == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(last_key_info->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", last_key_info->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      last_key_info->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");

   if(!ctrlm_main_iarm_call_last_key_info_get(last_key_info)) {
      last_key_info->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_network_status_get(void *arg) {
   ctrlm_main_iarm_call_network_status_t *status = (ctrlm_main_iarm_call_network_status_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(status->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", status->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      status->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_network_status_get(status)) {
      status->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_property_set(void *arg) {
   ctrlm_main_iarm_call_property_t *property = (ctrlm_main_iarm_call_property_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == property) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_property_set(property)) {
      property->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_property_get(void *arg) {
   ctrlm_main_iarm_call_property_t *property = (ctrlm_main_iarm_call_property_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == property) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_property_get(property)) {
      property->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_discovery_config_set(void *arg) {
   ctrlm_main_iarm_call_discovery_config_t *config = (ctrlm_main_iarm_call_discovery_config_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == config) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   
   if(config->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", config->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_discovery_config_set(config)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_autobind_config_set(void *arg) {
   ctrlm_main_iarm_call_autobind_config_t *config = (ctrlm_main_iarm_call_autobind_config_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == config) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(config->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", config->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   if(!ctrlm_main_iarm_call_autobind_config_set(config)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_precommission_config_set(void *arg) {
   ctrlm_main_iarm_call_precommision_config_t *config = (ctrlm_main_iarm_call_precommision_config_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == config) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(config->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", config->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   if(!ctrlm_main_iarm_call_precommission_config_set(config)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_controller_unbind(void *arg) {
   ctrlm_main_iarm_call_controller_unbind_t *unbind = (ctrlm_main_iarm_call_controller_unbind_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == unbind) {
      XLOGD_ERROR("NULL Property Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(unbind->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", unbind->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      unbind->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   if(!ctrlm_main_iarm_call_controller_unbind(unbind)) {
      unbind->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

void ctrlm_main_iarm_event_binding_button(gboolean active) {
   ctrlm_main_iarm_event_binding_button_t event;
   event.api_revision = CTRLM_MAIN_IARM_BUS_API_REVISION;
   event.active       = active ? 1 : 0;
   XLOGD_INFO("<%s>", active ? "ACTIVE" : "INACTIVE");
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_MAIN_IARM_EVENT_BINDING_BUTTON, &event, sizeof(event));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_main_iarm_event_binding_line_of_sight(gboolean active) {
   ctrlm_main_iarm_event_binding_button_t event;
   event.api_revision = CTRLM_MAIN_IARM_BUS_API_REVISION;
   event.active       = active ? 1 : 0;
   XLOGD_INFO("<%s>", active ? "ACTIVE" : "INACTIVE");
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_MAIN_IARM_EVENT_BINDING_LINE_OF_SIGHT, &event, sizeof(event));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

void ctrlm_main_iarm_event_autobind_line_of_sight(gboolean active) {
   ctrlm_main_iarm_event_binding_button_t event;
   event.api_revision = CTRLM_MAIN_IARM_BUS_API_REVISION;
   event.active       = active ? 1 : 0;
   XLOGD_INFO("<%s>", active ? "ACTIVE" : "INACTIVE");
   IARM_Result_t result = IARM_Bus_BroadcastEvent(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_MAIN_IARM_EVENT_AUTOBIND_LINE_OF_SIGHT, &event, sizeof(event));
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_ERROR("IARM Bus Error!");
   }
}

IARM_Result_t ctrlm_main_iarm_call_audio_capture_start(void *arg) {
   ctrlm_main_iarm_call_audio_capture_t *capture_start = (ctrlm_main_iarm_call_audio_capture_t *)arg;

   if(NULL == capture_start) {
      XLOGD_ERROR("null parameters");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }

   if(capture_start->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", capture_start->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      capture_start->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }

   XLOGD_INFO("");

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_audio_capture_start_t *msg = (ctrlm_main_queue_msg_audio_capture_start_t *)g_malloc(sizeof(ctrlm_main_queue_msg_audio_capture_start_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(IARM_RESULT_OOM);
   }

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_START;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_INVALID;
   msg->container         = capture_start->container;
   errno_t safec_rc = strcpy_s(msg->file_path, sizeof(msg->file_path), capture_start->file_path);
   ERR_CHK(safec_rc);
   msg->raw_mic_enable    = (capture_start->raw_mic_enable) ? true : false;
   ctrlm_main_queue_msg_push(msg);

   capture_start->result = CTRLM_IARM_CALL_RESULT_SUCCESS;

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_audio_capture_stop(void *arg) {
   ctrlm_main_iarm_call_audio_capture_t *capture_stop = (ctrlm_main_iarm_call_audio_capture_t *)arg;

   if(NULL == capture_stop) {
      XLOGD_ERROR("null parameters");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(capture_stop->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", capture_stop->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      capture_stop->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }

   XLOGD_INFO("");

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_audio_capture_stop_t *msg = (ctrlm_main_queue_msg_audio_capture_stop_t *)g_malloc(sizeof(ctrlm_main_queue_msg_audio_capture_stop_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(IARM_RESULT_OOM);
   }
   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_STOP;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_INVALID;
   ctrlm_main_queue_msg_push(msg);

   capture_stop->result = CTRLM_IARM_CALL_RESULT_SUCCESS;

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_power_state_change(void *arg) {
   //This function will only be reached by ctrlm-testapp
   ctrlm_main_iarm_call_power_state_change_t *power_state = (ctrlm_main_iarm_call_power_state_change_t *)arg;

   if(NULL == power_state) {
      XLOGD_ERROR("null parameters");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(power_state->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION ) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", power_state->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      power_state->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }

   XLOGD_INFO("new state %s", ctrlm_power_state_str(power_state->new_state));
   if(!ctrlm_power_state_change(power_state->new_state)) {
      return(IARM_RESULT_OOM);
   }

   power_state->result = CTRLM_IARM_CALL_RESULT_SUCCESS;

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_control_service_set_values(void *arg) {
   ctrlm_main_iarm_call_control_service_settings_t *settings = (ctrlm_main_iarm_call_control_service_settings_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == settings) {
      XLOGD_ERROR("NULL Settings Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(settings->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", settings->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      settings->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_control_service_set_values(settings)) {
      settings->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_control_service_get_values(void *arg) {
   ctrlm_main_iarm_call_control_service_settings_t *settings = (ctrlm_main_iarm_call_control_service_settings_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == settings) {
      XLOGD_ERROR("NULL Settings Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(settings->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", settings->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      settings->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   
   if(!ctrlm_main_iarm_call_control_service_get_values(settings)) {
      settings->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_control_service_can_find_my_remote(void *arg) {
   ctrlm_main_iarm_call_control_service_can_find_my_remote_t *can_find_my_remote = (ctrlm_main_iarm_call_control_service_can_find_my_remote_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == can_find_my_remote) {
      XLOGD_ERROR("NULL Can Find My Remote Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(can_find_my_remote->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", can_find_my_remote->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      can_find_my_remote->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   
   if(!ctrlm_main_iarm_call_control_service_can_find_my_remote(can_find_my_remote)) {
      can_find_my_remote->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_control_service_start_pairing_mode(void *arg) {
   ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing = (ctrlm_main_iarm_call_control_service_pairing_mode_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == pairing) {
      XLOGD_ERROR("NULL Pairing Mode Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(pairing->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", pairing->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_control_service_start_pairing_mode(pairing)) {
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_control_service_end_pairing_mode(void *arg) {
   ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing = (ctrlm_main_iarm_call_control_service_pairing_mode_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(NULL == pairing) {
      XLOGD_ERROR("NULL Pairing Mode Argument");
      g_assert(0);
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(pairing->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", pairing->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");
   
   if(!ctrlm_main_iarm_call_control_service_end_pairing_mode(pairing)) {
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_pairing_metrics_get(void *arg) {
   ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics = (ctrlm_main_iarm_call_pairing_metrics_t *) arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(pairing_metrics == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(pairing_metrics->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", pairing_metrics->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      pairing_metrics->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");

   if(!ctrlm_main_iarm_call_pairing_metrics_get(pairing_metrics)) {
      pairing_metrics->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_chip_status_get(void *arg) {
   ctrlm_main_iarm_call_chip_status_t *status = (ctrlm_main_iarm_call_chip_status_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(status->api_revision != CTRLM_MAIN_IARM_BUS_API_REVISION) {
      XLOGD_INFO("Unsupported API Revision (%u, %u)", status->api_revision, CTRLM_MAIN_IARM_BUS_API_REVISION);
      status->result = CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION;
      return(IARM_RESULT_SUCCESS);
   }
   XLOGD_INFO("");

#if CTRLM_HAL_RF4CE_API_VERSION < 15 || defined(CTRLM_RF4CE_CHIP_CONNECTIVITY_CHECK_NOT_SUPPORTED)
   status->result = CTRLM_IARM_CALL_RESULT_ERROR_NOT_SUPPORTED;
#else
   if(!ctrlm_main_iarm_call_chip_status_get(status)) {
      status->result = CTRLM_IARM_CALL_RESULT_ERROR;
   }
#endif
   return(IARM_RESULT_SUCCESS);
}

#if CTRLM_HAL_RF4CE_API_VERSION >= 10 && !defined(CTRLM_DPI_CONTROL_NOT_SUPPORTED)
IARM_Result_t ctrlm_event_handler_power_pre_change(void* pArgs)
{
    const IARM_Bus_CommonAPI_PowerPreChange_Param_t* pParams = (const IARM_Bus_CommonAPI_PowerPreChange_Param_t*) pArgs;
    if(0 == g_atomic_int_get(&running)) {
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

IARM_Result_t ctrlm_main_iarm_call_voice_session_begin(void *arg) {
   ctrlm_voice_iarm_call_voice_session_t *params = (ctrlm_voice_iarm_call_voice_session_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
      XLOGD_ERROR("Cannot begin voice session for multiple networks");
      return(IARM_RESULT_INVALID_PARAM);
   }

   XLOGD_INFO("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_voice_session_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_voice_session_begin, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_voice_session_end(void *arg) {
   ctrlm_voice_iarm_call_voice_session_t *params = (ctrlm_voice_iarm_call_voice_session_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
      XLOGD_ERROR("Cannot end voice session for multiple networks");
      return(IARM_RESULT_INVALID_PARAM);
   }

   XLOGD_INFO("params->network_id = <%d>", params->network_id);

   // Signal completion of the operation
   sem_t semaphore;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_voice_session_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_voice_session_end, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   return(IARM_RESULT_SUCCESS);
}

IARM_Result_t ctrlm_main_iarm_call_start_pair_with_code(void *arg) {
   ctrlm_iarm_call_StartPairWithCode_params_t *params = (ctrlm_iarm_call_StartPairWithCode_params_t *)arg;

   if(0 == g_atomic_int_get(&running)) {
      XLOGD_ERROR("IARM Call received when IARM component in stopped/terminated state, reply with ERROR");
      return(IARM_RESULT_INVALID_STATE);
   }
   if(params == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(IARM_RESULT_INVALID_PARAM);
   }
   if(params->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
      XLOGD_ERROR("Cannot start pair with code for multiple networks");
      return(IARM_RESULT_INVALID_PARAM);
   }

   XLOGD_INFO("params->network_id = <%d>, params->pair_code = 0x%X", params->network_id, params->pair_code);

   // Signal completion of the operation
   sem_t semaphore;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_pair_with_code_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);

   msg.params            = params;
   msg.params->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_pair_with_code, &msg, sizeof(msg), NULL, params->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   return(IARM_RESULT_SUCCESS);
}

ctrlm_power_state_t ctrlm_main_iarm_call_get_power_state(void) {
    IARM_Result_t err;
    IARM_Bus_PWRMgr_GetPowerState_Param_t param;
    ctrlm_power_state_t power_state = CTRLM_POWER_STATE_ON;

    err = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetPowerState, (void *)&param, sizeof(param));
    if(err == IARM_RESULT_SUCCESS) {
        power_state = ctrlm_iarm_power_state_map(param.curState);
        #ifdef ENABLE_DEEP_SLEEP
        //If ctrlm restarts with system STANDBY state, set to ON, will receive a DEEP_SLEEP or ON message shortly
        if(power_state == CTRLM_POWER_STATE_STANDBY) {
            power_state = CTRLM_POWER_STATE_ON;
        }
        #endif
        XLOGD_INFO("power state is : <%s>", ctrlm_power_state_str(power_state));
    } else {
        XLOGD_ERROR("IARM bus failed to read power state, defaulting to %s", ctrlm_power_state_str(power_state));
    }

    return power_state;
}

#ifdef ENABLE_DEEP_SLEEP
gboolean ctrlm_main_iarm_networked_standby(void) {
   IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {0};
   IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetNetworkStandbyMode, (void *)&param, sizeof(param));

   if (res != IARM_RESULT_SUCCESS) {
      XLOGD_ERROR("IARM query for network standby mode failed, default to NO!");
      return false;
   }

   return param.bStandbyMode ? true : false; //avoid any conflict between gboolean and bool
}

gboolean ctrlm_main_iarm_wakeup_reason_voice(void) {
   DeepSleep_WakeupReason_t wakeup_reason;
   IARM_Result_t res;
   bool pwrmgr2 = false; 

   if(CTRLM_TR181_RESULT_SUCCESS != ctrlm_tr181_bool_get(CTRLM_RT181_POWER_RFC_PWRMGR2, &pwrmgr2)) {
      XLOGD_INFO("failed to determine Power Manager revision, defaulting to 1");
   }

   res = IARM_Bus_Call(pwrmgr2 ? IARM_BUS_PWRMGR_NAME : IARM_BUS_DEEPSLEEPMGR_NAME, IARM_BUS_DEEPSLEEPMGR_API_GetLastWakeupReason, (void*)&wakeup_reason, sizeof(wakeup_reason));
   if(res != IARM_RESULT_SUCCESS) {
      XLOGD_ERROR("IARM query for wakeup reason failed, returning false!");
      return false;
   }

   XLOGD_INFO("wakeup_reason <%s>", ctrlm_wakeup_reason_str(wakeup_reason));
   
   if(wakeup_reason != DEEPSLEEP_WAKEUPREASON_VOICE) {
      return false;
   }

   return true;
}
#endif
