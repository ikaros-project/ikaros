# Ikaros-project.org
#
# Find the ArtoolkitPlus includes and library
#
# This module defines
# ARTOOLKITPLUS_INCLUDE_DIR
# ARTOOLKITPLUS_LIBRARIES
# ARTOOLKITPLUS_FOUND


find_path(ARTOOLKITPLUS_INCLUDE_DIR
  NAMES
    ARToolKitPlus.h
  PATHS
    /usr/include/ARToolKitPlus
    /usr/local/include/ARToolKitPlus
)

find_library(ARTOOLKITPLUS_LIBRARIES
  NAMES
    ARToolKitPlus
  PATHS
    /usr/local/lib/ARToolKitPlus
    /usr/lib/ARToolKitPlus
)

if (ARTOOLKITPLUS_INCLUDE_DIR AND ARTOOLKITPLUS_LIBRARIES)
  message(STATUS "Found ArToolKitPlus: Includes: ${ARTOOLKITPLUS_INCLUDE_DIR} Libraries: ${ARTOOLKITPLUS_LIBRARIES}")
  set(ARTOOLKITPLUS_FOUND "YES" )
endif ()
