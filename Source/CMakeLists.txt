# Include the directory itself as a path to include directories
#
# cmake -DCMAKE_BUILD_TYPE=Debug ..
#
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Kernel)
add_subdirectory(Modules)
add_subdirectory(UserModules)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
