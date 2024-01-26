# www.ikaros-project.org
#
# Find the FFMPEG includes and libraries
#
# This module defines
# FFMPEG_INCLUDE_DIR
# FFMPEG_LIBRARIES
# FFMPEG_FOUND

# Find header files
find_path(FFMPEG_INCLUDE_DIR1
  NAMES
  libavformat/avformat.h
    PATHS
      /usr/local/include/ffmpeg
      /usr/include/ffmpeg
      /usr/include/libavformat
      /usr/include/ffmpeg/libavformat
      /usr/local/include/libavformat
      /usr/include/x86_64-linux-gnu/libavformat
      /usr/include/i386-linux-gnu/libavformat
      /usr/include/arm-linux-gnueabihf/libavformat
      /opt/homebrew/include/
    )

find_path(FFMPEG_INCLUDE_DIR2
  NAMES
  libavutil/avutil.h
  PATHS
    /usr/local/include/ffmpeg
    /usr/include/ffmpeg
    /usr/include/libavutil
    /usr/include/ffmpeg/libavutil
    /usr/local/include/libavutil
    /usr/include/x86_64-linux-gnu/libavutil
    /usr/include/i386-linux-gnu/libavutil
    /usr/include/arm-linux-gnueabihf/libavutil
    /opt/homebrew/include/
  )

find_path(FFMPEG_INCLUDE_DIR3
  NAMES
  libavcodec/avcodec.h
  PATHS
    /usr/local/include/ffmpeg
    /usr/include/ffmpeg
    /usr/include/libavcodec
    /usr/include/ffmpeg/libavcodec
    /usr/local/include/libavcodec
    /usr/include/x86_64-linux-gnu/libavcodec
    /usr/include/i386-linux-gnu/libavcodec
    /usr/include/arm-linux-gnueabihf/libavcodec
    /opt/homebrew/include/
  )

find_path(FFMPEG_INCLUDE_DIR4
  NAMES
    libswscale/swscale.h
  PATHS
    /usr/local/include/ffmpeg
    /usr/include/ffmpeg
    /usr/include/libswscale
    /usr/include/ffmpeg/libswscale
    /usr/local/include/libswscale
    /usr/include/x86_64-linux-gnu/libswscale
    /usr/include/i386-linux-gnu/libswscale
    /usr/include/arm-linux-gnueabihf/libswscale
    /opt/homebrew/include/
  )

find_path(FFMPEG_INCLUDE_DIR5
  NAMES
  libavdevice/avdevice.h
  PATHS
    /usr/local/include/ffmpeg
    /usr/include/ffmpeg
    /usr/include/libavdevice
    /usr/include/ffmpeg/libavdevice
    /usr/local/include/libavdevice
    /usr/include/x86_64-linux-gnu/libavdevice
    /usr/include/i386-linux-gnu/libavdevice
    /usr/include/arm-linux-gnueabihf/libavdevice
    /opt/homebrew/include/
  )

if (FFMPEG_INCLUDE_DIR1 AND FFMPEG_INCLUDE_DIR2 AND FFMPEG_INCLUDE_DIR3 AND FFMPEG_INCLUDE_DIR4 AND FFMPEG_INCLUDE_DIR5)
  set(FFMPEG_INCLUDE_DIRS
  ${FFMPEG_INCLUDE_DIR1}
  ${FFMPEG_INCLUDE_DIR2}
  ${FFMPEG_INCLUDE_DIR3}
  ${FFMPEG_INCLUDE_DIR4}
  ${FFMPEG_INCLUDE_DIR5})
endif ()

# Finding libs
find_library(FFMPEG_LIBRARY1
  NAMES
    avformat
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/
  )

find_library(FFMPEG_LIBRARY2
  NAMES
    avcodec
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/
  )

find_library(FFMPEG_LIBRARY3
  NAMES
    avutil
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/
  )

   find_library(FFMPEG_LIBRARY4
  NAMES
    swscale
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/
   )

   find_library(FFMPEG_LIBRARY5
  NAMES
    swresample
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/
  )

find_library(FFMPEG_LIBRARY6
  NAMES
    avdevice
  PATHS
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /opt/homebrew/lib/

  )

find_library(FFMPEG_LIBRARY7
  NAMES
    z
  PATHS
    /usr/local/lib
    /usr/lib
  )

# Add this path to YOURLIB_LIBRARIES
if (FFMPEG_LIBRARY1 AND FFMPEG_LIBRARY2 AND FFMPEG_LIBRARY3 AND FFMPEG_LIBRARY4 AND FFMPEG_LIBRARY5 AND FFMPEG_LIBRARY6 AND FFMPEG_LIBRARY7)
  set(FFMPEG_LIBRARIES
  ${FFMPEG_LIBRARY1}
  ${FFMPEG_LIBRARY2}
  ${FFMPEG_LIBRARY3}
  ${FFMPEG_LIBRARY4}
  ${FFMPEG_LIBRARY5}
  ${FFMPEG_LIBRARY6}
  ${FFMPEG_LIBRARY7})
endif ()

if (FFMPEG_INCLUDE_DIR1 AND FFMPEG_INCLUDE_DIR2 AND FFMPEG_INCLUDE_DIR3 AND FFMPEG_INCLUDE_DIR4 AND FFMPEG_INCLUDE_DIR5 AND FFMPEG_LIBRARY1 AND FFMPEG_LIBRARY2 AND FFMPEG_LIBRARY3 AND FFMPEG_LIBRARY4 AND FFMPEG_LIBRARY5 AND FFMPEG_LIBRARY6)
  message(STATUS "Found FFMPEG: Includes: ${FFMPEG_INCLUDE_DIRS} Libraries: ${FFMPEG_LIBRARIES}")
  set(FFMPEG_FOUND "YES" )
endif ()