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
 * Supplements the entservices-testframework mocks with declarations ctrlm needs
 * that are not (yet) in the upstream testframework develop branch.  We can remove this file once the necessary declarations are merged upstream.
 */
#ifndef _CONTROL_TESTFRAMEWORK_OVERRIDES_H_
#define _CONTROL_TESTFRAMEWORK_OVERRIDES_H_

/* ctrlm-main calls IARM_Bus_RegisterEvent as a plain function (defined in stubs_iarm.cpp) */
extern IARM_Result_t IARM_Bus_RegisterEvent(IARM_EventId_t maxEventId);

/* IARM common API string not present in the pinned testframework mock */
#ifndef IARM_BUS_COMMON_API_PowerPreChange
#define IARM_BUS_COMMON_API_PowerPreChange "PowerPreChange"
#endif

#endif /* _CONTROL_TESTFRAMEWORK_OVERRIDES_H_ */
