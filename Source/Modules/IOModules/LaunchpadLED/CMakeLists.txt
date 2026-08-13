set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/LaunchpadProtocol.cc
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/LaunchpadLED.cc
    )
endif()

set(SOURCE ${SOURCE} PARENT_SCOPE)
