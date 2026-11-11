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
project(ctrlm-main VERSION ${CMAKE_PROJECT_VERSION})

# OPTIONS
option(AUTH_ENABLED "Enable AUTH" OFF)
option(BLE_ENABLED "Enable BLE" ON)
option(BLE_SERVICES "Enable BLE Services" OFF)
option(BREAKPAD "Enable BREAKPAD" OFF)
option(BUILD_CTRLM_FACTORY "Build Control Factory Test" OFF)
option(BUILD_CTRLM_SERVER "Build Control Server Daemon" OFF)
option(FDC_ENABLED "Enable FDC" OFF)
option(RF4CE_ENABLED "Enable RF4CE" ON)
option(TELEMETRY_SUPPORT "Enable TELEMETRY_SUPPORT" OFF)
option(THUNDER "Enable THUNDER" ON)
option(THUNDER_SECURITY "Enable THUNDER_SECURITY" OFF)
option(USE_SAFEC "Use safec" OFF)
option(XRSR_HTTP "Enable XRSR_HTTP" OFF)
option(XRSR_SDT "Enable XRSR_SDT" OFF)
option(ENABLE_ASYNC_SRVR_MSG "Enable Asynchronous Server Messaging Feature" OFF)
option(ENABLE_AVS_WITH_SDT "Enable SDT Based AVS Support" OFF)
option(INSTALL_HEADERS "Install public and private headers" OFF)

include(GNUInstallDirs)

# BUILD SYSTEM
set(STAGING_BINDIR_NATIVE, "invalid" CACHE PATH "staging bin dir native")
set(BUILD_SYSTEM "NONE" CACHE STRING "Specify the Build System being used")
set_property(CACHE BUILD_SYSTEM PROPERTY STRINGS "NONE" "YOCTO")

# BUILD VARIABLES
set(BEEP_ON_KWD_FILE "NONE" CACHE STRING "Keyword Beep file name")


# This is required to allow an executable to export symbols to be used by loadable modules (e.g. plugins using dlopen)
set(CMAKE_ENABLE_EXPORTS ON)

# EXECUTABLE(S)
add_executable(controlMgr src/ctrlm_config_default.json ctrlm_version_build.h ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c)

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
add_compile_definitions(_REENTRANT _POSIX_C_SOURCE=200809L _GNU_SOURCE _REENTRANT)

target_link_libraries(
      controlMgr

      xr-voice-sdk

      rdkversion
      rt
      pthread
      stdc++
      sqlite3
      curl
      gio-2.0
      gobject-2.0
      glib-2.0
      z
      archive
      jansson
      dbus-1
      uuid
      IARMBus
      ds
      nopoll
      ssl
      crypto
      systemd
      dshalcli
      rfcapi
      secure_wrapper
      evdev
      
      ${CMAKE_DL_LIBS}
      )

install(TARGETS controlMgr RUNTIME DESTINATION bin)

install(FILES ${CMAKE_BINARY_DIR}/ctrlm_config.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT config )

# DEFINES FROM OPTIONS
if(ANSI_CODES_DISABLED)
   add_compile_definitions(ANSI_CODES_DISABLED)
endif()

if(BLE_ENABLED)
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

if(ENABLE_ASYNC_SRVR_MSG)
   add_compile_definitions(SUPPORT_ASYNC_SRVR_MSG)
endif()

if(ENABLE_AVS_WITH_SDT)
    add_compile_definitions(SUPPORT_VOICE_DEST_ALSA SUPPORT_ASYNC_SRVR_MSG)
endif()

if(FDC_ENABLED)
   add_compile_definitions(FDC_ENABLED)
endif()

# RF4CE options
if(RF4CE_ENABLED)
   # All RF4CE platforms have HAL NVM
   add_compile_definitions(CTRLM_NETWORK_HAS_HAL_NVM)
endif()

if(TELEMETRY_SUPPORT)
   add_compile_definitions(TELEMETRY_SUPPORT)
   target_link_libraries(controlMgr telemetry_msgsender)
endif()

if(THUNDER)
   add_compile_definitions(CTRLM_THUNDER)
   target_link_libraries(controlMgr WPEFrameworkCore WPEFrameworkPlugins)
   if(WPE_FRAMEWORK_COM_SOCKET)
      target_link_libraries(controlMgr WPEFrameworkCOM WPEFrameworkWebSocket)
   endif()
   if(WPE_FRAMEWORK_PROTO_TRACING)
      target_link_libraries(controlMgr WPEFrameworkProtocols WPEFrameworkTracing)
   endif()
   if(THUNDER_SECURITY)
      add_compile_definitions(THUNDER_SECURITY)
      target_link_libraries(controlMgr WPEFrameworkSecurityUtil)
   endif()
   if(AUTH_ENABLED)
      add_compile_definitions(AUTH_ENABLED)
      add_compile_definitions(AUTH_ACCOUNT_ID)
      add_compile_definitions(AUTH_DEVICE_ID)
      add_compile_definitions(AUTH_PARTNER_ID)
      add_compile_definitions(AUTH_SAT_TOKEN)
      if(AUTH_ACTIVATION_STATUS)
         add_compile_definitions(AUTH_ACTIVATION_STATUS)
      endif()
      target_link_libraries(controlMgr RdkCertSelector)
   endif()
endif()

if(USE_SAFEC)
   find_package(PkgConfig)
   pkg_check_modules(SAFEC REQUIRED libsafec)
   if(SAFEC_FOUND)
      target_link_libraries(controlMgr ${SAFEC_LIBRARIES})
   endif()
else()
   add_compile_definitions(SAFEC_DUMMY_API)
endif()

install(TARGETS  controlMgr RUNTIME DESTINATION bin)

install(FILES ${CMAKE_BINARY_DIR}/ctrlm_config.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT config )

# HEADER FILES
# Only installed when INSTALL_HEADERS=ON (used by the ctrlm-headers recipe via the install_headers target)
if(INSTALL_HEADERS)
   set(CTRLM_PRIVATE_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/ctrlm_private)

   install(FILES include/ctrlm_ipc.h
                 include/ctrlm_ipc_rcu.h
                 include/ctrlm_ipc_voice.h
                 include/ctrlm_ipc_key_codes.h
                 include/ctrlm_ipc_device_update.h
                 include/ctrlm_ipc_ble.h
                 include/ctrlm_hal.h
                 include/ctrlm_hal_ble.h
                 include/ctrlm_hal_rf4ce.h
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)

   install(FILES src/ctrlm_vendor_network_factory.h
                 src/ctrlm_log.h
                 src/auth/ctrlm_auth.h
                 src/factory/ctrlm_fta_platform.h
                 src/asb/ctrlm_asb.h
                 src/ctrlm.h
                 src/ctrlm_rcu.h
                 src/ctrlm_recovery.h
                 src/ctrlm_utils.h
                 src/ctrlm_validation.h
                 src/ctrlm_network.h
                 src/ctrlm_controller.h
                 src/attributes/ctrlm_attr.h
                 src/attributes/ctrlm_attr_general.h
                 src/attributes/ctrlm_attr_voice.h
                 src/attributes/ctrlm_version.h
                 src/database/ctrlm_database.h
                 src/database/ctrlm_db_attr.h
                 src/rfc/ctrlm_rfc.h
                 src/rfc/ctrlm_rfc_attr.h
                 src/config/ctrlm_config_attr.h
                 src/voice/ctrlm_voice_obj.h
                 src/voice/ctrlm_voice_types.h
                 src/voice/ipc/ctrlm_voice_ipc.h
                 src/voice/telemetry/ctrlm_voice_telemetry_events.h
                 src/json_config.h
                 src/ctrlm_tr181.h
                 src/telemetry/ctrlm_telemetry.h
                 src/telemetry/ctrlm_telemetry_event.h
                 src/telemetry/ctrlm_telemetry_markers.h
                 src/thunder/ctrlm_thunder_plugin.h
                 src/thunder/ctrlm_thunder_controller.h
                 src/thunder/helpers/ctrlm_thunder_helpers.h
                 src/ipc/ctrlm_rcp_ipc_iarm_thunder.h
                 src/ipc/ctrlm_rcp_ipc_event.h
                 src/server/ctrlm_server_app.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR} COMPONENT headers)

   # BLE services
   install(FILES src/ble/hal/blercu/blercuerror.h
                 src/ble/hal/blercu/blegattservice.h
                 src/ble/hal/blercu/blegattcharacteristic.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/blercu COMPONENT headers)

   install(FILES src/ble/hal/blercu/bleservices/blercuaudioservice.h
                 src/ble/hal/blercu/bleservices/blercuinfraredservice.h
                 src/ble/hal/blercu/bleservices/blercuupgradeservice.h
                 src/ble/hal/blercu/bleservices/blercudeviceinfoservice.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/blercu/bleservices COMPONENT headers)

   install(FILES src/ble/hal/blercu/bleservices/gatt/gatt_audiopipe.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_audioservice.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_infraredservice.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_infraredsignal.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_upgradeservice.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_deviceinfoservice.h
                 src/ble/hal/blercu/bleservices/gatt/gatt_external_services.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/blercu/bleservices/gatt COMPONENT headers)

   install(FILES src/ble/hal/ctrlm_log_ble.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/ble/hal COMPONENT headers)

   install(FILES src/ble/hal/dbus/dbusvariant.h
                 src/ble/hal/dbus/dbusabstractinterface.h
                 src/ble/hal/dbus/gdbusabstractinterface.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/ble/hal/dbus COMPONENT headers)

   install(FILES src/ble/hal/configsettings/configsettings.h
                 src/ble/hal/configsettings/configmodelsettings.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/configsettings COMPONENT headers)

   install(FILES src/ble/hal/utils/slot.h
                 src/ble/hal/utils/bleuuid.h
                 src/ble/hal/utils/bleaddress.h
                 src/ble/hal/utils/audioformat.h
                 src/ble/hal/utils/pendingreply.h
                 src/ble/hal/utils/filedescriptor.h
                 src/ble/hal/utils/statemachine.h
                 src/ble/hal/utils/futureaggregator.h
                 src/ble/hal/utils/fwimagefile.h
           DESTINATION ${CTRLM_PRIVATE_INCLUDEDIR}/utils COMPONENT headers)

   # Installs only the headers component, without building controlMgr
   add_custom_target(install_headers
      COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component headers
      VERBATIM)
endif()

# GENERATED FILES
add_custom_command( OUTPUT ctrlm_version_build.h
   COMMAND echo -n "#define CTRLM_MAIN_BRANCH    \"" >> ctrlm_version_build.h
   COMMAND sh -c "src=\"${CMAKE_CURRENT_SOURCE_DIR}\"; ref=$(git -C \"$src\" describe --tags --exact-match 2>/dev/null || git -C \"$src\" branch -a --format='%(refname:short)' --contains HEAD 2>/dev/null | sed -n '2{p;q}'); echo -n \"$ref\" >> ctrlm_version_build.h"
   COMMAND echo "\"" >> ctrlm_version_build.h
   COMMAND echo -n "#define CTRLM_MAIN_COMMIT_ID " >> ctrlm_version_build.h
   COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} log --format=\"%H\" -n 1 >> ctrlm_version_build.h
   COMMAND echo -n "#define CTRLM_MAIN_VERSION   \"${CMAKE_PROJECT_VERSION}" >> ctrlm_version_build.h
   COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} diff --quiet || echo -n "++" >> ctrlm_version_build.h
   COMMAND echo "\"" >> ctrlm_version_build.h
   VERBATIM
)

add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/ctrlm_config.json
   COMMAND python3 ${CTRLM_UTILS_JSON_COMBINE} -i ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.json -a ${CTRLM_CONFIG_JSON_CPC} -s ${CTRLM_CONFIG_JSON_CPC_SUB} -a ${CTRLM_CONFIG_JSON_CPC_ADD} -s ${CTRLM_CONFIG_JSON_MAIN_SUB} -a ${CTRLM_CONFIG_JSON_MAIN_ADD} -o ${CMAKE_BINARY_DIR}/ctrlm_config.json
   DEPENDS src/ctrlm_config_default.json
   VERBATIM
   )

add_custom_command(
   OUTPUT ctrlm_config_default.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c
   COMMAND python3 ${CTRLM_UTILS_JSON_TO_HEADER} -i ${CMAKE_BINARY_DIR}/ctrlm_config.json -o ctrlm_config_default.h -c ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlm_config_default.c -v "ctrlm_global,network_rf4ce,network_ip,network_ble,ir,voice,device_update" -d "network_ble"
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json
   VERBATIM
   )

add_custom_command(
   OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c
   COMMAND gperf --output-file=${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.c ${CMAKE_CURRENT_SOURCE_DIR}/src/voice/ipc/ctrlm_voice_ipc_request_type.hash
   VERBATIM
   )

add_custom_target( ctrlm_config
   DEPENDS ${CMAKE_BINARY_DIR}/ctrlm_config.json
   )
