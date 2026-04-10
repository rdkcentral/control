#!/bin/bash
set -x
set -e
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
############################

# Build control (ctrlm-main)
echo "building control (ctrlm-main)"

CTRL_STUBS="$GITHUB_WORKSPACE/entservices-testframework/Tests/mocks/control"
MOCK_DIR="$GITHUB_WORKSPACE/entservices-testframework/Tests/mocks"
HEADERS_DIR="$GITHUB_WORKSPACE/entservices-testframework/Tests/headers"
EMPTY_JSON="$GITHUB_WORKSPACE/install/usr/include/ctrlm_config_empty.json"

cmake -G Ninja -S "$GITHUB_WORKSPACE" -B build/control \
-DCMAKE_INSTALL_PREFIX="${GITHUB_WORKSPACE}/install/usr" \
-DCMAKE_INSTALL_SYSCONFDIR="${GITHUB_WORKSPACE}/install/etc" \
-DCMAKE_MODULE_PATH="${GITHUB_WORKSPACE}/install/tools/cmake" \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DCMAKE_PROJECT_VERSION="1.0.0" \
-DUSE_THUNDER_R4=ON \
-DTHUNDER=ON \
-DTHUNDER_SECURITY=OFF \
-DBLE_ENABLED=OFF \
-DRF4CE_ENABLED=ON \
-DIP_ENABLED=OFF \
-DTELEMETRY_SUPPORT=OFF \
-DAUTH_ENABLED=OFF \
-DXRSR_HTTP=OFF \
-DXRSR_SDT=OFF \
-DBUILD_CTRLM_FACTORY=OFF \
-DBUILD_FACTORY_TEST=OFF \
-DUSE_SAFEC=OFF \
-DUSE_IARM_POWER_MANAGER=OFF \
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
-I ${CTRL_STUBS} \
-I ${MOCK_DIR} \
-I ${MOCK_DIR}/thunder \
-I ${MOCK_DIR}/devicesettings \
-I ${HEADERS_DIR} \
-I ${HEADERS_DIR}/rdk/iarmbus \
-I ${HEADERS_DIR}/rdk/ds \
-I ${HEADERS_DIR}/rdk/ds-rpc \
-I ${HEADERS_DIR}/rdk/ds-hal \
-I ${HEADERS_DIR}/rdk/halif/ds-hal \
-I ${HEADERS_DIR}/rdk/iarmmgrs-hal \
-I ${HEADERS_DIR}/audiocapturemgr \
-I ${HEADERS_DIR}/ccec/drivers \
-I ${HEADERS_DIR}/network \
-I ${HEADERS_DIR}/nopoll \
-I ${GITHUB_WORKSPACE}/install/usr/include \
-I ${GITHUB_WORKSPACE}/install/usr/include/WPEFramework \
-I /usr/include/glib-2.0 \
-I /usr/lib/x86_64-linux-gnu/glib-2.0/include \
-I /usr/include/libdrm \
-include ${MOCK_DIR}/Iarm.h \
-include ${MOCK_DIR}/devicesettings.h \
-include ${MOCK_DIR}/Rfc.h \
-include ${MOCK_DIR}/Telemetry.h \
-include ${MOCK_DIR}/secure_wrappermock.h \
-Wall -Wno-error \
-DSAFEC_DUMMY_API \
-DDISABLE_SECURITY_TOKEN \
-DUSE_THUNDER_R4=ON -DTHUNDER_VERSION=4 -DTHUNDER_VERSION_MAJOR=4 -DTHUNDER_VERSION_MINOR=4" \
-DCMAKE_C_FLAGS=" \
-I ${CTRL_STUBS} \
-I ${MOCK_DIR} \
-I ${HEADERS_DIR} \
-I ${HEADERS_DIR}/rdk/iarmbus \
-I ${HEADERS_DIR}/rdk/ds \
-I ${HEADERS_DIR}/rdk/iarmmgrs-hal \
-I ${GITHUB_WORKSPACE}/install/usr/include \
-I ${GITHUB_WORKSPACE}/install/usr/include/WPEFramework \
-Wall -Wno-error \
-DSAFEC_DUMMY_API \
-DDISABLE_SECURITY_TOKEN" \
-DCMAKE_EXE_LINKER_FLAGS="-L${GITHUB_WORKSPACE}/install/usr/lib -Wl,--unresolved-symbols=ignore-all"


# control's CMakeLists.txt adds -Werror via target_compile_options, which appends
# after CMAKE_CXX_FLAGS and overrides our -Wno-error. Strip it from generated build files.
find "${GITHUB_WORKSPACE}/build/control" \( -name "*.ninja" -o -name "flags.make" \) -exec sed -i 's/ -Werror\b//g' {} \;

cmake --build build/control -j$(nproc) 2>&1
echo "======================================================================================"
echo "control build complete"
exit 0
