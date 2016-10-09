# CMake script for the ikaros project. www.ikaros-project.org

# Set cmake version
cmake_minimum_required(VERSION 2.6)

# To use elseif
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Project name
project(ikaros)

# Checking OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   	message( STATUS "Detected APPLE system")
	add_definitions(-DMAC_OS_X)

 	# Set compiler
	set(CMAKE_C_COMPILER "clang")
	set(CMAKE_CXX_COMPILER "clang++")
	
   	#SET(GUI_TYPE MACOSX_BUNDLE)
   	include_directories( /Developer/Headers/FlatCarbon )
   	find_library(ACCELERATE_LIBRARY Accelerate)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(QTKIT_LIBRARY QTKit)
    find_library(QUARTZ_LIBRARY Quartz)
  
  	mark_as_advanced(ACCELERATE_LIBRARY
                     COCOA_LIBRARY
                     QTKIT_LIBRARY
                     QUARTZ_LIBRARY)
	set(EXTRA_LIBS ${ACCELERATE_LIBRARY} ${COCOA_LIBRARY} ${QTKIT_LIBRARY} ${QUARTZ_LIBRARY})
   
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Linux specific code
 	message( STATUS "Detected a Linux system")
	add_definitions(-DLINUX)
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	add_definitions(-DLINUX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows specific code
	message( STATUS "Detected a Windows system. Not yet implemented in CMake. See www.ikaros-project.org for compile instructions.")
	add_definitions(-DWIN32)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Compiler-specific C++11 activation.
# Ikaors WebUI is using atomic. Supported in Gnu GCC > 4.4
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    
    
    #if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
    #    message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
    
    
    if (NOT (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.6 or greater.")
    endif ()
    
    if (GCC_VERSION VERSION_EQUAL 4.6)	        
    	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	endif ()
	if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)	        
   	 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
else ()
    message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()

# Set Ikaros root path 
set(IKAROS_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# Set binary path
set(EXECUTABLE_OUTPUT_PATH ${IKAROS_SOURCE_DIR}/Bin)

# Setting module path (where ffmpeg etc. should be)
set(CMAKE_MODULE_PATH ${IKAROS_SOURCE_DIR}/Scripts/CMake)

# Include the source directory
include_directories(Source/)
include_directories(Source/Modules/)

# Add sub directory
add_subdirectory(Source)

# Crete a binary with source files
add_executable(ikaros ${SOURCE})


# Check and adding Kernel libraries
# BLAS
find_package (BLAS)
if (BLAS_FOUND)
  include_directories(${BLAS_INCLUDE_DIRS})
  target_link_libraries (ikaros ${BLAS_LIBRARIES})
else()
    message(FATAL_ERROR "\nIkaros require the BLAS library.\nVisit www.ikaros-project.org for information about dependencies")
endif (BLAS_FOUND)

# Threads
find_package(Threads)
if (Threads_FOUND)
  include_directories(${Threads_INCLUDE_DIRS})
  target_link_libraries (ikaros ${CMAKE_THREAD_LIBS_INIT})
  else()
    message(FATAL_ERROR "\nIkaros require the THREADS library. \nVisit www.ikaros-project.org for information about dependencies")
endif (Threads_FOUND)

# JPEG (is this working with multiple jpeg libs installed?)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	# Turbo JPEG
	# Using brew will not find turbojpeg. In linux it will detected by findJPEG
	find_package (Turbo_JPEG)
	if (TURBO_JPEG_FOUND)
  		include_directories(${TURBO_JPEG_INCLUDE_DIR})
  		target_link_libraries (ikaros ${TURBO_JPEG_LIBRARIES})
  		message(STATUS "Found a Turbo-JPEG library")	

  	else()
   		 message(FATAL_ERROR "\nIkaros require the JPEG library. \nVisit www.ikaros-project.org for information about dependencies")	
	endif (TURBO_JPEG_FOUND)


else (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	find_package (JPEG)
	if (JPEG_FOUND)
  		include_directories(${JPEG_INCLUDE_DIRS})
  		target_link_libraries (ikaros ${JPEG_LIBRARIES})
    else()
    	message(FATAL_ERROR "\nIkaros require the JPEG library. \nVisit www.ikaros-project.org for information about dependencies")
	endif (JPEG_FOUND)

endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# PNG
find_package (PNG)
if (PNG_FOUND)
  include_directories(${PNG_INCLUDE_DIRS})
  target_link_libraries (ikaros ${PNG_LIBRARIES})
    else()
    message(FATAL_ERROR "\nIkaros require the PNG library. \nVisit www.ikaros-project.org for information about dependencies")
endif (PNG_FOUND)


# Add extra libraries when linking
target_link_libraries(ikaros ${EXTRA_LIBS} )

# Ikaros modules' include directories and libraries
set(MODULES_INCLUDS ${MODULES_INCLUDS})
set(MODULES_LIBS ${MODULES_LIBS})
include_directories(${MODULES_INCLUDS})
target_link_libraries (ikaros ${MODULES_LIBS})
  add_dependencies(ikaros MPIFaceDetector)

# Set binary name to ikaros_d in debug mode 
set_target_properties(ikaros PROPERTIES DEBUG_POSTFIX "_d")


