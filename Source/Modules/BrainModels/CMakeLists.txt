set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(EyeModel)
add_subdirectory(Moren2002)
add_subdirectory(Nucleus)
add_subdirectory(NeuralArray)
add_subdirectory(NucleusEnsemble)
add_subdirectory(DecisionModel2020)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
