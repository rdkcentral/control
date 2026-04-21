#!/bin/bash
#
# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2026 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -x
set -e
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la "${GITHUB_WORKSPACE}"
############################

# Build control (ctrlm-main)
echo "building control (ctrlm-main)"

XRSDK_REAL_HEADERS="$GITHUB_WORKSPACE/ci/headers/xr-voice-sdk"
XLOG_COMPAT="$GITHUB_WORKSPACE/ci/mocks/xlog_ci_compat.h"
MOCK_DIR="$GITHUB_WORKSPACE/entservices-testframework/Tests/mocks"
MOCK_OVERRIDES="$GITHUB_WORKSPACE/ci/mocks/testframework_overrides.h"
HEADERS_DIR="$GITHUB_WORKSPACE/ci/headers"
EMPTY_JSON="$GITHUB_WORKSPACE/install/usr/include/ctrlm_config_empty.json"

cmake -G Ninja -S "$GITHUB_WORKSPACE" -B build/control \
-DCMAKE_INSTALL_PREFIX="${GITHUB_WORKSPACE}/install/usr" \
-DCMAKE_INSTALL_SYSCONFDIR="${GITHUB_WORKSPACE}/install/etc" \
-DCMAKE_MODULE_PATH="${GITHUB_WORKSPACE}/install/tools/cmake" \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DCMAKE_PROJECT_VERSION="1.0.0" \
-DTHUNDER=OFF \
-DTHUNDER_SECURITY=OFF \
-DBLE_ENABLED=OFF \
-DRF4CE_ENABLED=OFF \
-DIP_ENABLED=OFF \
-DTELEMETRY_SUPPORT=OFF \
-DAUTH_ENABLED=OFF \
-DXRSR_HTTP=OFF \
-DXRSR_SDT=OFF \
-DBUILD_CTRLM_FACTORY=OFF \
-DBUILD_FACTORY_TEST=OFF \
-DUSE_SAFEC=OFF \
-DUSE_IARM_POWER_MANAGER=ON \
-DBREAKPAD=OFF \
-DFDC_ENABLED=OFF \
-DENABLE_ASYNC_SRVR_MSG=OFF \
-DHIDE_NON_EXTERNAL_SYMBOLS=OFF \
-DBUILD_SYSTEM=NONE \
-DCTRLM_UTILS_JSON_COMBINE="${GITHUB_WORKSPACE}/xr-voice-sdk/scripts/vsdk_json_combine.py" \
-DCTRLM_UTILS_JSON_TO_HEADER="${GITHUB_WORKSPACE}/xr-voice-sdk/scripts/vsdk_json_to_header.py" \
-DCTRLM_CONFIG_JSON_CPC="${EMPTY_JSON}" \
-DCTRLM_CONFIG_JSON_CPC_SUB="${EMPTY_JSON}" \
-DCTRLM_CONFIG_JSON_CPC_ADD="${EMPTY_JSON}" \
-DCTRLM_CONFIG_JSON_MAIN_SUB="${EMPTY_JSON}" \
-DCTRLM_CONFIG_JSON_MAIN_ADD="${EMPTY_JSON}" \
-DCMAKE_CXX_FLAGS=" \
-I ${XRSDK_REAL_HEADERS} \
-I ${MOCK_DIR} \
-I ${MOCK_DIR}/devicesettings \
-I ${HEADERS_DIR} \
-I ${HEADERS_DIR}/rdk/iarmbus \
-I ${HEADERS_DIR}/rdk/ds \
-I ${HEADERS_DIR}/rdk/iarmmgrs-hal \
-I ${GITHUB_WORKSPACE}/install/usr/include \
-I /usr/include/glib-2.0 \
-I /usr/lib/x86_64-linux-gnu/glib-2.0/include \
-I /usr/include/libdrm \
-include ${XLOG_COMPAT} \
-include ${MOCK_DIR}/Iarm.h \
-include ${MOCK_OVERRIDES} \
-include ${MOCK_DIR}/devicesettings.h \
-include ${MOCK_DIR}/Rfc.h \
-Wall -Wno-error \
-DSAFEC_DUMMY_API \
-DDISABLE_SECURITY_TOKEN" \
-DCMAKE_C_FLAGS=" \
-I ${XRSDK_REAL_HEADERS} \
-I ${MOCK_DIR} \
-I ${HEADERS_DIR} \
-I ${HEADERS_DIR}/rdk/iarmbus \
-I ${HEADERS_DIR}/rdk/ds \
-I ${HEADERS_DIR}/rdk/iarmmgrs-hal \
-I ${GITHUB_WORKSPACE}/install/usr/include \
-Wall -Wno-error \
-DSAFEC_DUMMY_API \
-DDISABLE_SECURITY_TOKEN" \
-DCMAKE_EXE_LINKER_FLAGS="-L${GITHUB_WORKSPACE}/install/usr/lib -Wl,--unresolved-symbols=ignore-all"

# CMakeLists.txt unconditionally adds -Werror via target_compile_options, which
# appends after CMAKE_CXX_FLAGS and overrides our -Wno-error. Strip it from
# the generated build files after cmake configure.
find "${GITHUB_WORKSPACE}/build/control" \( -name "*.ninja" -o -name "flags.make" \) -exec sed -i 's/\(^\|[[:space:]]\)-Werror\([[:space:]]\|$\)/\1\2/g' {} \;

cmake --build build/control -j$(nproc) 2>&1
echo "======================================================================================"
echo "control build complete"
exit 0
