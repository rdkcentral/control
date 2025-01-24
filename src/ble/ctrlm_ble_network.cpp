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
// Includes

#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_ble_utils.h"
#include "ctrlm_hal_ble.h"
#include "ctrlm_ble_network.h"
#include "ctrlm_ble_controller.h"
#include "ctrlm_ble_iarm.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_key_codes.h"
#include "ctrlm_voice_obj.h"
#include "ctrlm_database.h"
#include "ctrlm_rcu.h"
#include "ctrlm_utils.h"
#include <ctrlm_config_default.h>
#include "ctrlm_vendor_network_factory.h"
#include "json_config.h"
#include "ctrlm_tr181.h"
#include "ctrlm_ipc_device_update.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <jansson.h>
#include <algorithm>
#include <vector>
#include <time.h>
#include <unordered_map>
#include "irMgr.h"   // for IARM_BUS_IRMGR_KEYSRC_
#include "ctrlm_rcp_ipc_iarm_thunder.h"

using namespace std;

// timer requires the value to be in milliseconds
#define MINUTE_IN_MILLISECONDS                (60 * 1000)
#define CTRLM_BLE_STALE_REMOTE_TIMEOUT        (MINUTE_IN_MILLISECONDS * 60)   // 60 minutes
#define CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT    (MINUTE_IN_MILLISECONDS * 5)    // 5 minutes
#define CTRLM_BLE_UPGRADE_PAUSE_TIMEOUT       (MINUTE_IN_MILLISECONDS * 2)    // 2 minutes

typedef struct {
   guint upgrade_controllers_timer_tag;
   guint upgrade_pause_timer_tag;
} ctrlm_ble_network_t;

static ctrlm_ble_network_t g_ctrlm_ble_network;

static const vector<ctrlm_key_code_t> ctrlm_ble_ir_key_names {
   CTRLM_KEY_CODE_POWER_TOGGLE, 
   CTRLM_KEY_CODE_VOL_UP, 
   CTRLM_KEY_CODE_VOL_DOWN, 
   CTRLM_KEY_CODE_MUTE, 
   CTRLM_KEY_CODE_INPUT_SELECT
};


static gboolean ctrlm_ble_upgrade_controllers(gpointer user_data);
static gboolean ctrlm_ble_upgrade_resume(gpointer user_data);
static bool ctrlm_ble_parse_upgrade_image_info(const std::string &filename, ctrlm_ble_upgrade_image_info_t &image_info);

ctrlm_ble_unpair_metrics_t::ctrlm_ble_unpair_metrics_t()
{
}

ctrlm_ble_unpair_metrics_t::ctrlm_ble_unpair_metrics_t(ctrlm_obj_network_ble_t &network) :
   ieee_address_(std::make_shared<ctrlm_ieee_db_addr_t>(0, &network, "last_rcu_unpaired_ieee")),
   unpaired_time_(std::make_shared<ctrlm_uint64_db_attr_t>("Time Unpaired", 0, &network, "last_rcu_unpaired_time")),
   reason_(std::make_shared<ctrlm_string_db_attr_t>("Unpair Reason", "", &network, "last_rcu_unpaired_reason"))
{
   ieee_address_->set_num_bytes(6);
   ieee_address_->set_name("Unpaired IEEE Address");
}

void ctrlm_ble_unpair_metrics_t::db_load() {
   ctrlm_db_attr_read(ieee_address_.get());
   ctrlm_db_attr_read(reason_.get());
   ctrlm_db_attr_read(unpaired_time_.get());
   
   log_rcu_unpair_event();
}
void ctrlm_ble_unpair_metrics_t::write_rcu_unpair_event(uint64_t ieee) {
   ieee_address_->set_value(ieee);
   unpaired_time_->set_value((uint64_t)time(NULL));

   ctrlm_db_attr_write(ieee_address_);
   ctrlm_db_attr_write(reason_);
   ctrlm_db_attr_write(unpaired_time_);

   log_rcu_unpair_event();
}

void ctrlm_ble_unpair_metrics_t::write_rcu_unpair_event(uint64_t ieee, std::string reason) {
   reason_->set_value(reason);
   write_rcu_unpair_event(ieee);
}
void ctrlm_ble_unpair_metrics_t::log_rcu_unpair_event() {
   XLOGD_INFO("Last RCU unpairing event: MAC Address: <%s>, time: <%s>, reason: <%s>", ieee_address_->to_string().c_str(), ctrlm_utils_time_as_string(unpaired_time_->get_value()).c_str(), reason_->to_string().c_str());
}

// Network class factory
static int ctrlm_ble_network_factory(vendor_network_opts_t *opts, json_t *json_config_root, networks_map_t& networks) {

   int num_networks_added = 0;

   #ifdef CTRLM_NETWORK_BLE
   json_t *json_obj_net_ble = NULL;
   if(json_config_root != NULL) { // Extract the BLE network configuration object
      json_obj_net_ble = json_object_get(json_config_root, JSON_OBJ_NAME_NETWORK_BLE);
      if(json_obj_net_ble == NULL || !json_is_object(json_obj_net_ble)) {
         XLOGD_WARN("BLE network object not found");
         json_obj_net_ble = NULL;
      }
   }

   // add network if enabled
   if ( !(opts->ignore_mask & (1 << CTRLM_NETWORK_TYPE_BLUETOOTH_LE)) ) {
      ctrlm_network_id_t network_id = network_id_get_next(CTRLM_NETWORK_TYPE_BLUETOOTH_LE);
      XLOGD_INFO("adding new BLE network object, network_id = %d", network_id);
      networks[network_id] = new ctrlm_obj_network_ble_t(CTRLM_NETWORK_TYPE_BLUETOOTH_LE, network_id, "BLE", opts->mask_key_codes, json_obj_net_ble, g_thread_self());
      ++num_networks_added;
   }
   #endif

   return num_networks_added;
}

// Add ctrlm_ble_network_factory to ctrlm_vendor_network_factory_func_chain automatically during init time
static class ctrlm_ble_network_factory_obj_t {
  public:
    ctrlm_ble_network_factory_obj_t(){
      ctrlm_vendor_network_factory_func_add(ctrlm_ble_network_factory);
    }
} _factory_obj;



// Function Implementations

ctrlm_obj_network_ble_t::ctrlm_obj_network_ble_t(ctrlm_network_type_t type, ctrlm_network_id_t id, const char *name, gboolean mask_key_codes, json_t *json_obj_net_ble, GThread *original_thread) :
   ctrlm_obj_network_t(type, id, name, mask_key_codes, original_thread),
   json_config_(json_incref(json_obj_net_ble)),
   last_rcu_unpair_metrics_(*this),
   ir_rf_database_(true), // remotes with both TV & AVR codes, TV code is favored in the power toggle spot
   is_alive_(make_shared<bool>(true)),
   ble_rcu_interface_(nullptr)
{
   XLOGD_INFO("constructor - Type (%u) Id (%u) Name (%s)", type_, id_, name_.c_str());
   version_                     = "unknown";
   init_result_                 = CTRLM_HAL_RESULT_ERROR;
   ready_                       = false;

   g_ctrlm_ble_network.upgrade_controllers_timer_tag = 0;
   g_ctrlm_ble_network.upgrade_pause_timer_tag = 0;

   ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
   if(rfc) {
      rfc->add_changed_listener(ctrlm_rfc_t::attrs::BLE, std::bind(&ctrlm_obj_network_ble_t::rfc_retrieved_handler, this, std::placeholders::_1));
   }
}

ctrlm_obj_network_ble_t::ctrlm_obj_network_ble_t() {
   XLOGD_INFO("constructor - default");
   is_alive_ = make_shared<bool>(true);
   ble_rcu_interface_ = nullptr;
}

ctrlm_obj_network_ble_t::~ctrlm_obj_network_ble_t() {
   *is_alive_ = false;

   XLOGD_INFO("destructor - Type (%u) Id (%u) Name (%s)", type_, id_, name_.c_str());

   ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_controllers_timer_tag);
   ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_pause_timer_tag);

   ctrlm_ble_iarm_terminate();

   for(auto it = controllers_.begin(); it != controllers_.end(); it++) {
      if(it->second != NULL) {
         delete it->second;
      }
   }

   clearUpgradeImages();
}

// ==================================================================================================================================================================
// BEGIN - Main init functions of the BLE Network and HAL
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
ctrlm_hal_result_t ctrlm_obj_network_ble_t::hal_init_request(GThread *ctrlm_main_thread) {
   ctrlm_main_thread_ = ctrlm_main_thread;
   THREAD_ID_VALIDATE();

   init_result_ = CTRLM_HAL_RESULT_ERROR;


   if (ctrlm_ble_iarm_init()) {
      ble_rcu_interface_ = make_shared<ctrlm_ble_rcu_interface_t>(json_config_);

      json_decref(json_config_);
      json_config_ = NULL;

      if (ble_rcu_interface_ == nullptr) {
         XLOGD_FATAL("Failed to create BLE RCU interface object.  Will be unable to communicate with BLE remotes.");

      } else {

         ble_rcu_interface_->addRcuStatusChangedHandler(Slot<ctrlm_hal_ble_RcuStatusData_t*>(is_alive_, 
               std::bind(&ctrlm_obj_network_ble_t::ind_rcu_status, this, std::placeholders::_1)));

         ble_rcu_interface_->addRcuPairedHandler(Slot<ctrlm_hal_ble_IndPaired_params_t*>(is_alive_, 
               std::bind(&ctrlm_obj_network_ble_t::ind_rcu_paired, this, std::placeholders::_1)));

         ble_rcu_interface_->addRcuUnpairedHandler(Slot<ctrlm_hal_ble_IndUnPaired_params_t*>(is_alive_, 
               std::bind(&ctrlm_obj_network_ble_t::ind_rcu_unpaired, this, std::placeholders::_1)));

         ble_rcu_interface_->addRcuKeypressHandler(Slot<ctrlm_hal_ble_IndKeypress_params_t*>(is_alive_, 
               std::bind(&ctrlm_obj_network_ble_t::ind_keypress, this, std::placeholders::_1)));


         ble_rcu_interface_->startKeyMonitorThread();
         
         m_config = ble_rcu_interface_->getConfigSettings();
         
         init_result_ = CTRLM_HAL_RESULT_SUCCESS;
         ready_ = true;
      }
   }

   return(init_result_);
}

std::string ctrlm_obj_network_ble_t::db_name_get() const {
   return("ble");
}


void ctrlm_obj_network_ble_t::start(GMainLoop* main_loop)
{
   XLOGD_DEBUG("ENTER *****************************************************************");
   THREAD_ID_VALIDATE();

   last_rcu_unpair_metrics_.db_load();

   // Get the controllers from DB
   controllers_load();

   if (ble_rcu_interface_) {
      ble_rcu_interface_->setGMainLoop(main_loop);
      ble_rcu_interface_->initialize();

      auto devices = ble_rcu_interface_->registerDevices();

      // Get the current list of devices from BLE interface object.  
      // If it doesn't have any yet, they will be sent up later through the device added callbacks.
      for(auto const &it : devices) {
         XLOGD_INFO("Getting all properties from HAL for controller <%s> ...", 
               ctrlm_convert_mac_long_to_string(it).c_str());
         ctrlm_hal_ble_rcu_data_t rcu_data = ble_rcu_interface_->getAllDeviceProperties(it);
         controller_add(rcu_data);
      }
   }

   // Print out pairing table
   XLOGD_INFO("====================================================================");
   XLOGD_INFO("                        %u Bound Controllers", controllers_.size());
   XLOGD_INFO("====================================================================");
   for(auto &controller : controllers_) {
      if (controller.second->is_stale(this->stale_remote_time_threshold_get())) {
         XLOGD_TELEMETRY("Stale remote suspected: <%s>", controller.second->ieee_address_get().to_string().c_str());
      }
      controller.second->print_status();
   }

   // Read IR RF Database from database
   ir_rf_database_.load_db();
   XLOGD_INFO("\n%s", this->ir_rf_database_.to_string(true).c_str());
}


void ctrlm_obj_network_ble_t::thread_monitor_poll(ctrlm_thread_monitor_response_t *response) {
   THREAD_ID_VALIDATE();

   if (ble_rcu_interface_) {
      ctrlm_hal_network_property_thread_monitor_t thread_monitor;
      thread_monitor.response = (ctrlm_hal_thread_monitor_response_t *)response;
   
      ble_rcu_interface_->tickleKeyMonitorThread(&thread_monitor);
   }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// END - Main init functions of the BLE Network and HAL
// ==================================================================================================================================================================


// ==================================================================================================================================================================
// BEGIN - Process Requests to the network in CTRLM Main thread context
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ctrlm_obj_network_ble_t::req_process_network_status(void *data, int size) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_main_network_status_t *dqm = (ctrlm_main_queue_msg_main_network_status_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_main_network_status_t));
   g_assert(dqm->cmd_result);

   ctrlm_network_status_ble_t *network_status  = &dqm->status->status.ble;
   errno_t safec_rc = strncpy_s(network_status->version_hal, sizeof(network_status->version_hal), version_get(), CTRLM_MAIN_VERSION_LENGTH - 1);
   ERR_CHK(safec_rc);
   network_status->version_hal[CTRLM_MAIN_VERSION_LENGTH - 1] = '\0';
   network_status->controller_qty = controllers_.size();
   XLOGD_INFO("HAL Version <%s> Controller Qty %u", network_status->version_hal, network_status->controller_qty);
   int index = 0;
   for(auto const &itr : controllers_) {
      network_status->controllers[index] = itr.first;
      index++;
      if(index >= CTRLM_MAIN_MAX_BOUND_CONTROLLERS) {
         break;
      }
   }
   dqm->status->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
   *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;

   ctrlm_obj_network_t::req_process_network_status(data, size);
}

void ctrlm_obj_network_ble_t::req_process_controller_status(void *data, int size) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_controller_status_t *dqm = (ctrlm_main_queue_msg_controller_status_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_controller_status_t));
   g_assert(dqm->cmd_result);

   ctrlm_controller_id_t controller_id = dqm->controller_id;
   if(!controller_exists(controller_id)) {
      XLOGD_WARN("Controller %u NOT present.", controller_id);
      *dqm->cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_ERROR;
   } else {
      controllers_[controller_id]->getStatus(dqm->status);
      *dqm->cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS;
   }

   ctrlm_obj_network_t::req_process_controller_status(data, size);
}

void ctrlm_obj_network_ble_t::req_process_voice_session_begin(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_voice_session_t *dqm = (ctrlm_main_queue_msg_voice_session_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_voice_session_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

#ifdef DISABLE_BLE_VOICE
   XLOGD_WARN("BLE Voice is disabled in ControlMgr, so not starting a voice session.");
   dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
#else
   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id;
      unsigned long long ieee_address = dqm->params->ieee_address;
      if (!getControllerId(ieee_address, &controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         // Currently BLE RCUs only support push-to-talk, so hardcoding here for now
         ctrlm_voice_device_t device = CTRLM_VOICE_DEVICE_PTT;
         ctrlm_voice_session_response_status_t voice_status;

         // only support ADPCM from ble-rcu component
         ctrlm_hal_ble_VoiceEncoding_t  encoding  = CTRLM_HAL_BLE_ENCODING_ADPCM;
         ctrlm_hal_ble_VoiceStreamEnd_t streamEnd = CTRLM_HAL_BLE_VOICE_STREAM_END_ON_KEY_UP;

         ctrlm_voice_format_t voice_format = { .type = CTRLM_VOICE_FORMAT_INVALID };

         if (ble_rcu_interface_) {
            AudioFormat audio_format;
            // Query the ble_rcu_interface to get the voice format for this controller based on the selected codec
            if(!ble_rcu_interface_->getAudioFormat(ieee_address, encoding, audio_format)) {
               XLOGD_ERROR("failed to get audio format from remote");
            } else {
               ctrlm_adpcm_frame_t *adpcm_frame = &voice_format.value.adpcm_frame;
               voice_format.type = CTRLM_VOICE_FORMAT_ADPCM_FRAME;

               audio_format.getFrameInfo(adpcm_frame->size_packet, adpcm_frame->size_header);
               audio_format.getHeaderInfoAdpcm(adpcm_frame->offset_step_size_index, adpcm_frame->offset_predicted_sample_lsb, adpcm_frame->offset_predicted_sample_msb, adpcm_frame->offset_sequence_value, adpcm_frame->sequence_value_min, adpcm_frame->sequence_value_max);

               bool pressAndHoldSupport = audio_format.getPressAndHoldSupport();
               if(!pressAndHoldSupport) {
                  streamEnd = CTRLM_HAL_BLE_VOICE_STREAM_END_ON_AUDIO_DURATION;
               }
               controllers_[controller_id]->setPressAndHoldSupport(pressAndHoldSupport);
            }
         }

         voice_status = ctrlm_get_voice_obj()->voice_session_req(network_id_get(), controller_id, device, voice_format, NULL,
                                                                controllers_[controller_id]->get_model().c_str(),
                                                                controllers_[controller_id]->get_sw_revision().to_string().c_str(),
                                                                controllers_[controller_id]->get_hw_revision().to_string().c_str(), 0.0,
                                                                false, NULL, NULL, NULL, true);
         if (!controllers_[controller_id]->get_capabilities().has_capability(ctrlm_controller_capabilities_t::capability::PAR) && (VOICE_SESSION_RESPONSE_AVAILABLE_PAR_VOICE == voice_status)) {
            XLOGD_WARN("PAR voice is enabled but not supported by BLE controller treating as normal voice session");
            voice_status = VOICE_SESSION_RESPONSE_AVAILABLE;
         }
         if (VOICE_SESSION_RESPONSE_AVAILABLE != voice_status) {
            XLOGD_TELEMETRY("Failed opening voice session in ctrlm_voice_t, error = <%d>", voice_status);
         } else {
            bool success = false;

            if (ble_rcu_interface_) {
               int fd = -1;

               if (!ble_rcu_interface_->startAudioStreaming(ieee_address, encoding, streamEnd, fd)) {
                     XLOGD_ERROR("failed to start audio streaming on remote");
               } else {

                  if (fd < 0) {
                     XLOGD_ERROR("Voice streaming pipe invalid (fd = <%d>), aborting voice session", fd);
                     success = false;
                  } else {
                     XLOGD_INFO("Acquired voice streaming pipe fd = <%d>, sending to voice engine", fd);
                     //Send the fd acquired from bluez to the voice engine
                     success = ctrlm_get_voice_obj()->voice_session_data(network_id_get(), controller_id, fd);
                  }
               }
            }

            if (false == success) {
               XLOGD_TELEMETRY("Failed to start voice streaming, ending voice session...");
               end_voice_session_for_controller(ieee_address, CTRLM_VOICE_SESSION_END_REASON_OTHER_ERROR);
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
   }
#endif   //DISABLE_BLE_VOICE
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

bool ctrlm_obj_network_ble_t::end_voice_session_for_controller(uint64_t ieee_address, ctrlm_voice_session_end_reason_t reason, uint32_t audioDuration) {
   ctrlm_controller_id_t controller_id;

   if (!getControllerId(ieee_address, &controller_id)) {
      XLOGD_ERROR("Controller doesn't exist!");
      return false;
   }

   ctrlm_get_voice_obj()->voice_session_end(network_id_get(), controller_id, reason);

   if (ble_rcu_interface_) {
      if (!ble_rcu_interface_->stopAudioStreaming(ieee_address, audioDuration)) {
         XLOGD_ERROR("failed to stop audio streaming");
         return false;
      }
   }
   return true;
}

void ctrlm_obj_network_ble_t::req_process_voice_session_end(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_voice_session_t *dqm = (ctrlm_main_queue_msg_voice_session_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_voice_session_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;
#ifdef DISABLE_BLE_VOICE
   dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
#else
   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id;
      unsigned long long ieee_address = dqm->params->ieee_address;

      if (!getControllerId(ieee_address, &controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         if (end_voice_session_for_controller(dqm->params->ieee_address, CTRLM_VOICE_SESSION_END_REASON_DONE)) {
            dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         }
      }
   }
#endif   //DISABLE_BLE_VOICE
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_start_pairing(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_start_pairing_t *dqm = (ctrlm_main_queue_msg_start_pairing_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_start_pairing_t));

   dqm->params->set_result(CTRLM_IARM_CALL_RESULT_ERROR, network_id_get());

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      if (ble_rcu_interface_) {
         if (!ble_rcu_interface_->startScanning(dqm->params->timeout * 1000)) {
            XLOGD_ERROR("failed to start BLE remote scan");
         } else {
            dqm->params->set_result(CTRLM_IARM_CALL_RESULT_SUCCESS, network_id_get());
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


void ctrlm_obj_network_ble_t::req_process_pair_with_code(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_pair_with_code_t *dqm = (ctrlm_main_queue_msg_pair_with_code_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_pair_with_code_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      if (ble_rcu_interface_) {
         
         if (dqm->params->key_code == KEY_BLUETOOTH) {
            // KEY_BLUETOOTH means the pairing code is random 3 digit code embedded in the name
            // so use pairWithCode
            if (!ble_rcu_interface_->pairWithCode(dqm->params->pair_code)) {
               // don't log error here, pairWithCode will handle printing the error.  We do
               // this because there is an error that is merely a warning that we don't want
               // logged because it only confuses those analyzing the logs.
               // XLOGD_ERROR("failed to start pairing with code");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         } else {
            // if key_code is either not available or KEY_CONNECT, it means the pairing code is a 
            // hash of the MAC, so use pairWithMacHash
            if (!ble_rcu_interface_->pairWithMacHash(dqm->params->pair_code)) {
               // don't log error here, pairWithMacHash will handle printing the error.  We do
               // this because there is an error that is merely a warning that we don't want
               // logged because it only confuses those analyzing the logs.
               // XLOGD_ERROR("failed to start pairing with MAC hash");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


void ctrlm_obj_network_ble_t::req_process_ir_set_code(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_ir_set_code_t *dqm = (ctrlm_main_queue_msg_ir_set_code_t *)data;
   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_ir_set_code_t));

   if(dqm->success) {*(dqm->success) = false;}

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->controller_id;
      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
      } else if (!controllers_[controller_id]->isSupportedIrdb(dqm->vendor)) {
         XLOGD_ERROR("Unsupported IRDB - not continuing with ir code download!");
      } else {
         if(dqm->ir_codes) {
            ctrlm_irdb_ir_codes_t ir_codes;
            // First add IR Codes to the IR RF Database (this contains all of the logic for maintaining TV vs AVR codes)
            ir_rf_database_.add_irdb_codes(dqm->ir_codes);
            XLOGD_INFO("\n%s", this->ir_rf_database_.to_string(true).c_str());
            // Now get the IR codes for the BLE IR slots
            for(auto key : ctrlm_ble_ir_key_names) {
               if(ir_rf_database_.has_entry(key)) {
                  ir_codes[key] = ir_rf_database_.get_ir_code(key)->get_code();
               }
            }
            // Check if we have both TV & AVR power codes. If so, put the AVR in the AVR power slot, which maps to the secondary power slot on the remote.
            // TV power code is favored in CTRLM_KEY_CODE_POWER_TOGGLE, which maps to the primary power slot on the remote.
            if(ir_rf_database_.has_entry(CTRLM_KEY_CODE_TV_POWER) && ir_rf_database_.has_entry(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)) {
               ir_codes[CTRLM_KEY_CODE_AVR_POWER_TOGGLE] = ir_rf_database_.get_ir_code(CTRLM_KEY_CODE_AVR_POWER_TOGGLE)->get_code();
            }
      
            if (ble_rcu_interface_) {
               if (!ble_rcu_interface_->programIrSignalWaveforms(controllers_[controller_id]->ieee_address_get().get_value(), 
                                                                std::move(ir_codes), dqm->vendor)) {

                  XLOGD_ERROR("failed to program IR signal waveforms on remote");
               } else {
                                                                  
                     if (dqm->success) { *(dqm->success) = true; }

                     controllers_[controller_id]->irdb_entry_id_name_set(CTRLM_IRDB_DEV_TYPE_TV, ir_rf_database_.get_tv_ir_code_id());
                     controllers_[controller_id]->irdb_entry_id_name_set(CTRLM_IRDB_DEV_TYPE_AVR, ir_rf_database_.get_avr_ir_code_id());
                     XLOGD_INFO("irdb_entry_id_name = <%s>", dqm->ir_codes->get_id().c_str());
               }
            }
            // Store the IR codes in the database
            ir_rf_database_.store_db();
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


void ctrlm_obj_network_ble_t::req_process_ir_clear_codes(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_ir_clear_t *dqm = (ctrlm_main_queue_msg_ir_clear_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_ir_clear_t));

   if(dqm->success) {*(dqm->success) = false;}

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->controller_id;
      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
      } else {

         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->eraseIrSignals(controllers_[controller_id]->ieee_address_get().get_value())) {
               XLOGD_ERROR("failed to erase IR signal waveforms on remote");
            } else {
               
               if (dqm->success) { *(dqm->success) = true; }

               ir_rf_database_.clear_ir_codes();
               XLOGD_INFO("\n%s", ir_rf_database_.to_string(true).c_str());
               controllers_[controller_id]->irdb_entry_id_name_set(CTRLM_IRDB_DEV_TYPE_TV, "0");
               controllers_[controller_id]->irdb_entry_id_name_set(CTRLM_IRDB_DEV_TYPE_AVR, "0");
            }
         }
         ir_rf_database_.store_db();
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


void ctrlm_obj_network_ble_t::req_process_get_ir_protocol_support(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_ir_protocol_characteristic_t *dqm = (ctrlm_main_queue_msg_ir_protocol_characteristic_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_ir_protocol_characteristic_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         if (ble_rcu_interface_) {
            const auto irSupport = controllers_[controller_id]->getSupportedIrdbs();
            dqm->params->num_irdbs_supported = irSupport.size();
            for (unsigned int i = 0; i < irSupport.size(); i++) {
               dqm->params->irdbs_supported[i] = irSupport[i];
            }
            dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


void ctrlm_obj_network_ble_t::req_process_set_ir_protocol_control(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_ir_protocol_characteristic_t *dqm = (ctrlm_main_queue_msg_ir_protocol_characteristic_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_ir_protocol_characteristic_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else if ( 0 == dqm->params->num_irdbs_supported ) {
         XLOGD_ERROR("IR protocol control value is NULL!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         if (ble_rcu_interface_) {
            for (int i = 0; i < dqm->params->num_irdbs_supported; i++) {
               if (!ble_rcu_interface_->setIrControl(controllers_[controller_id]->ieee_address_get().get_value(),
                                                     (ctrlm_irdb_vendor_t) dqm->params->irdbs_supported[i]))
               {
                  XLOGD_ERROR("failed to write IR control characteristic to the remote");
               } else {
                  dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
               }
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}


ctrlm_controller_id_t ctrlm_obj_network_ble_t::get_last_used_controller(void) {
   THREAD_ID_VALIDATE();
   ctrlm_controller_id_t controller_id = CTRLM_HAL_CONTROLLER_ID_INVALID;
   time_t time_last_used = 0, time_controller = 0;

   for (auto const &controller : controllers_) {
      time_controller = controller.second->last_key_time_get();
      if(controller.second->get_connected() && time_controller >= time_last_used) {
         time_last_used = time_controller;
         controller_id  = controller.first;
      }
   }
   return(controller_id);
}

void ctrlm_obj_network_ble_t::req_process_find_my_remote(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_find_my_remote_t *dqm = (ctrlm_main_queue_msg_find_my_remote_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_find_my_remote_t));

   dqm->params->set_result(CTRLM_IARM_CALL_RESULT_ERROR, network_id_get());

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = get_last_used_controller();

      if (CTRLM_HAL_CONTROLLER_ID_INVALID == controller_id) {
         XLOGD_ERROR("no connected controllers to find!!");
         dqm->params->set_result(CTRLM_IARM_CALL_RESULT_ERROR, network_id_get());
      } else {
         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->findMe(controllers_[controller_id]->ieee_address_get().get_value(), dqm->params->level)) {
               XLOGD_ERROR("failed to trigger findMe operation on the remote");
            } else {
               dqm->params->set_result(CTRLM_IARM_CALL_RESULT_SUCCESS, network_id_get());
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_get_rcu_unpair_reason(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_get_rcu_unpair_reason_t *dqm = (ctrlm_main_queue_msg_get_rcu_unpair_reason_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_get_rcu_unpair_reason_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {

         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->getUnpairReason(controllers_[controller_id]->ieee_address_get().get_value(), 
                                                    dqm->params->reason))
            {
               XLOGD_ERROR("failed to get unpair reason.");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_get_rcu_reboot_reason(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_get_rcu_reboot_reason_t *dqm = (ctrlm_main_queue_msg_get_rcu_reboot_reason_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_get_rcu_reboot_reason_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {

         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->getRebootReason(controllers_[controller_id]->ieee_address_get().get_value(), 
                                                    dqm->params->reason)) 
            {
               XLOGD_ERROR("failed to get reboot reason");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_get_rcu_last_wakeup_key(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_get_last_wakeup_key_t *dqm = (ctrlm_main_queue_msg_get_last_wakeup_key_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_get_last_wakeup_key_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         
         if (ble_rcu_interface_) {
            dqm->params->key = controllers_[controller_id]->get_last_wakeup_key();
            dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_send_rcu_action(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_send_rcu_action_t *dqm = (ctrlm_main_queue_msg_send_rcu_action_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_send_rcu_action_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      ctrlm_controller_id_t controller_id = dqm->params->controller_id;

      if (!controller_exists(controller_id)) {
         XLOGD_ERROR("Controller doesn't exist!");
         dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      } else {
         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->sendRcuAction(controllers_[controller_id]->ieee_address_get().get_value(), 
                                                  dqm->params->action,
                                                  dqm->params->wait_for_reply)) 
            {
               XLOGD_ERROR("failed to send RCU action to the remote");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::req_process_write_rcu_wakeup_config(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_write_advertising_config_t *dqm = (ctrlm_main_queue_msg_write_advertising_config_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_write_advertising_config_t));

   dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR;
   bool attempted = false;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } 
   else if ((dqm->params->config >= CTRLM_RCU_WAKEUP_CONFIG_INVALID) ||
            (dqm->params->config == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM && dqm->params->customListSize == 0)) {
      XLOGD_ERROR("Invalid parameters!");
      dqm->params->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
   } 
   else {
      for (auto const &controller : controllers_) {
         attempted = true;

         if (ble_rcu_interface_) {
            if (!ble_rcu_interface_->writeAdvertisingConfig(controller.second->ieee_address_get().get_value(), 
                                                            dqm->params->config,
                                                            dqm->params->customList,
                                                            dqm->params->customListSize)) 
            {
               XLOGD_ERROR("failed to write advertising config to the remote");
            } else {
               dqm->params->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
      }
      if (!attempted) {
         XLOGD_ERROR("No BLE paired and connected remotes!");
      }
   }
   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

void ctrlm_obj_network_ble_t::factory_reset(void) {
   THREAD_ID_VALIDATE();

   XLOGD_INFO("Sending RCU action unpair to all controllers.");

   // Since we are factory resetting anyway, don't waste time unpairing the remote after the
   // remote notifies us of unpair reason through RemoteControl service
   this->unpair_on_remote_request_ = false;

   for (auto const &controller : controllers_) {
      if (ble_rcu_interface_) {
         // Make this synchronous so that we wait until the remote successfully received
         // the message.  Otherwise, the system factory reset will continue and might shut
         // down dependent services before the remote receives the message
         ble_rcu_interface_->sendRcuAction(controller.second->ieee_address_get().get_value(), 
                                           CTRLM_BLE_RCU_ACTION_FACTORY_RESET);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool ctrlm_ble_parse_upgrade_image_info(const std::string &filename, ctrlm_ble_upgrade_image_info_t &image_info) {
   gchar *contents = NULL;
   string xml;

   XLOGD_DEBUG("parsing upgrade file <%s>", filename.c_str());

   if(!g_file_test(filename.c_str(), G_FILE_TEST_EXISTS) || !g_file_get_contents(filename.c_str(), &contents, NULL, NULL)) {
      XLOGD_ERROR("unable to get file contents <%s>", filename.c_str());
      return false;
   }
   xml = contents;
   g_free(contents);

   /////////////////////////////////////////////////////////////
   // Required parameters in firmware image metadata file
   /////////////////////////////////////////////////////////////
   image_info.product_name = ctrlm_xml_tag_text_get(xml, "image:productName");
   if(image_info.product_name.length() == 0) {
      XLOGD_ERROR("Missing Product Name");
      return false;
   }

   string version_string = ctrlm_xml_tag_text_get(xml, "image:softwareVersion");
   if(version_string.length() == 0) {
      XLOGD_ERROR("Missing Software Version");
      return false;
   }
   image_info.version_software.from_string(version_string);

   image_info.image_filename  = ctrlm_xml_tag_text_get(xml, "image:fileName");
   if(image_info.image_filename.length() == 0) {
      XLOGD_ERROR("Missing File Name");
      return false;
   }

   /////////////////////////////////////////////////////////////
   // Optional parameters in firmware image metadata file
   /////////////////////////////////////////////////////////////
   version_string = ctrlm_xml_tag_text_get(xml, "image:bootLoaderVersionMin");
   image_info.version_bootloader_min.from_string(version_string);

   version_string = ctrlm_xml_tag_text_get(xml, "image:hardwareVersionMin");
   image_info.version_hardware_min.from_string(version_string);

   string size  = ctrlm_xml_tag_text_get(xml, "image:size");
   if(size.length() != 0) {
      image_info.size = atol(size.c_str());
   }

   string crc  = ctrlm_xml_tag_text_get(xml, "image:CRC");
   if(crc.length() != 0) {
      image_info.crc = strtoul(crc.c_str(), NULL, 16);
   }

   string force_update = ctrlm_xml_tag_text_get(xml, "image:force_update");
   if(force_update.length() == 0) {
      image_info.force_update = false;
   } else if(force_update == "1"){
      XLOGD_INFO("force update flag = TRUE");
      image_info.force_update = true;
   } else {
      XLOGD_ERROR("Invalid force update flag <%s>, setting to FALSE", force_update.c_str());
      image_info.force_update = false;
   }

   return true;
}

void ctrlm_obj_network_ble_t::addUpgradeImage(const ctrlm_ble_upgrade_image_info_t &image_info) {
   if (upgrade_images_.end() != upgrade_images_.find(image_info.product_name)) {
      //compare version, and replace if newer
      if (upgrade_images_[image_info.product_name].version_software < image_info.version_software ||
          image_info.force_update) {
         upgrade_images_[image_info.product_name] = image_info;
      }
   } else {
      upgrade_images_[image_info.product_name] = image_info;
   }
}

void ctrlm_obj_network_ble_t::clearUpgradeImages() {
   // Delete the temp directories where upgrade images were extracted
   for (auto const &upgrade_image : upgrade_images_) {
      ctrlm_archive_remove(upgrade_image.second.path);
   }
   upgrade_images_.clear();
}

static gboolean ctrlm_ble_upgrade_controllers(gpointer user_data) {
   ctrlm_network_id_t* net_id =  (ctrlm_network_id_t*) user_data;
   if (net_id != NULL) {
      // Allocate a message and send it to Control Manager's queue
      ctrlm_main_queue_msg_continue_upgrade_t msg;
      errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
      ERR_CHK(safec_rc);
      msg.retry_all = false;

      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_upgrade_controllers, &msg, sizeof(msg), NULL, *net_id);
   }
   g_ctrlm_ble_network.upgrade_controllers_timer_tag = 0;
   return false;
}

static gboolean ctrlm_ble_upgrade_resume(gpointer user_data) {
   ctrlm_network_id_t* net_id =  (ctrlm_network_id_t*) user_data;
   if (net_id != NULL) {
      // Allocate a message and send it to Control Manager's queue
      ctrlm_main_queue_msg_continue_upgrade_t msg;
      errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
      ERR_CHK(safec_rc);
      msg.retry_all = true;   //clear upgrade_attempted flag for all controllers to retry upgrade for all

      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_upgrade_controllers, &msg, sizeof(msg), NULL, *net_id);
   }
   g_ctrlm_ble_network.upgrade_pause_timer_tag = 0;
   return false;
}

void ctrlm_obj_network_ble_t::req_process_upgrade_controllers(void *data, int size) {
   ctrlm_main_queue_msg_continue_upgrade_t *dqm = (ctrlm_main_queue_msg_continue_upgrade_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_continue_upgrade_t));

   if (upgrade_images_.empty()) {
      XLOGD_WARN("No upgrade images in the queue.");
      return;
   }

   if (dqm->retry_all) {
      XLOGD_INFO("Checking all controllers for upgrades from a clean state.");
      ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_controllers_timer_tag);
      ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_pause_timer_tag);
      for (auto &controller : controllers_) {
         controller.second->setUpgradeAttempted(false);
         controller.second->setUpgradePaused(false);
         controller.second->ota_failure_cnt_session_clear();
      }
   }

   if (!upgrade_in_progress_) {
      for (auto const &upgrade_image : upgrade_images_) {
         for (auto &controller : controllers_) {
            string ota_product_name;
            if (true == controller.second->getOTAProductName(ota_product_name)) {
               if (ota_product_name == upgrade_image.first) {
                  if (controller.second->swUpgradeRequired(upgrade_image.second.version_software, upgrade_image.second.force_update)) {
                     //--------------------------------------------------------------------------------------------------------------
                     // See if the controller is running a fw version that requires a BLE connection param update before an OTA
                     ctrlm_hal_ble_SetBLEConnParams_params_t conn_params;
                     if (controller.second->needsBLEConnParamUpdateBeforeOTA(conn_params.connParams)) {
                        conn_params.ieee_address = controller.second->ieee_address_get().get_value();

                        if (ble_rcu_interface_) {
                           if (!ble_rcu_interface_->setBleConnectionParams(conn_params.ieee_address, conn_params.connParams)) {
                              XLOGD_ERROR("Failed to set BLE connection parameters");
                           } else {
                              XLOGD_INFO("Successfully set BLE connection parameters");
                           }
                        } else {
                           XLOGD_ERROR("Failed to set BLE connection parameters");
                        }
                     }
                     //--------------------------------------------------------------------------------------------------------------

                     XLOGD_INFO("Attempting to upgrade controller %s from <%s> to <%s>",
                           controller.second->ieee_address_get().to_string().c_str(),
                           controller.second->get_sw_revision().to_string().c_str(),
                           upgrade_image.second.version_software.to_string().c_str());

                     // Mark that an upgrade was attempted for the remote.  If there is a failure, it will retry only
                     // a couple times to prevent continuously failing upgrade attempts.
                     controller.second->setUpgradeAttempted(true);


                     if (ble_rcu_interface_) {
                        ble_rcu_interface_->startUpgrade(controller.second->ieee_address_get().get_value(), 
                                                         upgrade_image.second.path + "/" + upgrade_image.second.image_filename);
                     }

                     // Not only will this timer be used to check if other remotes need upgrades, it will also check if the previous 
                     // upgrades succeeded successfully.  Some OTA problems will not cause any errors to be returned, but the remote
                     // simply will not take the reboot and the software revision will remain unchanged.
                     XLOGD_DEBUG("Upgrading one remote at a time, setting timer for %d minutes to check if other remotes need upgrades.", CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT / MINUTE_IN_MILLISECONDS);
                     ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_controllers_timer_tag);
                     g_ctrlm_ble_network.upgrade_controllers_timer_tag = ctrlm_timeout_create(CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT, ctrlm_ble_upgrade_controllers, &id_);
                     return;
                  } else {
                     XLOGD_INFO("Software upgrade not required for controller %s.", controller.second->ieee_address_get().to_string().c_str());
                  }
               }
            }
         }
      }
      // We looped through all the upgrade images and all the controllers and didn't trigger any upgrades, so stop upgrade process.
      XLOGD_INFO("Exiting controller upgrade procedure.");

      for (auto &controller : controllers_) {
         if (true == controller.second->is_controller_type_z() && false == controller.second->getUpgradeAttempted()) {
            // For type Z controllers, if no upgrade was even attempted after checking all available images then increment the type Z counter
            XLOGD_WARN("Odd... Controller %s is type Z and an upgrade was not attempted.  Increment counter to ensure it doesn't get stuck as type Z.",
                  controller.second->ieee_address_get().to_string().c_str());
            controller.second->ota_failure_type_z_cnt_set(controller.second->ota_failure_type_z_cnt_get() + 1);
         }
      }

      // Make sure xconf config file is updated
      ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
      if(msg == NULL) {
         XLOGD_ERROR("Out of memory");
      } else {
         msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
         ctrlm_main_queue_msg_push((gpointer)msg);
      }
   } else {
      XLOGD_INFO("Upgrade currently in progress, setting timer for %d minutes to check if other remotes need upgrades.", CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT / MINUTE_IN_MILLISECONDS);
      ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_controllers_timer_tag);
      g_ctrlm_ble_network.upgrade_controllers_timer_tag = ctrlm_timeout_create(CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT, ctrlm_ble_upgrade_controllers, &id_);
   }
}

void ctrlm_obj_network_ble_t::req_process_network_managed_upgrade(void *data, int size) {
   ctrlm_main_queue_msg_network_fw_upgrade_t *dqm = (ctrlm_main_queue_msg_network_fw_upgrade_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_network_fw_upgrade_t));
   g_assert(dqm->network_managing_upgrade != NULL);

   XLOGD_INFO("%s network will manage the RCU firmware upgrade", name_get());
   *dqm->network_managing_upgrade = true;

   if (!ready_) {
      XLOGD_FATAL("Network is not ready!");
   } else {
      string archive_file_path = dqm->archive_file_path;
      size_t idx = archive_file_path.rfind('/');
      string archive_file_name = archive_file_path.substr(idx + 1);

      string dest_path = string(dqm->temp_dir_path) + "ctrlm/" + archive_file_name;

      if (ctrlm_file_exists(dest_path.c_str())) {
         XLOGD_INFO("dest_path <%s> already exists, removing it...", dest_path.c_str());
         ctrlm_archive_remove(dest_path);
      }
      if (ctrlm_archive_extract(archive_file_path, dqm->temp_dir_path, archive_file_name)) {
         //extract metadata from image.xml
         ctrlm_ble_upgrade_image_info_t image_info;
         image_info.path = dest_path;
         string image_xml_filename = dest_path + "/image.xml";
         if (ctrlm_ble_parse_upgrade_image_info(image_xml_filename, image_info)) {
            addUpgradeImage(image_info);
         }
      }

      // Allocate a message and send it to Control Manager's queue
      ctrlm_main_queue_msg_continue_upgrade_t msg;
      errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
      ERR_CHK(safec_rc);
      msg.retry_all = true;

      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_upgrade_controllers, &msg, sizeof(msg), NULL, id_);
   }

   if(dqm->semaphore) {
      sem_post(dqm->semaphore);
   }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// END - Process Requests to the network in CTRLM Main thread context
// ==================================================================================================================================================================


// ==================================================================================================================================================================
// BEGIN - Process Indications from HAL to the network in CTRLM Main thread context
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ctrlm_obj_network_ble_t::ind_rcu_status(ctrlm_hal_ble_RcuStatusData_t *params) {

   // push to the main queue and process it synchronously there
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, 
         (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::ind_process_rcu_status, 
         params, sizeof(*params), NULL, id_);
}
   
void ctrlm_obj_network_ble_t::ind_process_rcu_status(void *data, int size) {
   // XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();
   bool report_status = true;
   bool print_status = true;

   ctrlm_hal_ble_RcuStatusData_t *dqm = (ctrlm_hal_ble_RcuStatusData_t *)data;
   g_assert(dqm);
   if (sizeof(ctrlm_hal_ble_RcuStatusData_t) != size) {
      XLOGD_ERROR("Invalid size!");
      return;
   }
   if (!ready_) {
      XLOGD_INFO("Network is not ready!");
      return;
   }

   switch (dqm->property_updated) {
      // These properties are associated with the network, not a specific RCU
      case CTRLM_HAL_BLE_PROPERTY_IS_PAIRING:
         // don't send up status event for this since CTRLM_HAL_BLE_PROPERTY_STATE handles all pairing states.
         report_status = false;
         print_status = false;
         break;
      case CTRLM_HAL_BLE_PROPERTY_PAIRING_CODE:
         report_status = false;  // don't send up status event for this.
         break;
      case CTRLM_HAL_BLE_PROPERTY_STATE:
         XLOGD_TELEMETRY("BLE remote RF pairing state changed to <%s>", ctrlm_rf_pair_state_str(dqm->state));
         state_ = dqm->state;
         break;
      case CTRLM_HAL_BLE_PROPERTY_IR_STATE:
         XLOGD_TELEMETRY("BLE remote IR programming state changed to <%s>", ctrlm_ir_state_str(dqm->ir_state));
         ir_state_ = dqm->ir_state;
         break;
      default:
      {
         // These properties are associated with a specific RCU, so make sure the controller object exists before continuing
         ctrlm_controller_id_t controller_id;
         if (false == getControllerId(dqm->rcu_data.ieee_address, &controller_id)) {
            XLOGD_ERROR("Controller <%s> NOT found in the network!!", 
                  ctrlm_convert_mac_long_to_string(dqm->rcu_data.ieee_address).c_str());
            report_status = false;
            print_status = false;
         } else {
            auto controller = controllers_[controller_id];

            switch (dqm->property_updated) {
               case CTRLM_HAL_BLE_PROPERTY_DEVICE_ID:
                  controller->set_device_minor_id(dqm->rcu_data.device_minor_id);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_NAME:
                  controller->setName(string(dqm->rcu_data.name), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_MANUFACTURER:
                  controller->setManufacturer(string(dqm->rcu_data.manufacturer), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_MODEL:
                  controller->setModel(string(dqm->rcu_data.model), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_SERIAL_NUMBER:
                  controller->setSerialNumber(string(dqm->rcu_data.serial_number), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_HW_REVISION:
                  controller->setHwRevision(string(dqm->rcu_data.hw_revision), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_FW_REVISION:
                  controller->setFwRevision(string(dqm->rcu_data.fw_revision), true);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_SW_REVISION: {
                  controller->setSwRevision(string(dqm->rcu_data.sw_revision), true);
                  // SW Rev updated, make sure xconf config file is updated
                  ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
                  if(msg == NULL) {
                     XLOGD_ERROR("Out of memory");
                  } else {
                     msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
                     ctrlm_main_queue_msg_push((gpointer)msg);
                  }
                  break;
               }
               case CTRLM_HAL_BLE_PROPERTY_IR_CODE:
                  controller->setIrCode(dqm->rcu_data.ir_code);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_TOUCH_MODE:
                  controller->setTouchMode(dqm->rcu_data.touch_mode);
                  print_status = false;
                  report_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_TOUCH_MODE_SETTABLE:
                  controller->setTouchModeSettable(dqm->rcu_data.touch_mode_settable);
                  print_status = false;
                  report_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_BATTERY_LEVEL:
                  controller->setBatteryPercent(dqm->rcu_data.battery_level, true);
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_CONNECTED:
                  controller->setConnected(dqm->rcu_data.connected);
                  break;
               case CTRLM_HAL_BLE_PROPERTY_AUDIO_STREAMING:
                  controller->setAudioStreaming(dqm->rcu_data.audio_streaming);
                  report_status = false;
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_AUDIO_GAIN_LEVEL:
                  controller->setAudioGainLevel(dqm->rcu_data.audio_gain_level);
                  report_status = false;
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_AUDIO_CODECS:
                  controller->setAudioCodecs(dqm->rcu_data.audio_codecs);
                  report_status = false;
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_IS_UPGRADING:
                  XLOGD_INFO("Controller <%s> firmware upgrading = %s", controller->ieee_address_get().to_string().c_str(), dqm->rcu_data.is_upgrading ? "TRUE" : "FALSE");
                  upgrade_in_progress_ = dqm->rcu_data.is_upgrading;
                  if (!dqm->rcu_data.is_upgrading) {
                     // If we get FALSE here, make sure the controller upgrade progress flag is cleared.  But we don't want to set the controller progress
                     // flag to TRUE based on this property.  Controller upgrade progress flag should only be set to true if packets are actively being sent
                     controller->setUpgradeInProgress(false);
                  }
                  report_status = false;
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_UPGRADE_PROGRESS:
                  XLOGD_INFO("Controller <%s> firmware upgrade %d%% complete...", controller->ieee_address_get().to_string().c_str(), dqm->rcu_data.upgrade_progress);
                  // From a controller perspective, we cannot use the CTRLM_HAL_BLE_PROPERTY_IS_UPGRADING flag above to determine if its actively upgrading.
                  // Instead, its more accurate to use the progress percentage to determine if the remote is actively receiving firmware packets.
                  controller->setUpgradeInProgress(dqm->rcu_data.upgrade_progress > 0 && dqm->rcu_data.upgrade_progress < 100);
                  report_status = false;
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_UPGRADE_ERROR:
                  XLOGD_ERROR("Controller <%s> firmware upgrade FAILED with error <%s>.", controller->ieee_address_get().to_string().c_str(), dqm->rcu_data.upgrade_error);
                  report_status = false;
                  print_status = false;
                  if (controller->retry_ota()) {
                     controller->setUpgradeAttempted(false);
                     XLOGD_WARN("Upgrade failed, setting timer for %d minutes to retry.", CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT / MINUTE_IN_MILLISECONDS);
                     ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_controllers_timer_tag);
                     g_ctrlm_ble_network.upgrade_controllers_timer_tag = ctrlm_timeout_create(CTRLM_BLE_UPGRADE_CONTINUE_TIMEOUT, ctrlm_ble_upgrade_controllers, &id_);
                  } else {
                     controller->setUpgradeError(string(dqm->rcu_data.upgrade_error));
                     XLOGD_ERROR("Controller <%s> OTA upgrade keeps failing and reached maximum retries.  Won't try again until a new request is sent.", controller->ieee_address_get().to_string().c_str());

                     // Make sure xconf config file is updated
                     ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
                     if(msg == NULL) {
                        XLOGD_ERROR("Out of memory");
                     } else {
                        msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
                        ctrlm_main_queue_msg_push((gpointer)msg);
                     }
                  }
                  controller->ota_failure_cnt_incr();
                  break;
               case CTRLM_HAL_BLE_PROPERTY_UNPAIR_REASON:
                  XLOGD_INFO("Controller <%s> notified reason for unpairing = <%s>", controller->ieee_address_get().to_string().c_str(), ctrlm_ble_unpair_reason_str(dqm->rcu_data.unpair_reason));
                  last_rcu_unpair_metrics_.write_rcu_unpair_event(controller->ieee_address_get().get_value(), string(ctrlm_ble_unpair_reason_str(dqm->rcu_data.unpair_reason)));
                  report_status = false;
                  print_status = false;
                  if (this->unpair_on_remote_request_ && 
                     (dqm->rcu_data.unpair_reason == CTRLM_BLE_RCU_UNPAIR_REASON_SFM ||
                      dqm->rcu_data.unpair_reason == CTRLM_BLE_RCU_UNPAIR_REASON_FACTORY_RESET ||
                      dqm->rcu_data.unpair_reason == CTRLM_BLE_RCU_UNPAIR_REASON_RCU_ACTION) ) {
                     
                     if (ble_rcu_interface_) {
                        ble_rcu_interface_->unpairDevice(dqm->rcu_data.ieee_address);
                     }
                  }
                  break;
               case CTRLM_HAL_BLE_PROPERTY_REBOOT_REASON:
                  XLOGD_TELEMETRY("Controller <%s> notified reason for rebooting = <%s%s%s%s>", 
                        controller->ieee_address_get().to_string().c_str(), 
                        ctrlm_ble_reboot_reason_str(dqm->rcu_data.reboot_reason),
                        dqm->rcu_data.reboot_reason == CTRLM_BLE_RCU_REBOOT_REASON_ASSERT ? " - \"" : "",
                        dqm->rcu_data.reboot_reason == CTRLM_BLE_RCU_REBOOT_REASON_ASSERT ? dqm->rcu_data.assert_report : "" ,
                        dqm->rcu_data.reboot_reason == CTRLM_BLE_RCU_REBOOT_REASON_ASSERT ? "\"" : "");

                  report_status = false;
                  print_status = false;
                  if (dqm->rcu_data.reboot_reason == CTRLM_BLE_RCU_REBOOT_REASON_FW_UPDATE) {
                     controller->ota_clear_all_failure_counters();

                     // Make sure xconf config file is updated
                     ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
                     if(msg == NULL) {
                        XLOGD_ERROR("Out of memory");
                     } else {
                        msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
                        ctrlm_main_queue_msg_push((gpointer)msg);
                     }
                  }
                  break;
               case CTRLM_HAL_BLE_PROPERTY_LAST_WAKEUP_KEY:
                  XLOGD_INFO("Controller <%s> notified last wakeup key = %u (%s key)", controller->ieee_address_get().to_string().c_str(),
                        dqm->rcu_data.last_wakeup_key, ctrlm_linux_key_code_str(dqm->rcu_data.last_wakeup_key, false));
                  controller->setLastWakeupKey(dqm->rcu_data.last_wakeup_key);
                  print_status = false;
                  break;
               case CTRLM_HAL_BLE_PROPERTY_WAKEUP_CONFIG:
                  controller->setWakeupConfig(dqm->rcu_data.wakeup_config);
                  XLOGD_INFO("Controller <%s> notified wakeup config = <%s>", controller->ieee_address_get().to_string().c_str(), ctrlm_rcu_wakeup_config_str(controller->get_wakeup_config()));
                  if (controller->get_wakeup_config() == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
                     // Don't report status yet if its a custom config.  Do that after we receive the custom list
                     report_status = false;
                     print_status = false;
                  }
                  break;
               case CTRLM_HAL_BLE_PROPERTY_WAKEUP_CUSTOM_LIST:
                  controller->setWakeupCustomList(dqm->rcu_data.wakeup_custom_list, dqm->rcu_data.wakeup_custom_list_size);
                  XLOGD_INFO("Controller <%s> notified wakeup custom list = <%s>", controller->ieee_address_get().to_string().c_str(), controller->wakeupCustomListToString().c_str());
                  if (controller->get_wakeup_config() != CTRLM_RCU_WAKEUP_CONFIG_CUSTOM) {
                     // Only report status if the config is set to custom
                     report_status = false;
                     print_status = false;
                  }
                  break;
               case CTRLM_HAL_BLE_PROPERTY_IRDBS_SUPPORTED:
                  controller->setSupportedIrdbs(dqm->rcu_data.irdbs_supported, dqm->rcu_data.num_irdbs_supported);
                  report_status = false;
                  print_status = false;
                  break;
               default:
                  XLOGD_WARN("Unhandled Property: %d !!!!!!!!!!!!!!!!!!!!!!!!", dqm->property_updated);
                  report_status = false;
                  print_status = false;
                  break;
            }
         }
         break;
      }
   }

   if (true == print_status) {
      printStatus();
   }
   if (true == report_status) {
      iarm_event_rcu_status();
   }
}

void ctrlm_obj_network_ble_t::populate_rcu_status_message(ctrlm_iarm_RcuStatus_params_t *msg) {
   XLOGD_DEBUG("Enter...");

   msg->api_revision  = CTRLM_BLE_IARM_BUS_API_REVISION;
   msg->network_id = network_id_get();
   msg->status = state_;
   msg->ir_state = ir_state_;

   int i = 0;
   errno_t safec_rc = -1;
   for(auto it = controllers_.begin(); it != controllers_.end(); it++) {
      msg->remotes[i].controller_id             = it->second->controller_id_get();
      msg->remotes[i].deviceid                  = it->second->get_device_minor_id();
      msg->remotes[i].batterylevel              = it->second->get_battery_percent();
      msg->remotes[i].connected                 = (it->second->get_connected()) ? 1 : 0;
      msg->remotes[i].wakeup_key_code           = (CTRLM_KEY_CODE_INVALID == it->second->get_last_wakeup_key()) ? -1 : it->second->get_last_wakeup_key();
      msg->remotes[i].wakeup_config             = it->second->get_wakeup_config();
      msg->remotes[i].wakeup_custom_list_size   = it->second->get_wakeup_custom_list().size();
      int idx = 0;
      for (auto const key : it->second->get_wakeup_custom_list()) {
         msg->remotes[i].wakeup_custom_list[idx] = key;
         idx++;
      }

      safec_rc = strncpy_s(msg->remotes[i].ieee_address_str, sizeof(msg->remotes[i].ieee_address_str), it->second->ieee_address_get().to_string().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].btlswver, sizeof(msg->remotes[i].btlswver), it->second->get_fw_revision().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].hwrev, sizeof(msg->remotes[i].hwrev), it->second->get_hw_revision().to_string().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].rcuswver, sizeof(msg->remotes[i].rcuswver), it->second->get_sw_revision().to_string().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);

      safec_rc = strncpy_s(msg->remotes[i].make, sizeof(msg->remotes[i].make), it->second->get_manufacturer().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].model, sizeof(msg->remotes[i].model), it->second->get_model().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].name, sizeof(msg->remotes[i].name), it->second->get_name().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].serialno, sizeof(msg->remotes[i].serialno), it->second->get_serial_number().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);

      safec_rc = strncpy_s(msg->remotes[i].tv_code, sizeof(msg->remotes[i].tv_code), it->second->get_irdb_entry_id_name_tv().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      safec_rc = strncpy_s(msg->remotes[i].avr_code, sizeof(msg->remotes[i].avr_code), it->second->get_irdb_entry_id_name_avr().c_str(), CTRLM_MAX_PARAM_STR_LEN-1); ERR_CHK(safec_rc);
      i++;
   }
   msg->num_remotes = i;
}

ctrlm_controller_id_t ctrlm_obj_network_ble_t::controller_add(ctrlm_hal_ble_rcu_data_t &rcu_data) {
   ctrlm_controller_id_t id;
   if (false == getControllerId(rcu_data.ieee_address, &id)) {
      XLOGD_INFO("Controller (%s) doesn't exist in the network, adding...", 
            ctrlm_convert_mac_long_to_string(rcu_data.ieee_address).c_str());
      
      id = controller_id_assign();
      controllers_[id] = new ctrlm_obj_controller_ble_t(id, *this, rcu_data.ieee_address, CTRLM_BLE_RESULT_VALIDATION_PENDING);
      controllers_[id]->db_create();

      // It already paired successfully if it indicated up to the network.  No further validation needed so we can validate now
      controllers_[id]->validation_result_set(CTRLM_BLE_RESULT_VALIDATION_SUCCESS);
   } else {
      XLOGD_INFO("Controller (%s) already exists in the network, updating data...", 
            ctrlm_convert_mac_long_to_string(rcu_data.ieee_address).c_str());
   }
   if (controller_exists(id)) {
      auto controller = controllers_[id];
      controller->setName(string(rcu_data.name));
      controller->setAudioCodecs(rcu_data.audio_codecs);
      controller->setConnected(rcu_data.connected);
      // only update these parameters if they are not empty or invalid.
      if (rcu_data.serial_number[0] != '\0') { controller->setSerialNumber(string(rcu_data.serial_number)); }
      if (rcu_data.manufacturer[0] != '\0') { controller->setManufacturer(string(rcu_data.manufacturer)); }
      if (rcu_data.model[0] != '\0') { controller->setModel(string(rcu_data.model)); }
      if (rcu_data.fw_revision[0] != '\0') { controller->setFwRevision(string(rcu_data.fw_revision)); }
      if (rcu_data.hw_revision[0] != '\0') { controller->setHwRevision(string(rcu_data.hw_revision)); }
      if (rcu_data.sw_revision[0] != '\0') { controller->setSwRevision(string(rcu_data.sw_revision)); }
      if (rcu_data.audio_gain_level != 0xFF) { controller->setAudioGainLevel(rcu_data.audio_gain_level); }
      if (rcu_data.battery_level != 0xFF) { controller->setBatteryPercent(rcu_data.battery_level); }
      if (rcu_data.wakeup_config != 0xFF) { controller->setWakeupConfig(rcu_data.wakeup_config); }
      if (rcu_data.wakeup_custom_list_size != 0) { controller->setWakeupCustomList(rcu_data.wakeup_custom_list, rcu_data.wakeup_custom_list_size); }
      if (rcu_data.num_irdbs_supported != 0) { controller->setSupportedIrdbs(rcu_data.irdbs_supported, rcu_data.num_irdbs_supported); }
      if (rcu_data.last_wakeup_key != 0xFF) { controller->setLastWakeupKey(rcu_data.last_wakeup_key); }

      controller->db_store();
   }
   return id;
}


void ctrlm_obj_network_ble_t::ind_rcu_paired(ctrlm_hal_ble_IndPaired_params_t *params) {

   // push to the main queue and process it synchronously there
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, 
         (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::ind_process_paired, 
         params, sizeof(*params), NULL, id_);
}

void ctrlm_obj_network_ble_t::ind_process_paired(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();

   ctrlm_hal_ble_IndPaired_params_t *dqm = (ctrlm_hal_ble_IndPaired_params_t *)data;
   g_assert(dqm);
   if (sizeof(ctrlm_hal_ble_IndPaired_params_t) != size) {
      XLOGD_ERROR("Invalid size!");
      return;
   }
   if (!ready_) {
      XLOGD_INFO("Network is not ready!");
      return;
   }

   ctrlm_controller_id_t id = controller_add(dqm->rcu_data);
   if (controller_exists(id)) {
      controllers_[id]->print_status();
   }

   // Sync currently connected devices with the HAL

   if (ble_rcu_interface_) {
      auto devices = ble_rcu_interface_->getManagedDevices();

      for(auto it = controllers_.cbegin(); it != controllers_.cend(); ) {

         if (devices.end() == std::find(devices.begin(), devices.end(), it->second->ieee_address_get().get_value()) ) {

            XLOGD_TELEMETRY("Controller (ID = %u), MAC Address: <%s> not paired according to HAL, so removing...", 
                  it->first, it->second->ieee_address_get().to_string().c_str());

            //remote stored in network database is not a paired remote as reported by the HAL, so remove it.
            controller_remove(it->first);
            it = controllers_.erase(it);
         } else {
            ++it;
         }
      }
   }


   // Export new controller to xconf config file
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
   } else {
      msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
      ctrlm_main_queue_msg_push((gpointer)msg);
   }
}

void ctrlm_obj_network_ble_t::ind_rcu_unpaired(ctrlm_hal_ble_IndUnPaired_params_t *params) {

   // push to the main queue and process it synchronously there
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, 
         (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::ind_process_unpaired, 
         params, sizeof(*params), NULL, id_);
}

void ctrlm_obj_network_ble_t::ind_process_unpaired(void *data, int size) {
   XLOGD_DEBUG("Enter...");
   THREAD_ID_VALIDATE();

   ctrlm_hal_ble_IndUnPaired_params_t *dqm = (ctrlm_hal_ble_IndUnPaired_params_t *)data;
   g_assert(dqm);
   if (sizeof(ctrlm_hal_ble_IndUnPaired_params_t) != size) {
      XLOGD_ERROR("Invalid size!");
      return;
   }
   if (!ready_) {
      XLOGD_INFO("Network is not ready!");
      return;
   }

   ctrlm_controller_id_t id;
   if (true == getControllerId(dqm->ieee_address, &id)) {
       XLOGD_TELEMETRY("Removing controller with ieee_address = %s", controllers_[id]->ieee_address_get().to_string().c_str());
       
      // This indication comes after the CTRLM_HAL_BLE_PROPERTY_UNPAIR_REASON notification, so we don't want to overwrite the reason.
      // However, this creates the possibility of the reason being from the previous unpair event and not this one.
      last_rcu_unpair_metrics_.write_rcu_unpair_event(controllers_[id]->ieee_address_get().get_value());

      controller_remove(id);
      controllers_.erase(id);
      // report updated controller status to the plugin
      printStatus();
      iarm_event_rcu_status();

      // Update xconf config file
      ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
      if(msg == NULL) {
         XLOGD_ERROR("Out of memory");
      } else {
         msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST;
         ctrlm_main_queue_msg_push((gpointer)msg);
      }
   } else {
      XLOGD_WARN("Controller (%s) doesn't exist in the network, doing nothing...", 
            ctrlm_convert_mac_long_to_string(dqm->ieee_address).c_str());
   }
}

void ctrlm_obj_network_ble_t::ind_keypress(ctrlm_hal_ble_IndKeypress_params_t *params) {

   // push to the main queue and process it synchronously there
   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, 
         (ctrlm_msg_handler_network_t)&ctrlm_obj_network_ble_t::ind_process_keypress, 
         params, sizeof(*params), NULL, id_);
}

void ctrlm_obj_network_ble_t::ind_process_keypress(void *data, int size) {
   THREAD_ID_VALIDATE();

   ctrlm_hal_ble_IndKeypress_params_t *dqm = (ctrlm_hal_ble_IndKeypress_params_t *)data;
   g_assert(dqm);
   if (sizeof(ctrlm_hal_ble_IndKeypress_params_t) != size) {
      XLOGD_ERROR("Invalid size!");
      return;
   }
   if (!ready_) {
      XLOGD_INFO("Network is not ready!");
      return;
   }

   ctrlm_key_status_t key_status = CTRLM_KEY_STATUS_INVALID;
   switch (dqm->event.value) {
      case 0: { key_status = CTRLM_KEY_STATUS_UP; break; }
      case 1: { key_status = CTRLM_KEY_STATUS_DOWN; break; }
      case 2: { key_status = CTRLM_KEY_STATUS_REPEAT; break; }
      default: break;
   }

   ctrlm_controller_id_t controller_id;

   if (true == getControllerId(dqm->ieee_address, &controller_id)) {
      ctrlm_obj_controller_ble_t *controller = controllers_[controller_id];

      if (key_status == CTRLM_KEY_STATUS_DOWN) {

         if (controller->isVoiceKey(dqm->event.code)) {
            rdkx_timestamp_t keyDownTime;
            keyDownTime.tv_sec  = dqm->event.time.tv_sec;
            keyDownTime.tv_nsec = dqm->event.time.tv_usec * 1000;

            controller->setVoiceStartTime(keyDownTime);

            XLOGD_INFO("------------------------------------------------------------------------");
            XLOGD_INFO("CODE_VOICE_KEY button PRESSED event for device: %s", controller->ieee_address_get().to_string().c_str());
            XLOGD_INFO("------------------------------------------------------------------------");

            ctrlm_voice_iarm_call_voice_session_t v_params;
            v_params.ieee_address = dqm->ieee_address;

            ctrlm_main_queue_msg_voice_session_t msg;
            errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
            ERR_CHK(safec_rc);
            msg.params = &v_params;

            req_process_voice_session_begin(&msg, sizeof(msg));
         }

         if (controller->getUpgradeInProgress()) {
            if (controller->getUpgradePauseSupported()) {

               if (ble_rcu_interface_) {
                  if (!ble_rcu_interface_->cancelUpgrade(dqm->ieee_address)) {
                     XLOGD_ERROR("failed to cancel remote firmware upgrade");
                  } else {
                     controller->setUpgradePaused(true);
                  }
               }
            } else {
               XLOGD_WARN("Remote upgrade in progress, but cannot pause the upgrade because its not supported in this remote firmware.");
            }
         }
         if (controller->getUpgradePaused()) {

            XLOGD_INFO("Upgrade cancelled since remote is being used.  Setting timer to resume upgrade in %d minutes.", CTRLM_BLE_UPGRADE_PAUSE_TIMEOUT / MINUTE_IN_MILLISECONDS);
            //delete existing timer and start again
            ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_pause_timer_tag);
            g_ctrlm_ble_network.upgrade_pause_timer_tag = ctrlm_timeout_create(CTRLM_BLE_UPGRADE_PAUSE_TIMEOUT, ctrlm_ble_upgrade_resume, &id_);

         } else if (controller->getUpgradeStuck() && !upgrade_in_progress_) {

            XLOGD_INFO("Remote <%s> could be in an upgrade stuck state.  Triggering another attempt now while the remote is being used, which can resolve the stuck state.",
                  controller->ieee_address_get().to_string().c_str());
            //delete existing timer and immediately kick off another upgrade
            ctrlm_timeout_destroy(&g_ctrlm_ble_network.upgrade_pause_timer_tag);
            g_ctrlm_ble_network.upgrade_pause_timer_tag = ctrlm_timeout_create(500, ctrlm_ble_upgrade_resume, &id_);
         }

      } else if (key_status == CTRLM_KEY_STATUS_UP) {

         if (controller->isVoiceKey(dqm->event.code)) {
            if(!controller->getPressAndHoldSupport()) { // if the voice session is "Press and Release" then don't end session on voice key up event
               XLOGD_INFO("------------------------------------------------------------------------");
               XLOGD_INFO("CODE_VOICE_KEY button RELEASED event for device: %s (ignored for PAR session)", controller->ieee_address_get().to_string().c_str());
               XLOGD_INFO("------------------------------------------------------------------------");
            } else {
               rdkx_timestamp_t keyUpTime, voiceStartTimeLocal, firstAudioDataTime;
               keyUpTime.tv_sec  = dqm->event.time.tv_sec;
               keyUpTime.tv_nsec = dqm->event.time.tv_usec * 1000;
               
               signed long long audioDurationKeys = ctrlm_timestamp_subtract_ms(controller->getVoiceStartTimeKey(), keyUpTime);
               
               bool hasStartLagTime = false;
               
               if(ble_rcu_interface_) {
                  if (!ble_rcu_interface_->getFirstAudioDataTime(dqm->ieee_address, firstAudioDataTime)) {
                     XLOGD_ERROR("failed to get first audio data time");
                  } else {
                     voiceStartTimeLocal = controller->getVoiceStartTimeLocal();
                     hasStartLagTime = true;
                  }
               }

               if(hasStartLagTime) { // Lag from voice key down to first audio data is available
                  signed long long startAudioLag = ctrlm_timestamp_subtract_ms(voiceStartTimeLocal, firstAudioDataTime);

                  if(startAudioLag < 0 || startAudioLag > UINT32_MAX || startAudioLag > audioDurationKeys) {
                     XLOGD_ERROR("invalid startAudioLag <%lld> audioDurationKeys <%lld>", startAudioLag, audioDurationKeys);
                     startAudioLag = 0;
                  }

                  // Compensate for the full round trip time to start the audio on the controller
                  audioDurationKeys -= startAudioLag;

                  XLOGD_INFO("------------------------------------------------------------------------");
                  XLOGD_INFO("CODE_VOICE_KEY button RELEASED event for device: %s duration <%lld ms> start lag <%lld ms>", controller->ieee_address_get().to_string().c_str(), audioDurationKeys, startAudioLag);
                  XLOGD_INFO("------------------------------------------------------------------------");
               } else {
                  XLOGD_INFO("------------------------------------------------------------------------");
                  XLOGD_INFO("CODE_VOICE_KEY button RELEASED event for device: %s duration <%lld ms>", controller->ieee_address_get().to_string().c_str(), audioDurationKeys);
                  XLOGD_INFO("------------------------------------------------------------------------");
               }

               uint32_t audioDuration = 0;
               if(audioDurationKeys >= 0 && audioDurationKeys <= UINT32_MAX) {
                  audioDuration = audioDurationKeys;
               }

               end_voice_session_for_controller(dqm->ieee_address, CTRLM_VOICE_SESSION_END_REASON_DONE, audioDuration);
            }
         }
      }

      controller->process_event_key(key_status, dqm->event.code, mask_key_codes_get());
   } else {
      XLOGD_WARN("Controller (%s) doesn't exist in the network, doing nothing...", 
            ctrlm_convert_mac_long_to_string(dqm->ieee_address).c_str());
   }
}
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
// END - Process Indications from HAL to the network in CTRLM Main thread context
// ==================================================================================================================================================================

// This is called from voice obj after a voice session in CTRLM Main thread context
void ctrlm_obj_network_ble_t::process_voice_controller_metrics(void *data, int size) {
   ctrlm_main_queue_msg_controller_voice_metrics_t *dqm = (ctrlm_main_queue_msg_controller_voice_metrics_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_controller_voice_metrics_t));

   THREAD_ID_VALIDATE();
   ctrlm_controller_id_t controller_id = dqm->controller_id;

   if(!controller_exists(controller_id)) {
      XLOGD_ERROR("Controller object doesn't exist for controller id %u!", controller_id);
      return;
   }
                           
   if (ble_rcu_interface_) {
      uint32_t lastError;
      uint32_t expectedPackets;
      uint32_t actualPackets;
      if (!ble_rcu_interface_->getAudioStatus(controllers_[controller_id]->ieee_address_get().get_value(),
                                         lastError, expectedPackets, actualPackets)) 
      {
         XLOGD_ERROR("failed to get audio stats from BLE RCU interface");
      } else {
         dqm->packets_total = expectedPackets;
         dqm->packets_lost = expectedPackets - actualPackets;
         XLOGD_INFO("Audio Stats -> error_status = <%u>, packets_received = <%u>, packets_expected = <%u>",
               lastError, actualPackets, expectedPackets);
      }
   }
   // ctrlm_obj_network_t::process_voice_controller_metrics(dqm, sizeof(*dqm));
   controllers_[controller_id]->update_voice_metrics(dqm->short_utterance ? true : false, dqm->packets_total, dqm->packets_lost);
}


void ctrlm_obj_network_ble_t::ind_process_voice_session_end(void *data, int size) {
   THREAD_ID_VALIDATE();
   ctrlm_main_queue_msg_voice_session_end_t *dqm = (ctrlm_main_queue_msg_voice_session_end_t *)data;

   g_assert(dqm);
   g_assert(size == sizeof(ctrlm_main_queue_msg_voice_session_end_t));
   
   ctrlm_controller_id_t controller_id = dqm->controller_id;
   if(!controller_exists(controller_id)) {
      XLOGD_ERROR("Controller object doesn't exist for controller id %u!", controller_id);
      return;
   }
}

// ==================================================================================================================================================================
// BEGIN - Class Functions
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ctrlm_obj_network_ble_t::getControllerId(unsigned long long ieee_address, ctrlm_controller_id_t *controller_id) {
   bool found = false;
   for(auto it = controllers_.begin(); it != controllers_.end(); it++) {
      if(it->second->ieee_address_get() == ieee_address) { 
         found = true;
         *controller_id = it->first;
      }
   }
   if ( !found ) {
      XLOGD_WARN("Controller matching ieee_address (%s) NOT FOUND.", 
            ctrlm_convert_mac_long_to_string(ieee_address).c_str());
   }
   return found;
}

void ctrlm_obj_network_ble_t::voice_command_status_set(void *data, int size) {
   ctrlm_main_queue_msg_voice_command_status_t *dqm = (ctrlm_main_queue_msg_voice_command_status_t *)data;

   if(size != sizeof(ctrlm_main_queue_msg_voice_command_status_t) || NULL == data) {
      XLOGD_ERROR("Incorrect parameters");
      return;
   } else if(!controller_exists(dqm->controller_id)) {
      XLOGD_WARN("Controller %u NOT present.", dqm->controller_id);
      return;
   }

   switch(dqm->status) {
      case VOICE_COMMAND_STATUS_PENDING: XLOGD_INFO("PENDING"); break;
      case VOICE_COMMAND_STATUS_TIMEOUT: XLOGD_INFO("TIMEOUT"); break;
      case VOICE_COMMAND_STATUS_OFFLINE: XLOGD_INFO("OFFLINE"); break;
      case VOICE_COMMAND_STATUS_SUCCESS: XLOGD_INFO("SUCCESS"); break;
      case VOICE_COMMAND_STATUS_FAILURE: XLOGD_INFO("FAILURE"); break;
      case VOICE_COMMAND_STATUS_NO_CMDS: XLOGD_INFO("NO CMDS"); break;
      default:                           XLOGD_WARN("UNKNOWN"); break;
   }
}

bool ctrlm_obj_network_ble_t::controller_is_bound(ctrlm_controller_id_t controller_id) const {
   return (CTRLM_BLE_RESULT_VALIDATION_SUCCESS == controllers_.at(controller_id)->validation_result_get());
}

bool ctrlm_obj_network_ble_t::controller_exists(ctrlm_controller_id_t controller_id) {
   return (controllers_.count(controller_id) > 0);
}

void ctrlm_obj_network_ble_t::controller_remove(ctrlm_controller_id_t controller_id) {
   if(!controller_exists(controller_id)) {
      XLOGD_WARN("Controller id %u doesn't exist", controller_id);
      return;
   }
   controllers_[controller_id]->db_destroy();
   delete controllers_[controller_id];
   XLOGD_INFO("Removed controller %u", controller_id);
}

ctrlm_controller_id_t ctrlm_obj_network_ble_t::controller_id_assign() {
   // Get the next available controller id
   for(ctrlm_controller_id_t index = 1; index < CTRLM_MAIN_CONTROLLER_ID_ALL; index++) {
      if(!controller_exists(index)) {
         XLOGD_INFO("controller id %u", index);
         return(index);
      }
   }
   XLOGD_ERROR("Unable to assign a controller id!");
   return(0);
}

void ctrlm_obj_network_ble_t::controllers_load() {
   std::vector<ctrlm_controller_id_t> controller_ids;
   ctrlm_network_id_t network_id = network_id_get();

   ctrlm_db_ble_controllers_list(network_id, &controller_ids);

   for (auto &id : controller_ids) {
      ctrlm_obj_controller_ble_t *add_controller = new ctrlm_obj_controller_ble_t(id, *this, 0, CTRLM_BLE_RESULT_VALIDATION_SUCCESS);
      add_controller->db_load();
      if (add_controller->get_name().find("IR Device") != std::string::npos) {
         XLOGD_WARN("deleting legacy IR controller object");
         add_controller->db_destroy();
         delete add_controller;
      } else {
         XLOGD_INFO("adding BLE controller with ID = 0x%X", id);
         controllers_[id] = add_controller;
      }
   }
}

void ctrlm_obj_network_ble_t::controller_list_get(std::vector<ctrlm_controller_id_t>& list) const {
   THREAD_ID_VALIDATE();
   if(!list.empty()) {
      XLOGD_WARN("Invalid list.");
      return;
   }
   std::vector<ctrlm_controller_id_t>::iterator it_vector = list.begin();

   std::map<ctrlm_controller_id_t, ctrlm_obj_controller_ble_t *>::const_iterator it_map;
   for(it_map = controllers_.begin(); it_map != controllers_.end(); it_map++) {
      if(controller_is_bound(it_map->first)) {
         it_vector = list.insert(it_vector, it_map->first);
      }
      else {
         XLOGD_WARN("Controller %u NOT bound.", it_map->first);
      }
   }
}

void ctrlm_obj_network_ble_t::printStatus() {
   XLOGD_WARN(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
   XLOGD_WARN("                                       NUMBER OF REMOTES = %d", controllers_.size());

   for(auto it = controllers_.begin(); it != controllers_.end(); it++) {
      it->second->print_status();
   }
   XLOGD_INFO("BLE Network Status:    <%s>", ctrlm_rf_pair_state_str(state_));
   XLOGD_TELEMETRY("IR Programming Status: <%s>", ctrlm_ir_state_str(ir_state_));
   XLOGD_WARN("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

json_t *ctrlm_obj_network_ble_t::xconf_export_controllers() {
   THREAD_ID_VALIDATE();
   XLOGD_DEBUG("Enter...");

   // map to store unique types and min versions.
   unordered_map<std::string, tuple <ctrlm_sw_version_t, ctrlm_hw_version_t> > minVersions;

   for(auto const &ctr_it : controllers_) {
      ctrlm_obj_controller_ble_t *controller = ctr_it.second;
      if (controller->is_stale(this->stale_remote_time_threshold_get())) {
         XLOGD_WARN("Controller <%s> is suspected to be stale, omitting from xconf...", controller->ieee_address_get().to_string().c_str());
         continue;
      }

      string xconf_name;
      ctrlm_sw_version_t revController = controller->get_sw_revision();
      if (controller->is_controller_type_z()) {
         revController.from_string(string("0.0.0"));
      }
      // we only want to send one line for each type with min version
      if (true == controller->getOTAProductName(xconf_name)) {
         auto type_it = minVersions.find(xconf_name);
         if (type_it != minVersions.end()) {

            ctrlm_sw_version_t revSw = get<0>(type_it->second);
            ctrlm_hw_version_t revHw = get<1>(type_it->second);

            // we already have a product of this type so check for min version
            if (revSw > revController) {
               // revController is less than what we have in minVersions, so update value
               revSw = revController;
            }
            //Currently we ignore hw version so dont check it

            minVersions[type_it->first] = make_tuple(revSw, revHw);
         }
         else {
            // we dont have type in map so add it;
            minVersions[xconf_name] = make_tuple(revController, controller->get_hw_revision());
         }
      } else {
         XLOGD_WARN("controller of type %s ignored", controller->controller_type_str_get().c_str());
      }
   }
   json_t *ret = json_array();

   for (auto const &ctrlType : minVersions) {
      ctrlm_sw_version_t revSw = get<0>(ctrlType.second);
      ctrlm_hw_version_t revHw = get<1>(ctrlType.second);

      json_t *temp = json_object();
      XLOGD_INFO("adding to json - Product = <%s>, FwVer = <%s>, HwVer = <%s>", ctrlType.first.c_str(), revSw.to_string().c_str(), revHw.to_string().c_str());
      json_object_set(temp, "Product", json_string(ctrlType.first.c_str()));
      json_object_set(temp, "FwVer", json_string(revSw.to_string().c_str()));
      json_object_set(temp, "HwVer", json_string(revHw.to_string().c_str()));

      json_array_append(ret, temp);
   }
   return ret;
}

void ctrlm_obj_network_ble_t::power_state_change(gboolean waking_up) {

   if (!waking_up) {
      // When going into deep sleep, need to reset wakeup key to invalid
      for(auto &controller : controllers_) {
         controller.second->setLastWakeupKey(CTRLM_KEY_CODE_INVALID);
      }
   }

   if (ble_rcu_interface_) {
      ble_rcu_interface_->handleDeepsleep(waking_up);
   }
}

void ctrlm_obj_network_ble_t::rfc_retrieved_handler(const ctrlm_rfc_attr_t &attr) {
   // TODO - No RFC parameters as of now
}

std::vector<ctrlm_obj_controller_t *> ctrlm_obj_network_ble_t::get_controller_obj_list() const {
    std::vector<ctrlm_obj_controller_t *> controller_obj_list;

    for (auto const &it : controllers_) {
        controller_obj_list.push_back(it.second);
    }
    return controller_obj_list;
}

std::shared_ptr<ConfigSettings> ctrlm_obj_network_ble_t::getConfigSettings() {
   return(m_config);
}
