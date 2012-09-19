# CmakeLists.txt in Test dir
# Make sure the compiler can find include files from our Hello library.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE
   ${SOURCE}
   ${CMAKE_CURRENT_SOURCE_DIR}/WebUI.cc
   PARENT_SCOPE
)
