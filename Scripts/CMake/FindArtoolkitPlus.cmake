# Ikaros-project.org
#
# Find the ArtoolkitPlus includes and library
#
# This module defines
# ARTOOLKITPLUS_INCLUDE_DIRS
# ARTOOLKITPLUS_LIBRARIES
# ARTOOLKITPLUS_FOUND


find_path(ARTOOLKITPLUS_INCLUDE_DIRS
  NAMES
    ARToolKitPlus/ARToolKitPlus.h
  PATHS
    /usr/include/
    /usr/local/include/
)

find_library(ARTOOLKITPLUS_LIBRARIES
  NAMES
    ARToolKitPlus
  PATHS
    /usr/local/lib/ARToolKitPlus
    /usr/lib/ARToolKitPlus
)

if (ARTOOLKITPLUS_INCLUDE_DIRS AND ARTOOLKITPLUS_LIBRARIES)
  message(STATUS "Found ArToolKitPlus: Includes: ${ARTOOLKITPLUS_INCLUDE_DIRS} Libraries: ${ARTOOLKITPLUS_LIBRARIES}")
  set(ARTOOLKITPLUS_FOUND "YES" )
endif ()
