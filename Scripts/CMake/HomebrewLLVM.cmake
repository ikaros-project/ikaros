include_guard(GLOBAL)

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "The Homebrew LLVM toolchain is only available on macOS")
endif()

find_program(IKAROS_HOMEBREW_EXECUTABLE
    NAMES brew
    HINTS /opt/homebrew/bin /usr/local/bin
)
if(NOT IKAROS_HOMEBREW_EXECUTABLE)
    message(FATAL_ERROR "Homebrew was not found. Install it from https://brew.sh/")
endif()

execute_process(
    COMMAND "${IKAROS_HOMEBREW_EXECUTABLE}" --prefix llvm
    RESULT_VARIABLE IKAROS_HOMEBREW_LLVM_RESULT
    OUTPUT_VARIABLE IKAROS_HOMEBREW_LLVM_PREFIX
    ERROR_VARIABLE IKAROS_HOMEBREW_LLVM_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT IKAROS_HOMEBREW_LLVM_RESULT EQUAL 0)
    string(STRIP "${IKAROS_HOMEBREW_LLVM_ERROR}" IKAROS_HOMEBREW_LLVM_ERROR)
    message(FATAL_ERROR
        "Homebrew LLVM was not found. Run 'brew install llvm'. "
        "Homebrew reported: ${IKAROS_HOMEBREW_LLVM_ERROR}"
    )
endif()

set(IKAROS_HOMEBREW_CLANG "${IKAROS_HOMEBREW_LLVM_PREFIX}/bin/clang")
set(IKAROS_HOMEBREW_CLANGXX "${IKAROS_HOMEBREW_LLVM_PREFIX}/bin/clang++")
if(NOT EXISTS "${IKAROS_HOMEBREW_CLANG}" OR
   NOT EXISTS "${IKAROS_HOMEBREW_CLANGXX}")
    message(FATAL_ERROR
        "Homebrew LLVM is incomplete at ${IKAROS_HOMEBREW_LLVM_PREFIX}. "
        "Run 'brew reinstall llvm'."
    )
endif()

set(CMAKE_C_COMPILER "${IKAROS_HOMEBREW_CLANG}" CACHE FILEPATH
    "Homebrew LLVM C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${IKAROS_HOMEBREW_CLANGXX}" CACHE FILEPATH
    "Homebrew LLVM C++ compiler" FORCE)
set(CMAKE_OBJCXX_COMPILER "${IKAROS_HOMEBREW_CLANGXX}" CACHE FILEPATH
    "Homebrew LLVM Objective-C++ compiler" FORCE)
