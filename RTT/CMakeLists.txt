######################################################################
#                                                                    #
#        SEGGER RTT * Real Time Transfer for embedded targets        #
#                                                                    #
######################################################################
#
# --------------------------END-OF-HEADER-----------------------------
# Purpose : CMake build description for the shared-memory RTT public
#           API unit tests.
#
#           The test executable is built from the production RTT
#           sources and a standalone host-side test file.  It uses a
#           fixed shared-memory configuration with small ring buffers so
#           boundary conditions, wrap-around, and full-buffer handling
#           can be exercised quickly by CTest.
#
# ---------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

project(SystemViewRTTTests
  VERSION     1.0.0
  DESCRIPTION "SEGGER RTT shared-memory public API unit tests"
  LANGUAGES   C
)

######################################################################
#
#       Configuration options
#
######################################################################

option(RTT_ENABLE_COVERAGE     "Build RTT unit tests with compiler coverage instrumentation." OFF)
option(RTT_WARNINGS_AS_ERRORS  "Treat RTT unit-test warnings as errors." OFF)
option(RTT_BUILD_EXAMPLES      "Build RTT example applications." ON)

set(RTT_TEST_BUFFER_SIZE_UP        "16"  CACHE STRING "Unit-test up-buffer size in bytes.")
set(RTT_TEST_BUFFER_SIZE_DOWN      "16"  CACHE STRING "Unit-test down-buffer size in bytes.")
set(RTT_TEST_MAX_NUM_UP_BUFFERS    "3"   CACHE STRING "Unit-test maximum number of up-buffers.")
set(RTT_TEST_MAX_NUM_DOWN_BUFFERS  "3"   CACHE STRING "Unit-test maximum number of down-buffers.")
set(RTT_TEST_PRINTF_BUFFER_SIZE    "128" CACHE STRING "Unit-test SEGGER_RTT_printf staging buffer size.")

######################################################################
#
#       Global CMake settings
#
######################################################################

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

######################################################################
#
#       Common compile configuration
#
######################################################################

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

######################################################################
#
#       Example compile configuration
#
######################################################################

add_library(rtt_example_options INTERFACE)

target_link_libraries(rtt_example_options INTERFACE rtt_test_options)

if(MSVC)
  target_compile_options(rtt_example_options INTERFACE /wd4326)
else()
  target_compile_options(rtt_example_options INTERFACE -Wno-main -Wno-main-return-type)
endif()

######################################################################
#
#       _AddRTTPublicAPITest
#
# Function description
#   Builds a host executable that links the production RTT C sources
#   with the public API unit-test driver.
#
# Parameters
#   TargetName        Name of the executable target to create.
#   PrintfBufferSize  Size of the temporary SEGGER_RTT_printf() buffer.
#
######################################################################

function(_AddRTTPublicAPITest TargetName PrintfBufferSize)
  add_executable(${TargetName}
    Tests/SEGGER_RTT_PublicAPI_Test.c
    RTT/SEGGER_RTT.c
    RTT/SEGGER_RTT_printf.c
  )

  #
  # The RTT sources include headers relative to RTT.
  #
  target_include_directories(${TargetName}
    PRIVATE
      RTT
  )

  #
  # Use small deterministic buffers in the unit test target.  The reduced
  # sizes make full, empty, trim, skip, and wrap-around cases observable
  # without allocating large test vectors.
  #
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

######################################################################
#
#       RTT public API unit-test targets
#
######################################################################

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

######################################################################
#
#       _AddRTTExample
#
# Function description
#   Builds an RTT example application as a compile/link check.  Example
#   applications are not registered with CTest because they are
#   interactive, endless-loop, or board/RTOS demonstration programs.
#
# Parameters
#   TargetName  Name of the executable target to create.
#   SourceFile  Example source file to compile.
#
######################################################################

function(_AddRTTExample TargetName SourceFile)
  add_executable(${TargetName}
    ${SourceFile}
    RTT/SEGGER_RTT.c
    RTT/SEGGER_RTT_printf.c
  )

  target_include_directories(${TargetName}
    PRIVATE
      RTT
  )

  target_compile_features(${TargetName} PRIVATE c_std_99)
  target_link_libraries(${TargetName} PRIVATE rtt_example_options)
endfunction()

######################################################################
#
#       RTT example targets
#
######################################################################

if(RTT_BUILD_EXAMPLES)
  _AddRTTExample(SEGGER_RTT_InputEchoApp Examples/Main_RTT_InputEchoApp.c)
  _AddRTTExample(SEGGER_RTT_MenuApp      Examples/Main_RTT_MenuApp.c)
  _AddRTTExample(SEGGER_RTT_PrintfTest   Examples/Main_RTT_PrintfTest.c)
  _AddRTTExample(SEGGER_RTT_SpeedTestApp Examples/Main_RTT_SpeedTestApp.c)
endif()

######################################################################
#
#       Configuration summary
#
######################################################################

message(STATUS "")
message(STATUS "=== SEGGER RTT Unit Test Configuration ===")
message(STATUS "Version:              ${PROJECT_VERSION}")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Coverage:             ${RTT_ENABLE_COVERAGE}")
message(STATUS "Warnings as errors:   ${RTT_WARNINGS_AS_ERRORS}")
message(STATUS "Build examples:       ${RTT_BUILD_EXAMPLES}")
message(STATUS "Up buffer size:       ${RTT_TEST_BUFFER_SIZE_UP}")
message(STATUS "Down buffer size:     ${RTT_TEST_BUFFER_SIZE_DOWN}")
message(STATUS "Up buffers:           ${RTT_TEST_MAX_NUM_UP_BUFFERS}")
message(STATUS "Down buffers:         ${RTT_TEST_MAX_NUM_DOWN_BUFFERS}")
message(STATUS "Printf buffer size:   ${RTT_TEST_PRINTF_BUFFER_SIZE}")
message(STATUS "==========================================")
message(STATUS "")
