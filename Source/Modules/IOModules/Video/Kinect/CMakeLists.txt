set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Libfreenect
find_package(Freenect)

# if all libraries found add source file, include dirs and libraries
if(FREENECT_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/Kinect.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${FREENECT_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${FREENECT_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: Freenect not found. Kinect module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif()




