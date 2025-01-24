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

#  Find the gobject library
#
#  GOBJECT_FOUND - System has library
#  GOBJECT_INCLUDE_DIRS - The include directory(ies)
#  GOBJECT_LIBRARIES - The libraries needed
#=============================================================================

find_path( GOBJECT_INCLUDE_DIR glib-object.h PATH_SUFFIXES glib-2.0 )
find_library( GOBJECT_LIBRARY NAMES libgobject-2.0.so gobject-2.0 )

message( "GOBJECT_INCLUDE_DIR include dir = ${GOBJECT_INCLUDE_DIR}" )
message( "GOBJECT_LIBRARY lib = ${GOBJECT_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set GOBJECT_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( gobject
        DEFAULT_MSG GOBJECT_LIBRARY GOBJECT_INCLUDE_DIR )

mark_as_advanced( GOBJECT_INCLUDE_DIR GOBJECT_LIBRARY )


if( GOBJECT_FOUND )
    set( GOBJECT_INCLUDE_DIRS ${GOBJECT_INCLUDE_DIR} )
    set( GOBJECT_LIBRARIES ${GOBJECT_LIBRARY} )
endif()

if( GOBJECT_FOUND AND NOT TARGET gobject::libgobject )
    add_library( gobject::libgobject SHARED IMPORTED )
    set_target_properties( gobject::libgobject PROPERTIES
            IMPORTED_LOCATION "${GOBJECT_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GOBJECT_INCLUDE_DIR}" )
endif()
