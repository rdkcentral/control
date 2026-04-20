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
ls -la ${GITHUB_WORKSPACE}
cd ${GITHUB_WORKSPACE}

git config --global --add safe.directory "${GITHUB_WORKSPACE}"

# #############################
# 1. Install Dependencies and packages

apt update
apt install -y \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libsystemd-dev \
    libboost-all-dev \
    libglib2.0-dev \
    libgio2.0-cil-dev \
    libjansson-dev \
    libarchive-dev \
    libssl-dev \
    zlib1g-dev \
    libdbus-1-dev \
    uuid-dev \
    libevdev-dev \
    libdrm-dev \
    gperf \
    meson \
    valgrind \
    lcov \
    libsafec-dev \
    clang
pip install jsonref

###########################################
# 2. Clone the required repositories

# 1.0.13
git clone https://github.com/rdkcentral/xr-voice-sdk.git
git -C xr-voice-sdk checkout e55c99a0ec947b0ad3efc308bf8e3de0a42140d5

git clone https://github.com/rdkcentral/entservices-testframework.git
git -C entservices-testframework checkout 584e3ec70fd5e044982910b4eb15c465808bb6d1

git clone --depth 1 --filter=blob:none --sparse --branch develop https://github.com/rdkcentral/iarmmgrs.git
git -C iarmmgrs sparse-checkout set hal

git clone --depth 1 --filter=blob:none --sparse https://github.com/rdkcentral/rdk-halif-deepsleep_manager.git
git -C rdk-halif-deepsleep_manager sparse-checkout set include

git clone --depth 1 --filter=blob:none --sparse https://github.com/rdkcentral/rdk-halif-power_manager.git
git -C rdk-halif-power_manager sparse-checkout set include

IARMMGRS_DIR="$GITHUB_WORKSPACE/iarmmgrs"
DEEPSLEEP_HAL_DIR="$GITHUB_WORKSPACE/rdk-halif-deepsleep_manager"
POWER_HAL_DIR="$GITHUB_WORKSPACE/rdk-halif-power_manager"

############################
# 3. Create stub/empty headers for external dependencies
echo "======================================================================================"
echo "Creating stub headers"

HEADERS_DIR="$GITHUB_WORKSPACE/ci/headers"
XRSDK_HEADERS_DIR="$HEADERS_DIR/xr-voice-sdk"
mkdir -p "${HEADERS_DIR}"
mkdir -p "${HEADERS_DIR}/rdk/iarmbus"
mkdir -p "${HEADERS_DIR}/rdk/ds"
mkdir -p "${HEADERS_DIR}/rdk/ds-rpc"
mkdir -p "${HEADERS_DIR}/rdk/ds-hal"
mkdir -p "${HEADERS_DIR}/rdk/halif/ds-hal"
mkdir -p "${HEADERS_DIR}/rdk/iarmmgrs-hal"
mkdir -p "${XRSDK_HEADERS_DIR}"

# Copy real xr-voice-sdk headers where control's source matches the real API.
# xr_voice_sdk.h is NOT copied: it requires rdkx_logger.h installed types unavailable in source form.
# xr_fdc.h is NOT copied: only needed when FDC_ENABLED=ON
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-speech-vrex/xrsv.h" "${XRSDK_HEADERS_DIR}/"
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-speech-router/xrsr.h" "${XRSDK_HEADERS_DIR}/"
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-mq/xr_mq.h" "${XRSDK_HEADERS_DIR}/"
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-speech-vrex/xrsv_http/xrsv_http.h" "${XRSDK_HEADERS_DIR}/"
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-speech-vrex/xrsv_ws_nextgen/xrsv_ws_nextgen.h" "${XRSDK_HEADERS_DIR}/"
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-timestamp/xr_timestamp.h" "${XRSDK_HEADERS_DIR}/"

cd "${HEADERS_DIR}"

# IARM headers (types provided via -include Iarm.h)
touch rdk/iarmbus/libIARM.h
touch rdk/iarmbus/libIBus.h
touch rdk/iarmbus/libIBusDaemon.h

# IARM manager headers
# sysMgr.h is included by ctrlm, but the required SYSMgr types already come from the forced
# Iarm.h mock. Using the real header here causes duplicate typedefs, so provide a shim only.
cat > rdk/iarmmgrs-hal/sysMgr.h <<'EOF'
#ifndef CTRLM_CI_SYSMGR_SHIM_H
#define CTRLM_CI_SYSMGR_SHIM_H

/* SYSMgr declarations are provided by the forced Iarm.h mock in CI. */

#endif
EOF
cp "$DEEPSLEEP_HAL_DIR/include/deepSleepMgr.h" rdk/iarmmgrs-hal/deepSleepMgr.h
cp "$POWER_HAL_DIR/include/plat_power.h" rdk/iarmmgrs-hal/pwrMgr.h
[ -f rdk/iarmmgrs-hal/sysMgr.h ]
[ -f rdk/iarmmgrs-hal/deepSleepMgr.h ]
[ -f rdk/iarmmgrs-hal/pwrMgr.h ]

# Device settings headers (types provided via -include devicesettings.h)
touch rdk/ds/audioOutputPort.hpp
touch rdk/ds/compositeIn.hpp
touch rdk/ds/dsDisplay.h
touch rdk/ds/dsError.h
touch rdk/ds/dsMgr.h
touch rdk/ds/dsRpc.h
touch rdk/ds/dsTypes.h
touch rdk/ds/dsUtl.h
touch rdk/ds/exception.hpp
touch rdk/ds/hdmiIn.hpp
touch rdk/ds/host.hpp
touch rdk/ds/list.hpp
# manager.hpp is empty — device::Manager is provided by the force-included devicesettings.h
touch rdk/ds/manager.hpp
touch rdk/ds/sleepMode.hpp
touch rdk/ds/videoDevice.hpp
touch rdk/ds/videoOutputPort.hpp
touch rdk/ds/videoOutputPortConfig.hpp
touch rdk/ds/videoOutputPortType.hpp
touch rdk/ds/videoResolution.hpp
touch rdk/ds/frontPanelIndicator.hpp
touch rdk/ds/frontPanelConfig.hpp
touch rdk/ds/frontPanelTextDisplay.hpp

# rfcapi.h (types provided via -include Rfc.h)
touch rfcapi.h

# comcastIrKeyCodes.h (unconditionally included by ctrlm_main.cpp)
find "$IARMMGRS_DIR" -name comcastIrKeyCodes.h -print -quit | xargs -r -I{} cp "{}" comcastIrKeyCodes.h
[ -f comcastIrKeyCodes.h ]

# secure_wrapper (types provided via empty stub — no v_secure_* calls in core)
touch secure_wrapper.h

# safec compatibility header - committed in ci/mocks, copied here so it is
# resolved on the generated-headers include path.
cp "$GITHUB_WORKSPACE/ci/mocks/safec_lib.h" safec_lib.h

echo "Stub headers created successfully"

cd ${GITHUB_WORKSPACE}

mkdir -p "${GITHUB_WORKSPACE}/install/usr/include"
printf '{}\n' > "${GITHUB_WORKSPACE}/install/usr/include/ctrlm_config_empty.json"

# Copy system headers for compatibility
cp -r /usr/include/glib-2.0/* /usr/lib/x86_64-linux-gnu/glib-2.0/include/* "${HEADERS_DIR}/" 2>/dev/null || true

############################
# 4. Create stub shared libraries for linking
echo "======================================================================================"
echo "Creating stub libraries"

STUB_LIB_DIR="$GITHUB_WORKSPACE/install/usr/lib"
mkdir -p "${STUB_LIB_DIR}"

# Create a minimal C stub source
cat > /tmp/stub.c << 'STUB_EOF'
void __stub_placeholder(void) {}
STUB_EOF

# Build stub .so for each missing library
for lib in xr-voice-sdk rdkversion IARMBus ds dshalcli nopoll rfcapi secure_wrapper evdev; do
    gcc -shared -fPIC -o "${STUB_LIB_DIR}/lib${lib}.so" /tmp/stub.c
done

rm /tmp/stub.c

echo "Stub libraries created in ${STUB_LIB_DIR}"
echo "======================================================================================"
echo "build_dependencies.sh complete"
