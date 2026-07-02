# ==============================================================================
# RTT Simulation Test Programs
# ==============================================================================
#
# These test programs simulate heterogeneous multi-core system for RTT testing:
#   - rtt_sim_linux : Simulates Linux logs and owns RTT control block init
#   - rtt_sim_rtos  : Simulates RTOS logs and waits for RTT control block
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
set(TRACEHUB_TEST_COMMON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Unit/TestCommon.c)

# Common source files needed by test programs
set(COMMON_SOURCES
    ${TRACEHUB_RTT_DIR}/RTTMemory.c
    ${TRACEHUB_RTT_DIR}/RTTMemory_memshm.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeApi.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeRegion.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeChannel.c
    ${TRACEHUB_RTT_DIR}/RTTBridgeState.c
    ${TRACEHUB_LOG_DIR}/LogMain.c
    ${TRACEHUB_LOG_DIR}/LogFile.c
    ${TRACEHUB_LOG_DIR}/LogText.c
    ${TRACEHUB_LOG_DIR}/LogHexdump.c
    ${TRACEHUB_LOG_DIR}/LogSwimLane.c
    ${TRACEHUB_CORE_DIR}/Utils.c
    ${TRACEHUB_CORE_DIR}/SYS.c
    ${SEGGER_RTT_SOURCE_DIR}/SEGGER_RTT.c
)

# Include directories
set(COMMON_INCLUDES
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
)

# Compile definitions for Linux and RTOS RTT text channel support
set(COMMON_DEFINITIONS
    SEGGER_RTT_MAX_NUM_UP_BUFFERS=2
    SEGGER_RTT_MAX_NUM_DOWN_BUFFERS=2
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
        ${TRACEHUB_LOG_DIR}/LogCollectorStorage.c
        ${TRACEHUB_LOG_DIR}/LogCollectorError.c
        ${TRACEHUB_LOG_DIR}/LogCollectorConsumer.c
        ${TRACEHUB_LOG_DIR}/LogCollectorApi.c
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
        ${TRACEHUB_LOG_DIR}/LogMergerBuffer.c
        ${TRACEHUB_LOG_DIR}/LogMergerWatermark.c
        ${TRACEHUB_LOG_DIR}/LogMergerOutput.c
        ${TRACEHUB_LOG_DIR}/LogMergerFile.c
        ${TRACEHUB_LOG_DIR}/LogMergerApi.c
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
        ${TRACEHUB_LOG_DIR}/SwimLaneLayout.c
        ${TRACEHUB_LOG_DIR}/SwimLaneText.c
        ${TRACEHUB_LOG_DIR}/SwimLaneCell.c
        ${TRACEHUB_LOG_DIR}/SwimLaneRendererApi.c
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
        ${TRACEHUB_TEST_COMMON_SOURCE}
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
        ${TRACEHUB_RTT_DIR}/RTTBridgeApi.c
        ${TRACEHUB_RTT_DIR}/RTTBridgeRegion.c
        ${TRACEHUB_RTT_DIR}/RTTBridgeChannel.c
        ${TRACEHUB_RTT_DIR}/RTTBridgeState.c
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

    add_executable(test_app_options
        Unit/AppOptionsTest.c
        ${TRACEHUB_TEST_COMMON_SOURCE}
        ${TRACEHUB_APP_DIR}/OptionsApi.c
        ${TRACEHUB_APP_DIR}/OptionsParse.c
        ${TRACEHUB_APP_DIR}/OptionsGetoptWin.c
        ${TRACEHUB_APP_DIR}/CLI.c
    )

    target_include_directories(test_app_options PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_app_options PRIVATE ${COMMON_DEFINITIONS})
    add_test(NAME test_app_options COMMAND test_app_options)

    add_executable(test_app_service_plan
        Unit/AppServicePlanTest.c
        ${TRACEHUB_TEST_COMMON_SOURCE}
        ${TRACEHUB_APP_DIR}/ServicePlan.c
    )

    target_include_directories(test_app_service_plan PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_app_service_plan PRIVATE ${COMMON_DEFINITIONS})
    add_test(NAME test_app_service_plan COMMAND test_app_service_plan)

    add_executable(test_app_runloop
        Unit/AppRunLoopTest.c
        ${TRACEHUB_TEST_COMMON_SOURCE}
        ${TRACEHUB_APP_DIR}/CLI.c
        ${TRACEHUB_APP_DIR}/TraceHubSignal.c
        ${TRACEHUB_APP_DIR}/RunLoop.c
    )

    target_include_directories(test_app_runloop PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_app_runloop PRIVATE ${COMMON_DEFINITIONS})
    add_test(NAME test_app_runloop COMMAND test_app_runloop)

    add_executable(test_corelog_recorder_api
        Unit/CoreLogRecorderApiTest.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderState.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderRegistry.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderFile.c
        ${TRACEHUB_LOG_DIR}/CoreLogRecorderConsumer.c
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

    add_executable(test_terminal_api
        Unit/TerminalApiTest.c
        ${TRACEHUB_TEST_COMMON_SOURCE}
        ${TRACEHUB_SERVICES_DIR}/TerminalService.c
        ${TRACEHUB_SERVICES_DIR}/TerminalState.c
        ${TRACEHUB_CORE_DIR}/ByteQueue.c
    )

    target_include_directories(test_terminal_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_terminal_api PRIVATE
        TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE=8192u
    )
    add_test(NAME test_terminal_api COMMAND test_terminal_api)

    add_executable(test_systemview_api
        Unit/SystemViewApiTest.c
        ${TRACEHUB_TEST_COMMON_SOURCE}
        ${TRACEHUB_SERVICES_DIR}/SystemViewService.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewState.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewHandshake.c
        ${TRACEHUB_SERVICES_DIR}/SystemViewRecord.c
        ${TRACEHUB_CORE_DIR}/ByteQueue.c
    )

    target_include_directories(test_systemview_api PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(test_systemview_api PRIVATE
        SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE=8192u
    )
    add_test(NAME test_systemview_api COMMAND test_systemview_api)
endif()

# ==============================================================================
# Build Summary
# ==============================================================================

message(STATUS "==============================================================================")
message(STATUS "TraceHub Test Programs")
message(STATUS "==============================================================================")
message(STATUS "  TraceHub Source Dir:  ${TRACEHUB_SOURCE_DIR}")
if(BUILD_SIM_TESTS)
    message(STATUS "  UP Buffers:      2")
    message(STATUS "  DOWN Buffers:    2")
    message(STATUS "  Memory Backend:  MEMSHM (host shared memory)")
endif()
message(STATUS "  Unit Tests:      ${BUILD_UNIT_TESTS}")
message(STATUS "  Simulation:      ${BUILD_SIM_TESTS}")
message(STATUS "")
message(STATUS "Targets:")
if(BUILD_SIM_TESTS)
    message(STATUS "  - rtt_sim_linux : Linux text log simulator (owns RTT CB init)")
    message(STATUS "  - rtt_sim_rtos  : RTOS text log simulator (waits for RTT CB)")
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
    message(STATUS "  - test_app_options : application option parser checks")
    message(STATUS "  - test_app_service_plan : application service plan checks")
    message(STATUS "  - test_app_runloop : CLI, signal, and run loop API checks")
    message(STATUS "  - test_corelog_recorder_api : core log recorder API checks")
    message(STATUS "  - test_terminal_api : Terminal service API checks")
    message(STATUS "  - test_systemview_api : SystemView service API checks")
endif()
message(STATUS "==============================================================================")
