set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Sum)
add_subdirectory(Product)
add_subdirectory(Min)
add_subdirectory(Max)
add_subdirectory(Average)
add_subdirectory(Median)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

