set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(AttentionFocus)
add_subdirectory(ColorClassifier)
add_subdirectory(ColorMatch)
add_subdirectory(ColorTransform)
add_subdirectory(ImageConvolution)
add_subdirectory(ImageOperators)
add_subdirectory(SaliencyMap)
add_subdirectory(Scaling)
add_subdirectory(SpatialClustering)
add_subdirectory(WhiteBalance)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
