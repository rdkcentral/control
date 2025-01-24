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
#include <string>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_rcu.h"
#include "ctrlm_validation.h"
#include "ctrlm_rf4ce_network.h"

typedef enum {
   CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS               = 0x00, // Taken from GP RF4CE definitions
   CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER     = 0xE8,
   CTRLM_RF4CE_RIB_RSP_STATUS_UNSUPPORTED_ATTRIBUTE = 0xF4,
   CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX         = 0xF9
} ctrlm_rf4ce_rib_rsp_status_t;

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_get_target(ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guchar length, guchar *data_len, guchar *data) {
   ctrlm_timestamp_t timestamp;
   errno_t safec_rc = memset_s(&timestamp, sizeof(timestamp), 0, sizeof(timestamp));
   ERR_CHK(safec_rc);
   rf4ce_rib_get(true, timestamp, identifier, index, length, data_len, data);
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_get_controller(ctrlm_timestamp_t timestamp, ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guchar length) {
   rf4ce_rib_get(false, timestamp, identifier, index, length, NULL, NULL);
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_get(gboolean target, ctrlm_timestamp_t timestamp, ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guchar length, guchar *data_len, guchar *data) {
   ctrlm_rf4ce_rib_rsp_status_t status = CTRLM_RF4CE_RIB_RSP_STATUS_UNSUPPORTED_ATTRIBUTE;
   size_t value_length = length;
   guchar response[5 + CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE];
   guchar *data_buf;
   ctrlm_rf4ce_rib_t::status rib_status;

   if(target) {
      data_buf = data;
   } else {
      data_buf = &response[5];
   }

   rib_status = this->rib_.read_attribute(target ? ctrlm_rf4ce_rib_attr_t::access::TARGET : ctrlm_rf4ce_rib_attr_t::access::CONTROLLER, identifier, index, (char *)data_buf, &value_length);
   if(rib_status != ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
      XLOGD_INFO("(%u, %u) RIB read <%02x, %02x, %s>", network_id_get(), controller_id_get(), identifier, index, ctrlm_rf4ce_rib_t::status_str(rib_status).c_str());
      if(rib_status != ctrlm_rf4ce_rib_t::status::SUCCESS) {
         value_length = 0;
      }
   } else {
      ctrlm_rf4ce_rib_t *network_rib = this->obj_network_rf4ce_->get_rib();
      if(network_rib) {
         rib_status = network_rib->read_attribute(target ? ctrlm_rf4ce_rib_attr_t::access::TARGET : ctrlm_rf4ce_rib_attr_t::access::CONTROLLER, identifier, index, (char *)data_buf, &value_length);
         if(rib_status != ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
            XLOGD_INFO("(%u, %u) NTWK RIB read <%02x, %02x, %s>", network_id_get(), controller_id_get(), identifier, index, ctrlm_rf4ce_rib_t::status_str(rib_status).c_str());
            if(rib_status != ctrlm_rf4ce_rib_t::status::SUCCESS) {
               value_length = 0;
            }
         }
      }
      if(rib_status == ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
         XLOGD_DEBUG("falling back to legacy RIB implementation");
         value_length = 0;
         switch(identifier) {
            case CTRLM_RF4CE_RIB_ATTR_ID_PERIPHERAL_ID: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID) {
                  XLOGD_ERROR("PERIPHERAL ID - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("PERIPHERAL ID - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  XLOGD_INFO("PERIPHERAL ID");
                  // add the payload to the response
                  value_length = property_read_peripheral_id(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_RF_STATISTICS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS) {
                  XLOGD_ERROR("RF STATISTICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("RF STATISTICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  XLOGD_INFO("RF STATISTICS");
                  // add the payload to the response
                  value_length = property_read_rf_statistics(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_SHORT_RF_RETRY_PERIOD: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD) {
                  XLOGD_ERROR("SHORT RF RETRY PERIOD - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("SHORT RF RETRY PERIOD - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_short_rf_retry_period(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD);

                  // This rib entry is the last entry read by the remote after binding is completed
                  if((controller_type_ == RF4CE_CONTROLLER_TYPE_XR2 || controller_type_ == RF4CE_CONTROLLER_TYPE_XR5) &&
                     validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS && configuration_result_ == CTRLM_RCU_CONFIGURATION_RESULT_PENDING) {
                     XLOGD_INFO("(%u, %u) Configuration Complete", network_id_get(), controller_id_get());
                     configuration_result_ = CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS;
                     // Inform control manager that the configuration has completed
                     ctrlm_inform_configuration_complete(network_id_get(), controller_id_get(), CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS);
                  }
               }
               
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MAXIMUM_UTTERANCE_LENGTH: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH) {
                  XLOGD_ERROR("MAXIMUM UTTERANCE LENGTH - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAXIMUM UTTERANCE LENGTH - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_maximum_utterance_length(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_COMMAND_ENCRYPTION: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION) {
                  XLOGD_ERROR("VOICE COMMAND ENCRYPTION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE COMMAND ENCRYPTION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_voice_command_encryption(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MAX_VOICE_DATA_RETRY: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY) {
                  XLOGD_ERROR("MAX VOICE DATA RETRY - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAX VOICE DATA RETRY - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_max_voice_data_retry(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MAX_VOICE_CSMA_BACKOFF: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF) {
                  XLOGD_ERROR("MAX VOICE CSMA BACKOFF - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAX VOICE CSMA BACKOFF - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_max_voice_csma_backoff(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MIN_VOICE_DATA_BACKOFF: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF) {
                  XLOGD_ERROR("MIN VOICE DATA BACKOFF - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MIN VOICE DATA BACKOFF - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_min_voice_data_backoff(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_TARG_AUDIO_PROFILES: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_TARG_AUDIO_PROFILES) {
                  XLOGD_ERROR("VOICE TARG AUDIO PROFILES - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE TARG AUDIO PROFILES - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_voice_targ_audio_profiles(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_TARG_AUDIO_PROFILES);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_RIB_UPDATE_CHECK_INTERVAL: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL) {
                  XLOGD_ERROR("RIB UPDATE CHECK INTERVAL - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("RIB UPDATE CHECK INTERVAL - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_rib_update_check_interval(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_OPUS_ENCODING_PARAMS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS) {
                  XLOGD_ERROR("OPUS ENCODING PARAMS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("OPUS ENCODING PARAMS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_opus_encoding_params(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_SESSION_QOS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS) {
                  XLOGD_ERROR("VOICE SESSION QOS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE SESSION QOS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_voice_session_qos(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DOWNLOAD_RATE: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE) {
                  XLOGD_ERROR("DOWNLOAD RATE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DOWNLOAD RATE - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_download_rate(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_UPDATE_POLLING_PERIOD: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD) {
                  XLOGD_ERROR("UPDATE POLLING PERIOD - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("UPDATE POLLING PERIOD - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_update_polling_period(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DATA_REQUEST_WAIT_TIME: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME) {
                  XLOGD_ERROR("DATA REQUEST WAIT TIME - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DATA REQUEST WAIT TIME - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_data_request_wait_time(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME);
                  if(!target) {
                     // This rib entry is the last entry read by the remote after binding is completed
                     if((controller_type_ == RF4CE_CONTROLLER_TYPE_XR11 || controller_type_ == RF4CE_CONTROLLER_TYPE_XR15 || controller_type_ == RF4CE_CONTROLLER_TYPE_XR15V2 ||
                        controller_type_ == RF4CE_CONTROLLER_TYPE_XR16 || controller_type_ == RF4CE_CONTROLLER_TYPE_XR18 || controller_type_ == RF4CE_CONTROLLER_TYPE_XRA) &&
                        validation_result_ == CTRLM_RF4CE_RESULT_VALIDATION_SUCCESS && configuration_result_ == CTRLM_RCU_CONFIGURATION_RESULT_PENDING) {
                        XLOGD_INFO("(%u, %u) Configuration Complete", network_id_get(), controller_id_get());
                        configuration_result_ = CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS;
                        // Inform control manager that the configuration has completed
                        ctrlm_inform_configuration_complete(network_id_get(), controller_id_get(), CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS);
                        obj_network_rf4ce_->set_rf_pair_state(CTRLM_RF_PAIR_STATE_COMPLETE);
                        obj_network_rf4ce_->iarm_event_rcu_status();
                     }
                  }
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_IR_RF_DATABASE: {
               if(target && length != CTRLM_RF4CE_RIB_ATTR_LEN_IR_RF_DATABASE) {
                  XLOGD_ERROR("IR RF DATABASE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else {
                  // add the payload to the response
                  value_length = property_read_ir_rf_database(index, data_buf, CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE);

                  if(value_length == 0) {
                     XLOGD_ERROR("IR RF DATABASE - Invalid Index (%u)", index);
                     status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
                  }
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VALIDATION_CONFIGURATION: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION) {
                  XLOGD_ERROR("VALIDATION CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VALIDATION CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_validation_configuration(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_TARGET_IRDB_STATUS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_IRDB_STATUS) {
                  XLOGD_ERROR("TARGET IRDB STATUS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("TARGET IRDB STATUS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // add the payload to the response
                  value_length = property_read_target_irdb_status(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_IRDB_STATUS);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_TARGET_ID_DATA: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
                  XLOGD_ERROR("TARGET ID DATA - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_DEVICE_ID) {
                  XLOGD_ERROR("TARGET ID DATA - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_DEVICE_ID) {
                  value_length = property_read_device_id(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA);
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_RECEIVER_ID) {
                  value_length = property_read_receiver_id(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA);
               } else {
                  XLOGD_WARN("Account ID not implemented yet");
                  value_length = 0;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_GENERAL_PURPOSE: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_GENERAL_PURPOSE) {
                  XLOGD_ERROR("GENERAL PURPOSE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index == 0x00) { // Reset type
                  value_length = property_read_reboot_diagnostics(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_DIAGNOSTICS);
               } else if(index == 0x01) { // Memory Stats
                  value_length = property_read_memory_statistics(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATISTICS);
               } else {
                  XLOGD_ERROR("GENERAL PURPOSE - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MFG_TEST: {
               if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_MFG_TEST) { // Mfg Test timing data
                  if((length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST) || (length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_HAPTICS)) {
                     value_length = property_read_mfg_test(data_buf, length);
                  } else {
                     XLOGD_ERROR("MFG Test - Invalid Length (%u)", length);
                     status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
                  }
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_MFG_TEST_RESULT) { // Mfg Security Key Test Rib Result
                  if(length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT) {
                     value_length = property_read_mfg_test_result(data_buf, length);
                  } else {
                     XLOGD_ERROR("MFG Security Key Test Result  - Invalid Length (%u)", length);
                     status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
                  }
               } else {
                  XLOGD_ERROR("MFG Test - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_POLLING_METHODS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS) {
                  XLOGD_ERROR("POLLING METHODS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               }  else if(index > 0x00) {
                  XLOGD_ERROR("POLLING METHODS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  value_length = property_read_polling_methods(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS);
               }
               break;
            }  
            case CTRLM_RF4CE_RIB_ATTR_ID_POLLING_CONFIGURATION: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
                  XLOGD_ERROR("POLLING CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > CTRLM_RF4CE_RIB_ATTR_INDEX_POLLING_CONFIGURATION_MAC) {
                  XLOGD_ERROR("POLLING CONFIGURATION - Invalid Index (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_POLLING_CONFIGURATION_MAC) {
                  value_length = property_read_polling_configuration_mac(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
               } else {
                  value_length = property_read_polling_configuration_heartbeat(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_FAR_FIELD_CONFIGURATION: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_CONFIGURATION) {
                  XLOGD_ERROR("FAR FIELD CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("FAR FIELD CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  value_length = property_read_far_field_configuration(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_CONFIGURATION);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_FAR_FIELD_METRICS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS) {
                  XLOGD_ERROR("FAR FIELD METRICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("FAR FIELD METRICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  value_length = property_read_far_field_metrics(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DSP_CONFIGURATION: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_CONFIGURATION) {
                  XLOGD_ERROR("DSP CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DSP CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  value_length = property_read_dsp_configuration(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_CONFIGURATION);
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DSP_METRICS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS) {
                  XLOGD_ERROR("DSP METRICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DSP METRICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  value_length = property_read_dsp_metrics(data_buf, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS);
               }
               break;
            }
            default: {
               XLOGD_INFO("invalid identifier (0x%02X)", identifier);
               break;
            }
         }
      }
   }

   if(target) {
      *data_len = value_length;
   } else {
      if(value_length > 0) {
         status   = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
      }
      // Determine when to send the response (50 ms after receipt)
      ctrlm_timestamp_add_ms(&timestamp, CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME);
      unsigned long delay = ctrlm_timestamp_until_us(timestamp);

      if(delay == 0) {
         ctrlm_timestamp_t now;
         ctrlm_timestamp_get(&now);

         long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
         if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
            XLOGD_WARN("LATE response packet - diff %ld ms", diff);
         }
      }

      response[0] = RF4CE_FRAME_CONTROL_GET_ATTRIBUTE_RESPONSE;
      response[1] = identifier;
      response[2] = index;
      response[3] = (guchar) status;
      response[4] = value_length;

      // Send the response back to the controller
      req_data(CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU, timestamp, 5 + value_length, response, NULL, NULL);
      if(identifier != CTRLM_RF4CE_RIB_ATTR_ID_MEMORY_DUMP) { // Send an IARM event for controller RIB read access
         ctrlm_rcu_iarm_event_rib_access_controller(network_id_get(), controller_id_get(), (ctrlm_rcu_rib_attr_id_t)identifier, index, CTRLM_ACCESS_TYPE_READ);
      }
   }
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_set_target(ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guint8 length, guchar *data) {
   ctrlm_timestamp_t timestamp;
   errno_t safec_rc = memset_s(&timestamp, sizeof(timestamp), 0, sizeof(timestamp));
   ERR_CHK(safec_rc);
   rf4ce_rib_set(true, timestamp, identifier, index, length, data);
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_set_controller(ctrlm_timestamp_t timestamp, ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guint8 length, guchar *data) {
   rf4ce_rib_set(false, timestamp, identifier, index, length, data);
}

void ctrlm_obj_controller_rf4ce_t::rf4ce_rib_set(gboolean target, ctrlm_timestamp_t timestamp, ctrlm_rf4ce_rib_attr_id_t identifier, guchar index, guint8 length, guchar *data) {
   ctrlm_rf4ce_rib_rsp_status_t status = CTRLM_RF4CE_RIB_RSP_STATUS_UNSUPPORTED_ATTRIBUTE;
   gboolean dont_send_iarm_message = false;
   ctrlm_rf4ce_rib_t::status rib_status;
   ctrlm_rf4ce_rib_attr_t::access accessor = target ? ctrlm_rf4ce_rib_attr_t::access::TARGET : ctrlm_rf4ce_rib_attr_t::access::CONTROLLER;
   bool importing = obj_network_rf4ce_->is_importing_controller();

   // if we are importing, technically it's the controller writing
   if(accessor == ctrlm_rf4ce_rib_attr_t::access::TARGET && importing) {
      accessor = ctrlm_rf4ce_rib_attr_t::access::CONTROLLER;
   }

   rf4ce_rib_export_api_t export_api(std::bind(&ctrlm_obj_network_rf4ce_t::req_process_rib_export, this->obj_network_rf4ce_, this->controller_id_get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_4, std::placeholders::_3));
   rib_status = this->rib_.write_attribute(accessor, identifier, index, (char *)data, (size_t)length, &export_api, importing);
   if(rib_status != ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
      XLOGD_INFO("(%u, %u) RIB write <%02x, %02x, %s>", network_id_get(), controller_id_get(), identifier, index, ctrlm_rf4ce_rib_t::status_str(rib_status).c_str());
      if(rib_status == ctrlm_rf4ce_rib_t::status::SUCCESS) {
         status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
      }
   } else {
      ctrlm_rf4ce_rib_t *network_rib = this->obj_network_rf4ce_->get_rib();
      if(network_rib) {
         rib_status = network_rib->write_attribute(accessor, identifier, index, (char *)data, (size_t)length, &export_api, importing);
         if(rib_status != ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
            XLOGD_INFO("(%u, %u) NTWK RIB write <%02x, %02x, %s>", network_id_get(), controller_id_get(), identifier, index, ctrlm_rf4ce_rib_t::status_str(rib_status).c_str());
            if(rib_status == ctrlm_rf4ce_rib_t::status::SUCCESS) {
               status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
            }
         }
      }

      if(rib_status == ctrlm_rf4ce_rib_t::status::DOES_NOT_EXIST) {
         XLOGD_DEBUG("falling back to legacy RIB implementation");

         switch(identifier) {
            case CTRLM_RF4CE_RIB_ATTR_ID_MAXIMUM_UTTERANCE_LENGTH: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier MAXIMUM UTTERANCE LENGTH");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH) {
                  XLOGD_ERROR("MAXIMUM UTTERANCE LENGTH - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAXIMUM UTTERANCE LENGTH - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_maximum_utterance_length(data, CTRLM_RF4CE_RIB_ATTR_LEN_MAXIMUM_UTTERANCE_LENGTH);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_COMMAND_ENCRYPTION: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier VOICE COMMAND ENCRYPTION");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION) {
                  XLOGD_ERROR("VOICE COMMAND ENCRYPTION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE COMMAND ENCRYPTION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_voice_command_encryption(data, CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_COMMAND_ENCRYPTION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MAX_VOICE_DATA_RETRY: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier MAX VOICE DATA RETRY");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY) {
                  XLOGD_ERROR("MAX VOICE DATA RETRY - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAX VOICE DATA RETRY - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_max_voice_data_retry(data, CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_DATA_RETRY);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MAX_VOICE_CSMA_BACKOFF: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier MAX VOICE CSMA BACKOFF");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF) {
                  XLOGD_ERROR("MAX VOICE CSMA BACKOFF - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MAX VOICE CSMA BACKOFF - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_max_voice_csma_backoff(data, CTRLM_RF4CE_RIB_ATTR_LEN_MAX_VOICE_CSMA_BACKOFF);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MIN_VOICE_DATA_BACKOFF: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier MIN VOICE DATA BACKOFF");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF) {
                  XLOGD_ERROR("MIN VOICE DATA BACKOFF - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("MIN VOICE DATA BACKOFF - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_min_voice_data_backoff(data, CTRLM_RF4CE_RIB_ATTR_LEN_MIN_VOICE_DATA_BACKOFF);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_TARG_AUDIO_PROFILES: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier VOICE TARG AUDIO PROFILES");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_TARG_AUDIO_PROFILES) {
                  XLOGD_ERROR("VOICE TARG AUDIO PROFILES - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE TARG AUDIO PROFILES - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  XLOGD_ERROR("VOICE TARG AUDIO PROFILES - NOT SUPPORTED");
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_RIB_UPDATE_CHECK_INTERVAL: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier RIB UPDATE CHECK INTERVAL");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL) {
                  XLOGD_ERROR("RIB UPDATE CHECK INTERVAL - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("RIB UPDATE CHECK INTERVAL - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_rib_update_check_interval(data, CTRLM_RF4CE_RIB_ATTR_LEN_RIB_UPDATE_CHECK_INTERVAL);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DOWNLOAD_RATE: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier DOWNLOAD RATE");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE) {
                  XLOGD_ERROR("DOWNLOAD RATE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DOWNLOAD RATE - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_download_rate(data, CTRLM_RF4CE_RIB_ATTR_LEN_DOWNLOAD_RATE);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_UPDATE_POLLING_PERIOD: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier UPDATE POLLING PERIOD");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD) {
                  XLOGD_ERROR("UPDATE POLLING PERIOD - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("UPDATE POLLING PERIOD - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_update_polling_period(data, CTRLM_RF4CE_RIB_ATTR_LEN_UPDATE_POLLING_PERIOD);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DATA_REQUEST_WAIT_TIME: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier DATA REQUEST WAIT TIME");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME) {
                  XLOGD_ERROR("DATA REQUEST WAIT TIME - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DATA REQUEST WAIT TIME - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_data_request_wait_time(data, CTRLM_RF4CE_RIB_ATTR_LEN_DATA_REQUEST_WAIT_TIME);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_IR_RF_DATABASE: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier IR RF DATABASE");
               } else if(length > CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE) {
                  XLOGD_ERROR("IR RF DATABASE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else {
                  // Store this data in the object
                  property_write_ir_rf_database(index, data, length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_SHORT_RF_RETRY_PERIOD: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier SHORT RF RETRY PERIOD");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD) {
                  XLOGD_ERROR("SHORT RF RETRY PERIOD - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("SHORT RF RETRY PERIOD - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_short_rf_retry_period(data, CTRLM_RF4CE_RIB_ATTR_LEN_SHORT_RF_RETRY_PERIOD);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VALIDATION_CONFIGURATION: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier VALIDATION CONFIGURATION");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION) {
                  XLOGD_ERROR("VALIDATION CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VALIDATION CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_validation_configuration(data, CTRLM_RF4CE_RIB_ATTR_LEN_VALIDATION_CONFIGURATION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_TARGET_IRDB_STATUS: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier TARGET IRDB STATUS");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_IRDB_STATUS) {
                  XLOGD_ERROR("TARGET IRDB STATUS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("TARGET IRDB STATUS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_target_irdb_status(data, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_IRDB_STATUS);

                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_PERIPHERAL_ID: {
               if(target && !ctrlm_is_production_build()) {
                  XLOGD_ERROR("target failed to write to controller attribute identifier PERIPHERAL ID");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID) {
                  XLOGD_ERROR("PERIPHERAL ID - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("PERIPHERAL ID - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_peripheral_id(data, CTRLM_RF4CE_RIB_ATTR_LEN_PERIPHERAL_ID);

                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_RF_STATISTICS: {
               if(target && !ctrlm_is_production_build()) {
                  XLOGD_ERROR("target failed to write to controller attribute identifier RF STATISTICS");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS) {
                  XLOGD_ERROR("RF STATISTICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("RF STATISTICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_rf_statistics(data, CTRLM_RF4CE_RIB_ATTR_LEN_RF_STATISTICS);

                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_OPUS_ENCODING_PARAMS: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier OPUS ENCODING PARAMS");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_OPUS_ENCODING_PARAMS) {
                  XLOGD_ERROR("OPUS ENCODING PARAMS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("OPUS ENCODING PARAMS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_opus_encoding_params(data, length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_VOICE_SESSION_QOS: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier VOICE SESSION QOS");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_VOICE_SESSION_QOS) {
                  XLOGD_ERROR("VOICE SESSION QOS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("VOICE SESSION QOS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  // Store this data in the object
                  property_write_voice_session_qos(data, length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_TARGET_ID_DATA: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier TARGET ID DATA");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA) {
                  XLOGD_ERROR("TARGET ID DATA - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_DEVICE_ID) {
                  XLOGD_ERROR("TARGET ID DATA - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_DEVICE_ID) {
                  property_write_device_id(data, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA);
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_TARGET_ID_DATA_RECEIVER_ID) {
                  property_write_receiver_id(data, CTRLM_RF4CE_RIB_ATTR_LEN_TARGET_ID_DATA);
               } else {
                  XLOGD_WARN("Account ID not implemented yet");
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_UNSUPPORTED_ATTRIBUTE;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_MFG_TEST: {
               if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_MFG_TEST) { // Mfg Test timing data
                  if(target) {
                     if((length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST) || (length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_HAPTICS)) {
                        // Store this data in the object
                        property_write_mfg_test(data, length);
                        status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
                     } else {
                        XLOGD_ERROR("MFG Test - Invalid Length (%u)", length);
                        status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
                     }
                  } else {
                     XLOGD_ERROR("controller write to read only identifier MFG TEST");
                  }
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_MFG_TEST_RESULT) { // Mfg Security Key Test Rib Result
                  if(!target || ctrlm_is_production_build()) {
                     if(length == CTRLM_RF4CE_RIB_ATTR_LEN_MFG_TEST_RESULT) {
                        // Store this data in the object
                        property_write_mfg_test_result(data, length);
                        status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
                     } else {
                        XLOGD_ERROR("MFG Security Key Test Result - Invalid Length (%u)", length);
                        status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
                     }
                  } else {
                     XLOGD_ERROR("target failed to write to controller attribute identifier MFG SECURITY KEY TEST RESULT");
                  }
               } else {
                  XLOGD_ERROR("MFG TEST - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_POLLING_METHODS: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier POLLING METHODS");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS) {
                  XLOGD_ERROR("POLLING METHODS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               }  else if(index > 0x00) {
                  XLOGD_ERROR("POLLING METHODS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_polling_methods(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_METHODS);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_POLLING_CONFIGURATION: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier POLLING CONFIGURATION");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION) {
                  XLOGD_ERROR("POLLING CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > CTRLM_RF4CE_RIB_ATTR_INDEX_POLLING_CONFIGURATION_MAC) {
                  XLOGD_ERROR("POLLING CONFIGURATION - Invalid Index (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else if(index == CTRLM_RF4CE_RIB_ATTR_INDEX_POLLING_CONFIGURATION_MAC) {
                  property_write_polling_configuration_mac(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               } else {
                  property_write_polling_configuration_heartbeat(data, CTRLM_RF4CE_RIB_ATTR_LEN_POLLING_CONFIGURATION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_PRIVACY: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY) {
                  XLOGD_ERROR("PRIVACY - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("PRIVACY - Invalid Index (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_privacy(data, CTRLM_RF4CE_RIB_ATTR_LEN_PRIVACY);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_FAR_FIELD_CONFIGURATION: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier FAR FIELD CONFIGURATION");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_CONFIGURATION) {
                  XLOGD_ERROR("FAR FIELD CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("FAR FIELD CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_far_field_configuration(data, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_CONFIGURATION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_FAR_FIELD_METRICS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS) {
                  XLOGD_ERROR("FAR FIELD METRICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("FAR FIELD METRICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_far_field_metrics(data, CTRLM_RF4CE_RIB_ATTR_LEN_FAR_FIELD_METRICS);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DSP_CONFIGURATION: {
               if(!target) {
                  XLOGD_ERROR("controller write to read only identifier DSP CONFIGURATION");
               } else if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_CONFIGURATION) {
                  XLOGD_ERROR("DSP CONFIGURATION - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DSP CONFIGURATION - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_dsp_configuration(data, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_CONFIGURATION);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_DSP_METRICS: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS) {
                  XLOGD_ERROR("DSP METRICS - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x00) {
                  XLOGD_ERROR("DSP METRICS - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else {
                  property_write_dsp_metrics(data, CTRLM_RF4CE_RIB_ATTR_LEN_DSP_METRICS);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            case CTRLM_RF4CE_RIB_ATTR_ID_GENERAL_PURPOSE: {
               if(length != CTRLM_RF4CE_RIB_ATTR_LEN_GENERAL_PURPOSE) {
                  XLOGD_ERROR("GENERAL PURPOSE - Invalid Length (%u)", length);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_PARAMETER;
               } else if(index > 0x01) {
                  XLOGD_ERROR("GENERAL PURPOSE - Invalid Index (%u)", index);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_INVALID_INDEX;
               } else if(index == 0x01) { // Memory Stats
                  // Store this data in the object
                  property_write_memory_stats(data, CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_STATS);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               } else { // Reboot Stats
                  // Store this data in the object
                  property_write_reboot_stats(data, CTRLM_RF4CE_RIB_ATTR_LEN_REBOOT_STATS);
                  status = CTRLM_RF4CE_RIB_RSP_STATUS_SUCCESS;
               }
               break;
            }
            default: {
               XLOGD_INFO("invalid identifier (0x%02X)", identifier);
               break;
            }
         }
      }
   }

   if(!target) { // Send the response back to the controller
      guchar response[4];

      // Determine when to send the response (50 ms after receipt)
      ctrlm_timestamp_add_ms(&timestamp, CTRLM_RF4CE_CONST_RESPONSE_IDLE_TIME);
      unsigned long delay = ctrlm_timestamp_until_us(timestamp);

      if(delay == 0) {
         ctrlm_timestamp_t now;
         ctrlm_timestamp_get(&now);

         long diff = ctrlm_timestamp_subtract_ms(timestamp, now);
         if(diff >= CTRLM_RF4CE_CONST_RESPONSE_WAIT_TIME) {
            XLOGD_WARN("LATE response packet - diff %ld ms", diff);
         }
      }

      response[0] = RF4CE_FRAME_CONTROL_SET_ATTRIBUTE_RESPONSE;
      response[1] = identifier;
      response[2] = index;
      response[3] = (guchar) status;

      req_data(CTRLM_RF4CE_PROFILE_ID_COMCAST_RCU, timestamp, 4, response, NULL, NULL);
      if(dont_send_iarm_message) {
         XLOGD_INFO("Skipping iarm message.");
         return;
      }
      if(identifier != CTRLM_RF4CE_RIB_ATTR_ID_MEMORY_DUMP) { // Send an IARM event for controller RIB write access
         ctrlm_rcu_iarm_event_rib_access_controller(network_id_get(), controller_id_get(), (ctrlm_rcu_rib_attr_id_t)identifier, index, CTRLM_ACCESS_TYPE_WRITE);
      }
   }
}
