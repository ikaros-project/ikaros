set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(AudioInput)
add_subdirectory(AudioOutput)
add_subdirectory(AudioSynth)
add_subdirectory(AudioOscillator)
add_subdirectory(ADSREnvelope)
add_subdirectory(AudioFilter)
add_subdirectory(AudioDelay)
add_subdirectory(AudioReverb)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
