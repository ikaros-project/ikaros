set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Dilate)
add_subdirectory(Erode)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)

