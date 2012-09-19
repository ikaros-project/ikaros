# CmakeLists.txt in Test dir
# Make sure the compiler can find include files from our Hello library.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Add binary called "helloWorld" that is built from the source file "test.cc".
# The extension is automatically found.

set(SOURCE
   ${SOURCE}
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Math.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Serial_BSD.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Serial_WIN.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Signal.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Socket_BSD.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Socket_WIN.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Threads_none.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Threads_pthread.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Timer_POSIX.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Timer_WIN.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_Utils.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS_XML.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/IKAROS.cc
   ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
   PARENT_SCOPE
)
