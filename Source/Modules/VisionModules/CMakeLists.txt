set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(SobelEdgeDetector)
add_subdirectory(CannyEdgeDetector)
add_subdirectory(ImageMatching)
add_subdirectory(DlibFaceDetector)
add_subdirectory(DlibFaceRecognizer)
add_subdirectory(AppleVisionFaceDetector)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
