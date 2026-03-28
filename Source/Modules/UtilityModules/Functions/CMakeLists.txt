set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Abs)
add_subdirectory(Clip)
add_subdirectory(Exp)
add_subdirectory(Invert)
add_subdirectory(Negate)
add_subdirectory(Not)
add_subdirectory(Square)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
