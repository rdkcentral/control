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

#  Find the jansson library
#
#  JANSSON_FOUND - System has library
#  JANSSON_INCLUDE_DIRS - The include directory(ies)
#  JANSSON_LIBRARIES - The libraries needed
#=============================================================================

find_path( JANSSON_INCLUDE_DIR jansson.h )
find_library( JANSSON_LIBRARY NAMES libjansson.so jansson )

message( "JANSSON_INCLUDE_DIR include dir = ${JANSSON_INCLUDE_DIR}" )
message( "JANSSON_LIBRARY lib = ${JANSSON_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set JANSSON_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( jansson
        DEFAULT_MSG JANSSON_LIBRARY JANSSON_INCLUDE_DIR )

mark_as_advanced( JANSSON_INCLUDE_DIR JANSSON_LIBRARY )


if( JANSSON_FOUND )
    set( JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR} )
    set( JANSSON_LIBRARIES ${JANSSON_LIBRARY} )
endif()

if( JANSSON_FOUND AND NOT TARGET jansson::libjansson )
    add_library( jansson::libjansson SHARED IMPORTED )
    set_target_properties( jansson::libjansson PROPERTIES
            IMPORTED_LOCATION "${JANSSON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JANSSON_INCLUDE_DIR}" )
endif()
