set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CannyEdgeDetector)
add_subdirectory(GaussianEdgeDetector)
add_subdirectory(HysteresisThresholding)
add_subdirectory(PrewittEdgeDetector)
add_subdirectory(RobertsEdgeDetector)
add_subdirectory(SobelEdgeDetector)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
