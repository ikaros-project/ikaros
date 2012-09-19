set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(MinimalModule)
add_subdirectory(MyModule)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)