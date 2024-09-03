set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(UtilityModules)
add_subdirectory(BrainModels)
#add_subdirectory(RobotModules)
add_subdirectory(IOModules)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

