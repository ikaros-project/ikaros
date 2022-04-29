set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(LM22_Perception)
add_subdirectory(LM22_SpatialAttention)
add_subdirectory(LM22_ValueAccumulator)
add_subdirectory(LM22_Approach)
add_subdirectory(LM22_ContinuousWorld)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

