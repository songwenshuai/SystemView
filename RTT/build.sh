#!/bin/bash
# ==============================================================================
# SEGGER RTT Shared-Memory Unit Test Build Script
# ==============================================================================
#
# CMake wrapper script for configuring, building, running, and reporting
# coverage for the shared-memory SEGGER RTT public API unit tests.
#
# Usage:
#   ./build.sh              # Debug build and run tests
#   ./build.sh --release    # Release build and run tests
#   ./build.sh --clean      # Clean and rebuild
#   ./build.sh --coverage   # Build, run tests, and print coverage
#   ./build.sh --no-examples
#                            # Skip example compile targets
#   ./build.sh -n           # Use Ninja generator
#   ./build.sh --help       # Show all options
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-"${SCRIPT_DIR}/build"}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
CLEAN_BUILD=false
VERBOSE=false
RUN_TESTS=true
BUILD_EXAMPLES=true
COVERAGE="${COVERAGE:-OFF}"
WARNINGS_AS_ERRORS=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

CMAKE_OPTIONS=()
CMAKE_GENERATOR=()
VERBOSE_FLAG=()

# ==============================================================================
# Functions
# ==============================================================================

print_color() { printf '\033[%sm%s\033[0m\n' "$1" "$2"; }
log_info()    { print_color 34 "[INFO] $1"; }
log_success() { print_color 32 "[OK]   $1"; }
log_warn()    { print_color 33 "[WARN] $1"; }
log_error()   { print_color 31 "[ERROR] $1" >&2; exit 1; }

show_help() {
  cat << 'EOF'
Usage: ./build.sh [OPTIONS]

Build Options:
  -d, --debug       Build debug configuration (default)
  -r, --release     Build release configuration
  -c, --clean       Clean build directory before configuring
  -j, --jobs N      Number of parallel jobs (default: auto)
  -v, --verbose     Verbose build output
  -n, --ninja       Use Ninja build system
  --no-test         Build only; do not run CTest
  --examples        Build example applications (default)
  --no-examples     Do not build example applications
  --werror          Treat warnings as errors

Coverage Options:
  --coverage        Enable compiler coverage instrumentation and print summary
  --no-coverage     Disable coverage instrumentation

Other:
  -h, --help        Show this help message

Environment:
  BUILD_DIR         Build directory. Defaults to ./build.
  CMAKE_BUILD_TYPE  Initial build type. Defaults to Debug.
  COVERAGE          Initial coverage switch. Use ON or OFF.

Examples:
  ./build.sh
  ./build.sh --release
  ./build.sh --clean --coverage
  ./build.sh --clean --no-examples
  ./build.sh -n -j 8

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

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -d|--debug)       BUILD_TYPE="Debug"; shift ;;
      -r|--release)     BUILD_TYPE="Release"; shift ;;
      -c|--clean)       CLEAN_BUILD=true; shift ;;
      -j|--jobs)        require_next_arg "$1" "$#"; JOBS="$2"; shift 2 ;;
      -v|--verbose)     VERBOSE=true; shift ;;
      -n|--ninja)       CMAKE_GENERATOR=(-G Ninja); shift ;;
      --no-test)        RUN_TESTS=false; shift ;;
      --examples)       BUILD_EXAMPLES=true; shift ;;
      --no-examples)    BUILD_EXAMPLES=false; shift ;;
      --coverage)       COVERAGE=ON; shift ;;
      --no-coverage)    COVERAGE=OFF; shift ;;
      --werror)         WARNINGS_AS_ERRORS=true; shift ;;
      -h|--help)        show_help ;;
      *)                log_error "Unknown option: $1. Use --help for usage." ;;
    esac
  done
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
      xcrun llvm-cov gcov "$gcno_file"
      ;;
    llvm-cov)
      llvm-cov gcov "$gcno_file"
      ;;
    gcov)
      gcov "$gcno_file"
      ;;
    *)
      return 1
      ;;
  esac
}

print_gcov_summary() {
  local tool="$1"
  local gcov_dir="${BUILD_DIR}/gcov"
  local gcno_file
  local object_dir
  local object_index=0

  rm -rf "${gcov_dir}"
  mkdir -p "${gcov_dir}"
  log_info "Generating combined gcov line coverage summary..."

  while IFS= read -r gcno_file; do
    object_index=$((object_index + 1))
    object_dir="${gcov_dir}/object_${object_index}"
    mkdir -p "${object_dir}"
    (cd "${object_dir}" && run_gcov "${tool}" "${gcno_file}" >/dev/null)
  done < <(find "${BUILD_DIR}/CMakeFiles" -path '*SEGGER_RTT_PublicAPI*.dir/*' -name '*.gcno' -type f | sort)

  if [[ "${object_index}" -eq 0 ]]; then
    log_warn "No .gcno files were found under ${BUILD_DIR}/CMakeFiles."
    return
  fi

  awk '
    function trim(s) {
      gsub(/^[ \t]+/, "", s)
      gsub(/[ \t]+$/, "", s)
      return s
    }
    function source_label(path) {
      if (path ~ /\/RTT\/RTT\/SEGGER_RTT\.c$/) {
        return "RTT/RTT/SEGGER_RTT.c"
      }
      if (path ~ /\/RTT\/RTT\/SEGGER_RTT_printf\.c$/) {
        return "RTT/RTT/SEGGER_RTT_printf.c"
      }
      if (path ~ /\/RTT\/Tests\/SEGGER_RTT_PublicAPI_Test\.c$/) {
        return "RTT/Tests/SEGGER_RTT_PublicAPI_Test.c"
      }
      return ""
    }
    /^ *-: *0:Source:/ {
      Source = $0
      sub(/^ *-: *0:Source:/, "", Source)
      Label = source_label(Source)
      if (Label != "") {
        Labels[Label] = 1
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
      Wanted[1] = "RTT/RTT/SEGGER_RTT.c"
      Wanted[2] = "RTT/RTT/SEGGER_RTT_printf.c"
      Wanted[3] = "RTT/Tests/SEGGER_RTT_PublicAPI_Test.c"
      for (Index = 1; Index <= 3; Index++) {
        Label = Wanted[Index]
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
        }
      }
    }
  ' "${gcov_dir}"/object_*/*.gcov
}

print_coverage_report() {
  local gcov_tool

  if [[ "${RUN_TESTS}" != true ]]; then
    log_warn "Coverage was enabled, but tests were not run; no coverage counters were generated."
    return
  fi

  log_info "Coverage instrumentation is enabled."
  if command -v gcovr >/dev/null 2>&1; then
    log_info "Generating gcovr summary..."
    gcovr -r "${SCRIPT_DIR}" "${BUILD_DIR}"
    return
  fi

  gcov_tool="$(select_gcov_tool)"
  if [[ -n "${gcov_tool}" ]]; then
    print_gcov_summary "${gcov_tool}"
  else
    log_warn "No gcov-compatible coverage reporter was found; raw coverage data remains in ${BUILD_DIR}."
  fi
}

# ==============================================================================
# Main
# ==============================================================================

main() {
  local build_args
  local configure_args

  parse_args "$@"

  COVERAGE="$(normalize_on_off "${COVERAGE}")"
  CMAKE_OPTIONS+=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
  CMAKE_OPTIONS+=("-DRTT_ENABLE_COVERAGE=${COVERAGE}")
  CMAKE_OPTIONS+=("-DRTT_BUILD_EXAMPLES=$(bool_to_on_off "${BUILD_EXAMPLES}")")
  CMAKE_OPTIONS+=("-DRTT_WARNINGS_AS_ERRORS=$(bool_to_on_off "${WARNINGS_AS_ERRORS}")")

  if [[ "${VERBOSE}" == true ]]; then
    VERBOSE_FLAG=(--verbose)
  fi

  log_info "Build type: ${BUILD_TYPE}"
  log_info "Parallel jobs: ${JOBS}"
  log_info "Coverage: ${COVERAGE}"
  log_info "Build examples: $(bool_to_on_off "${BUILD_EXAMPLES}")"
  if [[ ${#CMAKE_GENERATOR[@]} -gt 0 ]]; then
    log_info "Generator: Ninja"
  else
    log_info "Generator: CMake default"
  fi

  if [[ "${CLEAN_BUILD}" == true ]]; then
    log_info "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
  fi

  log_info "Configuring with CMake..."
  configure_args=(-S "${SCRIPT_DIR}" -B "${BUILD_DIR}")
  if [[ ${#CMAKE_GENERATOR[@]} -gt 0 ]]; then
    configure_args+=("${CMAKE_GENERATOR[@]}")
  fi
  configure_args+=("${CMAKE_OPTIONS[@]}")
  cmake "${configure_args[@]}"

  if [[ "${COVERAGE}" == "ON" ]]; then
    find "${BUILD_DIR}" -name '*.gcda' -exec rm -f {} +
  fi

  log_info "Building RTT targets..."
  build_args=(--build "${BUILD_DIR}" -j "${JOBS}")
  if [[ ${#VERBOSE_FLAG[@]} -gt 0 ]]; then
    build_args+=("${VERBOSE_FLAG[@]}")
  fi
  cmake "${build_args[@]}"
  log_success "Build completed successfully."

  if [[ "${RUN_TESTS}" == true ]]; then
    log_info "Running RTT unit tests..."
    ctest --test-dir "${BUILD_DIR}" --output-on-failure -j "${JOBS}"
    log_success "RTT unit tests passed."
  else
    log_info "Skipping CTest."
  fi

  if [[ "${COVERAGE}" == "ON" ]]; then
    print_coverage_report
  fi
}

main "$@"
