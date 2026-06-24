#!/usr/bin/env bash

#==============================================================================
# SharedMem Kernel Module Container Entrypoint
# Platform: Linux container
# Purpose: Invoke kbuild for the host-mounted SharedMem kernel module
#==============================================================================
#
# This script runs inside the Docker image.  It validates the mounted repository
# and target kernel build directory before calling kbuild.  The target kernel
# build directory must come from the Linux system or SDK that will load the
# generated module.
#
# Required environment:
#   KERNEL_BUILD_DIR       Container path to the target kernel build directory
#
# Optional environment:
#   REPO_DIR               Container path to the SystemView repository
#   SHAREDMEM_KERNEL_DIR   Container path to SharedMem/Kernel
#   ARCH                   Linux kbuild architecture, such as arm or arm64
#   CROSS_COMPILE          Cross-compiler prefix visible inside the container
#   JOBS                   Parallel make job count
#
# Output:
#   SharedMem/Kernel/SharedMem.ko
#
#==============================================================================

set -euo pipefail

#==============================================================================
# Logging Helpers
#==============================================================================

log_info() {
  printf '[INFO] %s\n' "$1"
}

log_error() {
  printf '[ERROR] %s\n' "$1" >&2
  exit 1
}

#==============================================================================
# Validation Helpers
#==============================================================================

require_dir() {
  local name="$1"
  local path="$2"

  if [[ ! -d "${path}" ]]; then
    log_error "${name} does not exist or is not a directory: ${path}"
  fi
}

require_file() {
  local name="$1"
  local path="$2"

  if [[ ! -f "${path}" ]]; then
    log_error "${name} does not exist: ${path}"
  fi
}

#==============================================================================
# Build Configuration
#==============================================================================

REPO_DIR="${REPO_DIR:-/work/SystemView}"
SHAREDMEM_KERNEL_DIR="${SHAREDMEM_KERNEL_DIR:-${REPO_DIR}/SharedMem/Kernel}"
KERNEL_BUILD_DIR="${KERNEL_BUILD_DIR:-}"
JOBS="${JOBS:-$(nproc)}"

#==============================================================================
# Pre-build Validation
#==============================================================================

if [[ -z "${KERNEL_BUILD_DIR}" ]]; then
  log_error "KERNEL_BUILD_DIR must point to a mounted Linux kernel build directory."
fi

require_dir "Repository directory" "${REPO_DIR}"
require_dir "SharedMem kernel module directory" "${SHAREDMEM_KERNEL_DIR}"
require_dir "Kernel build directory" "${KERNEL_BUILD_DIR}"
require_file "SharedMem.c" "${SHAREDMEM_KERNEL_DIR}/SharedMem.c"
require_file "SharedMem kernel Makefile" "${SHAREDMEM_KERNEL_DIR}/Makefile"
require_file "Kernel build Makefile" "${KERNEL_BUILD_DIR}/Makefile"

#==============================================================================
# Kbuild Argument Assembly
#==============================================================================

MAKE_ARGS=(
  -C "${KERNEL_BUILD_DIR}"
  M="${SHAREDMEM_KERNEL_DIR}"
  -j "${JOBS}"
)

if [[ -n "${ARCH:-}" ]]; then
  MAKE_ARGS+=(ARCH="${ARCH}")
fi

if [[ -n "${CROSS_COMPILE:-}" ]]; then
  MAKE_ARGS+=(CROSS_COMPILE="${CROSS_COMPILE}")
fi

#==============================================================================
# Build Summary
#==============================================================================

log_info "Repository directory: ${REPO_DIR}"
log_info "Module directory: ${SHAREDMEM_KERNEL_DIR}"
log_info "Kernel build directory: ${KERNEL_BUILD_DIR}"
log_info "Parallel jobs: ${JOBS}"

if [[ -n "${ARCH:-}" ]]; then
  log_info "ARCH: ${ARCH}"
fi

if [[ -n "${CROSS_COMPILE:-}" ]]; then
  log_info "CROSS_COMPILE: ${CROSS_COMPILE}"
fi

#==============================================================================
# Build Execution
#==============================================================================

make "${MAKE_ARGS[@]}" modules

log_info "Module output: ${SHAREDMEM_KERNEL_DIR}/SharedMem.ko"
