set(CMAKE_INCLUDE_CURRENT_DIR ON)


	
# YARP
find_package (YARP)

# YARP Uses ACE. Usgin Yarps cmake config file to find ACE.

# Pick up our CMake scripts - they are all in the conf subdirectory.
set(CMAKE_MODULE_PATH ${YARP_INCLUDE_DIRS}/../share/yarp/cmake/)


find_package(ACE)
if(YARP_FOUND AND ACE_FOUND)
    set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/YARPPort.cc
	PARENT_SCOPE
	
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
)
else()
MESSAGE( STATUS "Ace can not be found. Ikaros is using yarp's cmake script for finding ace." )

endif(YARP_FOUND AND ACE_FOUND)

