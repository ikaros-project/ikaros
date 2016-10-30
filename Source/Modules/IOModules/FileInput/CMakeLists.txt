set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(InputFile)
add_subdirectory(InputJPEG)
add_subdirectory(InputPNG)
add_subdirectory(InputRawImage)
add_subdirectory(InputVideoFile)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
