#!/bin/bash
# ==============================================================================
# SEGGER SystemView Unit Test Build Script
# ==============================================================================
#
# CMake wrapper script for configuring, building, running, and reporting
# coverage for the SEGGER SystemView public API unit tests.
#
# Usage:
#   ./build.sh              # Debug build and run tests
#   ./build.sh --release    # Release build and run tests
#   ./build.sh --clean      # Clean and rebuild
#   ./build.sh --coverage   # Build, run tests, and print coverage
#   ./build.sh -n           # Use Ninja generator
#   ./build.sh --werror     # Treat warnings as errors
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Configuration
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
CLEAN_BUILD=false
RUN_TESTS=true
COVERAGE="${COVERAGE:-OFF}"
WARNINGS_AS_ERRORS=false
VERBOSE=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

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
print_section() { print_color 36 "=== $1 ==="; }

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
  --werror          Treat warnings as errors

Test Options:
  --test            Run CTest after building
  --no-test         Build only; do not run CTest

Coverage Options:
  --coverage        Enable compiler coverage instrumentation, run unit tests, and print summary
  --no-coverage     Disable coverage instrumentation

Other:
  -h, --help        Show this help message

Environment:
  BUILD_DIR         Build directory. Defaults to ./build/<type>.
                    --clean only removes directories under ./build.
  CMAKE_BUILD_TYPE  Initial build type. Defaults to Debug.
  COVERAGE          Initial coverage switch. Use ON or OFF.

Commands:
  ./build.sh
  ./build.sh --release
  ./build.sh --clean --coverage
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
      -r|--release)
        BUILD_TYPE="Release"
        shift
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

validate_args() {
  if [[ "${BUILD_TYPE}" != "Debug" && "${BUILD_TYPE}" != "Release" ]]; then
    log_error "Invalid build type: ${BUILD_TYPE}. Use Debug or Release."
  fi

  if ! [[ "${JOBS}" =~ ^[0-9]+$ ]]; then
    log_error "Invalid number of jobs: ${JOBS}"
  fi

  if [[ "${JOBS}" -lt 1 ]]; then
    log_error "Invalid number of jobs: ${JOBS}"
  fi
}

configure_environment() {
  local build_type

  COVERAGE="$(normalize_on_off "${COVERAGE}")"

  case "${BUILD_TYPE}" in
    Debug)
      build_type="debug"
      ;;
    Release)
      build_type="release"
      ;;
  esac

  if [[ -z "${BUILD_DIR}" ]]; then
    BUILD_DIR="${SCRIPT_DIR}/build/${build_type}"
  elif [[ "${BUILD_DIR}" != /* ]]; then
    BUILD_DIR="${SCRIPT_DIR}/${BUILD_DIR}"
  fi

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
  local gcov_dir="${BUILD_DIR}/gcov"
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
  done < <(find "${BUILD_DIR}/CMakeFiles" -path '*SEGGER_SYSVIEW_PublicAPI*.dir/*' -name '*.gcno' -type f | sort)

  if [[ "${object_index}" -eq 0 ]]; then
    log_warn "No .gcno files were found under ${BUILD_DIR}/CMakeFiles."
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
    function source_label(path) {
      if (path ~ /\/SystemView\/SYSVIEW\/SEGGER_SYSVIEW\.c$/) {
        return "SystemView/SYSVIEW/SEGGER_SYSVIEW.c"
      }
      if (path ~ /\/SystemView\/Tests\/SEGGER_SYSVIEW_PublicAPI_Test\.c$/) {
        return "SystemView/Tests/SEGGER_SYSVIEW_PublicAPI_Test.c"
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
      Wanted[1] = "SystemView/SYSVIEW/SEGGER_SYSVIEW.c"
      Wanted[2] = "SystemView/Tests/SEGGER_SYSVIEW_PublicAPI_Test.c"
      for (Index = 1; Index <= 2; Index++) {
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
    log_warn "No gcov-compatible coverage reporter was found; raw coverage data remains in ${BUILD_DIR}."
  fi
}

get_ctest_test_count() {
  local ctest_output
  local test_count

  if ! ctest_output="$(ctest --test-dir "${BUILD_DIR}" -N 2>&1)"; then
    printf '%s\n' "${ctest_output}" >&2
    log_error "CTest test discovery failed."
  fi

  test_count="$(printf '%s\n' "${ctest_output}" | awk '/Total Tests:/ { count = $3 } END { print count + 0 }')"
  printf '%s\n' "${test_count}"
}

print_configuration() {
  print_section "SystemView Build Configuration"
  echo ""
  echo "Build Type:          ${BUILD_TYPE}"
  echo "Build Directory:     ${BUILD_DIR}"
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
  echo "  Warnings as Errors: $(bool_to_on_off "${WARNINGS_AS_ERRORS}")"
  echo ""
  echo "Build Target:"
  echo "  - SEGGER_SYSVIEW_PublicAPI_Test"
  echo ""
}

clean_outputs() {
  if [[ "${CLEAN_BUILD}" != true ]]; then
    return
  fi

  if [[ "${BUILD_DIR}" != "${SCRIPT_DIR}/build/"* ]]; then
    log_error "Refusing to clean unexpected build directory: ${BUILD_DIR}"
  fi

  if [[ -d "${BUILD_DIR}" ]]; then
    log_info "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
  fi
}

configure_cmake() {
  local configure_args

  print_section "Configuring CMake"
  configure_args=(
    -S "${SCRIPT_DIR}"
    -B "${BUILD_DIR}"
  )
  if [[ "${#CMAKE_GENERATOR[@]}" -gt 0 ]]; then
    configure_args+=("${CMAKE_GENERATOR[@]}")
  fi
  configure_args+=(
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DSYSTEMVIEW_ENABLE_COVERAGE="${COVERAGE}"
    -DSYSTEMVIEW_WARNINGS_AS_ERRORS="$(bool_to_on_off "${WARNINGS_AS_ERRORS}")"
  )
  cmake "${configure_args[@]}"

  if [[ "${COVERAGE}" == "ON" ]]; then
    find "${BUILD_DIR}" -name '*.gcda' -exec rm -f {} +
  fi
}

build_systemview() {
  local build_args

  print_section "Building SystemView"
  build_args=(--build "${BUILD_DIR}" -j "${JOBS}")
  if [[ "${#VERBOSE_FLAG[@]}" -gt 0 ]]; then
    build_args+=("${VERBOSE_FLAG[@]}")
  fi
  cmake "${build_args[@]}"
  log_success "Build completed successfully."
}

run_unit_tests() {
  local test_count

  if [[ "${RUN_TESTS}" != true ]]; then
    log_info "Skipping CTest."
    return
  fi

  print_section "Running SystemView Unit Tests"
  test_count="$(get_ctest_test_count)"
  if [[ "${test_count}" -eq 0 ]]; then
    log_error "CTest did not discover any unit tests."
  fi

  log_info "Discovered ${test_count} unit tests."
  ctest --test-dir "${BUILD_DIR}" --output-on-failure -j "${JOBS}"
  log_success "SystemView unit tests passed."
}

print_build_summary() {
  local exe="${BUILD_DIR}/bin/SEGGER_SYSVIEW_PublicAPI_Test"

  echo ""
  print_section "Build Summary"
  echo ""
  echo "Build Directory:   ${BUILD_DIR}"
  echo "Build Type:        ${BUILD_TYPE}"
  echo ""
  echo "Build Outputs:"
  if [[ -f "${exe}" ]]; then
    echo "  ${exe}"
  fi
  echo ""
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
  build_systemview
  echo ""
  run_unit_tests
  echo ""

  if [[ "${COVERAGE}" == "ON" ]]; then
    print_section "Coverage Summary"
    print_coverage_report
    echo ""
  fi

  print_build_summary
  log_success "SystemView unit-test build completed"
}

main "$@"
