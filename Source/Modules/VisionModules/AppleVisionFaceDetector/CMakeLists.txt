set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(APPLE)
	find_library(VISION_FRAMEWORK Vision)
	find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
	find_library(FOUNDATION_FRAMEWORK Foundation)

	if(VISION_FRAMEWORK AND COREGRAPHICS_FRAMEWORK AND FOUNDATION_FRAMEWORK)
		set_source_files_properties(
			${CMAKE_CURRENT_SOURCE_DIR}/AppleVisionFaceDetector.mm
			PROPERTIES COMPILE_FLAGS "-std=c++17 -stdlib=libc++"
		)
		set(SOURCE
			${SOURCE}
			${CMAKE_CURRENT_SOURCE_DIR}/AppleVisionFaceDetector.mm
			PARENT_SCOPE
		)
		set(MODULES_LIBS
			${MODULES_LIBS}
			${VISION_FRAMEWORK}
			${COREGRAPHICS_FRAMEWORK}
			${FOUNDATION_FRAMEWORK}
			PARENT_SCOPE
		)
	else()
		message(STATUS "AppleVisionFaceDetector disabled: Apple Vision framework was not found")
	endif()
else()
	message(STATUS "AppleVisionFaceDetector disabled: only available on Apple platforms")
endif()
