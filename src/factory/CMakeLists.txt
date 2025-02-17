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

option(FACTORY_AUDIO_PLAYBACK "AUDIO PLAYBACK" OFF)

add_executable(controlFactory ctrlmf_main.c)
add_library(ctrlm-fta SHARED)
set_target_properties( ctrlm-fta PROPERTIES
   PREFIX    "lib"
   SOVERSION "0.0"
)

include_directories(
   .
   iarm
   thunder
)

target_sources(ctrlm-fta PRIVATE
   ctrlmf_version.c
   ctrlmf_reset.cpp
   ctrlmf_systemd.c
   ctrlmf_utils.c
   iarm/ctrlmf_iarm_client.cpp
   iarm/ctrlmf_iarm_control_manager.cpp
)

target_link_libraries(ctrlm-fta c rdkversion dbus-1 glib-2.0 IARMBus xr-voice-sdk pthread nopoll secure_wrapper)
target_link_libraries(controlFactory c ctrlm-fta secure_wrapper)

if(CUSTOM_AUDIO_ANALYSIS_LIB)
   add_compile_definitions(PRIVATE CTRLMF_CUSTOM_AUDIO_ANALYSIS)
   target_link_libraries(controlFactory ${CUSTOM_AUDIO_ANALYSIS_LIB})
endif()

if(CUSTOM_AUTH_LIB)
   add_compile_definitions(PRIVATE CTRLMF_WSS_ENABLED)
   target_link_libraries(controlFactory ${CUSTOM_AUTH_LIB})
endif()

if(LOCAL_MIC)
   add_compile_definitions(PRIVATE CTRLMF_LOCAL_MIC)
   if(MIC_TAP)
      add_compile_definitions(PRIVATE CTRLMF_LOCAL_MIC_TAP)
   endif()
endif()

if(THUNDER)
   add_compile_definitions(PRIVATE CTRLMF_THUNDER)
   target_sources(ctrlm-fta PRIVATE
      thunder/ctrlmf_thunder_controller.cpp
      thunder/ctrlmf_thunder_plugin.cpp
   )

   target_link_libraries(controlFactory WPEFrameworkCore WPEFrameworkPlugins)
   if(WPE_FRAMEWORK_COM_SOCKET)
      target_link_libraries(controlFactory WPEFrameworkCOM WPEFrameworkWebSocket)
   endif()
   if(WPE_FRAMEWORK_PROTO_TRACING)
      target_link_libraries(controlFactory WPEFrameworkProtocols WPEFrameworkTracing)
   endif()
endif()

if(THUNDER_SECURITY)
   add_compile_definitions(PRIVATE THUNDER_SECURITY)
   target_link_libraries(controlFactory WPEFrameworkSecurityUtil secure_wrapper)
endif()

if(FACTORY_AUDIO_PLAYBACK)
   add_compile_definitions(PRIVATE CTRLMF_AUDIO_PLAYBACK)
   include_directories( ${CMAKE_SYSROOT}/usr/include/rdk/ds
                   ${CMAKE_SYSROOT}/usr/include/rdk/ds-rpc
                   ${CMAKE_SYSROOT}/usr/include/rdk/ds-hal
                  )
   target_sources(ctrlm-fta PRIVATE ctrlmf_audio_playback.cpp)
   if(THUNDER)
      target_sources(ctrlm-fta PRIVATE thunder/ctrlmf_thunder_plugin_system_audio_player.cpp)
   endif()

   install(FILES ${CMAKE_SOURCE_DIR}/../tone_1khz.wav DESTINATION share )
endif()

if(FACTORY_AUDIO_CONTROL)
   add_compile_definitions(PRIVATE CTRLMF_AUDIO_CONTROL)
   target_sources(ctrlm-fta PRIVATE
      ctrlmf_audio_control.cpp
   )
endif()

if(LOCAL_MIC)
   target_sources(ctrlm-fta PRIVATE
      ctrlmf_audio_capture.cpp
      ctrlmf_mic_test.cpp
      ctrlmf_ws.c
   )
endif()

install(TARGETS controlFactory DESTINATION bin)
install(TARGETS ctrlm-fta LIBRARY DESTINATION lib)
