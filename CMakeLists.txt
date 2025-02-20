##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
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
##########################################################################

# CMAKE OPTIONS
cmake_minimum_required(VERSION 3.16)

if(CMAKE_VERSION GREATER_EQUAL 3.21)
   set(CMAKE_C_STANDARD 23)
else()
   set(CMAKE_C_STANDARD 11)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

# PROJECT
project(ctrlm-main)

# OPTIONS
option(A5000_ENABLE "ES1 A5000 use libse051" OFF)
option(ANSI_CODES_DISABLED "Disable ANSI code logging" OFF)
option(ASSERT_ON_WRONG_THREAD "Assert on wrong thread" OFF)
option(ASB "Enable ASB" OFF)
option(AUTH_ENABLED "Enable AUTH" OFF)
option(BLE_ENABLED "Enable BLE" OFF)
option(BLE_SERVICES "Enable BLE Services" OFF)
option(BREAKPAD "Enable BREAKPAD" OFF)
option(BUILD_CTRLM_FACTORY "Build Control Factory Test" OFF)
option(CPC "Enable CPC" OFF)
option(DISABLE_BLE_VOICE "Disable BLE voice" OFF)
option(DEEPSLEEP_CLOSE_DB "Deep Sleep Close DB" OFF)
option(DUNFELL_BUILD_SUPPORT "Enable DUNFELL_BUILD_SUPPORT" OFF)
option(ENABLE_DEEP_SLEEP "Enable deep sleep (different from deep sleep close DB)" OFF)
option(FACTORY_AUDIO_PLAYBACK "Factory test audio playback" OFF)
option(FACTORY_CUSTOM_AUDIO_ANALYSIS "Factory custom audio analysis" OFF)
option(FDC_ENABLED "Enable FDC" OFF)
option(GIT_BRANCH, "Repository Branch" OFF)
option(IP_ENABLED "Enable IP" OFF)
option(IRDB "Enable IRDB" OFF)
option(IRDB_HDMI_DISCOVERY "IRDB HDMI Discovery" OFF)
option(IRDB_CEC_DISCOVERY "IRDB_CEC Discovery" OFF)
option(IRDB_CEC_FLEX2 "IRDB CEC Flex2)" OFF)
option(LOCAL_MIC "Local Microphone" OFF)
option(LOCAL_MIC_DISABLE_VIA_PRIVACY "Use Privacy to disable microphone" OFF)
option(MAC_POLLING "MAC polling" OFF)
option(MEM_DEBUG "Enable memory debugging" OFF)
option(MEMORY_LOCK "Memory Lock" OFF)
option(MIC_TAP "Enable MIC_TAP" OFF)
option(RF4CE_ENABLED "Enable RF4CE" OFF)
option(RF4CE_HAL_QORVO "Enable RF4CE_HAL_QORVO" OFF)
option(RF4CE_PACKET_ANALYSIS "Enable RF4CE_PACKET_ANALYSIS" OFF)
option(TELEMETRY_SUPPORT "Enable TELEMETRY_SUPPORT" OFF)
option(THUNDER "Enable THUNDER" OFF)
option(THUNDER_SECURITY "Enable THUNDER_SECURITY" OFF)
option(USE_SAFEC "Use safec" OFF)
option(VOICE_KEYWORD_BEEP "Enable VOICE_KEYWORD_BEEP" OFF)
option(VOICE_NEXTGEN_MAC "Enable VOICE_NEXTGEN_MAC" OFF)
option(XRSR_HTTP "Enable XRSR_HTTP" OFF)
option(XRSR_SDT "Enable XRSR_SDT" OFF)
option(ENABLE_ASYNC_SRVR_MSG "Enable Asynchronous Server Messaging Feature" OFF)
option(ENABLE_AVS_WITH_SDT "Enable SDT Based AVS Support" OFF)

# BUILD SYSTEM
set(STAGING_BINDIR_NATIVE, "invalid" CACHE PATH "staging bin dir native")
set(BUILD_SYSTEM "NONE" CACHE STRING "Specify the Build System being used")
set_property(CACHE BUILD_SYSTEM PROPERTY STRINGS "NONE" "YOCTO")

# BUILD VARIABLES
set(BEEP_ON_KWD_FILE "NONE" CACHE STRING "Keyword Beep file name")
set(CTRLM_HAL_RF4CE_API_VERSION "0" CACHE STRING "RF4CE HAL API version")



# EXECUTABLE(S)
add_executable(controlMgr src/ctrlm_config_default.json ctrlm_version_build.h ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c)

if(BUILD_CTRLM_FACTORY)
    add_library(ctrlm-fta)
    add_executable(controlFactory)
endif()

# INCLUDES
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     include
                     src
                     src/asb
                     src/attributes
                     src/auth
                     src/ble/hal/blercu
                     src/ble/hal
                     src/ble/hal/utils
                     src/config
                     src/shared_memory
                     src/database
                     src/network
                     src/rfc
                     src/rf4ce/rib
                     src/irdb
                     src/irdb/ipc
                     src/voice
                     src/voice/endpoints
                     src/voice/ipc
                     src/voice/telemetry
                     src/thunder
                     src/thunder/helpers
                     src/thunder/plugins
                     src/telemetry
                     src/ipc
                     src/input_event
                     ${CMAKE_SYSROOT}/usr/include/safeclib
                     ${CMAKE_SYSROOT}/usr/include/libsafec
                     ${CMAKE_SYSROOT}/usr/include/nopoll
                     ${CMAKE_SYSROOT}/usr/include
                     ${CMAKE_SYSROOT}/usr/lib/dbus-1.0/include
                     ${CMAKE_SYSROOT}/usr/include/dbus-1.0
                     ${CMAKE_SYSROOT}/usr/include/glib-2.0
                     ${CMAKE_SYSROOT}/usr/lib/glib-2.0/include
                     ${CMAKE_SYSROOT}/usr/include/gio-unix-2.0
                     ${CMAKE_SYSROOT}/usr/include/rdk/iarmbus
                     ${CMAKE_SYSROOT}/usr/include/nopoll
                     ${CMAKE_SYSROOT}/usr/include/rdk/iarmmgrs-hal
                     ${CMAKE_SYSROOT}/usr/include/wdmp-c
                     ${CMAKE_SYSROOT}/usr/include/rdk/ds
                     ${CMAKE_SYSROOT}/usr/include/rdk/ds-rpc
                     ${CMAKE_SYSROOT}/usr/include/rdk/ds-hal
                     ${CMAKE_SYSROOT}/usr/include/rdk/halif/ds-hal
                     ${CMAKE_SYSROOT}/usr/include/WPEFramework
                     ${CMAKE_SYSROOT}/usr/include/breakpad
                   )

# SOURCES
add_subdirectory(src)

# COMPILER OPTIONS
target_compile_options(controlMgr PUBLIC -fPIC -rdynamic -Wall -Werror)
add_compile_definitions(PUBLIC _REENTRANT _POSIX_C_SOURCE=200809L _GNU_SOURCE SYSTEMD_NOTIFY XR15_704 _REENTRANT)
target_link_libraries(controlMgr rt pthread stdc++ sqlite3 curl gio-2.0 gobject-2.0 glib-2.0 z archive jansson dbus-1 uuid IARMBus ds nopoll ssl crypto systemd dshalcli rfcapi rdkversion secure_wrapper xr-voice-sdk evdev)

install(TARGETS controlMgr RUNTIME DESTINATION bin)

install(FILES ${CMAKE_BINARY_DIR}/ctrlm_config.json.template DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT config )

# DEFINES FROM OPTIONS
if(A5000_ENABLE)
   find_library( SE_HAL_PATH SE_HAL ${CMAKE_SYSROOT}/usr/lib/se051)
   target_link_libraries(controlMgr ${SE_HAL_PATH})
endif()

if(ANSI_CODES_DISABLED)
   add_compile_definitions(ANSI_CODES_DISABLED)
endif()

if(ASSERT_ON_WRONG_THREAD)
   add_comile_definitions(ASSERT_ON_WRONG_THREAD)
endif()

if(AUTH_ENABLED)
   add_compile_definitions(AUTH_ENABLED)
   add_compile_definitions(AUTH_ACCOUNT_ID)
   add_compile_definitions(AUTH_DEVICE_ID)
   add_compile_definitions(AUTH_PARTNER_ID)
   add_compile_definitions(AUTH_SAT_TOKEN)

   #By default disabled but can be enabled
   #add_compile_definitions(AUTH_RECEIVER_ID)
   #add_compile_definitions(AUTH_EXPERIENCE)
endif()

if(BLE_ENABLED)
   add_compile_definitions(CTRLM_NETWORK_BLE)
   target_link_libraries(controlMgr BTMgr)
   if(BLE_SERVICES)
      add_compile_definitions(CTRLM_BLE_SERVICES)
      target_link_libraries(controlMgr ctrlm-ble-services.a)
   endif()
endif()

if(BREAKPAD)
   target_link_libraries(controlMgr breakpadwrapper)
   add_compile_definitions(BREAKPAD_SUPPORT)
endif()

if(CPC_ENABLED)
   target_link_libraries(controlMgr ${CTRLM_CPC_LIBRARY})
endif()

if(CTRLM_HAL_RF4CE_API_VERSION)
   add_compile_definitions(CTRLM_HAL_RF4CE_API_VERSION=${CTRLM_HAL_RF4CE_API_VERSION})
endif()

if(DEEPSLEEP_CLOSE_DB)
   add_compile_definitions(DEEPSLEEP_CLOSE_DB)
endif()

if(DISABLE_BLE_VOICE)
   add_compile_definitions(DISABLE_BLE_VOICE)
endif()

if(ENABLE_ASYNC_SRVR_MSG)
   add_compile_definitions(SUPPORT_ASYNC_SRVR_MSG)
endif()

if(ENABLE_AVS_WITH_SDT)
    add_compile_definitions(SUPPORT_VOICE_DEST_ALSA SUPPORT_ASYNC_SRVR_MSG)
endif()

if(ENABLE_DEEP_SLEEP)
   add_compile_definitions(DEEP_SLEEP_ENABLED)
endif()

if(FDC_ENABLED)
   add_compile_definitions(PUBLIC FDC_ENABLED)
endif()

if(IP_ENABLED)
   add_compile_definitions(CTRLM_NETWORK_IP CTRLM_IP_HAL_LOG_ENABLED)
endif()

if(THUNDER)
   add_compile_definitions(CTRLM_THUNDER)
endif()

if(IRDB)
   add_compile_definitions(IRDB_ENABLED)
   if(IRDB_CEC_DISCOVERY)
      add_compile_definitions(IRDB_CEC_DISCOVERY)
   endif()
   if(IRDB_CEC_FLEX2)
      add_compile_definitions(IRDB_CEC_FLEX2)
   endif()
   if(IRDB_HDMI_DISCOVERY)
      add_compile_definitions(IRDB_HDMI_DISCOVERY)
   endif()
endif()

if(LOCAL_MIC)
   add_compile_definitions(CTRLM_LOCAL_MIC)
   if(MIC_TAP)
      add_compile_definitions(CTRLM_LOCAL_MIC_TAP)
   endif()
   if(LOCAL_MIC_DISABLE_VIA_PRIVACY)
      add_compile_definitions(CTRLM_LOCAL_MIC_DISABLE_VIA_PRIVACY)
   endif()
endif()

if(MAC_POLLING)
   add_compile_definitions(MAC_POLLING)
endif()

if(MEM_DEBUG)
   add_compile_definitions(MEM_DEBUG)
endif()

if(MEMORY_LOCK)
   add_compile_definitions(MEMORY_LOCK)
   target_link_libraries(controLMgr clnl)
endif()

if(RF4CE_ENABLED)
   target_link_libraries(controlMgr ${CTRLM_HAL_RF4CE_LIBRARY})
   add_compile_definitions(CTRLM_NETWORK_RF4CE CTRLM_NETWORK_HAS_HAL_NVM)
   if(RF4CE_HAL_QORVO)
      add_compile_definitions(CTRLM_RF4CE_HAL_QORVO)
   endif()
endif()

if(TELEMETRY_SUPPORT)
   add_compile_definitions(TELEMETRY_SUPPORT)
   target_link_libraries(controlMgr telemetry_msgsender)
endif()

if(THUNDER)
   add_compile_definitions(PUBLIC CTRLM_THUNDER)
   target_link_libraries(controlMgr WPEFrameworkCore WPEFrameworkPlugins)
   if(WPE_FRAMEWORK_COM_SOCKET)
      target_link_libraries(controlMgr WPEFrameworkCOM WPEFrameworkWebSocket)
   endif()
   if(WPE_FRAMEWORK_PROTO_TRACING)
      target_link_libraries(controlMgr WPEFrameworkProtocols WPEFrameworkTracing)
   endif()
endif()

if(THUNDER_SECURITY)
   add_compile_definitions(PUBLIC THUNDER_SECURITY)
   target_link_libraries(controlMgr WPEFrameworkSecurityUtil)
endif()

if(USE_SAFEC)
   find_package(PkgConfig)
   pkg_check_modules(SAFEC REQUIRED libsafec)
   if(SAFEC_FOUND)
      target_link_libraries(controlMgr ${SAFEC_LIBRARIES})
   endif()
else()
   add_compile_definitions(PUBLIC SAFEC_DUMMY_API)
endif()

if(VOICE_KEYWORD_BEEP)
   add_compile_definitions(BEEP_ON_KWD_ENABLED)
   add_definitions(-DBEEP_ON_KWD_FILE=\"${CMAKE_DATADIR}/${BEEP_ON_KWD_FILE}\")
   install(FILES ${CMAKE_SOURCE_DIR}/../${BEEP_ON_KWD_FILE} DESTINATION share )
endif()

if(VOICE_NEXTGEN_MAC)
   add_compile_definitions(VOICE_NEXTGEN_MAC)
endif()

install(TARGETS  controlMgr RUNTIME DESTINATION bin)

install(FILES ${CMAKE_BINARY_DIR}/ctrlm_config.json.template DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT config )

# GENERATED FILES
add_custom_command( OUTPUT ctrlm_version_build.h
   COMMAND echo "#define CTRLM_MAIN_VER_MAJOR \"1\"" > ctrlm_version_build.h
   COMMAND echo -n "#define CTRLM_MAIN_VER_MINOR \"" >> ctrlm_version_build.h
   COMMAND bash -c "echo -n $(git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-list --count 4b825dc642cb6eb9a060e54bf8d69288fbee4904..HEAD) >> ctrlm_version_build.h"
   COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} diff --quiet || echo -n "++" >> ctrlm_version_build.h
   COMMAND echo "\"" >> ctrlm_version_build.h
   COMMAND echo "#define CTRLM_MAIN_BRANCH    \"${GIT_BRANCH}\"" >> ctrlm_version_build.h
   COMMAND echo -n "#define CTRLM_MAIN_COMMIT_ID " >> ctrlm_version_build.h
   COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} log --format=\"%H\" -n 1 >> ctrlm_version_build.h
   COMMAND echo "#define CTRLM_MAIN_VERSION (CTRLM_MAIN_VER_MAJOR \".\" CTRLM_MAIN_VER_MINOR)" >> ctrlm_version_build.h
   VERBATIM
)

add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   COMMAND python3 ${CTRLM_UTILS_JSON_COMBINE} -i ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.json -a ${CTRLM_CONFIG_JSON_VSDK}:vsdk -a ${CTRLM_CONFIG_JSON_CPC} -s ${CTRLM_CONFIG_JSON_CPC_SUB} -a ${CTRLM_CONFIG_JSON_CPC_ADD} -s ${CTRLM_CONFIG_JSON_OEM_SUB} -a ${CTRLM_CONFIG_JSON_OEM_ADD} -s ${CTRLM_CONFIG_JSON_MAIN_SUB} -a ${CTRLM_CONFIG_JSON_MAIN_ADD} -o ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   DEPENDS src/ctrlm_config_default.json
   VERBATIM
   )

add_custom_command(
   OUTPUT ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c
   COMMAND python3 ${CTRLM_UTILS_JSON_TO_HEADER} -i ${CMAKE_BINARY_DIR}/ctrlm_config.json.template -o ctrlm_config_default.h -c ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c -v "ctrlm_global,network_rf4ce,network_ip,network_ble,ir,voice,device_update" -d "network_ble"
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   VERBATIM
   )

add_custom_command(
   OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c
   COMMAND gperf --output-file=${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.hash
   VERBATIM
   )

add_custom_target( ctrlm_config
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   )
