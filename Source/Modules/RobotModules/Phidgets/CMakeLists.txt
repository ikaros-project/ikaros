set(CMAKE_INCLUDE_CURRENT_DIR ON)


# Phidgets
find_package(PHIDGETS)

# if all libraries found add source file, include dirs and libraries
if(PHIDGETS_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/Phidgets.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${PHIDGETS_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${PHIDGETS_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: Phidgets libs not found. Phidgets module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif()
