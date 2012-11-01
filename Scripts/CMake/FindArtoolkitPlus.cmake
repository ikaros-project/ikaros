# Ikaros-project.org
#
# Find the ArtoolkitPlus includes and library
#
# This module defines
# ARTOOLKITPLUS_INCLUDE_DIR
# ARTOOLKITPLUS_LIBRARIES
# ARTOOLKITPLUS_FOUND


FIND_PATH(ARTOOLKITPLUS_INCLUDE_DIR ARToolKitPlus.h
  /usr/include/ARToolKitPlus
  /usr/local/include/ARToolKitPlus
)

FIND_LIBRARY(ARTOOLKITPLUS_LIBRARY ARToolKitPlus
  /usr/local/lib/ARToolKitPlus
  /usr/lib/ARToolKitPlus
)

IF (ARTOOLKITPLUS_LIBRARY)
	SET(ARTOOLKITPLUS_LIBRARIES
	${ARTOOLKITPLUS_LIBRARY}
	)
    message(STATUS "Found ArToolKitPlus:")
	message(STATUS " - Includes: ${ARTOOLKITPLUS_INCLUDE_DIR}")
	message(STATUS " - Libraries: ${ARTOOLKITPLUS_LIBRARIES}")
	SET(ARTOOLKITPLUS_FOUND "YES" )
ENDIF (ARTOOLKITPLUS_LIBRARY)

