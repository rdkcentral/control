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

#  Find the udev library
#
#  UDEV_FOUND - System has libudev
#  UDEV_INCLUDE_DIRS - The udev include directory(ies)
#  UDEV_LIBRARIES - The libraries needed to use udev
#=============================================================================

find_path( UDEV_INCLUDE_DIR libudev.h )
find_library( UDEV_LIBRARY NAMES libudev.so.1 libudev.so udev )

# message( "UDEV_INCLUDE_DIR include dir = ${UDEV_INCLUDE_DIR}" )
# message( "UDEV_LIBRARY lib = ${UDEV_LIBRARY}" )


# handle the QUIETLY and REQUIRED arguments and set UDEV_FOUND to TRUE if
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )

find_package_handle_standard_args( UDEV
        DEFAULT_MSG UDEV_LIBRARY UDEV_INCLUDE_DIR )

mark_as_advanced( UDEV_INCLUDE_DIR UDEV_LIBRARY )


if( UDEV_FOUND )
    set( UDEV_INCLUDE_DIRS ${UDEV_INCLUDE_DIR} )
    set( UDEV_LIBRARIES ${UDEV_LIBRARY} )
endif()

if( UDEV_FOUND AND NOT TARGET UDEV::libudev )
    add_library( UDEV::libudev SHARED IMPORTED )
    set_target_properties( UDEV::libudev PROPERTIES
            IMPORTED_LOCATION "${UDEV_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${UDEV_INCLUDE_DIR}" )
endif()
