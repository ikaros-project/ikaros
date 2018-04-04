# www.ikaros-project.org
#
# Example how to write a cmake module to use a external library if the library is not yet included in the official cmake script.
# To check included modules use "cmake --help-module-list"
#
#
# This module defines:
# YOURLIB_INCLUDE_DIR
# YOURLIB_LIBRARIES
# YOUR_LIB_FOUND


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
find_path(YOURLIB_INCLUDE_DIRS
  NAMES
    myHeaderFile.h
  PATHS
    /usr/local/include
    /usr/include
  )

# Find lib file
  find_library(YOURLIB_LIBRARIES
  NAMES
    mylib
  PATHS
    /usr/local/lib
    /usr/lib
  )

if (YOURLIB_INCLUDE_DIRS AND YOURLIB_LIBRARIES)
  message(STATUS "Found YOURLIB: Includes: ${YOURLIB_INCLUDE_DIRS} Libraries: ${YOURLIB_LIBRARIES}")
  set(YOUR_LIB_FOUND "YES" )
endif ()
#############################################




# Find multiple header files and libs
#############################################

# Find header files
# same path
find_path(YOURLIB_INCLUDE_DIRS
  NAMES
    myHeaderFile.h myHeaderFile2.h myHeaderFile3.h
  PATHS
    /usr/local/include
    /usr/include
  )

# Different paths
find_path(YOURLIB_INCLUDE_DIR1
  NAMES
    myHeaderFile1.h
  PATHS
    /usr/local/include
    /usr/include
  )

find_path(YOURLIB_INCLUDE_DIR2
  NAMES
    myHeaderFile2.h
  PATHS
    /usr/local/include
    /usr/include
  )

# Add this path to YOURLIB_INCLUDE_DIR
  if (YOURLIB_LIBRARY AND YOURLIB_LIBRARY2)
    set(YOURLIB_INCLUDE_DIR
    ${YOURLIB_INCLUDE_DIR1}
    ${YOURLIB_INCLUDE_DIR2})
  endif ()

# Find lib files
  find_library(YOURLIB_LIBRARY1
    NAMES
      mylib1
    PATHS
      /usr/local/lib
    /usr/lib
  )

  find_library(YOURLIB_LIBRARY2
  NAMES
    mylib2
  PATHS
    /usr/local/lib
    /usr/lib
  )

  # Add this path to YOURLIB_LIBRARIES
  if (YOURLIB_LIBRARY1 AND YOURLIB_LIBRARY2)
    set(YOURLIB_LIBRARIES
    ${YOURLIB_LIBRARY1}
    ${YOURLIB_LIBRARY2})
  endif ()

if (YOURLIB_INCLUDE_DIRS AND YOURLIB_LIBRARY1 AND YOURLIB_LIBRARY2)
  message(STATUS "Found YOURLIB: Includes: ${YOURLIB_INCLUDE_DIRS} Libraries: ${YOURLIB_LIBRARIES}")
  set(YOUR_LIB_FOUND "YES" )
endif ()
#############################################
