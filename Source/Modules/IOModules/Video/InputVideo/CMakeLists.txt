set(CMAKE_INCLUDE_CURRENT_DIR ON)

# FFMPEG
find_package(FFMPEG)

# if all libraries found add source file, include dirs and libraries
if(FFMPEG_FOUND)

    set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/InputVideoFFMPEG.cc
	PARENT_SCOPE
	)
	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${FFMPEG_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${FFMPEG_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message( "" )
	message(STATUS "Ikaros:" )
	message(STATUS "FFMpeg is not found. This module is not included into the build." )
	message(STATUS "Visit www.ikaros-project.org for information of module dependencies." )
	message( "" )
endif(FFMPEG_FOUND)
