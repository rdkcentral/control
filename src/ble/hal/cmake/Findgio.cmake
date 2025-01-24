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

#  Find the gio library
#
#  GIO_FOUND - System has library
#  GIO_INCLUDE_DIRS - The include directory(ies)
#  GIO_LIBRARIES - The libraries needed
#=============================================================================

find_path( GIO_INCLUDE_DIR gio/gio.h PATH_SUFFIXES glib-2.0 )
find_path( GIO_UNIX_INCLUDE_DIR gio/gunixfdlist.h PATH_SUFFIXES gio-unix-2.0 )
find_library( GIO_LIBRARY NAMES libgio-2.0.so gio-2.0 )

message( "GIO_INCLUDE_DIR include dir = ${GIO_INCLUDE_DIR}" )
message( "GIO_UNIX_INCLUDE_DIR include dir = ${GIO_UNIX_INCLUDE_DIR}" )
message( "GIO_LIBRARY lib = ${GIO_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set GIO_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( gio
        DEFAULT_MSG GIO_LIBRARY GIO_INCLUDE_DIR GIO_UNIX_INCLUDE_DIR )

mark_as_advanced( GIO_INCLUDE_DIR GIO_UNIX_INCLUDE_DIR GIO_LIBRARY )


if( GIO_FOUND )
    set( GIO_INCLUDE_DIRS ${GIO_INCLUDE_DIR} ${GIO_UNIX_INCLUDE_DIR} )
    set( GIO_LIBRARIES ${GIO_LIBRARY} )
endif()

if( GIO_FOUND AND NOT TARGET gio::libgio )
    add_library( gio::libgio SHARED IMPORTED )
    set_target_properties( gio::libgio PROPERTIES
            IMPORTED_LOCATION "${GIO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GIO_INCLUDE_DIR} ${GIO_UNIX_INCLUDE_DIR}" )
endif()
