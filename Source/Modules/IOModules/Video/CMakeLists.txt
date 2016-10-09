set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(InputVideo)
#add_subdirectory(InputVideoAV)
add_subdirectory(Kinect)
add_subdirectory(NetworkCamera)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)