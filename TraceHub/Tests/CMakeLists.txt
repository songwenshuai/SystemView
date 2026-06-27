# ==============================================================================
# RTT Simulation Test Programs
# ==============================================================================
#
# These test programs simulate heterogeneous multi-core system for RTT testing:
#   - rtt_sim_linux : Simulates Linux A53 core and owns RTT control block init
#   - rtt_sim_rtos  : Simulates RTOS R5 core and waits for RTT control block
# Both programs also generate SEGGER SystemView events on RTT channel 2.
#
# Usage:
#   mkdir build && cd build
#   cmake -DUSE_MEMSHM_BACKEND=ON ..
#   cmake --build .
#
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

project(rtt_sim_test
    VERSION 1.0.0
    DESCRIPTION "RTT Simulation Test Programs"
    LANGUAGES C
)

enable_testing()

# ==============================================================================
# Build Configuration
# ==============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==============================================================================
# Common Settings
# ==============================================================================

find_package(Threads REQUIRED)

# Parent directory contains RTT source files
set(RTT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SEGGER_RTT_SOURCE_DIR ${RTT_SOURCE_DIR}/../RTT/RTT)
set(SEGGER_SYSVIEW_SOURCE_DIR ${RTT_SOURCE_DIR}/../SystemView/SYSVIEW)
set(SEGGER_SOURCE_DIR ${RTT_SOURCE_DIR}/../SystemView/SEGGER)
set(SEGGER_CONFIG_DIR ${RTT_SOURCE_DIR}/../SystemView/Config)
set(SYSVIEW_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SystemViewConfig)

# Common source files needed by test programs
set(COMMON_SOURCES
    ${RTT_SOURCE_DIR}/RTTMemory.c
    ${RTT_SOURCE_DIR}/RTTMemory_memshm.c
    ${RTT_SOURCE_DIR}/RTTBridge.c
    ${RTT_SOURCE_DIR}/Log.c
    ${RTT_SOURCE_DIR}/Utils.c
    ${RTT_SOURCE_DIR}/SYS.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RTTSimSystemView.c
    ${SEGGER_RTT_SOURCE_DIR}/SEGGER_RTT.c
    ${SEGGER_SYSVIEW_SOURCE_DIR}/SEGGER_SYSVIEW.c
)

# Include directories
set(COMMON_INCLUDES
    ${SYSVIEW_CONFIG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RTT_SOURCE_DIR}
    ${SEGGER_RTT_SOURCE_DIR}
    ${SEGGER_SYSVIEW_SOURCE_DIR}
    ${SEGGER_SOURCE_DIR}
    ${SEGGER_CONFIG_DIR}
)

# Compile definitions for Linux, RTOS, and SystemView channel support
set(COMMON_DEFINITIONS
    SEGGER_RTT_MAX_NUM_UP_BUFFERS=3
    SEGGER_RTT_MAX_NUM_DOWN_BUFFERS=3
    BUFFER_SIZE_UP=1048576
    BUFFER_SIZE_DOWN=1048576
    SEGGER_RTT_MODE_DEFAULT=SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL
    RTTMEM_USE_MEMSHM=1
)

# Common link libraries
set(COMMON_LIBRARIES
    Threads::Threads
    $<$<PLATFORM_ID:Linux>:rt>
)

# ==============================================================================
# RTOS Simulator
# ==============================================================================

add_executable(rtt_sim_rtos
    RTTSimRTOS.c
    ${COMMON_SOURCES}
)

target_include_directories(rtt_sim_rtos PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(rtt_sim_rtos PRIVATE ${COMMON_DEFINITIONS})
target_link_libraries(rtt_sim_rtos PRIVATE ${COMMON_LIBRARIES})

# ==============================================================================
# Linux Simulator
# ==============================================================================

add_executable(rtt_sim_linux
    RTTSimLinux.c
    ${COMMON_SOURCES}
)

target_include_directories(rtt_sim_linux PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(rtt_sim_linux PRIVATE ${COMMON_DEFINITIONS})
target_link_libraries(rtt_sim_linux PRIVATE ${COMMON_LIBRARIES})

# ==============================================================================
# SystemView TCP Client
# ==============================================================================

add_executable(rtt_sim_sysview_tcp_client
    RTTSimSystemViewTCPClient.c
)

target_include_directories(rtt_sim_sysview_tcp_client PRIVATE ${RTT_SOURCE_DIR})
target_link_libraries(rtt_sim_sysview_tcp_client PRIVATE ${COMMON_LIBRARIES})

# ==============================================================================
# Unit Checks
# ==============================================================================

add_executable(test_logline_parser
    LogLineParserTest.c
    ${RTT_SOURCE_DIR}/LogLineParser.c
)

target_include_directories(test_logline_parser PRIVATE ${RTT_SOURCE_DIR})
add_test(NAME test_logline_parser COMMAND test_logline_parser)

add_executable(test_logcollector
    LogCollectorTest.c
    ${RTT_SOURCE_DIR}/LogLineParser.c
    ${RTT_SOURCE_DIR}/LogEntry.c
)

target_include_directories(test_logcollector PRIVATE ${RTT_SOURCE_DIR})
target_link_libraries(test_logcollector PRIVATE ${COMMON_LIBRARIES})
add_test(NAME test_logcollector COMMAND test_logcollector)

add_executable(test_logmerger
    LogMergerTest.c
    ${RTT_SOURCE_DIR}/LogMerger.c
    ${RTT_SOURCE_DIR}/LogEntry.c
    ${RTT_SOURCE_DIR}/Log.c
    ${RTT_SOURCE_DIR}/SYS.c
    ${RTT_SOURCE_DIR}/Utils.c
)

target_include_directories(test_logmerger PRIVATE ${RTT_SOURCE_DIR})
target_link_libraries(test_logmerger PRIVATE ${COMMON_LIBRARIES})
add_test(NAME test_logmerger COMMAND test_logmerger)

# ==============================================================================
# Build Summary
# ==============================================================================

message(STATUS "==============================================================================")
message(STATUS "RTT Simulation Test Programs")
message(STATUS "==============================================================================")
message(STATUS "  RTT Source Dir:  ${RTT_SOURCE_DIR}")
message(STATUS "  UP Buffers:      3")
message(STATUS "  DOWN Buffers:    3")
message(STATUS "  Memory Backend:  MEMSHM (POSIX shared memory)")
message(STATUS "")
message(STATUS "Targets:")
message(STATUS "  - rtt_sim_linux : Linux A53 simulator (owns RTT CB init, writes SystemView events)")
message(STATUS "  - rtt_sim_rtos  : RTOS R5 simulator (waits for RTT CB, writes SystemView events)")
message(STATUS "  - rtt_sim_sysview_tcp_client : SystemView TCP smoke client")
message(STATUS "  - test_logline_parser : timestamp parser unit checks")
message(STATUS "  - test_logcollector : collector timestamp assignment checks")
message(STATUS "  - test_logmerger : log merger unit checks")
message(STATUS "==============================================================================")
