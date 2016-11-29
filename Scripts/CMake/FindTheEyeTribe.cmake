# www.ikaros-project.org
# 
# 
# This module defines:
# EYETRIBE_INCLUDE_DIRS
# EYETRIBE_LIBRARIES
# EYETRIBE_LIB_FOUND

SET(Boost_USE_MULTITHREAD ON)
SET(Boost_USE_STATIC_LIBS ON)
SET(BOOST_MIN_VERSION "1.53.0")
FIND_PACKAGE( Boost REQUIRED thread system )

IF(Boost_FOUND)
# Find header files
find_path(EYETRIBE_INCLUDE_DIRS
	NAMES
		gazeapi.h gazeapi_interfaces.h gazeapi_types.h	
    PATHS
		/usr/local/include
		/usr/include
  	)
  	
# Find lib file
	find_library(EYETRIBE_LIBRARIES
    NAMES
    	GazeApiLib
    PATHS
    	/usr/local/lib
    	/usr/lib
	)

  SET(EYETRIBE_INCLUDE_DIRS ${EYETRIBE_INCLUDE_DIRS} "${Boost_INCLUDE_DIR}")
  SET(EYETRIBE_LIBRARIES ${EYETRIBE_LIBRARIES} "${Boost_LIBRARIES}")

if (EYETRIBE_INCLUDE_DIRS AND EYETRIBE_LIBRARIES)
    message(STATUS "Found The Eye Tribe. Includes: ${EYETRIBE_INCLUDE_DIRS} Libraries: ${EYETRIBE_LIBRARIES}")
	set(EYETRIBE_LIB_FOUND "YES" )
endif ()

ENDIF(Boost_FOUND)
