######################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2017-2020 Sky UK
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
######################################################################

#  Find the ctrlm library
#
#  CTRLM_FOUND - System has library
#  CTRLM_INCLUDE_DIRS - The include directory(ies)
#  CTRLM_LIBRARIES - The libraries needed
#=============================================================================

find_path( CTRLM_INCLUDE_DIR ctrlm_log.h PATH_SUFFIXES ctrlm_private)
find_path( RDKX_LOGGER_INCLUDE_DIR rdkx_logger.h)
find_library( RDKX_LOGGER_LIBRARY NAMES librdkx-logger.so )

message( "CTRLM_INCLUDE_DIR include dir = ${CTRLM_INCLUDE_DIR}" )
message( "RDKX_LOGGER_INCLUDE_DIR include dir = ${RDKX_LOGGER_INCLUDE_DIR}" )
message( "RDKX_LOGGER_LIBRARY lib = ${RDKX_LOGGER_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set CTRLM_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( CTRLM
        DEFAULT_MSG 
        CTRLM_INCLUDE_DIR 
        RDKX_LOGGER_INCLUDE_DIR
        RDKX_LOGGER_LIBRARY )

mark_as_advanced( CTRLM_INCLUDE_DIR )

if( CTRLM_FOUND )
    set( CTRLM_INCLUDE_DIRS ${CTRLM_INCLUDE_DIR} ${RDKX_LOGGER_INCLUDE_DIR} )
    set( CTRLM_LIBRARIES ${RDKX_LOGGER_LIBRARY} )
endif()

if( CTRLM_FOUND AND NOT TARGET libctrlm )
    add_library( libctrlm SHARED IMPORTED )
    set_target_properties( libctrlm PROPERTIES
            IMPORTED_LOCATION "${RDKX_LOGGER_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CTRLM_INCLUDE_DIR}" )
endif()


