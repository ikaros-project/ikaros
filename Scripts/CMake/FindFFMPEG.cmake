# www.ikaros-project.org
#
# Find the FFMPEG includes and libraries
#
# This module defines
# FFMPEG_INCLUDE_DIR
# FFMPEG_LIBRARIES
# FFMPEG_FOUND


# Specific parameters for different OS
#if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

#if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find header files
find_path(FFMPEG_INCLUDE_DIR1
	NAMES
		avformat.h
    PATHS
  		/usr/local/include/ffmpeg
  		/usr/include/ffmpeg
  		/usr/include/libavformat
  		/usr/include/ffmpeg/libavformat
  		/usr/local/include/libavformat
  		/usr/local/include/x86_64-linux-gnu/libavformat

  	)

find_path(FFMPEG_INCLUDE_DIR2
	NAMES
		avutil.h
    PATHS
  		/usr/local/include/ffmpeg
 	 	/usr/include/ffmpeg
  		/usr/include/libavutil
  		/usr/include/ffmpeg/libavutil
  		/usr/local/include/libavutil
  		/usr/local/include/x86_64-linux-gnu/libavutil

  	)
  	
find_path(FFMPEG_INCLUDE_DIR3
	NAMES
		avcodec.h
    PATHS
  		/usr/local/include/ffmpeg
 		/usr/include/ffmpeg
  		/usr/include/libavcodec
  		/usr/include/ffmpeg/libavcodec
  		/usr/local/include/libavcodec
  		/usr/local/include/x86_64-linux-gnu/libavcodec

  	)

find_path(FFMPEG_INCLUDE_DIR4
	NAMES
		swscale.h
    PATHS
  		/usr/local/include/ffmpeg
  		/usr/include/ffmpeg
  		/usr/include/libswscale
  		/usr/include/ffmpeg/libswscale
  		/usr/local/include/libswscale
  		/usr/local/include/x86_64-linux-gnu/libswscale

  	)

find_path(FFMPEG_INCLUDE_DIR5
	NAMES
		avdevice.h
    PATHS
 	 	/usr/local/include/ffmpeg
  		/usr/include/ffmpeg
  		/usr/include/libavdevice
  		/usr/include/ffmpeg/libavdevice
  		/usr/local/include/libavdevice
  		/usr/local/include/x86_64-linux-gnu/libavdevice

  	)
  	
  	if (FFMPEG_INCLUDE_DIR1 AND FFMPEG_INCLUDE_DIR2 AND FFMPEG_INCLUDE_DIR3 AND FFMPEG_INCLUDE_DIR4 AND FFMPEG_INCLUDE_DIR5)
		set(FFMPEG_INCLUDE_DIR 
		${FFMPEG_INCLUDE_DIR1} 
		${FFMPEG_INCLUDE_DIR2} 
		${FFMPEG_INCLUDE_DIR3} 
		${FFMPEG_INCLUDE_DIR4} 
		${FFMPEG_INCLUDE_DIR5})
	endif ()

# Finding libs

find_library(FFMPEG_LIBRARY1
    NAMES
    	avformat
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)
	
find_library(FFMPEG_LIBRARY2
    NAMES
    	avcodec
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)
	
find_library(FFMPEG_LIBRARY3
    NAMES
    	avutil
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)

find_library(FFMPEG_LIBRARY4
    NAMES
    	swscale
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)


find_library(FFMPEG_LIBRARY5
    NAMES
    	swresample
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)
	
find_library(FFMPEG_LIBRARY6
    NAMES
    	avdevice
    PATHS
    	/usr/local/lib
    	/usr/lib
    	/usr/lib/x86_64-linux-gnu
	)
	
	find_library(FFMPEG_LIBRARY7
    NAMES
    	z
    PATHS
    	/usr/local/lib
    	/usr/lib
	)
	
	# Add this path to YOURLIB_LIBRARIES
	if (FFMPEG_LIBRARY1 AND FFMPEG_LIBRARY2 AND FFMPEG_LIBRARY3 AND FFMPEG_LIBRARY4 AND FFMPEG_LIBRARY5 AND FFMPEG_LIBRARY6 AND FFMPEG_LIBRARY7)
		set(FFMPEG_LIBRARIES 
		${FFMPEG_LIBRARY1}
		${FFMPEG_LIBRARY2}
		${FFMPEG_LIBRARY3}
		${FFMPEG_LIBRARY4}
		${FFMPEG_LIBRARY5}
		${FFMPEG_LIBRARY6}
		${FFMPEG_LIBRARY7})
	endif ()
		
if (FFMPEG_INCLUDE_DIR1 AND FFMPEG_INCLUDE_DIR2 AND FFMPEG_INCLUDE_DIR3 AND FFMPEG_INCLUDE_DIR4 AND FFMPEG_INCLUDE_DIR5 AND FFMPEG_LIBRARY1 AND FFMPEG_LIBRARY2 AND FFMPEG_LIBRARY3 AND FFMPEG_LIBRARY4 AND FFMPEG_LIBRARY5 AND FFMPEG_LIBRARY6)
    message(STATUS "Found FFMPEG. Includes: ${FFMPEG_INCLUDE_DIR} Libraries: ${FFMPEG_LIBRARIES}")
	set(FFMPEG_FOUND "YES" )
endif ()