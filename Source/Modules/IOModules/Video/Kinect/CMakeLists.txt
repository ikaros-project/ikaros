set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Libfreenect
find_package(libfreenect COMPONENTS sync)
#find_package(libusb-1.0)

# if all libraries found add source file, include dirs and libraries
if(LIBFREENECT_FOUND)

	set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/Kinect.cc
	PARENT_SCOPE
	)

	set(MODULES_INCLUDS
	${MODULES_INCLUDS}
	${LIBFREENECT_INCLUDE_DIRS}
	PARENT_SCOPE
   	)
   	set(MODULES_LIBS
	${MODULES_LIBS}
	${LIBFREENECT_LIBRARIES}
	PARENT_SCOPE
    ) 

else()
	message(STATUS "Ikaros: Libfreenect not found. Kinect module is not included into the build." )
endif(LIBFREENECT_FOUND)




