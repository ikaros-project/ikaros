set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(KNN)
add_subdirectory(KNN_Pick)
add_subdirectory(LinearAssociator)
add_subdirectory(QLearning)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)
