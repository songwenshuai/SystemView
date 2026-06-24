#!/bin/bash

# Exit immediately if a command exits with a non-zero status
# -e: Exit on error
# -E: Inherit ERR trap in functions
# -u: Error on undefined variables
# -o pipefail: Fail pipe if any command fails
set -eEuo pipefail

#==============================================================================
# SharedMem Driver Build Script
# Platform: STM32MP ARM Cortex-A7 Linux Embedded System
# Purpose: Cross-compilation build automation for kernel module
#==============================================================================

#==============================================================================
# Build Configuration
#==============================================================================

# Determine script directory location
BUILD_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Default build target
BUILD_TARGET=${1:-arm}

echo "SharedMem Driver Build Script"
echo "===================================="
echo "Build target: $BUILD_TARGET"
echo "Script directory: $BUILD_SCRIPT_DIR"

#==============================================================================
# Environment Setup
#==============================================================================

# Cross-compilation environment variables (OPENSTLINUX_DIR, KERNEL_DIR, SYSROOT_PATH)
# are expected to be set by parent kernel/build.sh or exported by the user.
# Use default OPENSTLINUX_DIR if not set.
: "${OPENSTLINUX_DIR:=/home/wssong/workspace/openstlinux}"
export OPENSTLINUX_DIR

# Load ARM32 toolchain for arm builds (sources environment-setup script)
if [ "$BUILD_TARGET" = "arm" ]; then
    if [ -d "$OPENSTLINUX_DIR/sdk" ]; then
        echo "Loading ARM32 cross-compilation toolchain..."
        source "${OPENSTLINUX_DIR}/sdk/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi"
    else
        echo "Warning: OpenSTLinux SDK not found at $OPENSTLINUX_DIR"
    fi
fi

#==============================================================================
# Build Execution
#==============================================================================

# Change to script directory to ensure Makefile paths resolve correctly
cd "$BUILD_SCRIPT_DIR"

case "$BUILD_TARGET" in
    "arm")
        echo "Building for ARM Cortex-A7 (32-bit)..."
        make -f Makefile.arm
        ;;
    "aarch64")
        echo "Building for ARM Cortex-A76 (64-bit)..."
        make -f Makefile.aarch64
        ;;
    "native")
        echo "Building with native kernel build system..."
        KBUILD_DIR="${KERNEL_DIR:-${KERNEL_BUILD_DIR:-/lib/modules/$(uname -r)/build}}"
        if [ ! -d "$KBUILD_DIR" ]; then
            echo "KERNEL_DIR or KERNEL_BUILD_DIR must point to a kernel build directory"
            exit 1
        fi
        make -C "$KBUILD_DIR" M="${BUILD_SCRIPT_DIR}/Kernel" modules
        ;;
    *)
        echo "Usage: $0 [arm|aarch64|native]"
        echo "  arm      - Build for ARM Cortex-A7 (32-bit) with OpenSTLinux toolchain"
        echo "  aarch64  - Build for ARM Cortex-A76 (64-bit)"
        echo "  native   - Build with native kernel build system"
        exit 1
        ;;
esac

echo "Build completed."

#==============================================================================
# End of Build Script
#==============================================================================
