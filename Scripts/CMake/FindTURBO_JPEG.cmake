# Find turbo jpeg for OSX.
# 
# This module defines
# TURBO_JPEG_INCLUDE_DIR
# TURBO_JPEG_LIBRARIES
# TURBO_JPEG_FOUND


	find_path(TURBO_JPEG_INCLUDE_DIR
    NAMES
      jconfig.h jerror.h jmorecfg.h jpeglib.h turbojpeg.h
    PATHS
		/usr/local/opt/jpeg-turbo/include/
  	)
  
	find_library(TURBO_JPEG_LIBRARIES
    NAMES
    	jpeg
    PATHS
    	/usr/local/opt/jpeg-turbo/lib
	)
  
	if (TURBO_JPEG_LIBRARIES)
    	set(TURBO_JPEG_FOUND TRUE)
  	endif (TURBO_JPEG_LIBRARIES)
 
 
  	set(TURBO_JPEG_INCLUDE_DIRS
    	${TURBO_JPEG_INCLUDE_DIR}
  	)
  
    if (TURBO_JPEG_FOUND)
    	set(TURBO_JPEG_LIBRARIES
      	${TURBO_JPEG_LIBRARIES}
    	)
    endif(TURBO_JPEG_FOUND)
    
    # Find_library will find both turbo jpeg and ordinary jpeg
    if (NOT TURBO_JPEG_FOUND)
    	message(STATUS "Could not find TurboJPEG")
  	endif ()