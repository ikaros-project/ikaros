set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(TEMPLATE_MATCHER_APPLE_SILICON OFF)
if(APPLE AND (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR
              CMAKE_OSX_ARCHITECTURES MATCHES "(^|;)arm64(;|$)" OR
              UNAME_CMAKE_OSX_ARCHITECTURES STREQUAL "arm64"))
    set(TEMPLATE_MATCHER_APPLE_SILICON ON)
endif()

if(TEMPLATE_MATCHER_APPLE_SILICON)
    find_package(onnxruntime CONFIG QUIET)
    if(NOT onnxruntime_FOUND)
        find_program(TEMPLATE_MATCHER_BREW brew)
        if(TEMPLATE_MATCHER_BREW)
            execute_process(
                COMMAND ${TEMPLATE_MATCHER_BREW} --prefix onnxruntime
                OUTPUT_VARIABLE TEMPLATE_MATCHER_ONNXRUNTIME_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE TEMPLATE_MATCHER_BREW_RESULT
            )
            if(TEMPLATE_MATCHER_BREW_RESULT EQUAL 0)
                find_package(onnxruntime CONFIG QUIET
                    PATHS ${TEMPLATE_MATCHER_ONNXRUNTIME_PREFIX}
                    NO_DEFAULT_PATH)
            endif()
        endif()
    endif()

    if(onnxruntime_FOUND)
        set_property(TARGET onnxruntime::onnxruntime PROPERTY IMPORTED_GLOBAL TRUE)
        set(SOURCE
            ${SOURCE}
            ${CMAKE_CURRENT_SOURCE_DIR}/ALIKEDFeatureExtractor.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/FeaturePointBoxes.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureRegionFilter.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/LightGlueFeatureMatcher.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/NativeOnnxInference.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/ProjectiveGeometry.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/PyramidalLucasKanadeTracker.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/RobustTransformEstimator.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/TemplateFeatureBank.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/TemplatePolygonTransform.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/TemplateTrackingController.cc
            PARENT_SCOPE
        )
        set(MODULES_LIBS
            ${MODULES_LIBS}
            onnxruntime::onnxruntime
            PARENT_SCOPE
        )
        message(STATUS "TemplateMatcher native inference enabled")
    else()
        message(STATUS
            "TemplateMatcher native inference disabled: ONNX Runtime was not found")
    endif()
else()
    message(STATUS
        "TemplateMatcher native inference disabled: currently requires Apple Silicon")
endif()
