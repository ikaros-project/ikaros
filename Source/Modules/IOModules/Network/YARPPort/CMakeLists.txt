set(CMAKE_INCLUDE_CURRENT_DIR ON)

# YARP and ACE
find_package(YARP)
find_package(ACE)

# if all libraries found add source file, include dirs and libraries
if(YARP_FOUND AND ACE_FOUND)

    set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/YARPPort.cc
	PARENT_SCOPE
	)
	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${YARP_INCLUDE_DIRS}
	${ACE_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${YARP_LIBRARIES}
	${ACE_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: YARP or ACE is not found. YARPPort module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif(YARP_FOUND AND ACE_FOUND)
