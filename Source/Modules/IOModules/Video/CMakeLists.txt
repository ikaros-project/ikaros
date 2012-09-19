set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(InputVideoQT)
add_subdirectory(NetworkCamera)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
