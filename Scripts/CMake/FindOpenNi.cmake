# Ikaros-project.org
#
# Find the OpenNI includes and library
#
# This module defines
# OPENNI_INCLUDE_DIR
# OPENNI_LIBRARIES
# OPENNI_FOUND


# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

find_path(OPENNI_INCLUDE_DIR
    NAMES
        XnCppWrapper.h
    PATHS
		/usr/local/include/ni/
)
  	
find_library(OPENNI_LIBRARIES
    NAMES
    	OpenNI
    PATHS
		/usr/local/lib/
)
		
if (OPENNI_INCLUDE_DIRS AND OPENNI_LIBRARIES)
    message(STATUS "Found OpenNI. Includes: ${OPENNI_INCLUDE_DIR} Libraries: ${OPENNI_LIBRARIES}")
    set(OPENNI_FOUND TRUE)
endif ()

