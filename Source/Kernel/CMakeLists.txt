set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
    list(APPEND SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/dictionary.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/range.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utilities.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/signal.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/xml.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/profiler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/compute_engine.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/parameter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/component.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/module.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/module_class.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ikaros.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_diagnostics.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_paths.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_parsing.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_http.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_execution.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_scheduling.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_setup.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_shapes.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_state.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel_webui.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/circular_buffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/connection.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/request.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/session_logging.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/serial_bsd.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/socket.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/timing.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/maths.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/matrix.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/image_file_formats.cc
    )
    set(SOURCE ${SOURCE} PARENT_SCOPE)
endif()
