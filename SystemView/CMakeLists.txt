######################################################################
#                                                                    #
#        SEGGER SystemView public API unit tests                     #
#                                                                    #
######################################################################

cmake_minimum_required(VERSION 3.16)

project(SystemViewPublicAPITests
  VERSION     1.0.0
  DESCRIPTION "SEGGER SystemView public API unit tests"
  LANGUAGES   C
)

option(SYSTEMVIEW_ENABLE_COVERAGE    "Build SystemView unit tests with compiler coverage instrumentation." OFF)
option(SYSTEMVIEW_WARNINGS_AS_ERRORS "Treat SystemView unit-test warnings as errors." OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type specified -- defaulting to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_library(systemview_test_options INTERFACE)

if(MSVC)
  target_compile_options(systemview_test_options INTERFACE
    /W4
    $<$<BOOL:${SYSTEMVIEW_WARNINGS_AS_ERRORS}>:/WX>
  )
else()
  target_compile_options(systemview_test_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    $<$<BOOL:${SYSTEMVIEW_WARNINGS_AS_ERRORS}>:-Werror>
  )
endif()

if(SYSTEMVIEW_ENABLE_COVERAGE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(systemview_test_options INTERFACE -O0 -g --coverage)
    target_link_options(systemview_test_options INTERFACE --coverage)
    message(STATUS "Coverage: ENABLED")
  else()
    message(WARNING "SYSTEMVIEW_ENABLE_COVERAGE is only configured for GCC/Clang-compatible compilers.")
  endif()
endif()

add_executable(SEGGER_SYSVIEW_PublicAPI_Test
  Tests/SEGGER_SYSVIEW_PublicAPI_Test.c
  SYSVIEW/SEGGER_SYSVIEW.c
)

target_include_directories(SEGGER_SYSVIEW_PublicAPI_Test
  PRIVATE
    Tests/Config
    Config
    SEGGER
    SYSVIEW
    ../RTT/RTT
)

target_compile_features(SEGGER_SYSVIEW_PublicAPI_Test PRIVATE c_std_99)
target_link_libraries(SEGGER_SYSVIEW_PublicAPI_Test PRIVATE systemview_test_options)

add_test(NAME SEGGER_SYSVIEW_PublicAPI_Test COMMAND SEGGER_SYSVIEW_PublicAPI_Test)

message(STATUS "")
message(STATUS "=== SEGGER SystemView Unit Test Configuration ===")
message(STATUS "Version:              ${PROJECT_VERSION}")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Coverage:             ${SYSTEMVIEW_ENABLE_COVERAGE}")
message(STATUS "Warnings as errors:   ${SYSTEMVIEW_WARNINGS_AS_ERRORS}")
message(STATUS "===============================================")
message(STATUS "")
