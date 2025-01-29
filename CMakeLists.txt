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

cmake_minimum_required(VERSION 3.16)

if(CMAKE_VERSION GREATER_EQUAL 3.21)
   set(CMAKE_C_STANDARD 23)
else()
   set(CMAKE_C_STANDARD 11)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

project(ctrlm-main)

# OPTIONS
option( GIT_BRANCH,      "Repository Branch" OFF)
option( ASB "Enable ASB" OFF )
option( AUTH_ENABLED "Enable AUTH" OFF )
option( AUTH_ACCOUNT_ID "Enable AUTH_ACCOUNT_ID" OFF )
option( AUTH_DEVICE_ID "Enable AUTH_DEVICE_ID" OFF )
option( AUTH_EXPERIENCE "Enable AUTH_EXPERIENCE" OFF )
option( AUTH_PARTNER_ID "Enable AUTH_PARTNER_ID" OFF )
option( AUTH_RECEIVER_ID "Enable AUTH_RECEIVER_ID" OFF )
option( AUTH_SAT_TOKEN "Enable AUTH_SAT_TOKEN" OFF )
option( BLE_ENABLED "Enable BLE" OFF )
option( BLE_SERVICES "Enable BLE Services" OFF )
option( BREAKPAD "Enable BREAKPAD" OFF )
option( CPC "Enable CPC" OFF )
option( DUNFELL_BUILD_SUPPORT "Enable DUNFELL_BUILD_SUPPORT" OFF )
option( FDC "Enable FDC" OFF )
option( IP_ENABLED "Enable IP" OFF )
option( IRDB "Enable IRDB" OFF )
option( MIC_TAP "Enable MIC_TAP" OFF )
option( RF4CE_ENABLED "Enable RF4CE" OFF )
option( RF4CE_HAL_QORVO "Enable RF4CE_HAL_QORVO" OFF )
option( RF4CE_PACKET_ANALYSIS "Enable RF4CE_PACKET_ANALYSIS" OFF )
option( TELEMETRY_SUPPORT "Enable TELEMETRY_SUPPORT" OFF )
option( THUNDER "Enable THUNDER" OFF )
option( THUNDER_SECURITY "Enable THUNDER_SECURITY" OFF )
option( VOICE_KEYWORD_BEEP "Enable VOICE_KEYWORD_BEEP" OFF )
option( VOICE_NEXTGEN_MAC "Enable VOICE_NEXTGEN_MAC" OFF )
option( XRSR_HTTP "Enable XRSR_HTTP" OFF )
option( XRSR_SDT "Enable XRSR_SDT" OFF )
option( ENABLE_ASYNC_SRVR_MSG "Enable Asynchronous Server Messaging Feature" OFF)
option( ENABLE_AVS_WITH_SDT "Enable SDT Based AVS Support" OFF)


## Build System
set(STAGING_BINDIR_NATIVE, "invalid" CACHE PATH "staging bin dir native")
set(BUILD_SYSTEM "NONE" CACHE STRING "Specify the Build System being used")
set_property(CACHE BUILD_SYSTEM PROPERTY STRINGS "NONE" "YOCTO")

# EXECUTABLE
add_executable(controlMgr src/ctrlm_config_default.json ctrlm_version_build.h ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c)

# INCLUDES
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     include/
                     src/
                     src/asb/
                     src/attributes/
                     src/auth/
                     src/ble/hal/blercu/
                     src/ble/hal/
                     src/ble/hal/utils/
                     src/config/
                     src/shared_memory/
                     src/database/
                     src/network/
                     src/rfc/
                     src/rf4ce/rib
                     src/irdb/
                     src/irdb/ipc/
                     src/voice/
                     src/voice/endpoints/
                     src/voice/ipc/
                     src/voice/telemetry/
                     src/thunder/
                     src/thunder/helpers/
                     src/thunder/plugins/
                     src/telemetry/
                     src/ipc/
                     src/input_event/
                     ${CMAKE_SYSROOT}/usr/include/safeclib/
                     ${CMAKE_SYSROOT}/usr/include/libsafec/
                     ${CMAKE_SYSROOT}/usr/include/nopoll/
                  )

# SOURCE
add_subdirectory(src)

# COMPILER OPTIONS
add_compile_options(PUBLIC -fPIC -rdynamic -Wall -Werror)
add_compile_definitions(PUBLIC _REENTRANT _POSIX_C_SOURCE=200809L _GNU_SOURCE)
install(TARGETS  controlMgr RUNTIME DESTINATION bin)

install(FILES ${CMAKE_BINARY_DIR}/ctrlm_config.json.template DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT config )

if(CPC_ENABLED)
   target_link_libraries(controlMgr ${CTRLM_CPC_LIBRARY})
endif()

if(BLE_ENABLED)
   add_compile_definitions(CTRLM_NETWORK_BLE AUDIO_DECODE)
    if(BLE_SERVICES)
       target_link_libraries(controlMgr ctrlm-ble-services.a)
       target_compile_definitions(controlMgr PUBLIC CTRLM_BLE_SERVICES)
    endif()
endif()

if(RF4CE_ENABLED)
   target_link_libraries(controlMgr ${CTRLM_HAL_RF4CE_LIBRARY})
   add_compile_definitions(CTRLM_NETWORK_RF4CE CTRLM_NETWORK_HAS_HAL_NVM)
   if(RF4CE_HAL_QORVO)
      add_compile_definitions(CTRLM_RF4CE_HAL_QORVO)
   endif()
endif()

if(MIC_TAP)
   add_compile_definitions(CTRLM_LOCAL_MIC_TAP)
endif()

if(THUNDER)
   add_compile_definitions(CTRLM_THUNDER)
endif()

if(IRDB)
   add_compile_definitions(IRDB_ENABLED)
   if(IRDB_HDMI_DISCOVERY)
      add_compile_definitions(CTRLM_IRDB_HDMI_DISCOVERY)
   endif()
   if(IRDB_CEC_DISCOVERY)
      add_compile_definitions(CTRLM_IRDB_CEC_DISCOVERY)
   endif()
   if(IRDB_CEC_FLEX2)
      add_compile_definitions(CTRLM_IRDB_CEC_FLEX2)
   endif()
endif()

if(BREAKPAD)
   target_link_libraries(controlMgr breakpadwrapper)
   add_compile_definitions(BREAKPAD_SUPPORT)
endif()

if(ENABLE_ASYNC_SRVR_MSG)
   add_compile_definitions(SUPPORT_ASYNC_SRVR_MSG)
endif()

if(ENABLE_AVS_WITH_SDT)
    add_compile_definitions(SUPPORT_VOICE_DEST_ALSA SUPPORT_ASYNC_SRVR_MSG)
endif()

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
   VERBATIM)

add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   COMMAND python3 ${CTRLM_UTILS_JSON_COMBINE} -i ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.json -a ${CTRLM_CONFIG_JSON_VSDK}:vsdk -a ${CTRLM_CONFIG_JSON_CPC} -s ${CTRLM_CONFIG_JSON_CPC_SUB} -a ${CTRLM_CONFIG_JSON_CPC_ADD} -s ${CTRLM_CONFIG_JSON_OEM_SUB} -a ${CTRLM_CONFIG_JSON_OEM_ADD} -s ${CTRLM_CONFIG_JSON_MAIN_SUB} -a ${CTRLM_CONFIG_JSON_MAIN_ADD} -o ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   DEPENDS src/ctrlm_config_default.json
   VERBATIM
   )

add_custom_command( OUTPUT ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c
   COMMAND python3 ${CTRLM_UTILS_JSON_TO_HEADER} -i ${CMAKE_BINARY_DIR}/ctrlm_config.json.template -o ctrlm_config_default.h -c ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c -v "ctrlm_global,network_rf4ce,network_ip,network_ble,ir,voice,device_update" -d "network_ble"
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json.template
   VERBATIM)

add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c
   COMMAND gperf --output-file=${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.hash
   VERBATIM)

add_custom_target( ctrlm_config
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json.template)
