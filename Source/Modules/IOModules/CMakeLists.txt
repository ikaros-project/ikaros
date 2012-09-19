set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(FileInput)
add_subdirectory(FileOutput)
add_subdirectory(Video)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
