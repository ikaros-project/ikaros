# www.ikaros-project.org
#
# Find the FFMPEG includes and libraries
#
# This module defines
# ACE_INCLUDE_DIR
# ACE_LIBRARIES
# ACE_FOUND


# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find header files
find_path(ACE_INCLUDE_DIR
	NAMES
      ace/ACE.h
    PATHS
		/usr/include 
		/usr/local/include
 	)
 	
# Finding libs  
find_library(ACE_LIBRARIES
    NAMES
    	 ace
    PATHS
		/usr/lib 
		/usr/local/lib
    )
		
if (ACE_INCLUDE_DIR AND ACE_LIBRARIES)
    message(STATUS "Found ACE. Includes: ${ACE_INCLUDE_DIR} Libraries: ${ACE_LIBRARIES}")
	set(ACE_FOUND "YES" )
endif ()


