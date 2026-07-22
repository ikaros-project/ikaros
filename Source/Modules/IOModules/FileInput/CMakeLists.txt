set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(InputFile)
add_subdirectory(InputAudioFile)
add_subdirectory(InputJPEG)
add_subdirectory(InputImage)
add_subdirectory(InputVideoFile)

list(APPEND SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/image_sequence.cc)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
