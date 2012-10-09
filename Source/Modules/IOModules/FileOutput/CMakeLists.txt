set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(OutputFile)
add_subdirectory(OutputJPEG)
add_subdirectory(OutputPNG)
add_subdirectory(OutputQTAudioVisual)
add_subdirectory(OutputRawImage)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
