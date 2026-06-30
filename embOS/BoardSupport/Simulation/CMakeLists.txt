# ==============================================================================
# embOS Simulation - SEGGER embOS Desktop Prebuilt Library Build
# ==============================================================================
#
# Build:
#   ./build.sh
#   ./build.sh --release
#   ./build.sh --x86 --debug
#
# Target width selection:
#   -DEMBOS_SIM_ARCH=native   Use the compiler default target width
#   -DEMBOS_SIM_ARCH=x86      Build the 32-bit simulation profile
#   -DEMBOS_SIM_ARCH=x86_64   Build the 64-bit simulation profile
#
# Application selection:
#   -DEMBOS_SIM_APPLICATION_SOURCE=Application/OS_StartLEDBlink.c
#
# Application paths are relative to the embOS root directory.
#
# Linux and MinGW use -m32/-m64 when EMBOS_SIM_ARCH is set explicitly.
# macOS uses the compiler-selected Apple architecture and supports 64-bit only.
#
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(embOSSimulationStart
    VERSION     1.0.0
    DESCRIPTION "SEGGER embOS desktop simulation prebuilt library build"
    LANGUAGES   C
)

# ==============================================================================
# Build Options
# ==============================================================================

set(EMBOS_SIM_DEFAULT_APPLICATION_SOURCE "Application/OS_StartLEDBlink.c")

set(EMBOS_SIM_ARCH "native" CACHE STRING "Simulation target width")
set_property(CACHE EMBOS_SIM_ARCH PROPERTY STRINGS
    "native" "x86" "x86_64"
)

set(EMBOS_SIM_APPLICATION_SOURCE "${EMBOS_SIM_DEFAULT_APPLICATION_SOURCE}" CACHE FILEPATH
    "Single application source file that provides main()")

set(EMBOS_SIM_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Output" CACHE PATH
    "Directory for simulation executables")

option(EMBOS_SIM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(EMBOS_SIM_ENABLE_COVERAGE "Enable gcov coverage instrumentation" OFF)
option(EMBOS_SIM_ENABLE_ASAN "Enable AddressSanitizer" OFF)

set(EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMBOS_SIM_ENABLE_ASAN)
    set(EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION_DEFAULT ON)
endif()
option(EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION
    "Enable linker-wrapped allocator interposition"
    ${EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION_DEFAULT})
option(EMBOS_SIM_TRACE_TICK "Enable low-rate POSIX tick trace output" OFF)
option(EMBOS_SIM_GUI "Enable graphical simulation window" ON)

# ==============================================================================
# Global CMake Settings
#
# These are the only settings applied globally. Compiler flags, link flags,
# include paths and preprocessor definitions are attached to targets through
# interface libraries.
# ==============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified -- defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

get_filename_component(EMBOS_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    ABSOLUTE
)
get_filename_component(HETEROTRACE_ROOT
    "${EMBOS_ROOT}/.."
    ABSOLUTE
)
set(EMBOS_CONFIG_DIR                       "${EMBOS_ROOT}/Config")
set(EMBOS_INC_DIR                          "${EMBOS_ROOT}/Inc")
set(EMBOS_APPLICATION_DIR                  "${EMBOS_ROOT}/Application")
set(EMBOS_SIM_SEGGER_DIR                   "${CMAKE_CURRENT_SOURCE_DIR}/SEGGER")
set(EMBOS_RTT_ROOT                         "${HETEROTRACE_ROOT}/RTT")
set(EMBOS_SYSVIEW_ROOT                     "${HETEROTRACE_ROOT}/SystemView")
set(EMBOS_SIM_RTT_MEMORY_HEADER            "${EMBOS_SIM_SEGGER_DIR}/SEGGER_RTT_Memory.h")
set(EMBOS_SIM_RTT_MEMORY_INT_HEADER        "${EMBOS_SIM_SEGGER_DIR}/SEGGER_RTT_Memory_Int.h")
set(EMBOS_SIM_RTT_MEMORY_WIN32_SOURCE      "${EMBOS_SIM_SEGGER_DIR}/SEGGER_RTT_Memory_Win32.c")
set(EMBOS_SIM_RTT_MEMORY_POSIX_SOURCE      "${EMBOS_SIM_SEGGER_DIR}/SEGGER_RTT_Memory_Posix.c")
set(EMBOS_SIM_SYSVIEW_EMBOS_SOURCE         "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_embOS.c")
set(EMBOS_SIM_SYSVIEW_CONFIG_SOURCE        "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_Config_embOS.c")
set(EMBOS_SIM_SYSVIEW_WIN32_CONFIG_SOURCE  "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_Config_embOS_Win32.c")
set(EMBOS_SIM_SYSVIEW_POSIX_CONFIG_SOURCE  "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_Config_embOS_Posix.c")

set(EMBOS_SIM_LIBRARY_MODE "" CACHE STRING
    "embOS library mode suffix: xr, r, s, sp, d, dp or dt")
set_property(CACHE EMBOS_SIM_LIBRARY_MODE PROPERTY STRINGS
    "" "xr" "r" "s" "sp" "d" "dp" "dt"
)
set(EMBOS_SIM_PREBUILT_ROOT "${EMBOS_ROOT}/Lib" CACHE PATH
    "Root directory for prebuilt embOS simulation libraries")
set(EMBOS_SIM_PREBUILT_LIBRARY "" CACHE FILEPATH
    "Explicit prebuilt embOS static library path")
set(EMBOS_SIM_PREBUILT_INCLUDE_DIR "" CACHE PATH
    "Explicit prebuilt embOS RTOS.h include directory")

get_filename_component(EMBOS_SIM_DEFAULT_APPLICATION_SOURCE_ABS
    "${EMBOS_SIM_DEFAULT_APPLICATION_SOURCE}"
    ABSOLUTE
    BASE_DIR "${EMBOS_ROOT}"
)
if(IS_ABSOLUTE "${EMBOS_SIM_APPLICATION_SOURCE}")
    set(EMBOS_SIM_APPLICATION_SOURCE_ABS "${EMBOS_SIM_APPLICATION_SOURCE}")
else()
    get_filename_component(EMBOS_SIM_APPLICATION_SOURCE_ABS
        "${EMBOS_SIM_APPLICATION_SOURCE}"
        ABSOLUTE
        BASE_DIR "${EMBOS_ROOT}"
    )
endif()

set(EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE "${EMBOS_SIM_APPLICATION_SOURCE}")
set(EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS
    "${EMBOS_SIM_APPLICATION_SOURCE_ABS}"
)

foreach(_dir IN ITEMS
        "${EMBOS_CONFIG_DIR}"
        "${EMBOS_INC_DIR}"
        "${EMBOS_APPLICATION_DIR}"
        "${EMBOS_RTT_ROOT}"
        "${EMBOS_SYSVIEW_ROOT}"
        "${EMBOS_SIM_SEGGER_DIR}")
    if(NOT IS_DIRECTORY "${_dir}")
        message(FATAL_ERROR "Required embOS directory not found: ${_dir}")
    endif()
endforeach()

foreach(_file IN ITEMS
        "${EMBOS_RTT_ROOT}/CMakeLists.txt"
        "${EMBOS_SYSVIEW_ROOT}/CMakeLists.txt"
        "${EMBOS_SIM_SEGGER_DIR}/SEGGER_RTT_Conf.h"
        "${EMBOS_SIM_RTT_MEMORY_HEADER}"
        "${EMBOS_SIM_RTT_MEMORY_INT_HEADER}"
        "${EMBOS_SIM_RTT_MEMORY_WIN32_SOURCE}"
        "${EMBOS_SIM_RTT_MEMORY_POSIX_SOURCE}"
        "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_Conf.h"
        "${EMBOS_SIM_SYSVIEW_EMBOS_SOURCE}"
        "${EMBOS_SIM_SEGGER_DIR}/SEGGER_SYSVIEW_embOS.h"
        "${EMBOS_SIM_SYSVIEW_CONFIG_SOURCE}"
        "${EMBOS_SIM_SYSVIEW_WIN32_CONFIG_SOURCE}"
        "${EMBOS_SIM_SYSVIEW_POSIX_CONFIG_SOURCE}")
    if(NOT EXISTS "${_file}")
        message(FATAL_ERROR "Required SEGGER file not found: ${_file}")
    endif()
endforeach()

if(NOT EXISTS "${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}")
    message(FATAL_ERROR
        "Application source not found: ${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}")
endif()

file(RELATIVE_PATH EMBOS_SIM_APPLICATION_SOURCE_REL
    "${EMBOS_ROOT}"
    "${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}"
)

get_filename_component(EMBOS_SIM_APPLICATION_EXTENSION
    "${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}"
    EXT
)
string(TOLOWER "${EMBOS_SIM_APPLICATION_EXTENSION}"
    EMBOS_SIM_APPLICATION_EXTENSION_LOWER
)
set(EMBOS_SIM_APPLICATION_IS_CXX FALSE)
if(EMBOS_SIM_APPLICATION_EXTENSION_LOWER STREQUAL ".cc"
        OR EMBOS_SIM_APPLICATION_EXTENSION_LOWER STREQUAL ".cpp"
        OR EMBOS_SIM_APPLICATION_EXTENSION_LOWER STREQUAL ".cxx")
    enable_language(CXX)
    set(EMBOS_SIM_APPLICATION_IS_CXX TRUE)
endif()

# ==============================================================================
# Target Width
# ==============================================================================

if(EMBOS_SIM_ARCH STREQUAL "native")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(EMBOS_SIM_TARGET_BITS 64)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(EMBOS_SIM_TARGET_BITS 32)
    else()
        message(FATAL_ERROR "Unable to determine native target width")
    endif()
elseif(EMBOS_SIM_ARCH STREQUAL "x86")
    set(EMBOS_SIM_TARGET_BITS 32)
elseif(EMBOS_SIM_ARCH STREQUAL "x86_64")
    set(EMBOS_SIM_TARGET_BITS 64)
else()
    message(FATAL_ERROR "Unsupported EMBOS_SIM_ARCH: ${EMBOS_SIM_ARCH}")
endif()

if(EMBOS_SIM_TARGET_BITS EQUAL 64)
    set(EMBOS_SIM_IS_64_BIT TRUE)
    set(EMBOS_SIM_TARGET_BITS_LABEL "64-bit")
else()
    set(EMBOS_SIM_IS_64_BIT FALSE)
    set(EMBOS_SIM_TARGET_BITS_LABEL "32-bit")
endif()

if(APPLE AND EMBOS_SIM_TARGET_BITS EQUAL 32)
    message(FATAL_ERROR "macOS embOS simulation supports 64-bit builds only")
endif()

if(NOT EMBOS_SIM_IS_64_BIT)
    set(RTT_BUILD_LIBRARY        ON  CACHE BOOL "Build RTT library for embOS simulation" FORCE)
    set(RTT_BUILD_TESTS          OFF CACHE BOOL "Do not build RTT tests from embOS simulation" FORCE)
    set(RTT_BUILD_EXAMPLES       OFF CACHE BOOL "Do not build RTT examples from embOS simulation" FORCE)
    set(RTT_CONFIG_DIR           "${EMBOS_SIM_SEGGER_DIR}" CACHE PATH "embOS simulation RTT configuration directory" FORCE)
    set(SYSTEMVIEW_BUILD_LIBRARY ON  CACHE BOOL "Build SystemView library for embOS simulation" FORCE)
    set(SYSTEMVIEW_BUILD_TESTS   OFF CACHE BOOL "Do not build SystemView tests from embOS simulation" FORCE)
    set(SYSTEMVIEW_CONFIG_DIR    "${EMBOS_SIM_SEGGER_DIR}" CACHE PATH "embOS simulation SystemView configuration directory" FORCE)

    add_subdirectory("${EMBOS_RTT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/RTT")
    add_subdirectory("${EMBOS_SYSVIEW_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/SystemView")
endif()

if(MSVC AND NOT EMBOS_SIM_ARCH STREQUAL "native")
    if((EMBOS_SIM_TARGET_BITS EQUAL 64 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            OR (EMBOS_SIM_TARGET_BITS EQUAL 32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4))
        message(FATAL_ERROR
            "MSVC architecture is selected by the generator platform. "
            "Use -A Win32 for x86 or -A x64 for x86_64.")
    endif()
endif()

set(EMBOS_SIM_GNU_ARCH_FLAG "")
if(NOT EMBOS_SIM_ARCH STREQUAL "native" AND NOT APPLE)
    if(EMBOS_SIM_TARGET_BITS EQUAL 32)
        set(EMBOS_SIM_GNU_ARCH_FLAG "-m32")
    else()
        set(EMBOS_SIM_GNU_ARCH_FLAG "-m64")
    endif()
endif()

set(EMBOS_SIM_APPLE_ARCH_FLAGS)
if(APPLE)
    if(EMBOS_SIM_TARGET_BITS EQUAL 64)
        list(APPEND EMBOS_SIM_APPLE_ARCH_FLAGS
            -arch
            x86_64
        )
    endif()
endif()

set(EMBOS_SIM_LIBRARY_MODES "xr" "r" "s" "sp" "d" "dp" "dt")
if(EMBOS_SIM_LIBRARY_MODE STREQUAL "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(EMBOS_SIM_EFFECTIVE_LIBRARY_MODE "dp")
    else()
        set(EMBOS_SIM_EFFECTIVE_LIBRARY_MODE "r")
    endif()
else()
    string(TOLOWER "${EMBOS_SIM_LIBRARY_MODE}" EMBOS_SIM_EFFECTIVE_LIBRARY_MODE)
endif()

if(NOT EMBOS_SIM_EFFECTIVE_LIBRARY_MODE IN_LIST EMBOS_SIM_LIBRARY_MODES)
    message(FATAL_ERROR
        "Unsupported EMBOS_SIM_LIBRARY_MODE: ${EMBOS_SIM_LIBRARY_MODE}")
endif()

string(TOUPPER "${EMBOS_SIM_EFFECTIVE_LIBRARY_MODE}"
    EMBOS_SIM_LIBRARY_MODE_UPPER
)
set(EMBOS_SIM_LIBRARY_ARCH "${EMBOS_SIM_TARGET_BITS}")

# ==============================================================================
# Host Platform
# ==============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(EMBOS_SIM_HOST_LINUX TRUE)
    set(EMBOS_SIM_HOST_WINDOWS FALSE)
    set(EMBOS_SIM_HOST_POSIX TRUE)
    set(EMBOS_SIM_POSIX_SOURCE_LABEL "Linux")
    set(EMBOS_PORT_NAME "OS_POSIX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(EMBOS_SIM_HOST_LINUX FALSE)
    set(EMBOS_SIM_HOST_WINDOWS FALSE)
    set(EMBOS_SIM_HOST_POSIX TRUE)
    set(EMBOS_SIM_POSIX_SOURCE_LABEL "POSIX")
    set(EMBOS_PORT_NAME "OS_POSIX")
elseif(WIN32)
    set(EMBOS_SIM_HOST_LINUX FALSE)
    set(EMBOS_SIM_HOST_WINDOWS TRUE)
    set(EMBOS_SIM_HOST_POSIX FALSE)
    set(EMBOS_PORT_NAME "OS_MSVC")
    enable_language(RC)
else()
    message(FATAL_ERROR
        "The embOS simulation start project supports Linux, macOS and Windows hosts only")
endif()

if(MSVC)
    set(EMBOS_SIM_LIBRARY_SUFFIX ".lib")
else()
    set(EMBOS_SIM_LIBRARY_SUFFIX ".a")
endif()

set(EMBOS_SIM_LIBRARY_FILENAME
    "libos${EMBOS_SIM_LIBRARY_ARCH}${EMBOS_SIM_EFFECTIVE_LIBRARY_MODE}${EMBOS_SIM_LIBRARY_SUFFIX}"
)
set(EMBOS_SIM_DEFAULT_PREBUILT_LIBRARY
    "${EMBOS_SIM_PREBUILT_ROOT}/${EMBOS_SIM_LIBRARY_FILENAME}"
)
set(EMBOS_SIM_DEFAULT_PREBUILT_INCLUDE_DIR
    "${EMBOS_INC_DIR}"
)

if(EMBOS_SIM_PREBUILT_LIBRARY STREQUAL "")
    set(EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY
        "${EMBOS_SIM_DEFAULT_PREBUILT_LIBRARY}"
    )
elseif(IS_ABSOLUTE "${EMBOS_SIM_PREBUILT_LIBRARY}")
    set(EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY
        "${EMBOS_SIM_PREBUILT_LIBRARY}"
    )
else()
    get_filename_component(EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY
        "${EMBOS_SIM_PREBUILT_LIBRARY}"
        ABSOLUTE
        BASE_DIR "${EMBOS_ROOT}"
    )
endif()

if(EMBOS_SIM_PREBUILT_INCLUDE_DIR STREQUAL "")
    set(EMBOS_SIM_EFFECTIVE_PREBUILT_INCLUDE_DIR
        "${EMBOS_SIM_DEFAULT_PREBUILT_INCLUDE_DIR}"
    )
elseif(IS_ABSOLUTE "${EMBOS_SIM_PREBUILT_INCLUDE_DIR}")
    set(EMBOS_SIM_EFFECTIVE_PREBUILT_INCLUDE_DIR
        "${EMBOS_SIM_PREBUILT_INCLUDE_DIR}"
    )
else()
    get_filename_component(EMBOS_SIM_EFFECTIVE_PREBUILT_INCLUDE_DIR
        "${EMBOS_SIM_PREBUILT_INCLUDE_DIR}"
        ABSOLUTE
        BASE_DIR "${EMBOS_ROOT}"
    )
endif()

set(EMBOS_SIM_EFFECTIVE_PREBUILT_RTOS_H
    "${EMBOS_SIM_EFFECTIVE_PREBUILT_INCLUDE_DIR}/RTOS.h"
)

set(EMBOS_PORT_DIR_REL "prebuilt/${EMBOS_PORT_NAME}")
if(NOT EXISTS "${EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY}")
    message(FATAL_ERROR
        "Prebuilt embOS static library not found: ${EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY}")
endif()
if(NOT EXISTS "${EMBOS_SIM_EFFECTIVE_PREBUILT_RTOS_H}")
    message(FATAL_ERROR
        "Prebuilt embOS RTOS.h not found: ${EMBOS_SIM_EFFECTIVE_PREBUILT_RTOS_H}")
endif()

set(EMBOS_GENERATED_INCLUDE_DIR "${EMBOS_SIM_EFFECTIVE_PREBUILT_INCLUDE_DIR}")
set(EMBOS_GENERATED_RTOS_H      "${EMBOS_SIM_EFFECTIVE_PREBUILT_RTOS_H}")

set(EMBOS_SIM_SDL2_LIBRARIES)
set(EMBOS_SIM_SDL2_INCLUDE_DIRS)

if(EMBOS_SIM_HOST_POSIX)
    find_package(Threads REQUIRED)
    if(EMBOS_SIM_GUI)
        find_package(SDL2 REQUIRED)
        if(TARGET SDL2::SDL2)
            list(APPEND EMBOS_SIM_SDL2_LIBRARIES SDL2::SDL2)
        elseif(TARGET SDL2::SDL2-static)
            list(APPEND EMBOS_SIM_SDL2_LIBRARIES SDL2::SDL2-static)
        else()
            list(APPEND EMBOS_SIM_SDL2_LIBRARIES ${SDL2_LIBRARIES})
            list(APPEND EMBOS_SIM_SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS})
        endif()
        if(NOT EMBOS_SIM_SDL2_LIBRARIES)
            message(FATAL_ERROR
                "SDL2 was found but did not provide a link target or libraries")
        endif()
    endif()
endif()

# ==============================================================================
# Common Compile Configuration
# ==============================================================================

add_library(embos_sim_options INTERFACE)

if(EMBOS_SIM_ENABLE_COVERAGE AND NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "EMBOS_SIM_ENABLE_COVERAGE requires GNU or Clang")
endif()

if(EMBOS_SIM_ENABLE_ASAN AND NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "EMBOS_SIM_ENABLE_ASAN requires GNU or Clang")
endif()

set(EMBOS_SIM_ALLOCATOR_INTERPOSITION_LABEL "disabled")
set(EMBOS_SIM_ALLOCATOR_INTERPOSITION_LINK_OPTIONS)
if(EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION)
    if(NOT EMBOS_SIM_HOST_LINUX)
        message(FATAL_ERROR
            "EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION requires a Linux host")
    endif()
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR
            "EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION requires GNU or Clang")
    endif()
    if(EMBOS_SIM_ENABLE_ASAN)
        message(FATAL_ERROR
            "EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION cannot be combined "
            "with EMBOS_SIM_ENABLE_ASAN")
    endif()
    set(EMBOS_SIM_ALLOCATOR_INTERPOSITION_LABEL "linker wrap")
    list(APPEND EMBOS_SIM_ALLOCATOR_INTERPOSITION_LINK_OPTIONS
        "-Wl,--wrap=malloc"
        "-Wl,--wrap=free"
        "-Wl,--wrap=calloc"
        "-Wl,--wrap=realloc"
    )
endif()

target_compile_definitions(embos_sim_options INTERFACE
    OS_LIBMODE_${EMBOS_SIM_LIBRARY_MODE_UPPER}=1
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<BOOL:${EMBOS_SIM_ENABLE_ASAN}>:EMBOS_SIM_ENABLE_ASAN=1>
    $<$<BOOL:${EMBOS_SIM_TRACE_TICK}>:EMBOS_SIM_TRACE_TICK=1>
    $<$<BOOL:${EMBOS_SIM_IS_64_BIT}>:OS_TRACE_TICKLESS_ADJUSTTIME_WITH_FRACT=1>
)

if(EMBOS_SIM_HOST_POSIX)
    target_compile_definitions(embos_sim_options INTERFACE
        EMBOS_SIM_HOST_POSIX=1
        EMBOS_SIM_HOST_WINDOWS=0
        EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION=$<BOOL:${EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION}>
        OS_SUPPORT_CALL_ISR=0
    )
elseif(EMBOS_SIM_HOST_WINDOWS)
    target_compile_definitions(embos_sim_options INTERFACE
        EMBOS_SIM_HOST_POSIX=0
        EMBOS_SIM_HOST_WINDOWS=1
        OS_SUPPORT_CALL_ISR=1
    )
endif()

if(EMBOS_SIM_HOST_WINDOWS)
    target_compile_definitions(embos_sim_options INTERFACE
        WIN32
        _CRT_SECURE_NO_WARNINGS
    )
endif()

add_library(embos_sim_start_options INTERFACE)
target_link_libraries(embos_sim_start_options INTERFACE
    embos_sim_options
)
target_compile_definitions(embos_sim_start_options INTERFACE
    EMBOS_SIM_GUI=$<BOOL:${EMBOS_SIM_GUI}>
)

if(EMBOS_SIM_HOST_POSIX AND EMBOS_SIM_GUI)
    target_compile_definitions(embos_sim_start_options INTERFACE
        SDL_MAIN_HANDLED=1
        SIM_OS_DEVICE_BITMAP_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Posix/SIM_OS_Device.bmp"
    )
    if(EMBOS_SIM_SDL2_INCLUDE_DIRS)
        target_include_directories(embos_sim_start_options INTERFACE
            ${EMBOS_SIM_SDL2_INCLUDE_DIRS}
        )
    endif()
endif()

if(EMBOS_SIM_HOST_WINDOWS)
    target_compile_definitions(embos_sim_start_options INTERFACE
        $<$<BOOL:${EMBOS_SIM_GUI}>:_WINDOWS>
        $<$<NOT:$<BOOL:${EMBOS_SIM_GUI}>>:_CONSOLE>
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(embos_sim_options INTERFACE
        -Wall
        -Wextra
        -Wno-deprecated-declarations
        -ffunction-sections
        -fdata-sections
        -fno-common
        -fdiagnostics-color=always
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Release>:-g>
        $<$<CONFIG:Release>:-O2>
        $<$<BOOL:${EMBOS_SIM_WARNINGS_AS_ERRORS}>:-Werror>
    )
    if(EMBOS_SIM_ENABLE_COVERAGE)
        target_compile_options(embos_sim_options INTERFACE
            --coverage
            -O0
        )
        target_link_options(embos_sim_options INTERFACE
            --coverage
        )
    endif()
    if(EMBOS_SIM_ENABLE_ASAN)
        target_compile_options(embos_sim_options INTERFACE
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        target_link_options(embos_sim_options INTERFACE
            -fsanitize=address
        )
    endif()
    if(EMBOS_SIM_GNU_ARCH_FLAG)
        target_compile_options(embos_sim_options INTERFACE
            ${EMBOS_SIM_GNU_ARCH_FLAG}
        )
        target_link_options(embos_sim_options INTERFACE
            ${EMBOS_SIM_GNU_ARCH_FLAG}
        )
    endif()
    if(EMBOS_SIM_APPLE_ARCH_FLAGS)
        target_compile_options(embos_sim_options INTERFACE
            ${EMBOS_SIM_APPLE_ARCH_FLAGS}
        )
        target_link_options(embos_sim_options INTERFACE
            ${EMBOS_SIM_APPLE_ARCH_FLAGS}
        )
    endif()
    if(EMBOS_SIM_HOST_LINUX)
        target_compile_options(embos_sim_options INTERFACE
            -U_FORTIFY_SOURCE
        )
        target_compile_definitions(embos_sim_options INTERFACE
            _FORTIFY_SOURCE=0
        )
        target_link_options(embos_sim_options INTERFACE
            -Wl,--no-as-needed
        )
    endif()
elseif(MSVC)
    target_compile_options(embos_sim_options INTERFACE
        /W4
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
        $<$<BOOL:${EMBOS_SIM_WARNINGS_AS_ERRORS}>:/WX>
    )
endif()

if(TARGET SEGGER_RTT)
    target_link_libraries(SEGGER_RTT PUBLIC embos_sim_options)
endif()
if(TARGET SEGGER_SYSVIEW)
    target_link_libraries(SEGGER_SYSVIEW PUBLIC embos_sim_options)
endif()

# ==============================================================================
# embOS Source Lists
# ==============================================================================

set(EMBOS_SOURCE_HEADERS
    "${EMBOS_CONFIG_DIR}/OS_Config.h"
    "${EMBOS_GENERATED_RTOS_H}"
    "${EMBOS_INC_DIR}/BSP.h"
    "${EMBOS_INC_DIR}/Global.h"
    "${EMBOS_INC_DIR}/UDPCOM.h"
)

foreach(_source IN LISTS
        EMBOS_SOURCE_HEADERS)
    if(NOT EXISTS "${_source}")
        message(FATAL_ERROR "Required embOS file not found: ${_source}")
    endif()
endforeach()

# ==============================================================================
# Simulation Source Lists
# ==============================================================================

set(EMBOS_SIM_APPLICATION_SOURCES
    "${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}"
)

set(EMBOS_SIM_SETUP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Setup/BSP.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Setup/RTOSInit.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Setup/UDP_Sim_Process.c"
)

set(EMBOS_SIM_WINDOWS_SOURCES)
set(EMBOS_SIM_POSIX_SOURCES)
set(EMBOS_SIM_SEGGER_SOURCES)
set(EMBOS_SIM_START_PLATFORM_SOURCES)
set(EMBOS_SIM_PLATFORM_INCLUDE_DIRS)

if(EMBOS_SIM_HOST_WINDOWS)
    if(EMBOS_SIM_GUI)
        list(APPEND EMBOS_SIM_PLATFORM_INCLUDE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32"
        )
        list(APPEND EMBOS_SIM_WINDOWS_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32/SIM_OS.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32/SIM_OS.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32/SIM_OS_Resource.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32/SIM_OS_Simulation.rc"
            "${CMAKE_CURRENT_SOURCE_DIR}/Win32/WinMain.c"
        )
    endif()
elseif(EMBOS_SIM_HOST_POSIX)
    list(APPEND EMBOS_SIM_PLATFORM_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/Posix"
    )
    if(EMBOS_SIM_GUI)
        list(APPEND EMBOS_SIM_POSIX_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/Posix/SIM_OS.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/Posix/SIM_OS.h"
        )
        list(APPEND EMBOS_SIM_START_PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/Posix/PosixMain.c"
        )
    endif()
    list(APPEND EMBOS_SIM_POSIX_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Setup/OS_ThreadSafe.c"
    )
endif()

if(NOT EMBOS_SIM_IS_64_BIT)
    list(APPEND EMBOS_SIM_SEGGER_SOURCES
        "${EMBOS_SIM_SYSVIEW_EMBOS_SOURCE}"
    )
    if(EMBOS_SIM_HOST_WINDOWS)
        list(APPEND EMBOS_SIM_SEGGER_SOURCES
            "${EMBOS_SIM_RTT_MEMORY_WIN32_SOURCE}"
            "${EMBOS_SIM_SYSVIEW_WIN32_CONFIG_SOURCE}"
        )
    elseif(EMBOS_SIM_HOST_POSIX)
        list(APPEND EMBOS_SIM_SEGGER_SOURCES
            "${EMBOS_SIM_RTT_MEMORY_POSIX_SOURCE}"
            "${EMBOS_SIM_SYSVIEW_POSIX_CONFIG_SOURCE}"
        )
    endif()
endif()

set(EMBOS_SIM_TARGET_SOURCES
    ${EMBOS_SIM_APPLICATION_SOURCES}
    ${EMBOS_SIM_SETUP_SOURCES}
    ${EMBOS_SIM_WINDOWS_SOURCES}
    ${EMBOS_SIM_POSIX_SOURCES}
    ${EMBOS_SIM_SEGGER_SOURCES}
)

set(EMBOS_SIM_START_SOURCES
    ${EMBOS_SIM_TARGET_SOURCES}
    ${EMBOS_SIM_START_PLATFORM_SOURCES}
)

if(EMBOS_SIM_HOST_POSIX AND EMBOS_SIM_GUI)
    set_source_files_properties("${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}" PROPERTIES
        COMPILE_DEFINITIONS "main=SIM_OS_AppMain"
    )
    if(EMBOS_SIM_APPLICATION_IS_CXX)
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/Posix/PosixMain.c" PROPERTIES
            LANGUAGE            CXX
            COMPILE_DEFINITIONS "SIM_OS_APP_MAIN_CXX=1"
        )
    endif()
endif()

foreach(_source IN LISTS EMBOS_SIM_START_SOURCES)
    if(NOT EXISTS "${_source}")
        message(FATAL_ERROR "Required simulation source file not found: ${_source}")
    endif()
endforeach()

set(EMBOS_SOURCE_GROUP_EMBOS_FILES
    ${EMBOS_SOURCE_HEADERS}
    ${EMBOS_SIM_SEGGER_SOURCES}
)

set(EMBOS_SOURCE_GROUP_SIMULATION_FILES
    ${EMBOS_SIM_SETUP_SOURCES}
    ${EMBOS_SIM_WINDOWS_SOURCES}
    ${EMBOS_SIM_POSIX_SOURCES}
    ${EMBOS_SIM_START_PLATFORM_SOURCES}
)

source_group(TREE "${EMBOS_ROOT}" PREFIX "embOS" FILES
    ${EMBOS_SOURCE_GROUP_EMBOS_FILES}
)

source_group("Application" FILES
    "${EMBOS_SIM_EFFECTIVE_APPLICATION_SOURCE_ABS}"
)

source_group("Generated" FILES
    "${EMBOS_GENERATED_RTOS_H}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Simulation" FILES
    ${EMBOS_SOURCE_GROUP_SIMULATION_FILES}
)

# ==============================================================================
# embOS Kernel Library
# ==============================================================================

add_library(embos_kernel STATIC IMPORTED GLOBAL)
set_target_properties(embos_kernel PROPERTIES
    IMPORTED_LOCATION "${EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY}"
)
target_include_directories(embos_kernel INTERFACE
    "${EMBOS_GENERATED_INCLUDE_DIR}"
    "${EMBOS_CONFIG_DIR}"
    "${EMBOS_INC_DIR}"
)
target_link_libraries(embos_kernel INTERFACE
    embos_sim_options
)
if(EMBOS_SIM_HOST_POSIX)
    target_link_libraries(embos_kernel INTERFACE
        Threads::Threads
    )
endif()

# ==============================================================================
# Start Executable
# ==============================================================================

if(EMBOS_SIM_HOST_WINDOWS AND EMBOS_SIM_GUI)
    add_executable(Start WIN32
        ${EMBOS_SIM_START_SOURCES}
    )
else()
    add_executable(Start
        ${EMBOS_SIM_START_SOURCES}
    )
endif()

set_target_properties(Start PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        ON
)

if(MSVC)
    set_target_properties(Start PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
endif()

if(EMBOS_SIM_APPLICATION_IS_CXX)
    set_target_properties(Start PROPERTIES
        CXX_STANDARD          98
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        ON
    )
endif()

target_include_directories(Start PRIVATE
    "${EMBOS_GENERATED_INCLUDE_DIR}"
    "${EMBOS_INC_DIR}"
    "${EMBOS_APPLICATION_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Setup"
    "${EMBOS_SIM_SEGGER_DIR}"
    ${EMBOS_SIM_PLATFORM_INCLUDE_DIRS}
)

target_link_libraries(Start PRIVATE
    embos_kernel
    embos_sim_start_options
)
if(NOT EMBOS_SIM_IS_64_BIT)
    target_link_libraries(Start PRIVATE SEGGER_SYSVIEW)
endif()

if(EMBOS_SIM_ALLOCATOR_INTERPOSITION_LINK_OPTIONS)
    target_link_options(Start PRIVATE
        ${EMBOS_SIM_ALLOCATOR_INTERPOSITION_LINK_OPTIONS}
    )
endif()

set(EMBOS_SIM_START_OUTPUT_NAME "")
if(EMBOS_SIM_HOST_WINDOWS)
    target_link_libraries(Start PRIVATE
        winmm
        ws2_32
    )
    if(EMBOS_SIM_GUI)
        target_link_libraries(Start PRIVATE
            user32
            gdi32
        )
    endif()
    set(EMBOS_SIM_START_OUTPUT_NAME "Start")
    if(MSVC)
        target_link_options(Start PRIVATE
            $<$<CONFIG:Release>:/DEBUG>
        )
    endif()
elseif(EMBOS_SIM_HOST_POSIX)
    if(EMBOS_SIM_GUI)
        target_link_libraries(Start PRIVATE
            ${EMBOS_SIM_SDL2_LIBRARIES}
        )
    endif()
    set(EMBOS_SIM_START_OUTPUT_NAME "Start_Simulation")
endif()

set_target_properties(Start PROPERTIES
    OUTPUT_NAME "${EMBOS_SIM_START_OUTPUT_NAME}"
)

# ==============================================================================
# Output Layout
# ==============================================================================

if(EMBOS_SIM_HOST_WINDOWS)
    if(EMBOS_SIM_IS_64_BIT)
        set(EMBOS_SIM_DEBUG_OUTPUT_DIR   "${EMBOS_SIM_OUTPUT_ROOT}/Debug_64")
        set(EMBOS_SIM_RELEASE_OUTPUT_DIR "${EMBOS_SIM_OUTPUT_ROOT}/Release_64")
    else()
        set(EMBOS_SIM_DEBUG_OUTPUT_DIR   "${EMBOS_SIM_OUTPUT_ROOT}/Debug")
        set(EMBOS_SIM_RELEASE_OUTPUT_DIR "${EMBOS_SIM_OUTPUT_ROOT}/Release")
    endif()
elseif(EMBOS_SIM_HOST_POSIX)
    if(EMBOS_SIM_IS_64_BIT)
        set(EMBOS_SIM_DEBUG_OUTPUT_DIR   "${EMBOS_SIM_OUTPUT_ROOT}/64dp")
        set(EMBOS_SIM_RELEASE_OUTPUT_DIR "${EMBOS_SIM_OUTPUT_ROOT}/64r")
    else()
        set(EMBOS_SIM_DEBUG_OUTPUT_DIR   "${EMBOS_SIM_OUTPUT_ROOT}/32dp")
        set(EMBOS_SIM_RELEASE_OUTPUT_DIR "${EMBOS_SIM_OUTPUT_ROOT}/32r")
    endif()
endif()

set_target_properties(Start PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${EMBOS_SIM_DEBUG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${EMBOS_SIM_DEBUG_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${EMBOS_SIM_RELEASE_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${EMBOS_SIM_RELEASE_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${EMBOS_SIM_RELEASE_OUTPUT_DIR}"
)

# ==============================================================================
# Configuration Summary
# ==============================================================================

message(STATUS "")
message(STATUS "=== embOS Simulation Configuration ===")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Host:           ${CMAKE_SYSTEM_NAME}")
message(STATUS "Host processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler:       ${CMAKE_C_COMPILER_ID}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Target width:   ${EMBOS_SIM_TARGET_BITS_LABEL}")
message(STATUS "Arch option:    ${EMBOS_SIM_ARCH}")
message(STATUS "embOS root:     ${EMBOS_ROOT}")
message(STATUS "Port:           ${EMBOS_PORT_DIR_REL}")
message(STATUS "Library mode:   ${EMBOS_SIM_EFFECTIVE_LIBRARY_MODE}")
message(STATUS "Library:        ${EMBOS_SIM_EFFECTIVE_PREBUILT_LIBRARY}")
message(STATUS "RTOS.h:         ${EMBOS_GENERATED_RTOS_H}")
message(STATUS "Application:    ${EMBOS_SIM_APPLICATION_SOURCE_REL}")
message(STATUS "Coverage:       ${EMBOS_SIM_ENABLE_COVERAGE}")
message(STATUS "ASAN:           ${EMBOS_SIM_ENABLE_ASAN}")
message(STATUS "Allocator interposition: ${EMBOS_SIM_ALLOCATOR_INTERPOSITION_LABEL}")
message(STATUS "GUI:            ${EMBOS_SIM_GUI}")
message(STATUS "Tick trace:     ${EMBOS_SIM_TRACE_TICK}")
message(STATUS "")

list(LENGTH EMBOS_SIM_SETUP_SOURCES EMBOS_SIM_SETUP_SOURCE_COUNT)
list(LENGTH EMBOS_SIM_WINDOWS_SOURCES EMBOS_SIM_WINDOWS_SOURCE_COUNT)
list(LENGTH EMBOS_SIM_POSIX_SOURCES EMBOS_SIM_POSIX_SOURCE_COUNT)
list(LENGTH EMBOS_SIM_SEGGER_SOURCES EMBOS_SIM_SEGGER_SOURCE_COUNT)

message(STATUS "Sources:")
message(STATUS "  Kernel:       prebuilt static library")
message(STATUS "  Setup:        ${EMBOS_SIM_SETUP_SOURCE_COUNT} files")
if(EMBOS_SIM_HOST_WINDOWS)
    message(STATUS "  Windows:      ${EMBOS_SIM_WINDOWS_SOURCE_COUNT} files")
elseif(EMBOS_SIM_HOST_POSIX)
    message(STATUS "  ${EMBOS_SIM_POSIX_SOURCE_LABEL}:        ${EMBOS_SIM_POSIX_SOURCE_COUNT} files")
endif()
if(EMBOS_SIM_SEGGER_SOURCES)
    message(STATUS "  SEGGER:       ${EMBOS_SIM_SEGGER_SOURCE_COUNT} files")
else()
    message(STATUS "  SEGGER:       disabled for 64-bit simulation")
endif()
message(STATUS "")
message(STATUS "Output:")
message(STATUS "  Debug:        ${EMBOS_SIM_DEBUG_OUTPUT_DIR}")
message(STATUS "  Release:      ${EMBOS_SIM_RELEASE_OUTPUT_DIR}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Werror:       ${EMBOS_SIM_WARNINGS_AS_ERRORS}")
message(STATUS "======================================")
message(STATUS "")
