set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(ArtoolkitPlus)

# if all libraries found add source file, include dirs and libraries
if(ARTOOLKITPLUS_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/MarkerTracker.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${ARTOOLKITPLUS_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${ARTOOLKITPLUS_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message( "" )
	message(STATUS "Ikaros:" )
	message(STATUS "ArtoolkitPlus not found. Marker module is not included into the build." )
	message(STATUS "Visit www.ikaros-project.org for information of module dependencies." )
	message( "" )
endif(ARTOOLKITPLUS_FOUND)
