set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CoarseCoder)
add_subdirectory(IntervalCoder)
add_subdirectory(IntervalDecoder)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)