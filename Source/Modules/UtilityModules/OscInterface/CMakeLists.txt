set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_subdirectory(osc)
add_subdirectory(ip)
set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/OscInterface.cc
	PARENT_SCOPE
)
