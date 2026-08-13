# Locate the WebP headers and encoder/decoder library.

find_path(WebP_INCLUDE_DIR
    NAMES webp/decode.h
    PATH_SUFFIXES include
)

if(WebP_INCLUDE_DIR AND EXISTS "${WebP_INCLUDE_DIR}/webp/encode.h")
    set(WebP_HEADERS_FOUND TRUE)
else()
    set(WebP_HEADERS_FOUND FALSE)
endif()

find_library(WebP_LIBRARY
    NAMES webp
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebP
    REQUIRED_VARS WebP_INCLUDE_DIR WebP_HEADERS_FOUND WebP_LIBRARY
)

if(WebP_FOUND AND NOT TARGET WebP::webp)
    add_library(WebP::webp UNKNOWN IMPORTED)
    set_target_properties(WebP::webp PROPERTIES
        IMPORTED_LOCATION "${WebP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WebP_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(WebP_INCLUDE_DIR WebP_LIBRARY)
