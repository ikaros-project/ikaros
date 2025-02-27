# www.ikaros-project.org
#
# Example how to write a cmake module to use a external library if the library is not yet included in the official cmake script.
# To check included modules use "cmake --help-module-list"
#
#

# Specific parameters for different OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Find one header file and lib
#############################################

# Find header files
find_path(USB_INCLUDE_DIRS
  NAMES
    libusb-1.0/libusb.h
  PATHS
    /usr/local/include
    /usr/include
    /opt/local/include
    /opt/homebrew/include

  )

# Find lib file
  find_library(USB_LIBRARIES
  NAMES
  usb-1.0 usb
  PATHS
    /usr/local/lib
    /opt/local/lib
    /usr/lib
    /opt/homebrew/lib
  )

if (USB_INCLUDE_DIRS AND USB_LIBRARIES)
  message(STATUS "Found USBlib: Includes: ${USB_INCLUDE_DIRS} Libraries: ${USB_LIBRARIES}")
  set(USB_LIB_FOUND "YES" )
endif ()
#############################################

