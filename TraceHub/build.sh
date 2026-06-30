#!/bin/bash

# ==============================================================================
# TraceHub Build Script
# ==============================================================================
#
# This script builds the TraceHub application - RTT Trace and Debug Bridge.
#
# Usage:
#   ./build.sh [options]
#
# Options:
#   -p, --prefix PATH    Installation prefix (default: install)
#   -t, --type TYPE      Build type: Debug or Release (default: Debug)
#   -a, --arch ARCH      Build target: HOST or ARM64 (default: HOST)
#   -c, --clean          Clean build directory before building
#   -j, --jobs N         Number of parallel jobs (default: auto-detect)
#   -b, --backend NAME   Memory backend: memshm or smem (default: HOST memshm, ARM64 smem)
#   --toolchain PATH     CMake toolchain file for cross-compilation
#   -v, --verbose        Enable verbose output
#   -r, --run            Run the application after successful build
#   -h, --help           Show this help message
#
# Examples:
#   ./build.sh                              # Build with defaults
#   ./build.sh -t Release                   # Release build
#   ./build.sh -a ARM64 --toolchain path.cmake  # ARM64 cross-compilation
#   ./build.sh -c -j 4                      # Clean build with 4 jobs
#
# ==============================================================================

# Exit immediately if a command exits with a non-zero status
set -eEuo pipefail

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ==============================================================================
# Default Configuration
# ==============================================================================

BUILD_TYPE="Debug"
TARGET_ARCH="HOST"
BUILD_DIR=""
INSTALL_PREFIX=""
CLEAN_BUILD=0
VERBOSE=0
RUN_AFTER_BUILD=0
JOBS=$(nproc 2>/dev/null || echo 4)
MEMORY_BACKEND=""
BUILD_UNIT_TESTS=""
BUILD_SIM_TESTS=""
TOOLCHAIN_FILE=""
APP_BIN="tracehub"
INSTALL_BINDIR="bin"
INSTALL_LIBDIR="lib"
APP_ARGS=()  # Arguments to pass to tracehub when using -r

# ==============================================================================
# Color output
# ==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ==============================================================================
# Helper Functions
# ==============================================================================

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_section() {
    echo -e "${CYAN}=== $1 ===${NC}"
}

require_option_argument() {
    local option="$1"
    local value="${2-}"

    if [ -z "$value" ] || [ "$value" = "--" ]; then
        print_error "$option requires an argument"
        exit 1
    fi
}

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build Configuration:
  -p, --prefix PATH         Installation prefix (default: install)
  -t, --type TYPE           Build type: Debug or Release (default: Debug)
  -a, --arch ARCH           Build target: HOST or ARM64 (default: HOST)
  -c, --clean               Clean build directory before building
  -j, --jobs N              Number of parallel jobs (default: auto-detect)
  -b, --backend NAME        Memory backend: memshm or smem (default: HOST memshm, ARM64 smem)
  --toolchain PATH          CMake toolchain file for cross-compilation
  -v, --verbose             Enable verbose output
  -r, --run                 Run the application after successful build
  --                        Separator for app arguments (use with -r)

Help:
  -h, --help                Show this help message

Supported Build Targets:
  HOST                      - Native host build using the current compiler target
  ARM64                     - Linux ARM64 target build

Build Outputs:
  tracehub                  - RTT Trace and Debug Bridge

Examples:
  $0                                    # Build host simulation backend
  $0 -t Release                         # Build (Release)
  $0 -a ARM64 --toolchain path.cmake     # Build for ARM64
  $0 --backend smem                      # Build Linux SharedMem backend
  $0 -c -t Release                      # Clean release build

Notes:
  - Requires runtime library to be available
  - Use --toolchain with -a ARM64 when cross-compiling from a non-ARM64 host

EOF
}

# ==============================================================================
# Parse Command Line Arguments
# ==============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--prefix)
            require_option_argument "$1" "${2-}"
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -t|--type)
            require_option_argument "$1" "${2-}"
            BUILD_TYPE="$2"
            if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
                print_error "Invalid build type: $BUILD_TYPE (must be Debug or Release)"
                exit 1
            fi
            shift 2
            ;;
        -a|--arch)
            require_option_argument "$1" "${2-}"
            TARGET_ARCH="$2"
            if [[ "$TARGET_ARCH" != "HOST" && "$TARGET_ARCH" != "ARM64" ]]; then
                print_error "Invalid architecture: $TARGET_ARCH (must be HOST or ARM64)"
                exit 1
            fi
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD=1
            shift
            ;;
        -j|--jobs)
            require_option_argument "$1" "${2-}"
            JOBS="$2"
            if ! [[ "$JOBS" =~ ^[0-9]+$ ]]; then
                print_error "Invalid number of jobs: $JOBS"
                exit 1
            fi
            shift 2
            ;;
        -b|--backend)
            require_option_argument "$1" "${2-}"
            MEMORY_BACKEND="$2"
            if [[ "$MEMORY_BACKEND" != "smem" && "$MEMORY_BACKEND" != "memshm" ]]; then
                print_error "Invalid memory backend: $MEMORY_BACKEND (must be smem or memshm)"
                exit 1
            fi
            shift 2
            ;;
        --toolchain)
            require_option_argument "$1" "${2-}"
            TOOLCHAIN_FILE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -r|--run)
            RUN_AFTER_BUILD=1
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        --)
            # Everything after -- is passed to the application
            shift
            APP_ARGS=("$@")
            break
            ;;
        *)
            print_error "Unknown option: $1"
            echo ""
            show_usage
            exit 1
            ;;
    esac
done

# ==============================================================================
# Environment Setup
# ==============================================================================

# Set default installation prefix if not specified
if [ -z "${INSTALL_PREFIX:-}" ]; then
    INSTALL_PREFIX="$SCRIPT_DIR/install"
fi

# Select target and host platform
HOST_OS="$(uname -s)"
HOST_MACHINE="$(uname -m)"
HOST_IS_WINDOWS=0
case "$HOST_OS" in
    MINGW*|MSYS*|CYGWIN*)
        HOST_IS_WINDOWS=1
        ;;
esac

if [ -z "$MEMORY_BACKEND" ]; then
    if [[ "$TARGET_ARCH" == "HOST" ]]; then
        MEMORY_BACKEND="memshm"
    else
        MEMORY_BACKEND="smem"
    fi
fi

if [[ "$HOST_OS" == "Darwin" && "$TARGET_ARCH" == "HOST" && "$MEMORY_BACKEND" == "smem" ]]; then
    print_error "SMEM backend is not available for native macOS builds"
    print_info "Use --backend memshm for macOS host simulation"
    exit 1
fi

if [[ "$MEMORY_BACKEND" == "memshm" ]]; then
    if [ "$HOST_IS_WINDOWS" -eq 1 ]; then
        BUILD_SIM_TESTS="OFF"
    else
        BUILD_SIM_TESTS="ON"
    fi
else
    BUILD_SIM_TESTS="OFF"
fi

if [[ "$TARGET_ARCH" == "HOST" ]]; then
    BUILD_UNIT_TESTS="ON"
else
    BUILD_UNIT_TESTS="OFF"
fi

# Validate toolchain file
if [ -n "$TOOLCHAIN_FILE" ] && [ ! -f "$TOOLCHAIN_FILE" ]; then
    print_warning "Toolchain file not found: $TOOLCHAIN_FILE"
    TOOLCHAIN_FILE=""
fi

if [[ "$TARGET_ARCH" == "ARM64" && -z "$TOOLCHAIN_FILE" ]]; then
    if [[ "$HOST_OS" == "Linux" && ( "$HOST_MACHINE" == "aarch64" || "$HOST_MACHINE" == "arm64" ) ]]; then
        print_info "Using native Linux ARM64 system toolchain"
    else
        print_error "ARM64 target build requires --toolchain PATH or a native Linux ARM64 host"
        exit 1
    fi
fi

# Set build directory name
BUILD_DIR="build/${TARGET_ARCH}_${BUILD_TYPE}"

# ==============================================================================
# Pre-build Validation
# ==============================================================================

print_section "TraceHub Build Configuration"
echo ""
echo "Build Type:          $BUILD_TYPE"
echo "Build Target:        $TARGET_ARCH"
if [ -n "$TOOLCHAIN_FILE" ]; then
    echo "Toolchain File:      $TOOLCHAIN_FILE"
fi
echo "Build Directory:     $BUILD_DIR"
echo "Install Prefix:      $INSTALL_PREFIX"
echo "Parallel Jobs:       $JOBS"
echo ""
echo "Build Options:"
echo "  Clean Build:       $([ $CLEAN_BUILD -eq 1 ] && echo "ON" || echo "OFF")"
echo "  Verbose:           $([ $VERBOSE -eq 1 ] && echo "ON" || echo "OFF")"
echo "  Memory Backend:    ${MEMORY_BACKEND:-CMake default}"
echo "  Unit Tests:        $BUILD_UNIT_TESTS"
echo "  Simulation Tests:  $BUILD_SIM_TESTS"
echo ""
echo "Build Target:"
echo "  - $APP_BIN (RTT Trace and Debug Bridge)"
echo ""

# ==============================================================================
# Clean Build Directory (if requested)
# ==============================================================================

if [ $CLEAN_BUILD -eq 1 ]; then
    if [ -d "$SCRIPT_DIR/$BUILD_DIR" ]; then
        print_info "Cleaning build directory: $BUILD_DIR"
        rm -rf "$SCRIPT_DIR/$BUILD_DIR"
    fi
    if [ -f "$SCRIPT_DIR/$APP_BIN" ]; then
        print_info "Removing existing executable: $APP_BIN"
        rm -f "$SCRIPT_DIR/$APP_BIN"
    fi
    if [ -f "$SCRIPT_DIR/rtt_sim_rtos" ]; then
        rm -f "$SCRIPT_DIR/rtt_sim_rtos"
    fi
    if [ -f "$SCRIPT_DIR/rtt_sim_linux" ]; then
        rm -f "$SCRIPT_DIR/rtt_sim_linux"
    fi
fi

# ==============================================================================
# Create Build Directory
# ==============================================================================

print_info "Creating build directory..."
mkdir -p "$SCRIPT_DIR/$BUILD_DIR"

# ==============================================================================
# Build Process
# ==============================================================================

# Configure CMake
print_section "Configuring CMake"

CMAKE_ARGS=(
    "-B" "$SCRIPT_DIR/$BUILD_DIR"
    "-S" "$SCRIPT_DIR"
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
    "-DCMAKE_INSTALL_BINDIR=$INSTALL_BINDIR"
    "-DCMAKE_INSTALL_LIBDIR=$INSTALL_LIBDIR"
    "-DBUILD_UNIT_TESTS=$BUILD_UNIT_TESTS"
    "-DBUILD_SIM_TESTS=$BUILD_SIM_TESTS"
    "-GNinja"
)

# macOS: Ninja doesn't support RPATH modification for Mach-O binaries
# Set CMAKE_BUILD_WITH_INSTALL_RPATH to avoid the relinking step
if [[ "$(uname -s)" == "Darwin" ]]; then
    CMAKE_ARGS+=("-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON")
fi

case "$MEMORY_BACKEND" in
    memshm)
        CMAKE_ARGS+=("-DUSE_MEMSHM_BACKEND=ON")
        ;;
    smem)
        CMAKE_ARGS+=("-DUSE_MEMSHM_BACKEND=OFF")
        ;;
esac

# Add toolchain file if specified
if [ -n "$TOOLCHAIN_FILE" ]; then
    CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE")
    print_info "Using toolchain file: $TOOLCHAIN_FILE"
fi

if [ $VERBOSE -eq 1 ]; then
    CMAKE_ARGS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
fi

if ! cmake "${CMAKE_ARGS[@]}"; then
    print_error "CMake configuration failed"
    exit 1
fi

if [ -f "$SCRIPT_DIR/$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$SCRIPT_DIR/$BUILD_DIR/compile_commands.json" "$SCRIPT_DIR/build/compile_commands.json"
fi

echo ""

# Build
print_section "Building TraceHub"

if [ $VERBOSE -eq 1 ]; then
    if ! cmake --build "$SCRIPT_DIR/$BUILD_DIR" -j "$JOBS" --verbose; then
        print_error "Build failed"
        exit 1
    fi
else
    if ! cmake --build "$SCRIPT_DIR/$BUILD_DIR" -j "$JOBS"; then
        print_error "Build failed"
        exit 1
    fi
fi

echo ""

# ==============================================================================
# Install Executable
# ==============================================================================

print_section "Installing"

if ! cmake --install "$SCRIPT_DIR/$BUILD_DIR"; then
    print_error "Installation failed"
    exit 1
fi

# Copy executable to script directory for convenience
if [ -f "$SCRIPT_DIR/$BUILD_DIR/$APP_BIN" ]; then
    cp "$SCRIPT_DIR/$BUILD_DIR/$APP_BIN" "$SCRIPT_DIR/"
    chmod +x "$SCRIPT_DIR/$APP_BIN"
    print_success "Executable installed successfully"
else
    print_error "Executable not found: $SCRIPT_DIR/$BUILD_DIR/$APP_BIN"
    exit 1
fi

# Copy test programs if they exist
if [ -f "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_rtos" ]; then
    cp "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_rtos" "$SCRIPT_DIR/"
    chmod +x "$SCRIPT_DIR/rtt_sim_rtos"
fi
if [ -f "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_linux" ]; then
    cp "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_linux" "$SCRIPT_DIR/"
    chmod +x "$SCRIPT_DIR/rtt_sim_linux"
fi
if [ -f "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_sysview_tcp_client" ]; then
    cp "$SCRIPT_DIR/$BUILD_DIR/Tests/rtt_sim_sysview_tcp_client" "$SCRIPT_DIR/"
    chmod +x "$SCRIPT_DIR/rtt_sim_sysview_tcp_client"
fi
if [ "$BUILD_SIM_TESTS" = "OFF" ]; then
    rm -f "$SCRIPT_DIR/rtt_sim_rtos" \
          "$SCRIPT_DIR/rtt_sim_linux" \
          "$SCRIPT_DIR/rtt_sim_sysview_tcp_client"
fi

echo ""

# ==============================================================================
# Build Summary
# ==============================================================================

print_success "Build completed successfully!"
echo ""
print_section "Build Summary"
echo ""
echo "Build Directory:   $SCRIPT_DIR/$BUILD_DIR"
echo "Build Target:      $TARGET_ARCH"
echo "Build Type:        $BUILD_TYPE"
echo ""

# Show installed executables
if [ -d "$INSTALL_PREFIX/$INSTALL_BINDIR" ]; then
    echo "Installed Executables:"
    while IFS= read -r exe; do
        if [ -f "$exe" ]; then
            SIZE=$(ls -lh "$exe" | awk '{print $5}')
            EXENAME=$(basename "$exe")
            printf "  %-50s %10s\n" "$EXENAME" "$SIZE"
        fi
    done < <(find "$INSTALL_PREFIX/$INSTALL_BINDIR" -type f -executable 2>/dev/null | sort)
    echo ""
fi

# Show installed libraries
if [ -d "$INSTALL_PREFIX/$INSTALL_LIBDIR" ]; then
    echo "Installed Libraries:"
    while IFS= read -r lib; do
        if [ -f "$lib" ]; then
            SIZE=$(ls -lh "$lib" | awk '{print $5}')
            LIBNAME=$(basename "$lib")
            printf "  %-50s %10s\n" "$LIBNAME" "$SIZE"
        fi
    done < <(find "$INSTALL_PREFIX/$INSTALL_LIBDIR" \( -name "*.so*" -o -name "*.a" \) 2>/dev/null | sort)
    echo ""
fi

echo "Convenience Copy:"
echo "  $SCRIPT_DIR/$APP_BIN"
if [ "$BUILD_SIM_TESTS" = "ON" ] && [ -f "$SCRIPT_DIR/rtt_sim_rtos" ]; then
    echo "  $SCRIPT_DIR/rtt_sim_rtos"
fi
if [ "$BUILD_SIM_TESTS" = "ON" ] && [ -f "$SCRIPT_DIR/rtt_sim_linux" ]; then
    echo "  $SCRIPT_DIR/rtt_sim_linux"
fi
if [ "$BUILD_SIM_TESTS" = "ON" ] && [ -f "$SCRIPT_DIR/rtt_sim_sysview_tcp_client" ]; then
    echo "  $SCRIPT_DIR/rtt_sim_sysview_tcp_client"
fi
echo ""

# Print usage hint
if [ $RUN_AFTER_BUILD -eq 0 ]; then
    print_info "To run the application:"
    echo "  $INSTALL_PREFIX/$INSTALL_BINDIR/$APP_BIN --help"
    echo "  or"
    echo "  LD_LIBRARY_PATH=$INSTALL_PREFIX/$INSTALL_LIBDIR $SCRIPT_DIR/$APP_BIN --help"
    echo ""
fi

# ==============================================================================
# Run Application (if requested)
# ==============================================================================

if [ $RUN_AFTER_BUILD -eq 1 ]; then
    echo ""
    print_info "Running application..."
    echo ""

    LIB_PATH="${INSTALL_PREFIX}/$INSTALL_LIBDIR"

    if [ -n "${LD_LIBRARY_PATH:-}" ]; then
        export LD_LIBRARY_PATH="${LIB_PATH}:${LD_LIBRARY_PATH}"
    else
        export LD_LIBRARY_PATH="${LIB_PATH}"
    fi

    exec "$SCRIPT_DIR/$APP_BIN" "${APP_ARGS[@]}"
fi
