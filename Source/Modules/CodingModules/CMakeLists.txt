set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CoarseCoder)
add_subdirectory(PopulationCoder)
add_subdirectory(IntervalCoder)
add_subdirectory(IntervalDecoder)
add_subdirectory(TappedDelayLine)
add_subdirectory(SpectralTiming)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)