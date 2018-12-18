set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(MinimalModule)
add_subdirectory(MyModule)
#add_subdirectory(MagicalGarden)
add_subdirectory(2018/AdjustablePupilInRobots/EpiHeadExp)

add_subdirectory(2018/RobotMimicry/HeadTracking)
add_subdirectory(2018/RobotMimicry/MaximumToRobot)
add_subdirectory(2018/RobotMimicry/MimicHead)
add_subdirectory(2018/RobotMimicry/TrainHead)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
