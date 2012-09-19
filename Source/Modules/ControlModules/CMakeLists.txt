set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(KalmanFilter)
add_subdirectory(PIDController)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)