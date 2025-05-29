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

# This is required to allow an executable to export symbols to be used by loadable modules (e.g. plugins using dlopen)
set(CMAKE_ENABLE_EXPORTS ON)

add_executable(controlServer ctrlms_main.c)

include_directories(
   .
   ${CMAKE_SYSROOT}/usr/include/safeclib
   ${CMAKE_SYSROOT}/usr/include/libsafec
   ${CMAKE_SYSROOT}/usr/include/nopoll
   ${CMAKE_SYSROOT}/usr/include
)

target_sources(controlServer PRIVATE
   ctrlms_version.c
   ctrlms_utils.c
   ctrlms_ws.cpp
)

target_compile_options(controlServer PUBLIC -fPIC -rdynamic -Wall -Werror)

target_link_libraries(controlServer c rdkversion pthread nopoll jansson xr-voice-sdk secure_wrapper systemd ${CMAKE_DL_LIBS})

#if(CUSTOM_AUTH_LIB)
#   add_compile_definitions(PRIVATE CTRLMS_WSS_ENABLED)
#   target_link_libraries(controlServer ${CUSTOM_AUTH_LIB})
#endif()

#if(EXISTS ${CMAKE_SYSROOT}/usr/lib/libctrlm-hal-certificate.so)
#   target_link_libraries(controlServer ctrlm-hal-certificate)
#else()
#   if(AUTH_ENABLED)
#      message(WARNING "ctrlm-hal-certificate library is not provided, disabling authentication")
#      unset(AUTH_ENABLED)
#   endif()
#endif()

if(USE_SAFEC)
   find_package(PkgConfig)
   pkg_check_modules(SAFEC REQUIRED libsafec)
   if(SAFEC_FOUND)
      target_link_libraries(controlServer ${SAFEC_LIBRARIES})
   endif()
else()
   add_compile_definitions(PUBLIC SAFEC_DUMMY_API)
endif()

install(TARGETS controlServer DESTINATION bin)
