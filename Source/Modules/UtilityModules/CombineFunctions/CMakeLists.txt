set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Add)
add_subdirectory(Subtract)
add_subdirectory(Multiply)
add_subdirectory(Divide)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

