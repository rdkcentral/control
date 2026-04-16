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

git clone --depth 1 --branch R4.4.3 https://github.com/rdkcentral/ThunderTools.git

git clone --depth 1 --branch R4.4.1 https://github.com/rdkcentral/Thunder.git

git clone --depth 1 --branch develop https://github.com/rdkcentral/entservices-apis.git

# 1.0.13
git clone https://github.com/rdkcentral/xr-voice-sdk.git
git -C xr-voice-sdk checkout e55c99a0ec947b0ad3efc308bf8e3de0a42140d5

git clone https://github.com/rdkcentral/entservices-testframework.git
git -C entservices-testframework checkout 584e3ec70fd5e044982910b4eb15c465808bb6d1

# Patch testframework mocks with declarations ctrlm needs that are not yet upstream.
# We should remove as it is cleaner to just make changes to entservices-testframework directly
# device::Manager::IsInitialized (static bool member)
sed -i '/static void Initialize();/i\    inline static bool IsInitialized = false;' \
    entservices-testframework/Tests/mocks/devicesettings.h
# dsAudioDucking enums (after dsAudioPortType_t)
sed -i '/^} dsAudioPortType_t;/a\
typedef enum _dsAudioDuckingAction_t {\
    dsAUDIO_DUCKINGACTION_START = 0,\
    dsAUDIO_DUCKINGACTION_STOP  = 1\
} dsAudioDuckingAction_t;\
\
typedef enum _dsAudioDuckingType_t {\
    dsAUDIO_DUCKINGTYPE_ABSOLUTE = 0,\
    dsAUDIO_DUCKINGTYPE_RELATIVE = 1\
} dsAudioDuckingType_t;' \
    entservices-testframework/Tests/mocks/devicesettings.h
# AudioOutputPort::setAudioDucking (after reInitializeAudioOutputPort)
sed -i '/void reInitializeAudioOutputPort();/a\
    void setAudioDucking(dsAudioDuckingAction_t action, dsAudioDuckingType_t type, float level) {\
        (void)action;\
        (void)type;\
        (void)level;\
    }' \
    entservices-testframework/Tests/mocks/devicesettings.h

############################
# 3. Build Thunder-Tools
echo "======================================================================================"
echo "building thunderTools"
cd ThunderTools
patch -p1 < $GITHUB_WORKSPACE/entservices-testframework/patches/00010-R4.4-Add-support-for-project-dir.patch
cd -

cmake -G Ninja -S ThunderTools -B build/ThunderTools \
    -DEXCEPTIONS_ENABLE=ON \
    -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake"

cmake --build build/ThunderTools --target install

############################
# 4. Build Thunder
echo "======================================================================================"
echo "building thunder"

cd Thunder
patch -p1 < $GITHUB_WORKSPACE/entservices-testframework/patches/Use_Legact_Alt_Based_On_ThunderTools_R4.4.3.patch
patch -p1 < $GITHUB_WORKSPACE/entservices-testframework/patches/error_code_R4_4.patch
patch -p1 < $GITHUB_WORKSPACE/entservices-testframework/patches/1004-Add-support-for-project-dir.patch
patch -p1 < $GITHUB_WORKSPACE/entservices-testframework/patches/RDKEMW-733-Add-ENTOS-IDS.patch
cd -

cmake -G Ninja -S Thunder -B build/Thunder \
    -DMESSAGING=ON \
    -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake" \
    -DBUILD_TYPE=Debug \
    -DBINDING=127.0.0.1 \
    -DPORT=55555 \
    -DEXCEPTIONS_ENABLE=ON

cmake --build build/Thunder --target install

############################
# 5. Build entservices-apis
echo "======================================================================================"
echo "building entservices-apis"
cd entservices-apis
rm -rf jsonrpc/DTV.json
cd ..

cmake -G Ninja -S entservices-apis -B build/entservices-apis \
    -DEXCEPTIONS_ENABLE=ON \
    -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake"

cmake --build build/entservices-apis --target install

############################
# 6. Create stub/empty headers for external dependencies
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
mkdir -p "${HEADERS_DIR}/websocket"
mkdir -p "${HEADERS_DIR}/proc"
mkdir -p "${HEADERS_DIR}/audiocapturemgr"
mkdir -p "${HEADERS_DIR}/ccec/drivers"
mkdir -p "${HEADERS_DIR}/network"
mkdir -p "${HEADERS_DIR}/nopoll"
mkdir -p "${XRSDK_HEADERS_DIR}"

# Copy real xr-voice-sdk headers where control's source matches the real API.
# xr_voice_sdk.h is NOT copied: it requires rdkx_logger.h installed types unavailable in source form.
cp "$GITHUB_WORKSPACE/xr-voice-sdk/src/xr-fdc/xr_fdc.h" "${XRSDK_HEADERS_DIR}/"
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

# IARM managers (types provided via -include Iarm.h for sysMgr, or in control stubs)
touch rdk/iarmmgrs-hal/sysMgr.h
touch rdk/iarmmgrs-hal/irMgr.h
touch rdk/iarmmgrs-hal/deepSleepMgr.h
touch rdk/iarmmgrs-hal/mfrMgr.h
touch rdk/iarmmgrs-hal/pwrMgr.h
touch rdk/iarmmgrs-hal/plat_ir.h

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

# Other stubs
touch audiocapturemgr/audiocapturemgr_iarm.h
touch rdk_logger_milestone.h
touch btmgr.h
touch proc/readproc.h
touch nopoll/nopoll.h

# rfcapi.h (types provided via -include Rfc.h)
touch rfcapi.h

# telemetry (types provided via -include Telemetry.h)
touch telemetry_busmessage_sender.h
touch telemetry2_0.h

# secure_wrapper (types provided via -include secure_wrappermock.h)
touch secure_wrapper.h

# edid / drm
touch edid-parser.hpp
touch dsFPD.h

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
# 7. Create stub shared libraries for linking
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
