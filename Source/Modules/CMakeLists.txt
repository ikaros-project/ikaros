set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(ANN)
add_subdirectory(BrainModels)
add_subdirectory(CodingModules)
add_subdirectory(ControlModules)
add_subdirectory(EnvironmentModules)
add_subdirectory(Examples)
add_subdirectory(IOModules)
add_subdirectory(LearningModules)
add_subdirectory(RobotModules)
add_subdirectory(UtilityModules)
add_subdirectory(VisionModules)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)