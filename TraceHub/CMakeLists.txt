# ==============================================================================
# TraceHub - RTT Trace and Debug Bridge
# ==============================================================================
#
# This application provides RTT trace, terminal, and SystemView bridge services.
#
# Features:
#   - Real-time communication with embedded targets
#   - Swimlane log rendering for heterogeneous systems
#   - Terminal and SystemView bridge services
#
# Usage:
#   mkdir build && cd build
#   cmake -DCMAKE_BUILD_TYPE=Release ..
#   cmake --build . -j$(nproc)
#
# ==============================================================================

cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(TraceHub
    VERSION 1.0.0
    DESCRIPTION "TraceHub - RTT Trace and Debug Bridge"
    LANGUAGES C
)

set(TRACEHUB_TARGET tracehub)

# macOS + Ninja: RPATH modification is not supported for Mach-O binaries
# Set CMAKE_BUILD_WITH_INSTALL_RPATH to build with install RPATH directly
# Note: APPLE variable is only defined after project() call
if(APPLE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON CACHE BOOL "Build with install RPATH" FORCE)
endif()

# ==============================================================================
# Build Configuration Options
# ==============================================================================

option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(USE_MEMSHM_BACKEND "Use host shared memory backend for simulation" ON)
if(CMAKE_CROSSCOMPILING)
    set(BUILD_UNIT_TESTS_DEFAULT OFF)
else()
    set(BUILD_UNIT_TESTS_DEFAULT ON)
endif()
option(BUILD_UNIT_TESTS "Build TraceHub unit test programs" ${BUILD_UNIT_TESTS_DEFAULT})
set(TRACEHUB_DEFAULT_RTT_ADDR "0" CACHE STRING "Default RTT backend address used by the CLI")
set(TRACEHUB_DEFAULT_RTT_SIZE "0" CACHE STRING "Default RTT region size used by the CLI")
set(TRACEHUB_DEFAULT_MEMORY_PATH "" CACHE STRING "Default backend path used by the CLI")
set(TRACEHUB_DEFAULT_LINUX_CHANNEL "0" CACHE STRING "Default RTT text channel for Linux logs")
set(TRACEHUB_DEFAULT_RTOS_CHANNEL "1" CACHE STRING "Default RTT text channel for RTOS logs")
set(TRACEHUB_DEFAULT_TERMINAL_CHANNEL "${TRACEHUB_DEFAULT_LINUX_CHANNEL}" CACHE STRING "Default RTT text channel for single-source Terminal mode")
set(TRACEHUB_DEFAULT_SYSVIEW_CHANNEL "2" CACHE STRING "Default RTT SystemView channel")
set(TRACEHUB_DEFAULT_LINUX_LABEL "LINUX" CACHE STRING "Default swimlane label for Linux logs")
set(TRACEHUB_DEFAULT_RTOS_LABEL "RTOS" CACHE STRING "Default swimlane label for RTOS logs")
set(TRACEHUB_CORE_LOG_QUEUE_SIZE "1048576u" CACHE STRING "Default per-source core log consumer queue size")
set(TRACEHUB_TERMINAL_QUEUE_SIZE "1048576u" CACHE STRING "Default Terminal TCP backlog queue size")
set(TRACEHUB_SYSTEMVIEW_QUEUE_SIZE "1048576u" CACHE STRING "Default SystemView TCP backlog queue size")
set(TRACEHUB_SWIMLANE_TOTAL_WIDTH "120u" CACHE STRING "Default non-TTY swimlane output width")
set(TRACEHUB_SWIMLANE_TIMESTAMP_WIDTH "12u" CACHE STRING "Default swimlane timestamp column width")
set(TRACEHUB_LOG_COLLECTOR_PENDING_UNTIMED_SIZE "65536u" CACHE STRING "Default per-source untimestamped startup log staging size")
set(TRACEHUB_LOG_MERGER_FLUSH_THRESHOLD "4u" CACHE STRING "Default swimlane ready flush threshold")
set(TRACEHUB_LOG_MERGER_FLUSH_TIMEOUT_MS "1000u" CACHE STRING "Default swimlane ready flush timeout in milliseconds")

# ==============================================================================
# Global CMake Settings
# ==============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SMEM depends on the Linux SharedMem driver. Other host platforms use MEMSHM.
if((APPLE OR WIN32) AND NOT USE_MEMSHM_BACKEND)
    message(FATAL_ERROR "SMEM requires the Linux SharedMem driver; use USE_MEMSHM_BACKEND=ON on macOS and Windows")
endif()

if(USE_MEMSHM_BACKEND AND NOT WIN32)
    set(BUILD_SIM_TESTS_DEFAULT ON)
else()
    set(BUILD_SIM_TESTS_DEFAULT OFF)
endif()
option(BUILD_SIM_TESTS "Build RTT simulation test programs" ${BUILD_SIM_TESTS_DEFAULT})
if(BUILD_SIM_TESTS AND NOT USE_MEMSHM_BACKEND)
    message(FATAL_ERROR "BUILD_SIM_TESTS requires USE_MEMSHM_BACKEND=ON because simulation programs use the MEMSHM backend")
endif()
if(BUILD_SIM_TESTS AND WIN32)
    message(FATAL_ERROR "BUILD_SIM_TESTS contains POSIX simulation programs and is not supported on Windows; keep BUILD_UNIT_TESTS=ON for Windows unit coverage")
endif()

if(TRACEHUB_DEFAULT_MEMORY_PATH STREQUAL "")
    if(USE_MEMSHM_BACKEND)
        set(TRACEHUB_EFFECTIVE_DEFAULT_MEMORY_PATH "/rtt_sim")
    else()
        set(TRACEHUB_EFFECTIVE_DEFAULT_MEMORY_PATH "/dev/shared_mem0")
    endif()
else()
    set(TRACEHUB_EFFECTIVE_DEFAULT_MEMORY_PATH "${TRACEHUB_DEFAULT_MEMORY_PATH}")
endif()

set(SEGGER_RTT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../RTT/RTT")
set(SEGGER_RTT_SEGGER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../RTT/SEGGER")
set(SEGGER_SYSVIEW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SystemView/SYSVIEW")
set(SHAREDMEM_UAPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SharedMem/UAPI")
set(TRACEHUB_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/App")
set(TRACEHUB_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Core")
set(TRACEHUB_LOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Log")
set(TRACEHUB_NETWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Network")
set(TRACEHUB_RTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/RTT")
set(TRACEHUB_SERVICES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Services")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Debug")
endif()

# Set default install prefix for standalone builds
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Install path prefix" FORCE)
endif()

# ==============================================================================
# Language Standards
# ==============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==============================================================================
# Compiler Flags
# ==============================================================================

# Base flags
if(NOT CMAKE_C_FLAGS)
    if(MSVC)
        set(CMAKE_C_FLAGS "/W4" CACHE STRING "C compiler flags" FORCE)
    else()
        set(CMAKE_C_FLAGS "-Wall" CACHE STRING "C compiler flags" FORCE)
    endif()
endif()

# Debug-specific flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(MSVC)
        add_compile_options(/Zi)
    else()
        add_compile_options(-g2 -ggdb)
    endif()
    add_compile_definitions(DEBUG)
endif()

# Release-specific flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Treat warnings as errors for native builds (not cross-compilation)
if(UNIX AND NOT CMAKE_CROSSCOMPILING)
    add_compile_options(-Werror)
endif()

# Code coverage support
if(ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "ENABLE_COVERAGE requires a GNU or Clang compatible compiler")
    endif()
    message(STATUS "Code coverage enabled")
    add_compile_options(-fprofile-arcs -ftest-coverage -O0)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# Sanitizers support
if(ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "ENABLE_SANITIZERS requires a GNU or Clang compatible compiler")
    endif()
    message(STATUS "Address and undefined behavior sanitizers enabled")
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Memory backend selection
if(USE_MEMSHM_BACKEND)
    message(STATUS "Using MEMSHM backend (host shared memory)")
    set(RTT_MEMORY_BACKEND_DEFINITIONS RTTMEM_USE_MEMSHM=1)
    set(RTT_MEMORY_BACKEND_SOURCE ${TRACEHUB_RTT_DIR}/RTTMemory_memshm.c)
    # Enable Linux, RTOS, and SystemView channels for simulation testing
    set(RTT_MEMORY_CHANNEL_DEFINITIONS
        SEGGER_RTT_MAX_NUM_UP_BUFFERS=3
        SEGGER_RTT_MAX_NUM_DOWN_BUFFERS=3
    )
else()
    message(STATUS "Using SMEM backend (SharedMem driver)")
    set(RTT_MEMORY_BACKEND_DEFINITIONS RTTMEM_USE_SMEM=1)
    set(RTT_MEMORY_CHANNEL_DEFINITIONS)
    set(RTT_MEMORY_BACKEND_SOURCE ${TRACEHUB_RTT_DIR}/RTTMemory_smem.c)
endif()

set(RTT_MEMORY_COMMON_DEFINITIONS
    BUFFER_SIZE_UP=1048576
    BUFFER_SIZE_DOWN=1048576
    SEGGER_RTT_MODE_DEFAULT=SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL
)

# ==============================================================================
# Source Files
# ==============================================================================

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${TRACEHUB_APP_DIR}/CLI.c
    ${TRACEHUB_APP_DIR}/OptionsApi.c
    ${TRACEHUB_APP_DIR}/OptionsParse.c
    ${TRACEHUB_APP_DIR}/OptionsGetoptWin.c
    ${TRACEHUB_APP_DIR}/ServicePlan.c
    ${TRACEHUB_APP_DIR}/TraceHubSignal.c
    ${TRACEHUB_APP_DIR}/RunLoop.c
    ${TRACEHUB_NETWORK_DIR}/Socket.c
    ${TRACEHUB_RTT_DIR}/RTTMemory.c
    ${RTT_MEMORY_BACKEND_SOURCE}
    ${SEGGER_RTT_SOURCE_DIR}/SEGGER_RTT.c
    ${TRACEHUB_LOG_DIR}/LogMain.c
    ${TRACEHUB_LOG_DIR}/LogFile.c
    ${TRACEHUB_LOG_DIR}/LogText.c
    ${TRACEHUB_LOG_DIR}/LogHexdump.c
    ${TRACEHUB_LOG_DIR}/LogSwimLane.c
    ${TRACEHUB_CORE_DIR}/ByteQueue.c
    ${TRACEHUB_CORE_DIR}/Utils.c
    ${TRACEHUB_CORE_DIR}/SYS.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeApi.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeRegion.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeChannel.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeState.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderState.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderRegistry.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderFile.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderConsumer.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderRuntime.c
    ${TRACEHUB_LOG_DIR}/CoreLogRecorderStats.c
    ${TRACEHUB_NETWORK_DIR}/TelnetCodec.c
    ${TRACEHUB_SERVICES_DIR}/TerminalService.c
    ${TRACEHUB_SERVICES_DIR}/TerminalState.c
    ${TRACEHUB_SERVICES_DIR}/TerminalConsole.c
    ${TRACEHUB_SERVICES_DIR}/TerminalRuntime.c
    ${TRACEHUB_SERVICES_DIR}/SystemViewService.c
    ${TRACEHUB_SERVICES_DIR}/SystemViewState.c
    ${TRACEHUB_SERVICES_DIR}/SystemViewHandshake.c
    ${TRACEHUB_SERVICES_DIR}/SystemViewRecord.c
    ${TRACEHUB_SERVICES_DIR}/SystemViewRuntime.c
    ${TRACEHUB_LOG_DIR}/LogEntry.c
    ${TRACEHUB_LOG_DIR}/LogLineParser.c
    ${TRACEHUB_LOG_DIR}/LogCollectorStorage.c
    ${TRACEHUB_LOG_DIR}/LogCollectorError.c
    ${TRACEHUB_LOG_DIR}/LogCollectorConsumer.c
    ${TRACEHUB_LOG_DIR}/LogCollectorApi.c
    ${TRACEHUB_LOG_DIR}/LogCollectorSource.c
    ${TRACEHUB_LOG_DIR}/LogCollectorRuntime.c
    ${TRACEHUB_LOG_DIR}/LogMergerBuffer.c
    ${TRACEHUB_LOG_DIR}/LogMergerWatermark.c
    ${TRACEHUB_LOG_DIR}/LogMergerOutput.c
    ${TRACEHUB_LOG_DIR}/LogMergerFile.c
    ${TRACEHUB_LOG_DIR}/LogMergerApi.c
    ${TRACEHUB_LOG_DIR}/SwimLaneLayout.c
    ${TRACEHUB_LOG_DIR}/SwimLaneText.c
    ${TRACEHUB_LOG_DIR}/SwimLaneCell.c
    ${TRACEHUB_LOG_DIR}/SwimLaneRendererApi.c
)

# ==============================================================================
# Create Executable
# ==============================================================================

add_executable(${TRACEHUB_TARGET} ${SOURCES})

# ==============================================================================
# Configure Installation Directories
# ==============================================================================

set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "Binary install directory" FORCE)
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Library install directory" FORCE)
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "Header install directory" FORCE)
set(CMAKE_INSTALL_DATADIR "share" CACHE PATH "Data install directory" FORCE)

# ==============================================================================
# Find Required Packages
# ==============================================================================

find_package(Threads REQUIRED)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(${TRACEHUB_TARGET}
    PRIVATE
        ${TRACEHUB_APP_DIR}
        ${TRACEHUB_CORE_DIR}
        ${TRACEHUB_LOG_DIR}
        ${TRACEHUB_NETWORK_DIR}
        ${TRACEHUB_RTT_DIR}
        ${TRACEHUB_SERVICES_DIR}
        ${SEGGER_RTT_SOURCE_DIR}
        ${SEGGER_RTT_SEGGER_DIR}
        ${SEGGER_SYSVIEW_SOURCE_DIR}
        ${SHAREDMEM_UAPI_DIR}
)

# ==============================================================================
# Link Libraries
# ==============================================================================

target_link_libraries(${TRACEHUB_TARGET}
    PRIVATE
        Threads::Threads
        $<$<NOT:$<PLATFORM_ID:Windows>>:m>
        $<$<PLATFORM_ID:Linux>:rt>
        $<$<PLATFORM_ID:Windows>:ws2_32>
)

target_compile_definitions(${TRACEHUB_TARGET}
    PRIVATE
        TRACEHUB_VERSION=\"${PROJECT_VERSION}\"
        TRACEHUB_DEFAULT_RTT_ADDR=${TRACEHUB_DEFAULT_RTT_ADDR}
        TRACEHUB_DEFAULT_RTT_SIZE=${TRACEHUB_DEFAULT_RTT_SIZE}
        TRACEHUB_DEFAULT_MEMORY_PATH=\"${TRACEHUB_EFFECTIVE_DEFAULT_MEMORY_PATH}\"
        RTT_BRIDGE_DEFAULT_LINUX_CHANNEL=${TRACEHUB_DEFAULT_LINUX_CHANNEL}
        RTT_BRIDGE_DEFAULT_RTOS_CHANNEL=${TRACEHUB_DEFAULT_RTOS_CHANNEL}
        RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL=${TRACEHUB_DEFAULT_TERMINAL_CHANNEL}
        RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL=${TRACEHUB_DEFAULT_SYSVIEW_CHANNEL}
        LOG_SOURCE_DEFAULT_LINUX_LABEL=\"${TRACEHUB_DEFAULT_LINUX_LABEL}\"
        LOG_SOURCE_DEFAULT_RTOS_LABEL=\"${TRACEHUB_DEFAULT_RTOS_LABEL}\"
        CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE=${TRACEHUB_CORE_LOG_QUEUE_SIZE}
        TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE=${TRACEHUB_TERMINAL_QUEUE_SIZE}
        SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE=${TRACEHUB_SYSTEMVIEW_QUEUE_SIZE}
        SWIMLANE_DEFAULT_TOTAL_WIDTH=${TRACEHUB_SWIMLANE_TOTAL_WIDTH}
        SWIMLANE_DEFAULT_TIMESTAMP_WIDTH=${TRACEHUB_SWIMLANE_TIMESTAMP_WIDTH}
        LOG_COLLECTOR_DEFAULT_PENDING_UNTIMED_SIZE=${TRACEHUB_LOG_COLLECTOR_PENDING_UNTIMED_SIZE}
        LOG_MERGER_DEFAULT_FLUSH_THRESHOLD=${TRACEHUB_LOG_MERGER_FLUSH_THRESHOLD}
        LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS=${TRACEHUB_LOG_MERGER_FLUSH_TIMEOUT_MS}
        ${RTT_MEMORY_BACKEND_DEFINITIONS}
        ${RTT_MEMORY_CHANNEL_DEFINITIONS}
        ${RTT_MEMORY_COMMON_DEFINITIONS}
)

# ==============================================================================
# Install Rules
# ==============================================================================

install(TARGETS ${TRACEHUB_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)

# ==============================================================================
# Build Summary
# ==============================================================================

message(STATUS "==============================================================================")
message(STATUS "TraceHub Configuration Summary")
message(STATUS "==============================================================================")
message(STATUS "  Project:              ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  Target:               ${TRACEHUB_TARGET}")
message(STATUS "  Build Type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Code Coverage:        ${ENABLE_COVERAGE}")
message(STATUS "  Sanitizers:           ${ENABLE_SANITIZERS}")
message(STATUS "  Default Memory Path:  ${TRACEHUB_EFFECTIVE_DEFAULT_MEMORY_PATH}")
message(STATUS "  Default RTT Address:  ${TRACEHUB_DEFAULT_RTT_ADDR}")
message(STATUS "  Default RTT Size:     ${TRACEHUB_DEFAULT_RTT_SIZE}")
message(STATUS "  Linux Channel:        ${TRACEHUB_DEFAULT_LINUX_CHANNEL}")
message(STATUS "  RTOS Channel:         ${TRACEHUB_DEFAULT_RTOS_CHANNEL}")
message(STATUS "  Terminal Channel:     ${TRACEHUB_DEFAULT_TERMINAL_CHANNEL}")
message(STATUS "  SystemView Channel:   ${TRACEHUB_DEFAULT_SYSVIEW_CHANNEL}")
message(STATUS "  Linux Label:          ${TRACEHUB_DEFAULT_LINUX_LABEL}")
message(STATUS "  RTOS Label:           ${TRACEHUB_DEFAULT_RTOS_LABEL}")
message(STATUS "  Core Log Queue:       ${TRACEHUB_CORE_LOG_QUEUE_SIZE}")
message(STATUS "  Terminal Queue:       ${TRACEHUB_TERMINAL_QUEUE_SIZE}")
message(STATUS "  SystemView Queue:     ${TRACEHUB_SYSTEMVIEW_QUEUE_SIZE}")
message(STATUS "  Swimlane Width:       ${TRACEHUB_SWIMLANE_TOTAL_WIDTH}")
message(STATUS "  Swimlane Time Width:  ${TRACEHUB_SWIMLANE_TIMESTAMP_WIDTH}")
message(STATUS "  Untimed Log Staging:  ${TRACEHUB_LOG_COLLECTOR_PENDING_UNTIMED_SIZE}")
message(STATUS "  Merger Flush Count:   ${TRACEHUB_LOG_MERGER_FLUSH_THRESHOLD}")
message(STATUS "  Merger Flush Timeout: ${TRACEHUB_LOG_MERGER_FLUSH_TIMEOUT_MS}")
if(USE_MEMSHM_BACKEND)
    message(STATUS "  Memory Backend:       MEMSHM (host shared memory)")
else()
    message(STATUS "  Memory Backend:       SMEM (SharedMem driver)")
endif()
message(STATUS "  Unit Tests:           ${BUILD_UNIT_TESTS}")
message(STATUS "  Simulation Tests:     ${BUILD_SIM_TESTS}")
message(STATUS "  C Standard:           ${CMAKE_C_STANDARD}")
message(STATUS "")
message(STATUS "Linked Libraries:")
message(STATUS "  - Threads::Threads")
if(NOT WIN32)
    message(STATUS "  - m (math library)")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "  - rt (realtime library)")
endif()
if(WIN32)
    message(STATUS "  - ws2_32 (Winsock)")
endif()
message(STATUS "==============================================================================")

# ==============================================================================
# Test Programs
# ==============================================================================

if(BUILD_UNIT_TESTS OR BUILD_SIM_TESTS)
    message(STATUS "Building TraceHub test programs")
    add_subdirectory(Tests)
endif()
