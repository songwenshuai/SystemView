#!/usr/bin/env bash
# ==============================================================================
# embOS POSIX simulation static library build script
# ==============================================================================
#
# Rebuilds embOS/Lib/libos32*.a and embOS/Lib/libos64*.a from embOS/Src
# for the POSIX simulation port.
#
# Usage:
#   ./build_libs.sh
#   ./build_libs.sh --x86
#   ./build_libs.sh --mode dp
#   ./build_libs.sh --clean
#
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_ROOT="${SCRIPT_DIR}/Build/Lib"
LIB_DIR="${SCRIPT_DIR}/Lib"
HOST_SYSTEM="$(uname -s)"
CC_BIN="${CC:-cc}"
AR_BIN="${AR:-ar}"
STRIP_BIN="${STRIP:-strip}"
CLEAN_BUILD=false
VERBOSE=false
STRIP_LIBS=true

ALL_MODES=(xr r s sp d dp dt)
SELECTED_MODES=()
SELECTED_ARCHES=()

COMMON_SOURCES=(
    OS_Alloc.c
    OS_Com.c
    OS_Error.c
    OS_EventObject.c
    OS_Global.c
    OS_Info.c
    OS_Kern.c
    OS_LowPower.c
    OS_Mailbox.c
    OS_MemPool.c
    OS_Memcpy.c
    OS_MultiObj.c
    OS_Mutex.c
    OS_Queue.c
    OS_RWLock.c
    OS_Semaphore.c
    OS_SoftwareTimer.c
    OS_Spinlock.c
    OS_SysTick.c
    OS_Task.c
    OS_TaskEvent.c
    OS_Timing.c
    OS_Trace.c
)

PORT_SOURCES=(
    OS_POSIX/OS_GetCPUState.c
    OS_POSIX/RTOSASM.c
)

print_color() { printf '\033[%sm%s\033[0m\n' "$1" "$2"; }
log_info()    { print_color 34 "[INFO] $1"; }
log_success() { print_color 32 "[OK]   $1"; }
log_error()   { print_color 31 "[ERROR] $1" >&2; exit 1; }

show_help() {
    cat << 'EOF'
Usage: ./build_libs.sh [OPTIONS]

Build Options:
      --x86              Build 32-bit libraries
                         Not supported on macOS
      --x86_64           Build 64-bit libraries
      --x64              Alias for --x86_64
      --arch ARCH        Build one target width: x86 or x86_64
                         Can be specified multiple times
      --mode MODE        Build one library mode: xr, r, s, sp, d, dp or dt
                         Can be specified multiple times
      --clean            Remove selected intermediate directories before building
      --cc PATH          C compiler executable (default: CC environment or cc)
      --ar PATH          Archiver executable (default: AR environment or ar)
      --strip PATH       Strip executable (default: STRIP environment or strip)
      --no-strip         Keep debug and local symbols in generated archives
      --verbose          Print compiler and archiver commands
  -h, --help             Show this help message

Examples:
  ./build_libs.sh
  ./build_libs.sh --x86
  ./build_libs.sh --mode dp
  ./build_libs.sh --mode r --mode dp --clean
  ./build_libs.sh --cc clang --ar llvm-ar
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

append_arch() {
    local arch="$1"

    case "${arch}" in
        x86|i386|32|32-bit)
            SELECTED_ARCHES+=("x86")
            ;;
        x86_64|x64|amd64|64|64-bit)
            SELECTED_ARCHES+=("x86_64")
            ;;
        *)
            log_error "Unsupported embOS library architecture: ${arch}"
            ;;
    esac
}

append_mode() {
    local mode="$1"

    mode="$(printf '%s' "${mode}" | tr '[:upper:]' '[:lower:]')"
    case "${mode}" in
        xr|r|s|sp|d|dp|dt)
            SELECTED_MODES+=("${mode}")
            ;;
        *)
            log_error "Unsupported embOS library mode: ${mode}"
            ;;
    esac
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --x86)
                append_arch "x86"
                shift
                ;;
            --x86_64|--x64)
                append_arch "x86_64"
                shift
                ;;
            --arch)
                append_arch "$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --arch=*)
                append_arch "${1#*=}"
                shift
                ;;
            --mode)
                append_mode "$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --mode=*)
                append_mode "${1#*=}"
                shift
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --cc)
                CC_BIN="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --cc=*)
                CC_BIN="${1#*=}"
                shift
                ;;
            --ar)
                AR_BIN="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --ar=*)
                AR_BIN="${1#*=}"
                shift
                ;;
            --strip)
                STRIP_BIN="$(require_arg "$1" "${2-}")"
                shift 2
                ;;
            --strip=*)
                STRIP_BIN="${1#*=}"
                shift
                ;;
            --no-strip)
                STRIP_LIBS=false
                shift
                ;;
            --verbose)
                VERBOSE=true
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

    if [[ "${#SELECTED_MODES[@]}" -eq 0 ]]; then
        SELECTED_MODES=("${ALL_MODES[@]}")
    fi
    if [[ "${#SELECTED_ARCHES[@]}" -eq 0 ]]; then
        if [[ "${HOST_SYSTEM}" == "Darwin" ]]; then
            SELECTED_ARCHES=("x86_64")
        else
            SELECTED_ARCHES=("x86" "x86_64")
        fi
    fi
}

validate_arches() {
    local arch

    for arch in "${SELECTED_ARCHES[@]}"; do
        if [[ "${HOST_SYSTEM}" == "Darwin" && "${arch}" == "x86" ]]; then
            log_error "macOS embOS simulation supports 64-bit libraries only."
        fi
    done
}

arch_bits() {
    local arch="$1"

    case "${arch}" in
        x86)
            printf '32'
            ;;
        x86_64)
            printf '64'
            ;;
        *)
            log_error "Unsupported embOS library architecture: ${arch}"
            ;;
    esac
}

arch_flag() {
    local arch="$1"

    case "${arch}" in
        x86)
            printf -- '-m32'
            ;;
        x86_64)
            printf -- '-m64'
            ;;
        *)
            log_error "Unsupported embOS library architecture: ${arch}"
            ;;
    esac
}

mode_define() {
    local mode="$1"

    printf 'OS_LIBMODE_%s=1' "$(printf '%s' "${mode}" | tr '[:lower:]' '[:upper:]')"
}

validate_sources() {
    local source

    for source in "${COMMON_SOURCES[@]}"; do
        if [[ ! -f "${SCRIPT_DIR}/Src/${source}" ]]; then
            log_error "Required source file not found: Src/${source}"
        fi
    done
    for source in "${PORT_SOURCES[@]}"; do
        if [[ ! -f "${SCRIPT_DIR}/Src/${source}" ]]; then
            log_error "Required source file not found: Src/${source}"
        fi
    done
}

run_command() {
    if [[ "${VERBOSE}" == true ]]; then
        printf '+' >&2
        printf ' %q' "$@" >&2
        printf '\n' >&2
    fi
    "$@"
}

compile_source() {
    local mode="$1"
    local arch="$2"
    local source="$3"
    local obj_dir="$4"
    local object_name
    local object_path
    local cflags=()
    local carch_flag

    object_name="${source//\//__}"
    object_path="${obj_dir}/${object_name%.c}.o"
    case "${mode}" in
        d|dp|dt)
            cflags=(-O2 -DDEBUG=1 -D_DEBUG)
            ;;
        *)
            cflags=(-O2 -DNDEBUG)
            ;;
    esac
    carch_flag="$(arch_flag "${arch}")"

    run_command "${CC_BIN}" \
        -std=c99 \
        "${carch_flag}" \
        -D"$(mode_define "${mode}")" \
        -DOS_SUPPORT_CALL_ISR=0 \
        -DOS_TRACE_TICKLESS_ADJUSTTIME_WITH_FRACT=1 \
        -I"${SCRIPT_DIR}/Config" \
        -I"${SCRIPT_DIR}/Inc" \
        -I"${SCRIPT_DIR}/Src" \
        -I"${SCRIPT_DIR}/Src/OS_POSIX" \
        "${cflags[@]}" \
        -c "${SCRIPT_DIR}/Src/${source}" \
        -o "${object_path}"

    printf '%s\n' "${object_path}"
}

build_mode() {
    local arch="$1"
    local mode="$2"
    local bits
    local build_dir
    local lib_path
    local obj_dir
    local source
    local objects=()
    local object_path

    bits="$(arch_bits "${arch}")"
    build_dir="${BUILD_ROOT}/libos${bits}${mode}"
    obj_dir="${build_dir}/obj"
    lib_path="${LIB_DIR}/libos${bits}${mode}.a"
    log_info "Building ${lib_path}"
    if [[ "${CLEAN_BUILD}" == true ]]; then
        rm -rf "${build_dir}"
    fi
    mkdir -p "${obj_dir}" "${LIB_DIR}"

    for source in "${COMMON_SOURCES[@]}"; do
        object_path="$(compile_source "${mode}" "${arch}" "${source}" "${obj_dir}")"
        objects+=("${object_path}")
    done
    for source in "${PORT_SOURCES[@]}"; do
        object_path="$(compile_source "${mode}" "${arch}" "${source}" "${obj_dir}")"
        objects+=("${object_path}")
    done

    rm -f "${lib_path}"
    run_command "${AR_BIN}" rcs "${lib_path}" "${objects[@]}"
    if [[ "${STRIP_LIBS}" == true ]]; then
        run_command "${STRIP_BIN}" -S -x "${lib_path}"
    fi
}

main() {
    parse_args "$@"
    validate_arches
    validate_sources

    log_info "Host: ${HOST_SYSTEM}"
    log_info "Compiler: ${CC_BIN}"
    log_info "Archiver: ${AR_BIN}"
    if [[ "${STRIP_LIBS}" == true ]]; then
        log_info "Strip: ${STRIP_BIN} -S -x"
    else
        log_info "Strip: disabled"
    fi
    log_info "Output directory: ${LIB_DIR}"

    for arch in "${SELECTED_ARCHES[@]}"; do
        for mode in "${SELECTED_MODES[@]}"; do
            build_mode "${arch}" "${mode}"
        done
    done

    log_success "embOS static libraries rebuilt."
}

main "$@"
