set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Dlib)

if(DLIB_LIB_FOUND)
	set(SOURCE
		${SOURCE}
		${CMAKE_CURRENT_SOURCE_DIR}/DlibFaceDetector.cc
		PARENT_SCOPE
	)
	set(MODULES_INCLUDS
		${MODULES_INCLUDS}
		${DLIB_INCLUDE_DIRS}
		PARENT_SCOPE
	)
	set(MODULES_LIBS
		${MODULES_LIBS}
		${DLIB_LIBRARIES}
		PARENT_SCOPE
	)
else()
	message(STATUS "DlibFaceDetector disabled: DLIB was not found")
endif()
