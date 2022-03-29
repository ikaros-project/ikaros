# www.ikaros-project.org
#
#
# This module defines:
# DLIB_INCLUDE_DIRS
# DLIB_LIBRARIES
# DLIB_LIB_FOUND


# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find one header file and lib
#############################################

# Find header files
# Can not figure out how to use the dlib::dlib alias.

find_path(DLIB_INCLUDE_DIRS
  NAMES
    dlib/image_processing.h
  PATHS
    /usr/local/include/     # Homebrew
  )

  find_library(DLIB_LIBRARIES
  NAMES
    dlib
  PATHS
    /usr/local/lib/          # Homebrew
    /usr/lib
  )
if (DLIB_INCLUDE_DIRS AND DLIB_LIBRARIES)
  message(STATUS "Found DLIB: Includes: ${DLIB_INCLUDE_DIRS} Libraries: ${DLIB_LIBRARIES}")
  set(DLIB_LIB_FOUND "YES" )
endif ()
#############################################
