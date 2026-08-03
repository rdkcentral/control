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
#define CTRLM_TR181_TELEMETRY_REPORT_GLOBAL                  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.global"
#define CTRLM_TR181_TELEMETRY_REPORT_RF4CE                   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.rf4ce"
#define CTRLM_TR181_TELEMETRY_REPORT_BLE                     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.ble"
#define CTRLM_TR181_TELEMETRY_REPORT_IP                      "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.ip"
#define CTRLM_TR181_TELEMETRY_REPORT_VOICE                   "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ctrlm.telemetry_report.voice"
#define CTRLM_TR181_POWER_RFC_PWRMGR2                        "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Power.PwrMgr2.Enable"


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
