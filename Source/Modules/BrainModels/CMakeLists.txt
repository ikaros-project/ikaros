set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Nucleus)
add_subdirectory(SCN)
add_subdirectory(SpikingPopulation)
add_subdirectory(LeakyIntegrator)
add_subdirectory(Cerebellum)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)

