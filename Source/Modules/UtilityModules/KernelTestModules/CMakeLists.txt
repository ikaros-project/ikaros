set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(KernelTestModule)
add_subdirectory(KernelSizeTestModule)
add_subdirectory(KernelPlayground)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

