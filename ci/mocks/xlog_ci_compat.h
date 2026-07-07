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
 * CI compatibility shim.
 *
 * The production rdkx_logger.h is included transitively everywhere via
 * ctrlm_log.h.  In the Yocto build the SDK headers pull several std:: names
 * into the global namespace.  A handful of ctrlm TUs (e.g. ctrlm_utils.cpp)
 * rely on those names being globally visible.  This tiny header replicates
 * that behaviour for the CI native build.
 */
#ifndef CTRLM_CI_XLOG_COMPAT_H_
#define CTRLM_CI_XLOG_COMPAT_H_

#ifdef __cplusplus
#include <string>
#include <map>
#include <tuple>
using std::get;
using std::map;
using std::string;
using std::tuple;
#endif

#endif /* CTRLM_CI_XLOG_COMPAT_H_ */
