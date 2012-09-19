set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Downsample)
add_subdirectory(Upsample)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
