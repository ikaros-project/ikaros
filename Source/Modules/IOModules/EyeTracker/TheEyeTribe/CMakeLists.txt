set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Libfreenect
find_package(TheEyeTribe)

# if all libraries found add source file, include dirs and libraries
if(EYETRIBE_LIB_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/TheEyeTribe.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${EYETRIBE_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${EYETRIBE_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: The Eye Tribe not found. The Eye Tribe module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif()


