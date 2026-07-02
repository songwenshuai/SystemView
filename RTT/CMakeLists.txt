# ==============================================================================
# SEGGER RTT Public API Unit Tests
# ==============================================================================
#
# Builds the reusable SEGGER RTT library target and optional host unit tests for
# the shared-memory public API.
#
# Usage:
#   mkdir build && cd build
#   cmake -DCMAKE_BUILD_TYPE=Debug ..
#   cmake --build . -j$(nproc)
#
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

project(SystemViewRTTTests
    VERSION     1.0.0
    DESCRIPTION "SEGGER RTT shared-memory public API unit tests"
    LANGUAGES   C
)

# ==============================================================================
# Build Configuration Options
# ==============================================================================

option(RTT_ENABLE_COVERAGE "Build RTT unit tests with compiler coverage instrumentation." OFF)
option(RTT_WARNINGS_AS_ERRORS "Treat RTT unit-test warnings as errors." OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(RTT_STANDALONE ON)
else()
    set(RTT_STANDALONE OFF)
endif()

option(RTT_BUILD_LIBRARY "Build the reusable SEGGER RTT library target." ON)
option(RTT_BUILD_TESTS "Build RTT unit tests." ${RTT_STANDALONE})

set(RTT_TEST_BUFFER_SIZE_UP "16" CACHE STRING "Unit-test up-buffer size in bytes.")
set(RTT_TEST_BUFFER_SIZE_DOWN "16" CACHE STRING "Unit-test down-buffer size in bytes.")
set(RTT_TEST_MAX_NUM_UP_BUFFERS "3" CACHE STRING "Unit-test maximum number of up-buffers.")
set(RTT_TEST_MAX_NUM_DOWN_BUFFERS "3" CACHE STRING "Unit-test maximum number of down-buffers.")
set(RTT_TEST_PRINTF_BUFFER_SIZE "128" CACHE STRING "Unit-test SEGGER_RTT_printf staging buffer size.")
set(RTT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Config" CACHE PATH
    "Directory containing SEGGER_RTT_Conf.h for the reusable RTT library target.")

# ==============================================================================
# Global CMake Settings
# ==============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(RTT_STANDALONE AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified -- defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

if(RTT_STANDALONE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

if(RTT_BUILD_TESTS)
    enable_testing()
endif()

# ==============================================================================
# Reusable RTT Library Target
# ==============================================================================

if(RTT_BUILD_LIBRARY AND NOT TARGET SEGGER_RTT)
    add_library(SEGGER_RTT STATIC
        RTT/SEGGER_RTT.c
        RTT/SEGGER_RTT_printf.c
    )

    target_include_directories(SEGGER_RTT
        PUBLIC
            "${RTT_CONFIG_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/SEGGER"
            "${CMAKE_CURRENT_SOURCE_DIR}/RTT"
    )

    target_compile_features(SEGGER_RTT PUBLIC c_std_99)
    add_library(SEGGER::RTT ALIAS SEGGER_RTT)
endif()

# ==============================================================================
# Common Compile Configuration
# ==============================================================================

if(RTT_BUILD_TESTS)
    add_library(rtt_test_options INTERFACE)
    add_library(rtt_test_instrumentation INTERFACE)

    target_link_libraries(rtt_test_options INTERFACE rtt_test_instrumentation)

    if(MSVC)
        target_compile_options(rtt_test_options INTERFACE
            /W4
            $<$<BOOL:${RTT_WARNINGS_AS_ERRORS}>:/WX>
            "/DSEGGER_RTT_LOCK()="
            "/DSEGGER_RTT_UNLOCK()="
        )
    else()
        target_compile_options(rtt_test_options INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            $<$<BOOL:${RTT_WARNINGS_AS_ERRORS}>:-Werror>
            "-DSEGGER_RTT_LOCK()="
            "-DSEGGER_RTT_UNLOCK()="
        )
    endif()

    if(RTT_ENABLE_COVERAGE)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(rtt_test_instrumentation INTERFACE -O0 -g --coverage)
            target_link_options(rtt_test_instrumentation INTERFACE --coverage)
            message(STATUS "Coverage: ENABLED")
        else()
            message(WARNING "RTT_ENABLE_COVERAGE is only configured for GCC/Clang-compatible compilers.")
        endif()
    endif()
endif()

# ==============================================================================
# Test Program Helper
# ==============================================================================

function(_AddRTTPublicAPITest TargetName PrintfBufferSize)
    add_executable(${TargetName}
        Tests/SEGGER_RTT_PublicAPI_Test.c
        RTT/SEGGER_RTT.c
        RTT/SEGGER_RTT_printf.c
    )

    target_include_directories(${TargetName}
        PRIVATE
            SEGGER
            RTT
    )

    target_compile_definitions(${TargetName}
        PRIVATE
            BUFFER_SIZE_UP=${RTT_TEST_BUFFER_SIZE_UP}
            BUFFER_SIZE_DOWN=${RTT_TEST_BUFFER_SIZE_DOWN}
            SEGGER_RTT_MAX_NUM_UP_BUFFERS=${RTT_TEST_MAX_NUM_UP_BUFFERS}
            SEGGER_RTT_MAX_NUM_DOWN_BUFFERS=${RTT_TEST_MAX_NUM_DOWN_BUFFERS}
            SEGGER_RTT_PRINTF_BUFFER_SIZE=${PrintfBufferSize}
    )

    target_compile_features(${TargetName} PRIVATE c_std_99)
    target_link_libraries(${TargetName} PRIVATE rtt_test_options)
    add_test(NAME ${TargetName} COMMAND ${TargetName})
endfunction()

# ==============================================================================
# Test Programs
# ==============================================================================

if(RTT_BUILD_TESTS)
    _AddRTTPublicAPITest(SEGGER_RTT_PublicAPI_Test ${RTT_TEST_PRINTF_BUFFER_SIZE})
    _AddRTTPublicAPITest(SEGGER_RTT_PublicAPI_ByteLoop_Test ${RTT_TEST_PRINTF_BUFFER_SIZE})
    _AddRTTPublicAPITest(SEGGER_RTT_PublicAPI_PrintfSmallBuffer_Test 4)

    target_compile_definitions(SEGGER_RTT_PublicAPI_ByteLoop_Test
        PRIVATE
            SEGGER_RTT_MEMCPY_USE_BYTELOOP=1
    )

    target_compile_definitions(SEGGER_RTT_PublicAPI_PrintfSmallBuffer_Test
        PRIVATE
            TEST_RTT_PRINTF_SMALL_BUFFER=1
    )
endif()

# ==============================================================================
# Build Summary
# ==============================================================================

if(RTT_STANDALONE)
    message(STATUS "")
    message(STATUS "=== SEGGER RTT Unit Test Configuration ===")
    message(STATUS "Version:              ${PROJECT_VERSION}")
    message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
    message(STATUS "Coverage:             ${RTT_ENABLE_COVERAGE}")
    message(STATUS "Warnings as errors:   ${RTT_WARNINGS_AS_ERRORS}")
    message(STATUS "Build tests:          ${RTT_BUILD_TESTS}")
    message(STATUS "Config directory:     ${RTT_CONFIG_DIR}")
    message(STATUS "Up buffer size:       ${RTT_TEST_BUFFER_SIZE_UP}")
    message(STATUS "Down buffer size:     ${RTT_TEST_BUFFER_SIZE_DOWN}")
    message(STATUS "Up buffers:           ${RTT_TEST_MAX_NUM_UP_BUFFERS}")
    message(STATUS "Down buffers:         ${RTT_TEST_MAX_NUM_DOWN_BUFFERS}")
    message(STATUS "Printf buffer size:   ${RTT_TEST_PRINTF_BUFFER_SIZE}")
    message(STATUS "==========================================")
    message(STATUS "")
endif()
