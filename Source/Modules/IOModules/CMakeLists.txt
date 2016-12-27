set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(EyeTracker)
add_subdirectory(FileInput)
add_subdirectory(FileOutput)
add_subdirectory(Network)
add_subdirectory(Video)
add_subdirectory(SoundOutput)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
