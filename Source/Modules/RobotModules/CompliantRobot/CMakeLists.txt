set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_subdirectory(ForceCheck)
add_subdirectory(PositionSelection)
add_subdirectory(CurrentPositionMapping)
add_subdirectory(TestMapping)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
