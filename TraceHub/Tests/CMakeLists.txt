# ==============================================================================
# RTT Simulation Test Programs
# ==============================================================================
#
# These test programs simulate heterogeneous multi-core system for RTT testing:
#   - rtt_sim_linux : Simulates Linux logs and owns RTT control block init
#   - rtt_sim_rtos  : Simulates RTOS logs and waits for RTT control block
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

if(NOT DEFINED BUILD_UNIT_TESTS)
    option(BUILD_UNIT_TESTS "Build TraceHub unit test programs" ON)
endif()

if(NOT DEFINED BUILD_SIM_TESTS)
    if(WIN32)
        set(BUILD_SIM_TESTS_DEFAULT OFF)
    else()
        set(BUILD_SIM_TESTS_DEFAULT ON)
    endif()
    option(BUILD_SIM_TESTS "Build RTT simulation test programs" ${BUILD_SIM_TESTS_DEFAULT})
endif()

if(BUILD_SIM_TESTS AND WIN32)
    message(FATAL_ERROR "RTT simulation programs use POSIX process and socket APIs and are not supported on Windows")
endif()

# ==============================================================================
# Common Settings
# ==============================================================================

find_package(Threads REQUIRED)

set(TRACEHUB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(TRACEHUB_APP_DIR ${TRACEHUB_SOURCE_DIR}/App)
set(TRACEHUB_CORE_DIR ${TRACEHUB_SOURCE_DIR}/Core)
set(TRACEHUB_LOG_DIR ${TRACEHUB_SOURCE_DIR}/Log)
set(TRACEHUB_NETWORK_DIR ${TRACEHUB_SOURCE_DIR}/Network)
set(TRACEHUB_RTT_DIR ${TRACEHUB_SOURCE_DIR}/RTT)
set(TRACEHUB_SERVICES_DIR ${TRACEHUB_SOURCE_DIR}/Services)
set(SEGGER_RTT_SOURCE_DIR ${TRACEHUB_SOURCE_DIR}/../RTT/RTT)
set(SEGGER_RTT_SEGGER_DIR ${TRACEHUB_SOURCE_DIR}/../RTT/SEGGER)
set(SEGGER_SYSVIEW_SOURCE_DIR ${TRACEHUB_SOURCE_DIR}/../SystemView/SYSVIEW)
set(SEGGER_SOURCE_DIR ${TRACEHUB_SOURCE_DIR}/../SystemView/SEGGER)
set(SEGGER_CONFIG_DIR ${TRACEHUB_SOURCE_DIR}/../SystemView/Config)
set(SYSVIEW_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SystemViewConfig)

# Common source files needed by test programs
set(COMMON_SOURCES
    ${TRACEHUB_RTT_DIR}/RTTMemory.c
    ${TRACEHUB_RTT_DIR}/RTTMemory_memshm.c
    ${TRACEHUB_RTT_DIR}/RTTBridge.c
    ${TRACEHUB_LOG_DIR}/LogMain.c
    ${TRACEHUB_LOG_DIR}/LogFile.c
    ${TRACEHUB_LOG_DIR}/LogText.c
    ${TRACEHUB_LOG_DIR}/LogHexdump.c
    ${TRACEHUB_LOG_DIR}/LogSwimLane.c
    ${TRACEHUB_CORE_DIR}/Utils.c
    ${TRACEHUB_CORE_DIR}/SYS.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Sim/RTTSimSystemView.c
    ${SEGGER_RTT_SOURCE_DIR}/SEGGER_RTT.c
    ${SEGGER_SYSVIEW_SOURCE_DIR}/SEGGER_SYSVIEW.c
)

# Include directories
set(COMMON_INCLUDES
    ${SYSVIEW_CONFIG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Sim
    ${TRACEHUB_APP_DIR}
    ${TRACEHUB_CORE_DIR}
    ${TRACEHUB_LOG_DIR}
    ${TRACEHUB_NETWORK_DIR}
    ${TRACEHUB_RTT_DIR}
    ${TRACEHUB_SERVICES_DIR}
    ${SEGGER_RTT_SOURCE_DIR}
    ${SEGGER_RTT_SEGGER_DIR}
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
    $<$<PLATFORM_ID:Windows>:ws2_32>
)

# ==============================================================================
# RTT Simulation Programs
# ==============================================================================

if(BUILD_SIM_TESTS)
    add_executable(rtt_sim_rtos
        Sim/RTTSimRTOS.c
        ${COMMON_SOURCES}
    )

    target_include_directories(rtt_sim_rtos PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(rtt_sim_rtos PRIVATE ${COMMON_DEFINITIONS})
    target_link_libraries(rtt_sim_rtos PRIVATE ${COMMON_LIBRARIES})

    add_executable(rtt_sim_linux
        Sim/RTTSimLinux.c
        ${COMMON_SOURCES}
    )

    target_include_directories(rtt_sim_linux PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(rtt_sim_linux PRIVATE ${COMMON_DEFINITIONS})
    target_link_libraries(rtt_sim_linux PRIVATE ${COMMON_LIBRARIES})

    add_executable(rtt_sim_sysview_tcp_client
        Sim/RTTSimSystemViewTCPClient.c
    )

    target_include_directories(rtt_sim_sysview_tcp_client PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(rtt_sim_sysview_tcp_client PRIVATE ${COMMON_LIBRARIES})
endif()

# ==============================================================================
# Unit Checks
# ==============================================================================

if(BUILD_UNIT_TESTS)
    add_executable(test_logline_parser
        Unit/LogLineParserTest.c
        ${TRACEHUB_LOG_DIR}/LogLineParser.c
    )

    target_include_directories(test_logline_parser PRIVATE ${COMMON_INCLUDES})
    add_test(NAME test_logline_parser COMMAND test_logline_parser)

    add_executable(test_logcollector
        Unit/LogCollectorTest.c
        ${TRACEHUB_LOG_DIR}/LogCollectorState.c
        ${TRACEHUB_LOG_DIR}/LogCollectorSource.c
        ${TRACEHUB_LOG_DIR}/LogCollectorRuntime.c
        ${TRACEHUB_LOG_DIR}/LogLineParser.c
        ${TRACEHUB_LOG_DIR}/LogEntry.c
    )

    target_include_directories(test_logcollector PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(test_logcollector PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_logcollector COMMAND test_logcollector)

    add_executable(test_logmerger
        Unit/LogMergerTest.c
        ${TRACEHUB_LOG_DIR}/LogMerger.c
        ${TRACEHUB_LOG_DIR}/LogEntry.c
        ${TRACEHUB_LOG_DIR}/LogMain.c
        ${TRACEHUB_LOG_DIR}/LogFile.c
        ${TRACEHUB_LOG_DIR}/LogText.c
        ${TRACEHUB_LOG_DIR}/LogSwimLane.c
        ${TRACEHUB_CORE_DIR}/SYS.c
        ${TRACEHUB_CORE_DIR}/Utils.c
    )

    target_include_directories(test_logmerger PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_logmerger PRIVATE
        LOG_MERGER_DEFAULT_FLUSH_THRESHOLD=2
        LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS=100
    )
    target_link_libraries(test_logmerger PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_logmerger COMMAND test_logmerger)

    add_executable(test_swimlane_renderer
        Unit/SwimLaneRendererTest.c
        ${TRACEHUB_LOG_DIR}/LogEntry.c
    )

    target_include_directories(test_swimlane_renderer PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(test_swimlane_renderer PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_swimlane_renderer COMMAND test_swimlane_renderer)

    add_executable(test_core_api
        Unit/CoreApiTest.c
        ${TRACEHUB_CORE_DIR}/ByteQueue.c
        ${TRACEHUB_CORE_DIR}/SYS.c
        ${TRACEHUB_CORE_DIR}/Utils.c
    )

    target_include_directories(test_core_api PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(test_core_api PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_core_api COMMAND test_core_api)

    add_executable(test_logentry
        Unit/LogEntryTest.c
        ${TRACEHUB_LOG_DIR}/LogEntry.c
    )

    target_include_directories(test_logentry PRIVATE ${COMMON_INCLUDES})
    add_test(NAME test_logentry COMMAND test_logentry)

    add_executable(test_log_api
        Unit/LogApiTest.c
        ${TRACEHUB_LOG_DIR}/LogMain.c
        ${TRACEHUB_LOG_DIR}/LogFile.c
        ${TRACEHUB_LOG_DIR}/LogText.c
        ${TRACEHUB_LOG_DIR}/LogHexdump.c
        ${TRACEHUB_LOG_DIR}/LogSwimLane.c
        ${TRACEHUB_CORE_DIR}/SYS.c
        ${TRACEHUB_CORE_DIR}/Utils.c
    )

    target_include_directories(test_log_api PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(test_log_api PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_log_api COMMAND test_log_api)

    add_executable(test_network_api
        Unit/NetworkApiTest.c
        ${TRACEHUB_NETWORK_DIR}/Socket.c
        ${TRACEHUB_NETWORK_DIR}/TelnetCodec.c
    )

    target_include_directories(test_network_api PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(test_network_api PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_network_api COMMAND test_network_api)

    add_executable(test_rtt_api
        Unit/RTTApiTest.c
        ${TRACEHUB_RTT_DIR}/RTTMemory.c
        ${TRACEHUB_RTT_DIR}/RTTBridge.c
    )

    target_include_directories(test_rtt_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_rtt_api PRIVATE ${COMMON_DEFINITIONS})
    add_test(NAME test_rtt_api COMMAND test_rtt_api)

    add_executable(test_rtt_memshm_backend
        Unit/RTTMemoryMemshmBackendTest.c
        ${TRACEHUB_RTT_DIR}/RTTMemory.c
        ${TRACEHUB_RTT_DIR}/RTTMemory_memshm.c
    )

    target_include_directories(test_rtt_memshm_backend PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_rtt_memshm_backend PRIVATE ${COMMON_DEFINITIONS})
    target_link_libraries(test_rtt_memshm_backend PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_rtt_memshm_backend COMMAND test_rtt_memshm_backend)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(test_rtt_smem_backend
            Unit/RTTMemorySmemBackendTest.c
            ${TRACEHUB_RTT_DIR}/RTTMemory.c
            ${TRACEHUB_RTT_DIR}/RTTMemory_smem.c
        )

        target_include_directories(test_rtt_smem_backend PRIVATE ${COMMON_INCLUDES})
        target_compile_definitions(test_rtt_smem_backend PRIVATE RTTMEM_USE_SMEM=1)
        target_link_libraries(test_rtt_smem_backend PRIVATE ${COMMON_LIBRARIES})
        add_test(NAME test_rtt_smem_backend COMMAND test_rtt_smem_backend)
    endif()

    add_executable(test_app_api
        Unit/AppApiTest.c
        ${TRACEHUB_APP_DIR}/Options.c
        ${TRACEHUB_APP_DIR}/ServicePlan.c
        ${TRACEHUB_APP_DIR}/CLI.c
        ${TRACEHUB_APP_DIR}/TraceHubSignal.c
        ${TRACEHUB_APP_DIR}/RunLoop.c
    )

    target_include_directories(test_app_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_app_api PRIVATE ${COMMON_DEFINITIONS})
    add_test(NAME test_app_api COMMAND test_app_api)

    add_executable(test_corelog_recorder_api
        Unit/CoreLogRecorderApiTest.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderState.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderSource.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderRuntime.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderStats.c
        ${TRACEHUB_LOG_DIR}/LogMain.c
        ${TRACEHUB_LOG_DIR}/LogFile.c
        ${TRACEHUB_LOG_DIR}/LogText.c
        ${TRACEHUB_CORE_DIR}/SYS.c
        ${TRACEHUB_CORE_DIR}/Utils.c
    )

    target_include_directories(test_corelog_recorder_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_corelog_recorder_api PRIVATE
        CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE=8192u
    )
    target_link_libraries(test_corelog_recorder_api PRIVATE ${COMMON_LIBRARIES})
    add_test(NAME test_corelog_recorder_api COMMAND test_corelog_recorder_api)

    add_executable(test_services_api
        Unit/ServicesApiTest.c
        ${TRACEHUB_SERVICES_DIR}/TerminalService.c
        ${TRACEHUB_SERVICES_DIR}/TerminalState.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewService.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewState.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewRecord.c
        ${TRACEHUB_CORE_DIR}/ByteQueue.c
    )

    target_include_directories(test_services_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_services_api PRIVATE
        TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE=8192u
        SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE=8192u
    )
    add_test(NAME test_services_api COMMAND test_services_api)
endif()

# ==============================================================================
# Build Summary
# ==============================================================================

message(STATUS "==============================================================================")
message(STATUS "TraceHub Test Programs")
message(STATUS "==============================================================================")
message(STATUS "  TraceHub Source Dir:  ${TRACEHUB_SOURCE_DIR}")
if(BUILD_SIM_TESTS)
    message(STATUS "  UP Buffers:      3")
    message(STATUS "  DOWN Buffers:    3")
    message(STATUS "  Memory Backend:  MEMSHM (host shared memory)")
endif()
message(STATUS "  Unit Tests:      ${BUILD_UNIT_TESTS}")
message(STATUS "  Simulation:      ${BUILD_SIM_TESTS}")
message(STATUS "")
message(STATUS "Targets:")
if(BUILD_SIM_TESTS)
    message(STATUS "  - rtt_sim_linux : Linux simulator (owns RTT CB init, writes SystemView events)")
    message(STATUS "  - rtt_sim_rtos  : RTOS simulator (waits for RTT CB, writes SystemView events)")
    message(STATUS "  - rtt_sim_sysview_tcp_client : SystemView TCP smoke client")
endif()
if(BUILD_UNIT_TESTS)
    message(STATUS "  - test_logline_parser : timestamp parser unit checks")
    message(STATUS "  - test_logcollector : collector timestamp assignment checks")
    message(STATUS "  - test_logmerger : log merger unit checks")
    message(STATUS "  - test_swimlane_renderer : swimlane renderer layout checks")
    message(STATUS "  - test_core_api : core utility API checks")
    message(STATUS "  - test_logentry : log entry API checks")
    message(STATUS "  - test_log_api : logging API checks")
    message(STATUS "  - test_network_api : socket and Telnet codec API checks")
    message(STATUS "  - test_rtt_api : RTT memory and bridge API checks")
    message(STATUS "  - test_rtt_memshm_backend : real MEMSHM backend API checks")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(STATUS "  - test_rtt_smem_backend : Linux SMEM backend API checks")
    endif()
    message(STATUS "  - test_app_api : options, service plan, CLI, signal, and run loop API checks")
    message(STATUS "  - test_corelog_recorder_api : core log recorder API checks")
    message(STATUS "  - test_services_api : Terminal and SystemView service API checks")
endif()
message(STATUS "==============================================================================")
