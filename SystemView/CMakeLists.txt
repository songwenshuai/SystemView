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

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(SYSTEMVIEW_STANDALONE ON)
else()
  set(SYSTEMVIEW_STANDALONE OFF)
endif()

if(SYSTEMVIEW_STANDALONE)
  set(SYSTEMVIEW_BUILD_LIBRARY_DEFAULT OFF)
else()
  set(SYSTEMVIEW_BUILD_LIBRARY_DEFAULT ON)
endif()

option(SYSTEMVIEW_BUILD_LIBRARY "Build the reusable SEGGER SystemView library target." ${SYSTEMVIEW_BUILD_LIBRARY_DEFAULT})
option(SYSTEMVIEW_BUILD_TESTS   "Build SystemView unit tests." ${SYSTEMVIEW_STANDALONE})

set(SYSTEMVIEW_CONFIG_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/Config" CACHE PATH
    "Directory containing SEGGER_SYSVIEW_Conf.h for the reusable SystemView library target.")
set(SYSTEMVIEW_RTT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../RTT/RTT" CACHE PATH
    "RTT include directory used when the SEGGER_RTT target is not available.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(SYSTEMVIEW_STANDALONE AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type specified -- defaulting to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(SYSTEMVIEW_STANDALONE)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

if(SYSTEMVIEW_BUILD_TESTS)
  enable_testing()
endif()

if(SYSTEMVIEW_BUILD_LIBRARY AND NOT TARGET SEGGER_SYSVIEW)
  add_library(SEGGER_SYSVIEW STATIC
    SYSVIEW/SEGGER_SYSVIEW.c
  )

  target_include_directories(SEGGER_SYSVIEW
    PUBLIC
      "${SYSTEMVIEW_CONFIG_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}/SEGGER"
      "${CMAKE_CURRENT_SOURCE_DIR}/SYSVIEW"
  )

  if(TARGET SEGGER_RTT)
    target_link_libraries(SEGGER_SYSVIEW PUBLIC SEGGER_RTT)
  else()
    target_include_directories(SEGGER_SYSVIEW
      PUBLIC
        "${SYSTEMVIEW_RTT_INCLUDE_DIR}"
    )
  endif()

  target_compile_features(SEGGER_SYSVIEW PUBLIC c_std_99)
  add_library(SEGGER::SystemView ALIAS SEGGER_SYSVIEW)
endif()

if(SYSTEMVIEW_BUILD_TESTS)
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
endif()

if(SYSTEMVIEW_STANDALONE)
  message(STATUS "")
  message(STATUS "=== SEGGER SystemView Unit Test Configuration ===")
  message(STATUS "Version:              ${PROJECT_VERSION}")
  message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
  message(STATUS "Coverage:             ${SYSTEMVIEW_ENABLE_COVERAGE}")
  message(STATUS "Warnings as errors:   ${SYSTEMVIEW_WARNINGS_AS_ERRORS}")
  message(STATUS "Build tests:          ${SYSTEMVIEW_BUILD_TESTS}")
  message(STATUS "Config directory:     ${SYSTEMVIEW_CONFIG_DIR}")
  message(STATUS "===============================================")
  message(STATUS "")
endif()
