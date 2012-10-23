# - Try to find YARP
# Once done this will define
#
#  YARP_FOUND - system has YARP
#  YARP_INCLUDE_DIRS - the YARP include directory
#  YARP_LIBRARIES - Link these to use YARP
#  YARP_DEFINITIONS - Compiler switches required for using YARP
#
#  Copyright (c) 2008 Stefán Freyr Stefánsson <[EMAIL PROTECTED]>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (YARP_LIBRARIES AND YARP_INCLUDE_DIRS)
  # in cache already
  set(YARP_FOUND TRUE)
else (YARP_LIBRARIES AND YARP_INCLUDE_DIRS)
  find_path(YARP_INCLUDE_DIR
    NAMES
      yarp/os/all.h yarp/dev/all.h yarp/sig/all.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(YARP_OS_LIBRARY
    NAMES
      YARP_OS
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )
  find_library(YARP_DEV_LIBRARY
    NAMES
      YARP_dev
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )
  find_library(YARP_SIG_LIBRARY
    NAMES
      YARP_sig
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (YARP_OS_LIBRARY)
    set(YARP_OS_FOUND TRUE)
  endif (YARP_OS_LIBRARY)
  if (YARP_DEV_LIBRARY)
    set(YARP_DEV_FOUND TRUE)
  endif (YARP_DEV_LIBRARY)
  if (YARP_SIG_LIBRARY)
    set(YARP_SIG_FOUND TRUE)
  endif (YARP_SIG_LIBRARY)

  set(YARP_INCLUDE_DIRS
    ${YARP_INCLUDE_DIR}
  )

  if (YARP_OS_FOUND)
    set(YARP_LIBRARIES
      ${YARP_LIBRARIES}
      ${YARP_OS_LIBRARY}
    )
  endif (YARP_OS_FOUND)
  if (YARP_DEV_FOUND)
    set(YARP_LIBRARIES
      ${YARP_LIBRARIES}
      ${YARP_DEV_LIBRARY}
    )
  endif (YARP_DEV_FOUND)
  if (YARP_SIG_FOUND)
    set(YARP_LIBRARIES
      ${YARP_LIBRARIES}
      ${YARP_SIG_LIBRARY}
    )
  endif (YARP_SIG_FOUND)

  if (YARP_INCLUDE_DIRS AND YARP_LIBRARIES)
     set(YARP_FOUND TRUE)
  endif (YARP_INCLUDE_DIRS AND YARP_LIBRARIES)

  if (YARP_FOUND)
    if (NOT YARP_FIND_QUIETLY)
      message(STATUS "Found YARP: ${YARP_LIBRARIES}")
    endif (NOT YARP_FIND_QUIETLY)
  else (YARP_FOUND)
    if (YARP_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find YARP")
    endif (YARP_FIND_REQUIRED)
  endif (YARP_FOUND)

  # show the YARP_INCLUDE_DIRS and YARP_LIBRARIES variables only in the advanced view
  mark_as_advanced(YARP_INCLUDE_DIRS YARP_LIBRARIES)

endif (YARP_LIBRARIES AND YARP_INCLUDE_DIRS)
