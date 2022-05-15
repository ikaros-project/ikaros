# www.ikaros-project.org
#
# Example how to write a cmake module to use a external library if the library is not yet included in the official cmake script.
# To check included modules use "cmake --help-module-list"
#
#
# This module defines:
# DYNAMIXEL_INCLUDE_DIR
# DYNAMIXEL_LIBRARIES
# DYNAMIXEL_LIB_FOUND


# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
# Find one header file and lib
#############################################

# Find header files
find_path(DYNAMIXEL_INCLUDE_DIRS
  NAMES
    dynamixel_sdk.h
  PATHS
    /usr/local/include/dynamixel_sdk/
    /usr/include
  )

# Find lib file
  find_library(DYNAMIXEL_LIBRARIES
  NAMES
    libdxl_mac_cpp.dylib
  PATHS
    /usr/local/lib
    /usr/lib
  )
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
# Find one header file and lib 
#############################################
# Tested in a rasberry

# Find header files
find_path(DYNAMIXEL_INCLUDE_DIRS
  NAMES
    dynamixel_sdk.h
  PATHS
    /usr/local/include/dynamixel_sdk/
    /usr/include
  )

# Find lib file
  find_library(DYNAMIXEL_LIBRARIES
  NAMES
    libdxl_sbc_cpp.so
  PATHS
    /usr/local/lib
    /usr/lib
  )
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (DYNAMIXEL_INCLUDE_DIRS AND DYNAMIXEL_LIBRARIES)
  message(STATUS "Found Dynamixel SDK: Includes: ${DYNAMIXEL_INCLUDE_DIRS} Libraries: ${DYNAMIXEL_LIBRARIES}")
  set(DYNAMIXEL_LIB_FOUND "YES" )
endif ()
#############################################

