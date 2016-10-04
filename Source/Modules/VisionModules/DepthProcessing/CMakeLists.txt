set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(DepthBlobList)
add_subdirectory(DepthContourTrace)
add_subdirectory(DepthHistogram)
add_subdirectory(DepthPointList)
add_subdirectory(DepthSegmentation)
add_subdirectory(DepthTransform)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

