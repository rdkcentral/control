/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
/*
 * CI patch layer for gaps in the entservices-testframework mock headers.
 *
 * ctrlm needs a few IARM declarations that are missing from the testframework
 * headers used by this reduced build. This file adds only those missing pieces
 * and can be removed once they exist upstream.
 */
#ifndef CTRLM_CI_TESTFRAMEWORK_OVERRIDES_H_
#define CTRLM_CI_TESTFRAMEWORK_OVERRIDES_H_

/* ctrlm-main calls IARM_Bus_RegisterEvent as a plain function (defined in stubs_iarm.cpp) */
#ifdef __cplusplus
extern "C" {
#endif
extern IARM_Result_t IARM_Bus_RegisterEvent(IARM_EventId_t maxEventId);
#ifdef __cplusplus
}
#endif

/* Pinned testframework Iarm.h stops at UNKNOWN; ctrlm also references MAX. */
#ifndef DEEPSLEEP_WAKEUPREASON_MAX
#define DEEPSLEEP_WAKEUPREASON_MAX (DEEPSLEEP_WAKEUPREASON_UNKNOWN + 1)
#endif

/* IARM common API string not present in the pinned testframework mock */
#ifndef IARM_BUS_COMMON_API_PowerPreChange
#define IARM_BUS_COMMON_API_PowerPreChange "PowerPreChange"
#endif

/* Struct not present in the pinned testframework mock */
#ifndef IARM_BUS_COMMON_API_PowerPreChange_Param_t
typedef struct {
    IARM_Bus_PWRMgr_PowerState_t newState;
    IARM_Bus_PWRMgr_PowerState_t curState;
} IARM_Bus_CommonAPI_PowerPreChange_Param_t;
#define IARM_BUS_COMMON_API_PowerPreChange_Param_t IARM_Bus_CommonAPI_PowerPreChange_Param_t
#endif

#endif /* CTRLM_CI_TESTFRAMEWORK_OVERRIDES_H_ */
