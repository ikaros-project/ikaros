#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libfreenect) Copyright (c) 2011 Yannis Gravezas <wizgrav@infrael.com>
#  (Changes for SY27 project) Copyright (c) 2011 Samuel Gosselin <samuel.gosselin@etu.utc.fr>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Modification for SY27:
#  - Add debug messages.
#  - Add support for LIBFREENECT_DIR
#  - Add support for components (sync)
#  - Clean the script

# Allow user to use a specific libfreenect directory.
if(LIBFREENECT_DIR AND NOT libfreenect_FIND_QUIETLY)
  message(STATUS "The libfreenect directory is: ${LIBFREENECT_DIR}")
endif()

# Check if libfreenect has already been loaded.
if(LIBFREENECT_LIBRARIES AND LIBFREENECT_INCLUDE_DIRS)
  # Thus, libusb is arleady in cache.
  set(LIBUSB_FOUND TRUE)
else()
  # First list of element to search.
  set(LIB_TO_FIND "freenect")
  set(INC_TO_FIND "libfreenect.h")

  # Load the additionnal components.
  foreach(comp ${libfreenect_FIND_COMPONENTS})
    # Currently, only sync is supported.
    if (${comp} MATCHES "sync")
      list(APPEND LIB_TO_FIND "freenect_sync")
      list(APPEND INC_TO_FIND "libfreenect_sync.h")
    else()
      message(FATAL_ERROR "Component not supported: ${comp}")
    endif()
  endforeach()

  # Initialize the outputs.
  set(LIBFREENECT_INCLUDE_DIRS "")
  set(LIBFREENECT_LIBRARIES "")

  # Look for the include files.
  foreach(inc ${INC_TO_FIND})
    find_path(INC_DIR_${inc}
      NAMES
        ${inc}
      PATHS
        /usr/include/libfreenect
        /usr/local/include/libfreenect
        /opt/local/include/libfreenect
        /sw/include/libfreenect
        ${LIBFREENECT_DIR}
        ${LIBFREENECT_DIR}/include
    PATH_SUFFIXES
      libfreenect
    )

    # Add to the list.
    list(APPEND LIBFREENECT_INCLUDE_DIRS "${INC_DIR_${inc}}")
  endforeach()

  # Look for the library files.
  foreach(lib ${LIB_TO_FIND})
    find_library(LIB_DIR_${lib}
      NAMES
        ${lib}
      PATHS
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ${LIBFREENECT_DIR}
        ${LIBFREENECT_DIR}/lib
    )

    # Add to the list.
    list(APPEND LIBFREENECT_LIBRARIES "${LIB_DIR_${lib}}")
  endforeach()

  # Remove the duplicates in the header.
  list(REMOVE_DUPLICATES LIBFREENECT_INCLUDE_DIRS)

  # Check if the library has been found.
  if (LIBFREENECT_INCLUDE_DIRS AND LIBFREENECT_LIBRARIES)
     set(LIBFREENECT_FOUND TRUE)
  endif ()

  # Logging.
  if(LIBFREENECT_FOUND)
    if(NOT libfreenect_FIND_QUIETLY)
      message(STATUS "Found libfreenect:")
	    message(STATUS " - Includes: ${LIBFREENECT_INCLUDE_DIRS}")
	    message(STATUS " - Libraries: ${LIBFREENECT_LIBRARIES}")
    endif()
  else()
    if (libfreenect_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libfreenect")
      message(FATAL_ERROR "Have you set LIBFREENECT_DIR ?")
    endif()
  endif()

  # End.
  mark_as_advanced(LIBFREENECT_INCLUDE_DIRS LIBFREENECT_LIBRARIES)
endif()