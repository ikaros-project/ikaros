# www.ikaros-project.org
#
# Find the JPEG includes and libraries
# Used by the ikaros kernel
#
# This module defines
# TURBO_JPEG_INCLUDE_DIR
# TURBO_JPEG_LIBRARIES
# TURBO_JPEG_FOUND

# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find header files
find_path(TURBO_JPEG_INCLUDE_DIR
	NAMES
      jconfig.h jerror.h jmorecfg.h jpeglib.h turbojpeg.h
    PATHS
		/usr/local/opt/jpeg-turbo/include/
  	)
  	
# Finding libs  
find_library(TURBO_JPEG_LIBRARIES
    NAMES
    	jpeg
    PATHS
    	/usr/local/opt/jpeg-turbo/lib
	)

  		
if (TURBO_JPEG_INCLUDE_DIR AND TURBO_JPEG_LIBRARIES)
    #message("Found TURBO JPEG. Includes: ${TURBO_JPEG_INCLUDE_DIR} Libraries: ${TURBO_JPEG_LIBRARIES}")
	set(TURBO_JPEG_FOUND "YES" )
endif ()
