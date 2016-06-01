# Ikaros-project.org
#
# Find the Phidgets includes and library
#
# This module defines
# PHIDGETS_INCLUDE_DIR
# PHIDGETS_LIBRARIES
# PHIDGETS_FOUND


# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

find_library(PHIDGETS_LIBRARY Phidget21)
	if (PHIDGETS_INCLUDE_DIR1)
		set(PHIDGETS_INCLUDE_DIR ${PHIDGETS_INCLUDE_DIR1} )
	endif ()
	if (PHIDGETS_LIBRARY) 
		set(PHIDGETS_LIBRARIES ${PHIDGETS_LIBRARY} )
	endif ()
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_path(PHIDGETS_INCLUDE_DIR
   		NAMES
   	 		phidget21.h
   	 	PATHS
			/usr/local/include
			/usr/include
  	)
  	
	find_library(PHIDGETS_LIBRARIES
    	NAMES
    		phidget21
    	PATHS
    		/usr/local/lib
    		/usr/lib
	)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


if (PHIDGETS_LIBRARIES)
	message(STATUS "Found Phidgets. Includes: ${PHIDGETS_INCLUDE_DIR} Libraries: ${PHIDGETS_LIBRARIES}")
	set(PHIDGETS_FOUND "YES" )
endif (PHIDGETS_LIBRARIES)