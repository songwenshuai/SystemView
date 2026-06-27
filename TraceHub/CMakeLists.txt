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
option(USE_MEMSHM_BACKEND "Use POSIX shared memory backend for host simulation (default: MEMSHM)" ON)

# ==============================================================================
# Global CMake Settings
# ==============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SMEM depends on Linux SharedMem driver headers and is not available on macOS.
if(APPLE AND NOT USE_MEMSHM_BACKEND)
    set(USE_MEMSHM_BACKEND ON CACHE BOOL "Use POSIX shared memory backend" FORCE)
endif()

if(USE_MEMSHM_BACKEND)
    set(BUILD_SIM_TESTS_DEFAULT ON)
else()
    set(BUILD_SIM_TESTS_DEFAULT OFF)
endif()
option(BUILD_SIM_TESTS "Build RTT simulation test programs" ${BUILD_SIM_TESTS_DEFAULT})
if(BUILD_SIM_TESTS AND NOT USE_MEMSHM_BACKEND)
    message(FATAL_ERROR "BUILD_SIM_TESTS requires USE_MEMSHM_BACKEND=ON because simulation programs use the MEMSHM backend")
endif()

set(SEGGER_RTT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../RTT/RTT")
set(SHAREDMEM_UAPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SharedMem/UAPI")

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
    set(CMAKE_C_FLAGS "-Wall" CACHE STRING "C compiler flags" FORCE)
endif()

# Debug-specific flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g2 -ggdb)
    add_compile_definitions(DEBUG)
endif()

# Release-specific flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3)
endif()

# Treat warnings as errors for native builds (not cross-compilation)
if(UNIX AND NOT CMAKE_CROSSCOMPILING)
    add_compile_options(-Werror)
endif()

# Code coverage support
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")
    add_compile_options(-fprofile-arcs -ftest-coverage -O0)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# Sanitizers support
if(ENABLE_SANITIZERS)
    message(STATUS "Address and undefined behavior sanitizers enabled")
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Memory backend selection
if(USE_MEMSHM_BACKEND)
    message(STATUS "Using MEMSHM backend (POSIX shared memory)")
    set(RTT_MEMORY_BACKEND_DEFINITIONS RTTMEM_USE_MEMSHM=1)
    set(RTT_MEMORY_BACKEND_SOURCE RTTMemory_memshm.c)
    # Enable Linux, RTOS, and SystemView channels for simulation testing
    set(RTT_MEMORY_CHANNEL_DEFINITIONS
        SEGGER_RTT_MAX_NUM_UP_BUFFERS=3
        SEGGER_RTT_MAX_NUM_DOWN_BUFFERS=3
    )
else()
    message(STATUS "Using SMEM backend (SharedMem driver)")
    set(RTT_MEMORY_BACKEND_DEFINITIONS RTTMEM_USE_SMEM=1)
    set(RTT_MEMORY_CHANNEL_DEFINITIONS)
    set(RTT_MEMORY_BACKEND_SOURCE RTTMemory_smem.c)
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
    main.c
    CLI.c
    Socket.c
    RTTMemory.c
    ${RTT_MEMORY_BACKEND_SOURCE}
    ${SEGGER_RTT_SOURCE_DIR}/SEGGER_RTT.c
    Log.c
    Utils.c
    SYS.c
    RTTBridge.c
    CoreLogRecorder.c
    TelnetCodec.c
    Terminal.c
    SystemView.c
    LogEntry.c
    LogLineParser.c
    LogCollector.c
    LogMerger.c
    SwimLaneRenderer.c
)

# ==============================================================================
# Create Executable
# ==============================================================================

add_executable(${TRACEHUB_TARGET} ${SOURCES})

# ==============================================================================
# Configure Installation Directories
# ==============================================================================

set(CMAKE_INSTALL_BINDIR "usershell/TraceHub/bin" CACHE PATH "Binary install directory" FORCE)
set(CMAKE_INSTALL_LIBDIR "usershell/TraceHub/lib" CACHE PATH "Library install directory" FORCE)
set(CMAKE_INSTALL_INCLUDEDIR "usershell/TraceHub/include" CACHE PATH "Header install directory" FORCE)
set(CMAKE_INSTALL_DATADIR "usershell/TraceHub/share" CACHE PATH "Data install directory" FORCE)

# ==============================================================================
# Find Required Packages
# ==============================================================================

find_package(Threads REQUIRED)

# ==============================================================================
# Include Directories
# ==============================================================================

target_include_directories(${TRACEHUB_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SEGGER_RTT_SOURCE_DIR}
        ${SHAREDMEM_UAPI_DIR}
)

# ==============================================================================
# Link Libraries
# ==============================================================================

target_link_libraries(${TRACEHUB_TARGET}
    PRIVATE
        Threads::Threads
        m
        $<$<PLATFORM_ID:Linux>:rt>  # POSIX shared memory (shm_open/shm_unlink)
)

target_compile_definitions(${TRACEHUB_TARGET}
    PRIVATE
        ${RTT_MEMORY_BACKEND_DEFINITIONS}
        ${RTT_MEMORY_CHANNEL_DEFINITIONS}
        ${RTT_MEMORY_COMMON_DEFINITIONS}
)

# ==============================================================================
# Install Rules
# ==============================================================================

install(TARGETS ${TRACEHUB_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/usershell/TraceHub/bin
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
if(USE_MEMSHM_BACKEND)
    message(STATUS "  Memory Backend:       MEMSHM (POSIX shared memory)")
else()
    message(STATUS "  Memory Backend:       SMEM (SharedMem driver)")
endif()
message(STATUS "  Simulation Tests:     ${BUILD_SIM_TESTS}")
message(STATUS "  C Standard:           ${CMAKE_C_STANDARD}")
message(STATUS "")
message(STATUS "Linked Libraries:")
message(STATUS "  - Threads::Threads")
message(STATUS "  - m (math library)")
message(STATUS "==============================================================================")

# ==============================================================================
# RTT Simulation Test Programs
# ==============================================================================

if(BUILD_SIM_TESTS)
    message(STATUS "Building RTT simulation test programs")
    add_subdirectory(Tests)
endif()
