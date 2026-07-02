#!/bin/bash
# ==============================================================================
# TraceHub Build Script
# ==============================================================================
#
# CMake wrapper script for configuring, building, optional testing, coverage
# reporting, installing, and optionally running TraceHub.
#
# Usage:
#   ./build.sh                              # Debug host build
#   ./build.sh --release                    # Release host build
#   ./build.sh --clean --coverage           # Clean build, run tests, and print coverage
#   ./build.sh -a ARM64 --toolchain path.cmake
#   ./build.sh --run -- --help
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
TARGET_ARCH="${TARGET_ARCH:-HOST}"
BUILD_DIR=""
INSTALL_PREFIX="${INSTALL_PREFIX:-}"
CLEAN_BUILD=false
VERBOSE=false
RUN_AFTER_BUILD=false
RUN_TESTS=false
COVERAGE="${COVERAGE:-OFF}"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MEMORY_BACKEND="${MEMORY_BACKEND:-}"
BUILD_UNIT_TESTS=""
BUILD_SIM_TESTS=""
TOOLCHAIN_FILE=""
APP_BIN="tracehub"
INSTALL_BINDIR="bin"
INSTALL_LIBDIR="lib"
APP_ARGS=()

CMAKE_GENERATOR=(-G Ninja)
VERBOSE_FLAG=()

# ==============================================================================
# Functions
# ==============================================================================

print_color() { printf '\033[%sm%s\033[0m\n' "$1" "$2"; }
log_info()    { print_color 34 "[INFO] $1"; }
log_success() { print_color 32 "[OK]   $1"; }
log_warn()    { print_color 33 "[WARN] $1"; }
log_error()   { print_color 31 "[ERROR] $1" >&2; exit 1; }
print_section() { print_color 36 "=== $1 ==="; }

show_help() {
  cat << 'EOF'
Usage: ./build.sh [OPTIONS] [-- APP_ARGS...]

Build Options:
  -d, --debug           Build debug configuration (default)
  --release             Build release configuration
  -t, --type TYPE       Build type: Debug or Release
  -a, --arch ARCH       Build target: HOST or ARM64 (default: HOST)
  -c, --clean           Clean build directory before configuring
  -j, --jobs N          Number of parallel jobs (default: auto)
  -v, --verbose         Verbose build output
  -n, --ninja           Use Ninja build system (default)
  -p, --prefix PATH     Installation prefix (default: ./install)
  -b, --backend NAME    Memory backend: memshm or smem
  --toolchain PATH      CMake toolchain file for cross-compilation

Test Options:
  --test                Run CTest after building
  --no-test             Build only; do not run CTest

Coverage Options:
  --coverage            Enable compiler coverage instrumentation, run unit tests, and print summary
  --no-coverage         Disable coverage instrumentation

Run Options:
  -r, --run             Run tracehub after successful build
  --                    Separator for tracehub arguments

Other:
  -h, --help            Show this help message

Environment:
  CMAKE_BUILD_TYPE      Initial build type. Defaults to Debug.
  TARGET_ARCH           Initial target architecture. Defaults to HOST.
  INSTALL_PREFIX        Initial installation prefix. Defaults to ./install.
  MEMORY_BACKEND        Initial memory backend. Defaults by target.
  COVERAGE              Initial coverage switch. Use ON or OFF.

Commands:
  ./build.sh
  ./build.sh --release
  ./build.sh --clean --coverage
  ./build.sh -a ARM64 --toolchain path.cmake
  ./build.sh --run -- --help

EOF
  exit 0
}

require_next_arg() {
  local option_name="$1"
  local remaining_count="$2"

  if [[ "$remaining_count" -lt 2 ]]; then
    log_error "${option_name} requires a value."
  fi
}

normalize_on_off() {
  local value="$1"

  case "$value" in
    ON|on|On|1|true|TRUE|yes|YES)
      printf '%s\n' "ON"
      ;;
    OFF|off|Off|0|false|FALSE|no|NO)
      printf '%s\n' "OFF"
      ;;
    *)
      log_error "Invalid ON/OFF value: ${value}"
      ;;
  esac
}

bool_to_on_off() {
  if [[ "$1" == true ]]; then
    printf '%s\n' "ON"
  else
    printf '%s\n' "OFF"
  fi
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -d|--debug)
        BUILD_TYPE="Debug"
        shift
        ;;
      --release)
        BUILD_TYPE="Release"
        shift
        ;;
      -t|--type)
        require_next_arg "$1" "$#"
        BUILD_TYPE="$2"
        shift 2
        ;;
      -a|--arch)
        require_next_arg "$1" "$#"
        TARGET_ARCH="$2"
        shift 2
        ;;
      -c|--clean)
        CLEAN_BUILD=true
        shift
        ;;
      -j|--jobs)
        require_next_arg "$1" "$#"
        JOBS="$2"
        shift 2
        ;;
      -v|--verbose)
        VERBOSE=true
        shift
        ;;
      -n|--ninja)
        CMAKE_GENERATOR=(-G Ninja)
        shift
        ;;
      -p|--prefix)
        require_next_arg "$1" "$#"
        INSTALL_PREFIX="$2"
        shift 2
        ;;
      -b|--backend)
        require_next_arg "$1" "$#"
        MEMORY_BACKEND="$2"
        shift 2
        ;;
      --toolchain)
        require_next_arg "$1" "$#"
        TOOLCHAIN_FILE="$2"
        shift 2
        ;;
      --test)
        RUN_TESTS=true
        shift
        ;;
      --no-test)
        RUN_TESTS=false
        shift
        ;;
      --coverage)
        COVERAGE=ON
        RUN_TESTS=true
        shift
        ;;
      --no-coverage)
        COVERAGE=OFF
        shift
        ;;
      -r|--run)
        RUN_AFTER_BUILD=true
        shift
        ;;
      -h|--help)
        show_help
        ;;
      --)
        shift
        APP_ARGS=("$@")
        break
        ;;
      *)
        log_error "Unknown option: $1. Use --help for usage."
        ;;
    esac
  done
}

validate_args() {
  if [[ "${BUILD_TYPE}" != "Debug" && "${BUILD_TYPE}" != "Release" ]]; then
    log_error "Invalid build type: ${BUILD_TYPE}. Use Debug or Release."
  fi

  if [[ "${TARGET_ARCH}" != "HOST" && "${TARGET_ARCH}" != "ARM64" ]]; then
    log_error "Invalid architecture: ${TARGET_ARCH}. Use HOST or ARM64."
  fi

  if ! [[ "${JOBS}" =~ ^[0-9]+$ ]]; then
    log_error "Invalid number of jobs: ${JOBS}"
  fi

  if [[ -n "${MEMORY_BACKEND}" && "${MEMORY_BACKEND}" != "smem" && "${MEMORY_BACKEND}" != "memshm" ]]; then
    log_error "Invalid memory backend: ${MEMORY_BACKEND}. Use smem or memshm."
  fi
}

configure_environment() {
  local host_os
  local host_machine
  local host_is_windows=false

  if [[ -z "${INSTALL_PREFIX}" ]]; then
    INSTALL_PREFIX="${SCRIPT_DIR}/install"
  fi

  host_os="$(uname -s)"
  host_machine="$(uname -m)"

  case "${host_os}" in
    MINGW*|MSYS*|CYGWIN*)
      host_is_windows=true
      ;;
  esac

  if [[ -z "${MEMORY_BACKEND}" ]]; then
    if [[ "${TARGET_ARCH}" == "HOST" ]]; then
      MEMORY_BACKEND="memshm"
    else
      MEMORY_BACKEND="smem"
    fi
  fi

  if [[ "${host_os}" == "Darwin" && "${TARGET_ARCH}" == "HOST" && "${MEMORY_BACKEND}" == "smem" ]]; then
    log_error "SMEM backend is not available for native macOS builds. Use --backend memshm."
  fi

  if [[ "${MEMORY_BACKEND}" == "memshm" && "${host_is_windows}" != true ]]; then
    BUILD_SIM_TESTS="ON"
  else
    BUILD_SIM_TESTS="OFF"
  fi

  if [[ "${TARGET_ARCH}" == "HOST" ]]; then
    BUILD_UNIT_TESTS="ON"
  else
    BUILD_UNIT_TESTS="OFF"
  fi

  COVERAGE="$(normalize_on_off "${COVERAGE}")"
  if [[ "${COVERAGE}" == "ON" && "${BUILD_UNIT_TESTS}" != "ON" ]]; then
    log_error "Coverage requires a HOST build with unit tests enabled."
  fi

  if [[ -n "${TOOLCHAIN_FILE}" && ! -f "${TOOLCHAIN_FILE}" ]]; then
    log_error "Toolchain file not found: ${TOOLCHAIN_FILE}"
  fi

  if [[ "${TARGET_ARCH}" == "ARM64" && -z "${TOOLCHAIN_FILE}" ]]; then
    if [[ "${host_os}" == "Linux" && ( "${host_machine}" == "aarch64" || "${host_machine}" == "arm64" ) ]]; then
      log_info "Using native Linux ARM64 system toolchain."
    else
      log_error "ARM64 target build requires --toolchain PATH or a native Linux ARM64 host."
    fi
  fi

  BUILD_DIR="build/${TARGET_ARCH}_${BUILD_TYPE}"

  if [[ "${VERBOSE}" == true ]]; then
    VERBOSE_FLAG=(--verbose)
  fi
}

select_gcov_tool() {
  if command -v xcrun >/dev/null 2>&1 && xcrun -find llvm-cov >/dev/null 2>&1; then
    printf '%s\n' "xcrun-llvm-cov"
  elif command -v llvm-cov >/dev/null 2>&1; then
    printf '%s\n' "llvm-cov"
  elif command -v gcov >/dev/null 2>&1; then
    printf '%s\n' "gcov"
  else
    printf '%s\n' ""
  fi
}

run_gcov() {
  local tool="$1"
  local gcno_file="$2"

  case "$tool" in
    xcrun-llvm-cov)
      xcrun llvm-cov gcov -b "$gcno_file"
      ;;
    llvm-cov)
      llvm-cov gcov -b "$gcno_file"
      ;;
    gcov)
      gcov -b "$gcno_file"
      ;;
    *)
      return 1
      ;;
  esac
}

print_gcov_summary() {
  local tool="$1"
  local build_abs="${SCRIPT_DIR}/${BUILD_DIR}"
  local gcov_dir="${build_abs}/gcov"
  local gcno_file
  local object_dir
  local object_index=0
  local -a gcov_files

  rm -rf "${gcov_dir}"
  mkdir -p "${gcov_dir}"
  log_info "Generating gcov coverage summary..."

  while IFS= read -r gcno_file; do
    object_index=$((object_index + 1))
    object_dir="${gcov_dir}/object_${object_index}"
    mkdir -p "${object_dir}"
    (cd "${object_dir}" && run_gcov "${tool}" "${gcno_file}" >/dev/null)
  done < <(find "${build_abs}" -path '*/Tests/CMakeFiles/test_*.dir/*' -name '*.gcno' -type f | sort)

  if [[ "${object_index}" -eq 0 ]]; then
    log_warn "No unit-test .gcno files were found under ${build_abs}."
    return
  fi

  gcov_files=("${gcov_dir}"/object_*/*.gcov)
  if [[ ! -e "${gcov_files[0]}" ]]; then
    log_warn "No .gcov files were generated under ${gcov_dir}."
    return
  fi

  awk '
    function trim(s) {
      gsub(/^[ \t]+/, "", s)
      gsub(/[ \t]+$/, "", s)
      return s
    }
    function source_label(path, pos) {
      pos = index(path, "/TraceHub/")
      if (pos == 0) {
        return ""
      }
      return substr(path, pos + 1)
    }
    /^ *-: *0:Source:/ {
      Source = $0
      sub(/^ *-: *0:Source:/, "", Source)
      Label = source_label(Source)
      if ((Label != "") && !(Label in SeenLabel)) {
        SeenLabel[Label] = 1
        Order[++OrderCount] = Label
      }
      next
    }
    Label != "" && /^branch/ {
      BranchTotal[Label]++
      if ($0 !~ /never executed/) {
        BranchExecuted[Label]++
        if ($0 !~ /taken 0%/) {
          BranchTaken[Label]++
        }
      }
      next
    }
    Label != "" {
      split($0, Parts, ":")
      Count = trim(Parts[1])
      Line  = trim(Parts[2])
      if ((Line ~ /^[0-9]+$/) && (Count != "-")) {
        Executable[Label SUBSEP Line] = 1
        if ((Count != "#####") && (Count != "=====") && ((Count + 0) > 0)) {
          Covered[Label SUBSEP Line] = 1
        }
      }
    }
    END {
      for (Index = 1; Index <= OrderCount; Index++) {
        Label = Order[Index]
        Total = 0
        Hit = 0
        for (Key in Executable) {
          split(Key, Parts, SUBSEP)
          if (Parts[1] == Label) {
            Total++
            if (Key in Covered) {
              Hit++
            }
          }
        }
        if (Total > 0) {
          printf("File '\''%s'\''\n", Label)
          printf("Lines executed:%.2f%% of %u\n", (Hit * 100.0) / Total, Total)
          if (BranchTotal[Label] > 0) {
            printf("Branches executed:%.2f%% of %u\n", (BranchExecuted[Label] * 100.0) / BranchTotal[Label], BranchTotal[Label])
            printf("Taken at least once:%.2f%% of %u\n", (BranchTaken[Label] * 100.0) / BranchTotal[Label], BranchTotal[Label])
          }
        }
      }
    }
  ' "${gcov_files[@]}"
}

print_coverage_report() {
  local gcov_tool

  if [[ "${RUN_TESTS}" != true ]]; then
    log_warn "Coverage was enabled, but tests were not run; no coverage counters were generated."
    return
  fi

  gcov_tool="$(select_gcov_tool)"
  if [[ -n "${gcov_tool}" ]]; then
    print_gcov_summary "${gcov_tool}"
  else
    log_warn "No gcov-compatible coverage reporter was found; raw coverage data remains in ${SCRIPT_DIR}/${BUILD_DIR}."
  fi
}

get_ctest_test_count() {
  local ctest_output
  local test_count

  if ! ctest_output="$(ctest --test-dir "${SCRIPT_DIR}/${BUILD_DIR}" -N 2>&1)"; then
    printf '%s\n' "${ctest_output}" >&2
    log_error "CTest test discovery failed."
  fi

  test_count="$(printf '%s\n' "${ctest_output}" | awk '/Total Tests:/ { count = $3 } END { print count + 0 }')"
  printf '%s\n' "${test_count}"
}

print_configuration() {
  print_section "TraceHub Build Configuration"
  echo ""
  echo "Build Type:          ${BUILD_TYPE}"
  echo "Build Target:        ${TARGET_ARCH}"
  if [[ -n "${TOOLCHAIN_FILE}" ]]; then
    echo "Toolchain File:      ${TOOLCHAIN_FILE}"
  fi
  echo "Build Directory:     ${BUILD_DIR}"
  echo "Install Prefix:      ${INSTALL_PREFIX}"
  echo "Parallel Jobs:       ${JOBS}"
  if [[ "${#CMAKE_GENERATOR[@]}" -gt 0 ]]; then
    echo "Generator:           Ninja"
  else
    echo "Generator:           CMake default"
  fi
  echo ""
  echo "Build Options:"
  echo "  Clean Build:       $(bool_to_on_off "${CLEAN_BUILD}")"
  echo "  Verbose:           $(bool_to_on_off "${VERBOSE}")"
  echo "  Coverage:          ${COVERAGE}"
  echo "  Run Tests:         $(bool_to_on_off "${RUN_TESTS}")"
  echo "  Memory Backend:    ${MEMORY_BACKEND}"
  echo "  Unit Tests:        ${BUILD_UNIT_TESTS}"
  echo "  Simulation Tests:  ${BUILD_SIM_TESTS}"
  echo ""
  echo "Build Target:"
  echo "  - ${APP_BIN} (RTT Trace and Debug Bridge)"
  echo ""
}

clean_outputs() {
  if [[ "${CLEAN_BUILD}" != true ]]; then
    return
  fi

  if [[ -d "${SCRIPT_DIR}/${BUILD_DIR}" ]]; then
    log_info "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${SCRIPT_DIR}/${BUILD_DIR}"
  fi

  rm -f "${SCRIPT_DIR}/${APP_BIN}" \
        "${SCRIPT_DIR}/rtt_sim_rtos" \
        "${SCRIPT_DIR}/rtt_sim_linux" \
        "${SCRIPT_DIR}/rtt_sim_sysview_tcp_client"
}

configure_cmake() {
  local configure_args

  log_info "Creating build directory..."
  mkdir -p "${SCRIPT_DIR}/${BUILD_DIR}"

  print_section "Configuring CMake"
  configure_args=(
    -S "${SCRIPT_DIR}"
    -B "${SCRIPT_DIR}/${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DCMAKE_INSTALL_PREFIX:PATH="${INSTALL_PREFIX}"
    -DCMAKE_INSTALL_BINDIR:STRING="${INSTALL_BINDIR}"
    -DCMAKE_INSTALL_LIBDIR:STRING="${INSTALL_LIBDIR}"
    -DBUILD_UNIT_TESTS="${BUILD_UNIT_TESTS}"
    -DBUILD_SIM_TESTS="${BUILD_SIM_TESTS}"
    -DENABLE_COVERAGE="${COVERAGE}"
  )

  if [[ "${#CMAKE_GENERATOR[@]}" -gt 0 ]]; then
    configure_args+=("${CMAKE_GENERATOR[@]}")
  fi

  if [[ "$(uname -s)" == "Darwin" ]]; then
    configure_args+=(-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON)
  fi

  case "${MEMORY_BACKEND}" in
    memshm)
      configure_args+=(-DUSE_MEMSHM_BACKEND=ON)
      ;;
    smem)
      configure_args+=(-DUSE_MEMSHM_BACKEND=OFF)
      ;;
  esac

  if [[ -n "${TOOLCHAIN_FILE}" ]]; then
    configure_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
    log_info "Using toolchain file: ${TOOLCHAIN_FILE}"
  fi

  if [[ "${VERBOSE}" == true ]]; then
    configure_args+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
  fi

  cmake "${configure_args[@]}"

  if [[ -f "${SCRIPT_DIR}/${BUILD_DIR}/compile_commands.json" ]]; then
    ln -sf "${SCRIPT_DIR}/${BUILD_DIR}/compile_commands.json" "${SCRIPT_DIR}/build/compile_commands.json"
  fi

  if [[ "${COVERAGE}" == "ON" ]]; then
    find "${SCRIPT_DIR}/${BUILD_DIR}" -name '*.gcda' -exec rm -f {} +
  fi
}

build_tracehub() {
  local build_args

  print_section "Building TraceHub"
  build_args=(--build "${SCRIPT_DIR}/${BUILD_DIR}" -j "${JOBS}")
  if [[ "${#VERBOSE_FLAG[@]}" -gt 0 ]]; then
    build_args+=("${VERBOSE_FLAG[@]}")
  fi
  cmake "${build_args[@]}"
}

run_unit_tests() {
  local test_count

  if [[ "${RUN_TESTS}" != true ]]; then
    log_info "Skipping CTest."
    return
  fi

  print_section "Running TraceHub Unit Tests"
  test_count="$(get_ctest_test_count)"
  if [[ "${test_count}" -eq 0 ]]; then
    log_error "CTest did not discover any unit tests."
  fi

  log_info "Discovered ${test_count} unit tests."
  ctest --test-dir "${SCRIPT_DIR}/${BUILD_DIR}" --output-on-failure -j "${JOBS}"
  log_success "TraceHub unit tests passed."
}

install_tracehub() {
  print_section "Installing"
  cmake --install "${SCRIPT_DIR}/${BUILD_DIR}"

  if [[ -f "${SCRIPT_DIR}/${BUILD_DIR}/${APP_BIN}" ]]; then
    cp "${SCRIPT_DIR}/${BUILD_DIR}/${APP_BIN}" "${SCRIPT_DIR}/"
    chmod +x "${SCRIPT_DIR}/${APP_BIN}"
  else
    log_error "Executable not found: ${SCRIPT_DIR}/${BUILD_DIR}/${APP_BIN}"
  fi

  if [[ -f "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_rtos" ]]; then
    cp "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_rtos" "${SCRIPT_DIR}/"
    chmod +x "${SCRIPT_DIR}/rtt_sim_rtos"
  fi

  if [[ -f "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_linux" ]]; then
    cp "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_linux" "${SCRIPT_DIR}/"
    chmod +x "${SCRIPT_DIR}/rtt_sim_linux"
  fi

  if [[ -f "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_sysview_tcp_client" ]]; then
    cp "${SCRIPT_DIR}/${BUILD_DIR}/Tests/rtt_sim_sysview_tcp_client" "${SCRIPT_DIR}/"
    chmod +x "${SCRIPT_DIR}/rtt_sim_sysview_tcp_client"
  fi

  if [[ "${BUILD_SIM_TESTS}" == "OFF" ]]; then
    rm -f "${SCRIPT_DIR}/rtt_sim_rtos" \
          "${SCRIPT_DIR}/rtt_sim_linux" \
          "${SCRIPT_DIR}/rtt_sim_sysview_tcp_client"
  fi
}

print_build_summary() {
  local exe
  local lib
  local size
  local name

  log_success "Build completed successfully."
  echo ""
  print_section "Build Summary"
  echo ""
  echo "Build Directory:   ${SCRIPT_DIR}/${BUILD_DIR}"
  echo "Build Target:      ${TARGET_ARCH}"
  echo "Build Type:        ${BUILD_TYPE}"
  echo ""

  if [[ -d "${INSTALL_PREFIX}/${INSTALL_BINDIR}" ]]; then
    echo "Installed Executables:"
    while IFS= read -r exe; do
      if [[ -f "${exe}" ]]; then
        size="$(ls -lh "${exe}" | awk '{print $5}')"
        name="$(basename "${exe}")"
        printf "  %-50s %10s\n" "${name}" "${size}"
      fi
    done < <(find "${INSTALL_PREFIX}/${INSTALL_BINDIR}" -type f -executable 2>/dev/null | sort)
    echo ""
  fi

  if [[ -d "${INSTALL_PREFIX}/${INSTALL_LIBDIR}" ]]; then
    echo "Installed Libraries:"
    while IFS= read -r lib; do
      if [[ -f "${lib}" ]]; then
        size="$(ls -lh "${lib}" | awk '{print $5}')"
        name="$(basename "${lib}")"
        printf "  %-50s %10s\n" "${name}" "${size}"
      fi
    done < <(find "${INSTALL_PREFIX}/${INSTALL_LIBDIR}" \( -name "*.so*" -o -name "*.a" \) 2>/dev/null | sort)
    echo ""
  fi

  echo "Convenience Copy:"
  echo "  ${SCRIPT_DIR}/${APP_BIN}"
  if [[ "${BUILD_SIM_TESTS}" == "ON" && -f "${SCRIPT_DIR}/rtt_sim_rtos" ]]; then
    echo "  ${SCRIPT_DIR}/rtt_sim_rtos"
  fi
  if [[ "${BUILD_SIM_TESTS}" == "ON" && -f "${SCRIPT_DIR}/rtt_sim_linux" ]]; then
    echo "  ${SCRIPT_DIR}/rtt_sim_linux"
  fi
  if [[ "${BUILD_SIM_TESTS}" == "ON" && -f "${SCRIPT_DIR}/rtt_sim_sysview_tcp_client" ]]; then
    echo "  ${SCRIPT_DIR}/rtt_sim_sysview_tcp_client"
  fi
  echo ""

  if [[ "${RUN_AFTER_BUILD}" != true ]]; then
    log_info "To run the application:"
    echo "  ${INSTALL_PREFIX}/${INSTALL_BINDIR}/${APP_BIN} --help"
    echo "  or"
    echo "  LD_LIBRARY_PATH=${INSTALL_PREFIX}/${INSTALL_LIBDIR} ${SCRIPT_DIR}/${APP_BIN} --help"
    echo ""
  fi
}

run_application() {
  local lib_path

  if [[ "${RUN_AFTER_BUILD}" != true ]]; then
    return
  fi

  log_info "Running application..."
  lib_path="${INSTALL_PREFIX}/${INSTALL_LIBDIR}"
  if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
    export LD_LIBRARY_PATH="${lib_path}:${LD_LIBRARY_PATH}"
  else
    export LD_LIBRARY_PATH="${lib_path}"
  fi

  exec "${SCRIPT_DIR}/${APP_BIN}" "${APP_ARGS[@]}"
}

# ==============================================================================
# Main
# ==============================================================================

main() {
  parse_args "$@"
  validate_args
  configure_environment
  print_configuration
  clean_outputs
  configure_cmake
  echo ""
  build_tracehub
  echo ""
  run_unit_tests
  echo ""

  if [[ "${COVERAGE}" == "ON" ]]; then
    print_section "Coverage Summary"
    print_coverage_report
    echo ""
  fi

  install_tracehub
  echo ""
  print_build_summary
  run_application
}

main "$@"
