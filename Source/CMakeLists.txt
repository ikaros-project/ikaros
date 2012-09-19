# Include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Kernel)
add_subdirectory(Modules)
add_subdirectory(UserModules)
add_subdirectory(WebUI)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)




