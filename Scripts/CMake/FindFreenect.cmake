# www.ikaros-project.org
#
# Find the libfreenect includes and libraries
#
# This module defines
# FREENECT_INCLUDE_DIR
# FREENECT_LIBRARIES
# FREENECT_FOUND

# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find header files
find_path(FREENECT_INCLUDE_DIR
	NAMES
      libfreenect.h libfreenect_sync.h
    PATHS
        /usr/include/libfreenect
        /usr/local/include/libfreenect
        /opt/local/include/libfreenect
 	)
	
# Finding libs  
find_library(FREENECT_LIBRARY1
    NAMES
    	 freenect
    PATHS
 		/usr/include/libfreenect
        /usr/local/include/libfreenect
        /opt/local/include/libfreenect
    )

# Finding libs  
find_library(FREENECT_LIBRARY2
    NAMES
    	freenect_sync
    PATHS
 		/usr/include/libfreenect
        /usr/local/include/libfreenect
        /opt/local/include/libfreenect
    )
    
if (FREENECT_LIBRARY1 AND FREENECT_LIBRARY2)
	set(FREENECT_LIBRARIES 
	${FREENECT_LIBRARY1}
	${FREENECT_LIBRARY2}
	)
endif ()
		
if (FREENECT_INCLUDE_DIR AND FREENECT_LIBRARIES)
    message(STATUS "Found Freenect. Includes: ${FREENECT_INCLUDE_DIR} Libraries: ${FREENECT_LIBRARIES}")
	set(FREENECT_FOUND "YES" )
endif ()

