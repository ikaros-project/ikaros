set(CMAKE_INCLUDE_CURRENT_DIR ON)

# USBlib
find_package(USB)

# if all libraries found add source file, include dirs and libraries
if(USB_LIB_FOUND)
set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/FadeCandy.cc
	${CMAKE_CURRENT_SOURCE_DIR}/FadeCandy_driver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Util.cpp
	PARENT_SCOPE
)

set(MODULES_INCLUDS
${MODULES_INCLUDS}
${USB_INCLUDE_DIRS}
PARENT_SCOPE
   )
   set(MODULES_LIBS
${MODULES_LIBS}
${USB_LIBRARIES}
PARENT_SCOPE
) 


else()
	message(STATUS "Ikaros: LibUSB is not found. FadeCandy module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif(USB_LIB_FOUND)
