set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Dynamixel)
add_subdirectory(Epuck)
add_subdirectory(SSC32)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
