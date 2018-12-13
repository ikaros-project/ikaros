set(CMAKE_INCLUDE_CURRENT_DIR ON)

# DLIB
find_package(Dlib)

# if all libraries found add source file, include dirs and libraries
if(DLIB_LIB_FOUND)
    set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/DTracker.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${dlib_INCLUDE_DIRS}
	PARENT_SCOPE
  	)

   	set(MODULES_LIBS
	${MODULES_LIBS}
    ${DLIB_LIBRARIES}
    PARENT_SCOPE
    ) 
    #message(STATUS "DLIB")
    #message(STATUS ${MODULES_LIBS} )
else()
	message(STATUS "Ikaros: Dlib not found. DFaceDetector module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif(DLIB_LIB_FOUND)
