# CMake script for the ikaros project. www.ikaros-project.org

# Set cmake version
cmake_minimum_required(VERSION 3.10)

# To use elseif
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# Project name
project(ikaros)

# Checking OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   	message( STATUS "Detected APPLE system")
	add_definitions(-DMAC_OS_X)

  # Detect if the system using x86 or arm64 (intel or apple silicon)
  execute_process(COMMAND uname -m
  OUTPUT_VARIABLE UNAME_CMAKE_OSX_ARCHITECTURES
  OUTPUT_STRIP_TRAILING_WHITESPACE)
  
  #message(STATUS ${UNAME_CMAKE_OSX_ARCHITECTURES})
  
  if(${UNAME_CMAKE_OSX_ARCHITECTURES} STREQUAL "arm64")
    message(STATUS "Detected apple silicon")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64")

  else()
    message(STATUS "Detected intel")
    # Using default flag
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x86_64")
  endif()

  # ARM64 flag
  #if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64")
  #endif()



 	# Set compiler
	set(CMAKE_C_COMPILER "clang")
	set(CMAKE_CXX_COMPILER "clang++")

  find_library(ACCELERATE_LIBRARY Accelerate) # Hardware Acceleration
  find_library(COCOA_LIBRARY Cocoa)           # Wrapper for including the Cocoa frameworks AppKit.framework, Foundation.framework, and CoreData.framework.
  find_library(QUARTZ_LIBRARY QuartzCore)     # Contains the interfaces for Core Image, Core Animation, and Core Video. See Quartz Core Framework Reference. Used by CIFaceDetector
  find_library(AUDIO_LIBRARY AudioToolbox)
  # An advanced variable will not be displayed in any of the cmake GUIs
  mark_as_advanced(ACCELERATE_LIBRARY COCOA_LIBRARY QUARTZ_LIBRARY AUDIO_LIBRARY)
    set(EXTRA_LIBS ${ACCELERATE_LIBRARY} ${COCOA_LIBRARY} ${QUARTZ_LIBRARY} ${AUDIO_LIBRARY})

  # CoreMIDI Not part of the core
  #add_definitions(-D__MACOSX_CORE__)
  #find_library(COREAUDIO CoreAudio)
  #find_library(COREFOUNDATION CoreFoundation)
  #find_library(COREMIDI CoreMidi)
  #find_library(CORESERVICES CoreServices)
  #set(EXTRA_LIBS ${EXTRA_LIBS} ${COREAUDIO} ${COREFOUNDATION} ${COREMIDI} ${CORESERVICES})
  
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Linux specific code
 	message( STATUS "Detected a Linux system")
	add_definitions(-DLINUX)
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	add_definitions(-DLINUX)
  # Remove a few warnings
  add_definitions(-Wno-unused-value)
  add_definitions(-Wno-unused-result)

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows specific code
	message( STATUS "Windows is not supported. More info at www.ikaros-project.org")
	add_definitions(-DWIN32)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Compiler-specific C++17 activation.
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
   	 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++")
else ()
    message(FATAL_ERROR "Your C++ compiler does not support C++17.")
endif ()

# Set Ikaros root path
set(IKAROS_SOURCE_DIR ${PROJECT_SOURCE_DIR})

# Set binary path
set(EXECUTABLE_OUTPUT_PATH ${IKAROS_SOURCE_DIR}/Bin)

# Setting module path (where cmake scripts etc. should be)
set(CMAKE_MODULE_PATH ${IKAROS_SOURCE_DIR}/Scripts/CMake)

# Include the source directory
include_directories(Source/)
include_directories(Source/Modules/)

# Add sub directory
add_subdirectory(Source)
#add_subdirectory(Projects)

# Crete a binary with source files
add_executable(ikaros ${SOURCE})

# Check and adding Kernel libraries
# BLAS
find_package (BLAS)
if (BLAS_FOUND)
  include_directories(${BLAS_INCLUDE_DIRS})
  target_link_libraries (ikaros ${BLAS_LIBRARIES})
else()
    message(FATAL_ERROR "\nIkaros requires the BLAS library.\nVisit www.ikaros-project.org for information about dependencies")
endif (BLAS_FOUND)

# Threads Not part of the core
#find_package(Threads)
#if (Threads_FOUND)
#  include_directories(${Threads_INCLUDE_DIRS})
#  target_link_libraries (ikaros ${CMAKE_THREAD_LIBS_INIT})
#  else()
#    message(FATAL_ERROR "\nIkaros require the THREADS library.\nVisit www.ikaros-project.org for information about dependencies")
#endif (Threads_FOUND)

# Turbo JPEG
find_package (Jpeg-Turbo)
  if (TURBO_JPEG_FOUND)
  	include_directories(${TURBO_JPEG_INCLUDE_DIRS})
  	target_link_libraries (ikaros ${TURBO_JPEG_LIBRARIES})
  	else()
   		 message(FATAL_ERROR "\nIkaros require the Turbo-JPEG library. \nVisit www.ikaros-project.org for information about dependencies")
	endif (TURBO_JPEG_FOUND)

# PNG not part of the core
# find_package (PNG)
#if (PNG_FOUND)
#  include_directories(${PNG_INCLUDE_DIRS})
#  target_link_libraries (ikaros ${PNG_LIBRARIES})
#    else()
#    message(FATAL_ERROR "\nIkaros require the PNG library. \nVisit www.ikaros-project.org for information about dependencies")
#endif (PNG_FOUND)

# ALSA Midi not part of the core
#find_package(ALSA)                                    
#if (ALSA_FOUND)      
#    add_definitions(-D__LINUX_ALSA__)                                        
#    include_directories(${ALSA_INCLUDE_DIRS})                  
#    target_link_libraries (ikaros ${ALSA_LIBRARIES})      
#endif(ALSA_FOUND)

# Add extra libraries when linking
target_link_libraries(ikaros ${EXTRA_LIBS} )

# Ikaros modules' include directories and libraries
set(MODULES_INCLUDS ${MODULES_INCLUDS})
set(MODULES_LIBS ${MODULES_LIBS})
include_directories(${MODULES_INCLUDS})
target_link_libraries (ikaros ${MODULES_LIBS})

# Set binary name to ikaros_d in debug mode
#set_target_properties(ikaros PROPERTIES DEBUG_POSTFIX "_d")

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

# Print all variables
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_compile_definitions(DEBUG)
endif()