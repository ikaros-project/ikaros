set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(MinimalModule)
add_subdirectory(MyModule)
add_subdirectory(BlobChooser)
add_subdirectory(ToLineCombination)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
