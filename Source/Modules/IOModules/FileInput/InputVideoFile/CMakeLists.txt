set(CMAKE_INCLUDE_CURRENT_DIR ON)

# FFMPEG
find_package(FFMPEG)

# if all libraries found add source file, include dirs and libraries
if(FFMPEG_FOUND)
    set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/InputVideoFile.cc
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
	message(STATUS "Ikaros: FFMPEG is not found. InputVideoFile module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif(FFMPEG_FOUND)
