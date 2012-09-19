set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(InputFile)
add_subdirectory(InputJPEG)
add_subdirectory(InputPNG)
#add_subdirectory(InputQTAudio)
add_subdirectory(InputQTMovie)
add_subdirectory(InputRawImage)


set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
