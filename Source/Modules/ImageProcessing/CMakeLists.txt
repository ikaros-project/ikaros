set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(StaufferGrimson)
add_subdirectory(Downsample)
add_subdirectory(Upsample)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

