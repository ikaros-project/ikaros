set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Perception)
add_subdirectory(ValueAccumulator)
add_subdirectory(MemorySequencer)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

