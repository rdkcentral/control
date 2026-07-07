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
 * Compatibility header: maps ctrlm's local include name to the real libsafec
 * package headers (libsafec-dev / safec-common-wrapper in production).
 *
 * In CI this file is copied from ci/mocks/ into ci/headers/ by
 * build_dependencies.sh so it is resolved on the generated-headers include
 * path used by cov_build.sh.
 *
 * This exists because ctrlm includes safec_lib.h directly, while the native CI
 * environment provides the underlying libsafec package headers instead of that
 * project-local wrapper.
 */

#ifndef CTRLM_CI_SAFEC_LIB_H_
#define CTRLM_CI_SAFEC_LIB_H_

#include <safeclib/safe_lib.h>
#include <safeclib/safe_str_lib.h>
#include <safeclib/safe_mem_lib.h>

#ifndef ERR_CHK
#define ERR_CHK(rc) do { (void)(rc); } while(0)
#endif

#endif /* CTRLM_CI_SAFEC_LIB_H_ */
