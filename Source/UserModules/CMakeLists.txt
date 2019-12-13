set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(MaxElement)

add_subdirectory(ComposeTransform)

add_subdirectory(BoundaryCheck)

add_subdirectory(DelayCounter)
add_subdirectory(GoSignal)

add_subdirectory(MinimalModule)
add_subdirectory(MyModule)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
