set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(GridWorld)
add_subdirectory(MazeGenerator)
add_subdirectory(PlanarArm)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)