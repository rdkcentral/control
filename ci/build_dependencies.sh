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
cd "${GITHUB_WORKSPACE}"

git config --global --add safe.directory "${GITHUB_WORKSPACE}"

# #############################
# 1. Install Dependencies and packages

apt update
apt install -y \
    pkg-config \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libsystemd-dev \
    libglib2.0-dev \
    libjansson-dev \
    libarchive-dev \
    libssl-dev \
    zlib1g-dev \
    libdbus-1-dev \
    uuid-dev \
    libevdev-dev \
    libdrm-dev \
    libbsd-dev \
    gperf \
    python3-pip
python3 -m pip install jsonref

###########################################
# 2. Clone the required repositories

git clone --depth 1 --filter=blob:none https://github.com/rdkcentral/xr-voice-sdk.git
git clone --depth 1 --filter=blob:none --branch feature/RDKEMW-18082 https://github.com/rdkcentral/xr-voice-sdk.git

git clone --depth 1 --filter=blob:none --branch feature/RDKEMW-18082 https://github.com/rdkcentral/entservices-testframework.git

git clone --depth 1 --filter=blob:none --sparse --branch develop https://github.com/rdkcentral/iarmmgrs.git
git -C iarmmgrs sparse-checkout set hal

git clone --depth 1 --filter=blob:none --sparse https://github.com/rdkcentral/rdk-halif-deepsleep_manager.git
git -C rdk-halif-deepsleep_manager sparse-checkout set include

git clone --depth 1 --filter=blob:none --sparse https://github.com/rdkcentral/rdk-halif-power_manager.git
git -C rdk-halif-power_manager sparse-checkout set include

git clone --depth 1 --filter=blob:none --sparse --branch develop https://github.com/rdkcentral/rdkversion.git
git -C rdkversion sparse-checkout set src

git clone --depth 1 --filter=blob:none --sparse https://github.com/rdkcentral/meta-rdk-oss-reference.git
git -C meta-rdk-oss-reference sparse-checkout set recipes-common/safec-common-wrapper/files

IARMMGRS_DIR="$GITHUB_WORKSPACE/iarmmgrs"
DEEPSLEEP_HAL_DIR="$GITHUB_WORKSPACE/rdk-halif-deepsleep_manager"
POWER_HAL_DIR="$GITHUB_WORKSPACE/rdk-halif-power_manager"
RDKVERSION_DIR="$GITHUB_WORKSPACE/rdkversion"
SAFEC_WRAPPER_DIR="$GITHUB_WORKSPACE/meta-rdk-oss-reference/recipes-common/safec-common-wrapper/files"

############################
# 3. Create stub/empty headers for external dependencies
echo "======================================================================================"
echo "Creating stub headers"

HEADERS_DIR="$GITHUB_WORKSPACE/ci/headers"
mkdir -p "${HEADERS_DIR}"
mkdir -p "${HEADERS_DIR}/rdk/iarmbus"
mkdir -p "${HEADERS_DIR}/rdk/ds"
mkdir -p "${HEADERS_DIR}/rdk/iarmmgrs-hal"

# Use the Yocto safec_lib.h sysroot header for CI builds without libsafec.
# Add include guards because the upstream header does not provide them.
cp "$SAFEC_WRAPPER_DIR/safec_lib.h" "$HEADERS_DIR/safec_lib.h"
sed -i '1s/^/#ifndef CTRLM_CI_SAFEC_LIB_H\n#define CTRLM_CI_SAFEC_LIB_H\n/' "$HEADERS_DIR/safec_lib.h"
printf '\n#endif /* CTRLM_CI_SAFEC_LIB_H */\n' >> "$HEADERS_DIR/safec_lib.h"

# Stage rdkversion.h before building xr-voice-sdk.
cp "$RDKVERSION_DIR/src/rdkversion.h" "$HEADERS_DIR/rdkversion.h"

# Build xr-voice-sdk and install its headers under ${HEADERS_DIR}/xr-voice-sdk/.
# Version doesn't matter here, but we try to get the latest tag for good measure since it's included in the generated headers and may be used by downstream code.
XRSDK_REF=$(git ls-remote --tags https://github.com/rdkcentral/xr-voice-sdk.git \
    | grep -oP '\d+\.\d+\.\d+$' | sort -V | tail -1)
cmake -G Ninja \
    -S "$GITHUB_WORKSPACE/xr-voice-sdk" \
    -B "$GITHUB_WORKSPACE/build/xr-voice-sdk" \
    -DCMAKE_INSTALL_PREFIX="${HEADERS_DIR}" \
    -DCMAKE_INSTALL_INCLUDEDIR="xr-voice-sdk" \
    -DCMAKE_C_FLAGS="-I${HEADERS_DIR} -DSAFEC_DUMMY_API" \
    -DSTAGING_BINDIR_NATIVE="/usr/bin" \
    -DCMAKE_PROJECT_VERSION="${XRSDK_REF}"

# xr-voice-sdk adds -Werror unconditionally, strip it for CI until warnings are dealt with
find "$GITHUB_WORKSPACE/build/xr-voice-sdk" \( -name "*.ninja" -o -name "flags.make" \) -exec sed -i 's/\(^\|[[:space:]]\)-Werror\([[:space:]]\|$\)/\1\2/g' {} \;

cmake --build "$GITHUB_WORKSPACE/build/xr-voice-sdk"
cmake --install "$GITHUB_WORKSPACE/build/xr-voice-sdk" --component headers

cd "${HEADERS_DIR}"

# IARM headers (types provided via -include Iarm.h)
touch rdk/iarmbus/libIARM.h
touch rdk/iarmbus/libIBus.h
touch rdk/iarmbus/libIBusDaemon.h

# IARM manager headers
# sysMgr.h conflicts with the forced Iarm.h mock, so use a shim instead.
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

# Device settings headers (types provided via force-included devicesettings.h mock)
touch rdk/ds/audioOutputPort.hpp
touch rdk/ds/dsDisplay.h
touch rdk/ds/dsError.h
touch rdk/ds/dsMgr.h
touch rdk/ds/dsRpc.h
touch rdk/ds/dsTypes.h
touch rdk/ds/dsUtl.h
touch rdk/ds/exception.hpp
touch rdk/ds/host.hpp
touch rdk/ds/manager.hpp
touch rdk/ds/videoOutputPort.hpp
touch rdk/ds/videoOutputPortConfig.hpp
touch rdk/ds/videoOutputPortType.hpp
touch rdk/ds/videoResolution.hpp
touch rdk/ds/frontPanelIndicator.hpp
touch rdk/ds/frontPanelConfig.hpp

# rfcapi.h (types provided via -include Rfc.h)
touch rfcapi.h

# comcastIrKeyCodes.h (unconditionally included by ctrlm_main.cpp)
find "$IARMMGRS_DIR" -name comcastIrKeyCodes.h -print -quit | xargs -r -I{} cp "{}" comcastIrKeyCodes.h
[ -f comcastIrKeyCodes.h ]

# rdkversion.h (used by ctrlm_main.cpp and xr-voice-sdk)
[ -f rdkversion.h ]

# secure_wrapper (types provided via empty stub — no v_secure_* calls in core)
touch secure_wrapper.h

echo "Stub headers created successfully"

cd "${GITHUB_WORKSPACE}"

mkdir -p "${GITHUB_WORKSPACE}/install/usr/include"
printf '{}\n' > "${GITHUB_WORKSPACE}/install/usr/include/ctrlm_config_empty.json"

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

# Build stub .so files for libraries still linked in CI.
# nopoll and dshalcli are factory-only but still linked unconditionally.
for lib in rdkversion IARMBus ds nopoll dshalcli rfcapi secure_wrapper evdev; do
    gcc -shared -fPIC -o "${STUB_LIB_DIR}/lib${lib}.so" /tmp/stub.c
done

# Copy the real xr-voice-sdk .so alongside the stubs.
cp "$GITHUB_WORKSPACE/build/xr-voice-sdk/src/libxr-voice-sdk.so" "${STUB_LIB_DIR}/libxr-voice-sdk.so"

rm /tmp/stub.c

echo "Stub libraries created in ${STUB_LIB_DIR}"
echo "======================================================================================"
echo "build_dependencies.sh complete"
