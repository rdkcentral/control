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
#ifndef __CTRLM_TR181_H__
#define __CTRLM_TR181_H__
#include "ctrlm.h"
#include <limits.h>
#include <float.h>
#include "rfcapi.h"

#define CTRLM_RFC_MAX_PARAM_LEN                         MAX_PARAM_LEN //from rfcapi.h is 2048
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRPollingConfiguration."
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_ENABLED CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "Enable"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_DEFAULT CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "Default"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XR11V2  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XR11v2"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XR15V1  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XR15v1"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XR15V2  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XR15v2"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XR16V1  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XR16v1"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XR19V1  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XR19v1"
#define CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_XRAV1  CTRLM_RF4CE_TR181_POLLING_CONFIGURATION_PREFIX "XRAv1"
#define CTRLM_RF4CE_TR181_PRESS_AND_RELEASE_EOS_TIMEOUT      "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PressAndRelease.EOSTimeout"
#define CTRLM_RF4CE_TR181_PRESS_AND_RELEASE_EOS_METHOD       "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PressAndRelease.EOSMethod"
#define CTRLM_RF4CE_TR181_MAC_POLLING_CONFIGURATION_ENABLE   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRmacPolling.Enable"
#define CTRLM_RF4CE_TR181_MAC_POLLING_CONFIGURATION_INTERVAL "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRmacPolling.macPollingInterval"
#define CTRLM_RF4CE_TR181_ASB_ENABLED                        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRPairing.ASBEnable"
#define CTRLM_RF4CE_TR181_ASB_DERIVATION_METHOD              "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRPairing.ASBDerivationMethod"
#define CTRLM_RF4CE_TR181_ASB_FAIL_THRESHOLD                 "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRPairing.ASBFailThreshold"
#define CTRLM_RF4CE_TR181_THREAD_MONITOR_MINIDUMP_ENABLE     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ThreadMonitorMinidump.Enable"
#define CTRLM_RF4CE_TR181_XR19_DSP_CONFIGURATION             "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.XRDsp.XR19.Configuration"
#define CTRLM_RF4CE_TR181_RF4CE_AUDIO_PROFILE_TARGET         "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.AudioProfileTarget"
#define CTRLM_RF4CE_TR181_RF4CE_OPUS_ENCODER_PARAMS          "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.OpusEncoderParams"
#define CTRLM_RF4CE_TR181_RF4CE_RSP_IDLE_FF                  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.FF.RspIdle"
#define CTRLM_RF4CE_TR181_RF4CE_VOICE_ENCRYPTION             "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.VoiceEncryption.Enable"
#define CTRLM_RF4CE_TR181_RF4CE_HOST_PACKET_DECRYPTION       "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.HostPacketDecryption.Enable"
#define CTRLM_RF4CE_TR181_RSP_TIME_PREFIX                    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RF4CE.RspTime."
#define CTRLM_RF4CE_TR181_RSP_TIME_XRC                       CTRLM_RF4CE_TR181_RSP_TIME_PREFIX "XRC"
#define CTRLM_RF4CE_TR181_RSP_TIME_XVP                       CTRLM_RF4CE_TR181_RSP_TIME_PREFIX "XVP"
#define CTRLM_RF4CE_TR181_RSP_TIME_XDIU                      CTRLM_RF4CE_TR181_RSP_TIME_PREFIX "XDIU"
#define CTRLM_TR181_VOICE_PARAMS_AUDIO_MODE                  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.AudioMode"
#define CTRLM_TR181_VOICE_PARAMS_AUDIO_TIMING                "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.AudioTiming"
#define CTRLM_TR181_VOICE_PARAMS_AUDIO_CONFIDENCE_THRESHOLD  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.AudioConfidenceThreshold"
#define CTRLM_TR181_VOICE_PARAMS_AUDIO_DUCKING_TYPE          "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.AudioDuckingType"
#define CTRLM_TR181_VOICE_PARAMS_AUDIO_DUCKING_LEVEL         "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.AudioDuckingLevel"
#define CTRLM_TR181_VOICE_PARAMS_VSDK_CONFIGURATION          "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.VSDKConfiguration"
#define CTRLM_TR181_VOICE_PARAMS_KEYWORD_SENSITIVITY         "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Voice.KeywordSensitivity"
#define CTRLM_TR181_TELEMETRY_REPORT_GLOBAL                  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.global"
#define CTRLM_TR181_TELEMETRY_REPORT_RF4CE                   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.rf4ce"
#define CTRLM_TR181_TELEMETRY_REPORT_BLE                     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.ble"
#define CTRLM_TR181_TELEMETRY_REPORT_IP                      "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.ip"
#define CTRLM_TR181_TELEMETRY_REPORT_VOICE                   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.voice"
#define CTRLM_RT181_POWER_RFC_PWRMGR2                        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Power.PwrMgr2.Enable"


typedef enum {
   CTRLM_TR181_RESULT_FAILURE = 0,
   CTRLM_TR181_RESULT_SUCCESS = 1
} ctrlm_tr181_result_t;

ctrlm_tr181_result_t ctrlm_tr181_string_get(const char *parameter, char *s, size_t len);
ctrlm_tr181_result_t ctrlm_tr181_bool_get(const char *parameter, bool *b);
ctrlm_tr181_result_t ctrlm_tr181_int_get(const char *parameter, int *i, int min = INT_MIN, int max = INT_MAX);
ctrlm_tr181_result_t ctrlm_tr181_real_get(const char *parameter, double *d, double min = DBL_MIN, double max = DBL_MAX);
template <typename T>
ctrlm_tr181_result_t ctrlm_tr181_int_get(const char *parameter, T *i, int min = INT_MIN, int max = INT_MAX) {
   ctrlm_tr181_result_t ret = CTRLM_TR181_RESULT_FAILURE;
   int temp = 0;
   if(i) {
      ret = ctrlm_tr181_int_get(parameter, &temp, min, max);
      if(ret == CTRLM_TR181_RESULT_SUCCESS) {
         *i = (T)temp;
      }
   }
   return(ret);
}
ctrlm_tr181_result_t ctrlm_tr181_string_set(const char *parameter, char *s, size_t len);
#endif
