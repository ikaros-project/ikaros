# www.ikaros-project.org
#
# Find the HDF5 includes and libraries
#
# This module defines:
# HDF5_INCLUDE_DIR
# HDF5_LIBRARIES
# HDF5_FOUND

# Find header files
find_path(HDF5_INCLUDE_DIRS
  NAMES
    hdf5.h
  PATHS
    /usr/local/include
    /usr/include
  )

# Find lib file
find_library(HDF5_LIBRARIES
  NAMES
    libhdf5
  PATHS
    /usr/local/lib
    /usr/lib
  )

if (HDF5_INCLUDE_DIRS AND HDF5_LIBRARIES)
    message(STATUS "Found HDF5: Includes: ${HDF5_INCLUDE_DIRS} Libraries: ${HDF5_LIBRARIES}")
    set(HDF5_FOUND "YES" )
endif ()
