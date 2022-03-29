# Ikaros-project.org
#
# Find the OpenNI includes and library
#
# This module defines
# OPENNI_INCLUDE_DIR
# OPENNI_LIBRARIES
# OPENNI_FOUND


find_path(OPENNI_INCLUDE_DIRS
  NAMES
    XnCppWrapper.h
  PATHS
    /usr/local/include/ni/
)

find_library(OPENNI_LIBRARIES
  NAMES
    OpenNI
  PATHS
    /usr/local/lib/
)

if (OPENNI_INCLUDE_DIRS AND OPENNI_LIBRARIES)
  message(STATUS "Found OpenNI: Includes: ${OPENNI_INCLUDE_DIRS} Libraries: ${OPENNI_LIBRARIES}")
  set(OPENNI_FOUND TRUE)
endif ()
