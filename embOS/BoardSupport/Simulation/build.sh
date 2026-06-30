#!/bin/bash
# ==============================================================================
# embOS Simulation Build Script
# ==============================================================================
#
# CMake wrapper script for convenient building of the SEGGER embOS simulation
# start project against prebuilt embOS static libraries.
#
# Usage:
#   ./build.sh              # 64-bit Debug build
#   ./build.sh --release    # 64-bit Release build
#   ./build.sh --x86        # 32-bit Debug build
#   ./build.sh --clean      # Clean and rebuild the selected profile
#   ./build.sh --coverage   # Enable gcov coverage instrumentation
#   ./build.sh --asan       # Enable AddressSanitizer
#   ./build.sh --asan --run # Build and run the Start executable
#   ./build.sh --trace-tick # Enable low-rate POSIX tick trace output
#   ./build.sh --help       # Show all options
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EMBOS_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
ASAN_GUI_LSAN_SUPPRESSIONS="${SCRIPT_DIR}/lsan_gui.supp"
HOST_SYSTEM="$(uname -s)"
HOST_PROCESSOR="$(uname -m)"
BUILD_TYPE="Debug"
ARCH="x86_64"
TARGET_BITS=""
CLEAN_BUILD=false
VERBOSE=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TARGET_NAME="Start"
APPLICATION_SOURCE=""
LIBRARY_MODE=""
RUN_START=false
GUI="ON"
GUI_EXPLICIT=false
ENABLE_COVERAGE=false
ENABLE_ASAN=false
TRACE_TICK=false
WARNINGS_AS_ERRORS=false
ALLOCATOR_INTERPOSITION="OFF"

CMAKE_OPTIONS=()
CMAKE_GENERATOR_PLATFORM=""
CMAKE_GENERATOR_NAME="Default"

# ==============================================================================
# Functions
# ==============================================================================

print_color() { printf '\033[%sm%s\033[0m\n' "$1" "$2"; }
log_info()    { print_color 34 "[INFO] $1"; }
log_success() { print_color 32 "[OK]   $1"; }
log_warn()    { print_color 33 "[WARN] $1"; }
log_error()   { print_color 31 "[ERROR] $1" >&2; exit 1; }

cmake_bool() {
    if [[ "$1" == true ]]; then
        printf 'ON'
    else
        printf 'OFF'
    fi
}

append_lsan_option() {
    local option_name="$1"
    local option_value="$2"
    local current_options="${LSAN_OPTIONS-}"

    case ":${current_options}:" in
        *":${option_name}="*)
            return
            ;;
    esac

    if [[ "${current_options}" == "" ]]; then
        LSAN_OPTIONS="${option_name}=${option_value}"
    else
        LSAN_OPTIONS="${current_options}:${option_name}=${option_value}"
    fi
    export LSAN_OPTIONS
}

show_help() {
    cat << 'EOF'
Usage: ./build.sh [OPTIONS]

Build Options:
  -d, --debug           Build debug configuration (default)
  -r, --release         Build release configuration
      --x86             Build 32-bit simulation profile
                        Not supported on macOS
      --x86_64          Build 64-bit simulation profile (default)
      --x64             Alias for --x86_64
  -a, --arch ARCH       Target width selector: x86 or x86_64
  -c, --clean           Clean selected build directory before building
  -j, --jobs N          Number of parallel jobs (default: auto)
  -v, --verbose         Verbose build output
  -n, --ninja           Use Ninja build system

Project Options:
      --application FILE
                        Application source file that provides main()
                        Relative paths are resolved from the embOS root
      --libmode MODE    embOS library mode suffix: xr, r, s, sp, d, dp or dt
                        Defaults to dp for Debug and r for Release
      --run             Build and run the Start executable directly
      --gui             Enable graphical simulation window
      --no-gui          Disable graphical simulation window
      --coverage        Enable gcov coverage instrumentation
      --asan            Enable AddressSanitizer
                        Disables allocator interposition
      --trace-tick      Enable low-rate POSIX tick trace output
      --werror          Treat warnings as errors

Other:
  -h, --help            Show this help message

Examples:
  ./build.sh
  ./build.sh --release
  ./build.sh --x86 --debug
  ./build.sh --x86 --release
  ./build.sh --clean --release
  ./build.sh --ninja --x86_64
  ./build.sh --application Application/OS_Start2Tasks.c
  ./build.sh --no-gui
  ./build.sh --coverage
  ./build.sh --asan
  ./build.sh --asan --run
  ./build.sh --trace-tick
  ./build.sh --werror

EOF
    exit 0
}

require_arg() {
    local option="$1"
    local value="${2-}"

    if [[ -z "${value}" ]]; then
        log_error "${option} requires an argument."
    fi
    printf '%s' "${value}"
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            --x86)
                ARCH="x86"
                shift
                ;;
            --x86_64|--x64)
                ARCH="x86_64"
                shift
                ;;
            -a|--arch)
                ARCH="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --arch=*)
                ARCH="${1#*=}"
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -j|--jobs)
                JOBS="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --jobs=*)
                JOBS="${1#*=}"
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -n|--ninja)
                CMAKE_GENERATOR_NAME="Ninja"
                shift
                ;;
            --application)
                APPLICATION_SOURCE="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --application=*)
                APPLICATION_SOURCE="${1#*=}"
                shift
                ;;
            --libmode)
                LIBRARY_MODE="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --libmode=*)
                LIBRARY_MODE="${1#*=}"
                shift
                ;;
            --run)
                RUN_START=true
                shift
                ;;
            --gui)
                GUI="ON"
                GUI_EXPLICIT=true
                shift
                ;;
            --no-gui)
                GUI="OFF"
                GUI_EXPLICIT=true
                shift
                ;;
            --coverage)
                ENABLE_COVERAGE=true
                shift
                ;;
            --asan)
                ENABLE_ASAN=true
                shift
                ;;
            --trace-tick)
                TRACE_TICK=true
                shift
                ;;
            --werror)
                WARNINGS_AS_ERRORS=true
                shift
                ;;
            -h|--help)
                show_help
                ;;
            *)
                log_error "Unknown option: $1. Use --help for usage."
                ;;
        esac
    done
}

validate_jobs() {
    case "${JOBS}" in
        ''|*[!0-9]*)
            log_error "Invalid job count: ${JOBS}"
            ;;
    esac

    if [[ "${JOBS}" -lt 1 ]]; then
        log_error "Invalid job count: ${JOBS}"
    fi
}

validate_application_source() {
    local application_path

    if [[ "${APPLICATION_SOURCE}" == "" ]]; then
        return
    fi

    application_path="${APPLICATION_SOURCE}"
    if [[ "${application_path}" != /* ]]; then
        application_path="${EMBOS_ROOT}/${application_path}"
    fi
    if [[ ! -f "${application_path}" ]]; then
        log_error "Application source not found: ${APPLICATION_SOURCE}"
    fi
}

validate_run_options() {
    if [[ "${RUN_START}" != true ]]; then
        return
    fi
}

validate_gui_options() {
    case "${GUI}" in
        ON|OFF)
            ;;
        *)
            log_error "Unsupported GUI option: ${GUI}"
            ;;
    esac
}

validate_library_mode() {
    if [[ "${LIBRARY_MODE}" == "" ]]; then
        return
    fi

    LIBRARY_MODE="$(printf '%s' "${LIBRARY_MODE}" | tr '[:upper:]' '[:lower:]')"
    case "${LIBRARY_MODE}" in
        xr|r|s|sp|d|dp|dt)
            ;;
        *)
            log_error "Unsupported embOS library mode: ${LIBRARY_MODE}"
            ;;
    esac
}

select_allocator_interposition() {
    if [[ "${ENABLE_ASAN}" == true ]]; then
        ALLOCATOR_INTERPOSITION="OFF"
    elif [[ "${HOST_SYSTEM}" == "Linux" ]]; then
        ALLOCATOR_INTERPOSITION="ON"
    else
        ALLOCATOR_INTERPOSITION="OFF"
    fi
}

append_cmake_options() {
    CMAKE_OPTIONS+=(
        "-DEMBOS_SIM_GUI=${GUI}"
        "-DEMBOS_SIM_ENABLE_COVERAGE=$(cmake_bool "${ENABLE_COVERAGE}")"
        "-DEMBOS_SIM_ENABLE_ASAN=$(cmake_bool "${ENABLE_ASAN}")"
        "-DEMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION=${ALLOCATOR_INTERPOSITION}"
        "-DEMBOS_SIM_TRACE_TICK=$(cmake_bool "${TRACE_TICK}")"
        "-DEMBOS_SIM_WARNINGS_AS_ERRORS=$(cmake_bool "${WARNINGS_AS_ERRORS}")"
    )
    if [[ "${APPLICATION_SOURCE}" != "" ]]; then
        CMAKE_OPTIONS+=("-DEMBOS_SIM_APPLICATION_SOURCE=${APPLICATION_SOURCE}")
    else
        CMAKE_OPTIONS+=("-UEMBOS_SIM_APPLICATION_SOURCE")
    fi
    if [[ "${LIBRARY_MODE}" != "" ]]; then
        CMAKE_OPTIONS+=("-DEMBOS_SIM_LIBRARY_MODE=${LIBRARY_MODE}")
    else
        CMAKE_OPTIONS+=("-UEMBOS_SIM_LIBRARY_MODE")
    fi
}

select_profile() {
    case "${ARCH}:${BUILD_TYPE}" in
        x86:Debug)
            TARGET_BITS="32"
            PROFILE="32dp"
            ;;
        x86:Release)
            TARGET_BITS="32"
            PROFILE="32r"
            ;;
        x86_64:Debug)
            TARGET_BITS="64"
            PROFILE="64dp"
            ;;
        x86_64:Release)
            TARGET_BITS="64"
            PROFILE="64r"
            ;;
        *)
            log_error "Unsupported build profile: ${ARCH}:${BUILD_TYPE}"
            ;;
    esac
}

select_generator_platform() {
    CMAKE_GENERATOR_PLATFORM=""
    case "${HOST_SYSTEM}" in
        MINGW*|MSYS*|CYGWIN*)
            if [[ "${CMAKE_GENERATOR_NAME}" == "Ninja" ]]; then
                return
            fi
            if [[ "${TARGET_BITS}" == "32" ]]; then
                CMAKE_GENERATOR_PLATFORM="Win32"
            else
                CMAKE_GENERATOR_PLATFORM="x64"
            fi
            ;;
    esac
}

validate_configuration() {
    case "${BUILD_TYPE}" in
        Debug|Release)
            ;;
        *)
            log_error "Unsupported build type: ${BUILD_TYPE}"
            ;;
    esac

    case "${ARCH}" in
        x86|x86_64)
            ;;
        *)
            log_error "Unsupported target width selector: ${ARCH}"
            ;;
    esac

    if [[ "${HOST_SYSTEM}" == "Darwin" && "${ARCH}" == "x86" ]]; then
        log_error "macOS embOS simulation supports 64-bit builds only."
    fi

    validate_jobs
    validate_run_options
    validate_gui_options
    validate_library_mode
    validate_application_source
    select_profile
    select_generator_platform
    select_allocator_interposition
    append_cmake_options
    BUILD_DIR="${SCRIPT_DIR}/build/${PROFILE}"
}

clean_build_dir() {
    if [[ "${BUILD_DIR}" != "${SCRIPT_DIR}/build/"* ]]; then
        log_error "Refusing to clean unexpected build directory: ${BUILD_DIR}"
    fi

    log_info "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
}

configure_project() {
    local cmake_args

    log_info "Configuring with CMake..."
    cmake_args=()
    if [[ "${CMAKE_GENERATOR_NAME}" == "Ninja" ]]; then
        cmake_args+=(-G "Ninja")
    fi
    if [[ "${CMAKE_GENERATOR_PLATFORM}" != "" ]]; then
        cmake_args+=(-A "${CMAKE_GENERATOR_PLATFORM}")
    fi
    cmake_args+=(
        -S "${SCRIPT_DIR}"
        -B "${BUILD_DIR}"
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
        -DEMBOS_SIM_ARCH="${ARCH}"
    )
    if [[ "${#CMAKE_OPTIONS[@]}" -gt 0 ]]; then
        cmake_args+=("${CMAKE_OPTIONS[@]}")
    fi
    cmake "${cmake_args[@]}"
}

build_project() {
    local build_args=(
        --build "${BUILD_DIR}"
        --config "${BUILD_TYPE}"
        --target "${TARGET_NAME}"
        --parallel "${JOBS}"
    )

    if [[ "${VERBOSE}" == true ]]; then
        build_args+=(--verbose)
    fi

    log_info "Building target: ${TARGET_NAME}"
    cmake "${build_args[@]}"
}

prepare_asan_runtime() {
    if [[ "${ENABLE_ASAN}" != true ]]; then
        return
    fi
    if [[ "${HOST_SYSTEM}" != "Linux" ]]; then
        return
    fi
    if [[ "${GUI}" != "ON" ]]; then
        return
    fi
    if [[ ! -f "${ASAN_GUI_LSAN_SUPPRESSIONS}" ]]; then
        log_error "LSan suppression file not found: ${ASAN_GUI_LSAN_SUPPRESSIONS}"
    fi

    append_lsan_option "suppressions" "${ASAN_GUI_LSAN_SUPPRESSIONS}"
    append_lsan_option "leak_check_at_exit" "0"
    append_lsan_option "print_suppressions" "1"
    log_info "LSan suppressions: ${ASAN_GUI_LSAN_SUPPRESSIONS}"
}

get_start_executable_path() {
    local output_dir

    case "${HOST_SYSTEM}" in
        Darwin|Linux)
            output_dir="${SCRIPT_DIR}/Output/${PROFILE}"
            printf '%s/Start_Simulation' "${output_dir}"
            ;;
        MINGW*|MSYS*|CYGWIN*)
            if [[ "${BUILD_TYPE}" == "Debug" ]]; then
                if [[ "${TARGET_BITS}" == "64" ]]; then
                    output_dir="${SCRIPT_DIR}/Output/Debug_64"
                else
                    output_dir="${SCRIPT_DIR}/Output/Debug"
                fi
            else
                if [[ "${TARGET_BITS}" == "64" ]]; then
                    output_dir="${SCRIPT_DIR}/Output/Release_64"
                else
                    output_dir="${SCRIPT_DIR}/Output/Release"
                fi
            fi
            printf '%s/Start.exe' "${output_dir}"
            ;;
        *)
            log_error "Unsupported host system for --run: ${HOST_SYSTEM}"
            ;;
    esac
}

run_start() {
    local executable_path

    executable_path="$(get_start_executable_path)"
    if [[ ! -f "${executable_path}" ]]; then
        log_error "Run executable not found: ${executable_path}"
    fi
    if [[ ! -x "${executable_path}" ]]; then
        log_error "Run executable is not executable: ${executable_path}"
    fi

    log_info "Running: ${executable_path}"
    prepare_asan_runtime
    exec "${executable_path}"
}

# ==============================================================================
# Main
# ==============================================================================

main() {
    parse_args "$@"
    validate_configuration

    log_info "Build type: ${BUILD_TYPE}"
    log_info "Host: ${HOST_SYSTEM}"
    log_info "Host processor: ${HOST_PROCESSOR}"
    log_info "Target width: ${TARGET_BITS}-bit"
    log_info "Arch option: ${ARCH}"
    log_info "Profile: ${PROFILE}"
    log_info "Build directory: ${BUILD_DIR}"
    log_info "Parallel jobs: ${JOBS}"
    log_info "Generator: ${CMAKE_GENERATOR_NAME}"
    if [[ "${CMAKE_GENERATOR_PLATFORM}" != "" ]]; then
        log_info "Generator platform: ${CMAKE_GENERATOR_PLATFORM}"
    fi
    log_info "Target: ${TARGET_NAME}"
    if [[ "${APPLICATION_SOURCE}" != "" ]]; then
        log_info "Application: ${APPLICATION_SOURCE}"
    else
        log_info "Application: CMake default"
    fi
    if [[ "${LIBRARY_MODE}" != "" ]]; then
        log_info "Library mode: ${LIBRARY_MODE}"
    else
        log_info "Library mode: CMake default"
    fi
    log_info "GUI: ${GUI}"
    log_info "Allocator interposition: ${ALLOCATOR_INTERPOSITION}"

    if [[ "${CLEAN_BUILD}" == true ]]; then
        clean_build_dir
    fi

    configure_project
    build_project

    log_success "Build completed successfully."
    log_info "Output root: ${SCRIPT_DIR}/Output"
    if [[ "${RUN_START}" == true ]]; then
        run_start
    fi
}

main "$@"
