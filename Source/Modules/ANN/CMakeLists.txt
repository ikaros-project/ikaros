set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Autoassociator)
add_subdirectory(CSOM)
add_subdirectory(Perceptron)
add_subdirectory(RNN)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

