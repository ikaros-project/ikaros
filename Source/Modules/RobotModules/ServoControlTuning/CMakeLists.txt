set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Dynamixel SDK
find_package(DYNAMIXEL)

# if all libraries found add source file, include dirs and libraries
if(DYNAMIXEL_LIB_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/ServoControlTuning.cc
	PARENT_SCOPE
)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${DYNAMIXEL_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${DYNAMIXEL_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: Dynamixel libs not found. ServoControlTuning module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif()


