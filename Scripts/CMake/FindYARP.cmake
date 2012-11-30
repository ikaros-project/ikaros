# www.ikaros-project.org
#
# Find the YAPRP includes and libraries
#
# This module defines
# YARP_INCLUDE_DIRS
# YARP_LIBRARIES
# YARP_FOUND


find_path(YARP_INCLUDE_DIR
    names
    	yarp/os/all.h 
    	yarp/dev/all.h 
    	yarp/sig/all.h
	paths
		/usr/include
		/usr/local/include
	)

find_library(YARP_OS_LIBRARY
	names
		YARP_OS
	paths
		/usr/lib
		/usr/local/lib
	)
  
find_library(YARP_DEV_LIBRARY
	names
		YARP_dev
	paths
		/usr/lib
		/usr/local/lib
	)
  
find_library(YARP_SIG_LIBRARY
	names
		YARP_sig
	paths
		/usr/lib
		/usr/local/lib
	)

  find_library(YARP_INIT_LIBRARY
	names
		YARP_init
	paths
		/usr/lib
		/usr/local/lib
	)
	
if (YARP_OS_LIBRARY AND YARP_DEV_LIBRARY AND YARP_SIG_LIBRARY AND YARP_OS_LIBRARY AND YARP_INIT_LIBRARY)
	set(YARP_LIBRARIES
	${YARP_OS_LIBRARY}
	${YARP_DEV_LIBRARY}
	${YARP_SIG_LIBRARY}
	${YARP_INIT_LIBRARY}
	)
	
	set(YARP_INCLUDE_DIRS
	${YARP_INCLUDE_DIR}
	)
	
    message(status "Found YARP:")
	message(status " - Includes: ${YARP_INCLUDE_DIRS}")
	message(status " - Libraries: ${YARP_LIBRARIES}")
	set(YARP_FOUND "YES" )
endif ()

