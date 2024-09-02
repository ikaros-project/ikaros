# www.ikaros-project.org
#
# Find the libfreenect includes and libraries
#
# This module defines
# FREENECT_INCLUDE_DIRS
# FREENECT_LIBRARIES
# FREENECT_FOUND

# Find header files
find_path(FREENECT_INCLUDE_DIRS
  NAMES
    libfreenect.h libfreenect_sync.h
  PATHS
    /usr/include/libfreenect
    /usr/local/include/libfreenect
    /opt/local/include/libfreenect
  )

# Finding libs
find_library(FREENECT_LIBRARY1
  NAMES
    freenect
  PATHS
    /usr/include/libfreenect
    /usr/local/include/libfreenect
    /opt/local/include/libfreenect
  )

# Finding libs
find_library(FREENECT_LIBRARY2
  NAMES
    freenect_sync
  PATHS
    /usr/include/libfreenect
    /usr/local/include/libfreenect
    /opt/local/include/libfreenect
  )

if (FREENECT_LIBRARY1 AND FREENECT_LIBRARY2)
  set(FREENECT_LIBRARIES
  ${FREENECT_LIBRARY1}
  ${FREENECT_LIBRARY2}
  )
endif ()

if (FREENECT_INCLUDE_DIRS AND FREENECT_LIBRARIES)
  message(STATUS "Found Freenect: Includes: ${FREENECT_INCLUDE_DIRS} Libraries: ${FREENECT_LIBRARIES}")
  set(FREENECT_FOUND "YES" )
endif ()
