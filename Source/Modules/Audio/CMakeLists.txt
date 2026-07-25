set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(APPLE)
    add_subdirectory(AudioInput)
    add_subdirectory(AudioOutput)
else()
    message(STATUS "AudioInput and AudioOutput disabled: AudioToolbox is only available on Apple platforms")
endif()

add_subdirectory(AudioSynth)
add_subdirectory(AudioOscillator)
add_subdirectory(ADSREnvelope)
add_subdirectory(AudioFilter)
add_subdirectory(AudioDelay)
add_subdirectory(AudioReverb)
add_subdirectory(OutputAudioFile)


set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
