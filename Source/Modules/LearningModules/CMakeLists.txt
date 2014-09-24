set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Trainer)
add_subdirectory(KNN)
add_subdirectory(KNN_Pick)
add_subdirectory(LocalApproximator)
add_subdirectory(LinearAssociator)
add_subdirectory(QLearning)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
