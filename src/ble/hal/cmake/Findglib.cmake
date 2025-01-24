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

#  Find the glib library
#
#  GLIB_FOUND - System has library
#  GLIB_INCLUDE_DIRS - The include directory(ies)
#  GLIB_LIBRARIES - The libraries needed
#=============================================================================

find_path( GLIB_INCLUDE_DIR glib.h PATH_SUFFIXES glib-2.0 )
find_path( GLIB_CONFIG_INCLUDE_DIR glibconfig.h PATH_SUFFIXES lib/glib-2.0/include)
find_library( GLIB_LIBRARY NAMES libglib-2.0.so glib-2.0 )

message( "GLIB_INCLUDE_DIR include dir = ${GLIB_INCLUDE_DIR}" )
message( "GLIB_CONFIG_INCLUDE_DIR include dir = ${GLIB_CONFIG_INCLUDE_DIR}" )
message( "GLIB_LIBRARY lib = ${GLIB_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set GLIB_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( glib
        DEFAULT_MSG GLIB_LIBRARY GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR )

mark_as_advanced( GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR GLIB_LIBRARY )


if( GLIB_FOUND )
    set( GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
    set( GLIB_LIBRARIES ${GLIB_LIBRARY} )
endif()

if( GLIB_FOUND AND NOT TARGET glib::libglib )
    add_library( glib::libglib SHARED IMPORTED )
    set_target_properties( glib::libglib PROPERTIES
            IMPORTED_LOCATION "${GLIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLIB_INCLUDE_DIR}" )
endif()
