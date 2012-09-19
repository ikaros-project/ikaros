set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(ChangeDetector)
add_subdirectory(CurvatureDetectors)
add_subdirectory(EdgeDetectors)
add_subdirectory(GaborFilter)
add_subdirectory(MorphologicalOperators)

set(SOURCE
   ${SOURCE}
   PARENT_SCOPE
)

