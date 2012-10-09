set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(M02_Amygdala)
add_subdirectory(M02_Cortex)
add_subdirectory(M02_Hippocampus)
add_subdirectory(M02_OFC)
add_subdirectory(M02_SensoryCortex)
add_subdirectory(M02_Thalamus)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)