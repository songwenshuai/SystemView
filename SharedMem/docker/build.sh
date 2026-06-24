#!/usr/bin/env bash

#==============================================================================
# SharedMem Docker Build Wrapper
# Platform: macOS or Linux host with Docker
# Purpose: Build the Docker kbuild image and compile SharedMem/Kernel in it
#==============================================================================
#
# This host-side wrapper creates the Docker image, mounts the SystemView
# repository, mounts the target Linux kernel build directory, and passes the
# explicit kbuild settings into the container entrypoint.
#
# Required environment:
#   KERNEL_BUILD_DIR                 Absolute host path to target kernel build dir
#
# Optional environment:
#   ARCH                             Linux kbuild architecture, such as arm
#                                    or arm64
#   CROSS_COMPILE                    Cross-compiler prefix inside the container
#   JOBS                             Parallel make job count
#   SDK_DIR                          Absolute host path to vendor SDK/toolchain
#   IMAGE_NAME                       Docker image name
#   DOCKER_PLATFORM                  Docker platform, such as linux/amd64
#   INSTALL_DEBIAN_CROSS_TOOLCHAINS  Set to 1 to install Debian ARM toolchains
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

require_absolute_path() {
  local name="$1"
  local path="$2"

  case "${path}" in
    /*) ;;
    *) log_error "${name} must be an absolute host path: ${path}" ;;
  esac
}

#==============================================================================
# Environment Forwarding Helpers
#==============================================================================

append_optional_env() {
  local name="$1"
  local value="${!name:-}"

  if [[ -n "${value}" ]]; then
    DOCKER_ENV+=("-e" "${name}=${value}")
  fi
}

#==============================================================================
# Path Discovery
#==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHAREDMEM_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_DIR="$(cd "${SHAREDMEM_DIR}/.." && pwd)"

#==============================================================================
# Build Configuration
#==============================================================================

IMAGE_NAME="${IMAGE_NAME:-systemview-sharedmem-kbuild}"
CONTAINER_REPO_DIR="${CONTAINER_REPO_DIR:-/work/SystemView}"
CONTAINER_KERNEL_BUILD_DIR="${CONTAINER_KERNEL_BUILD_DIR:-/work/kernel-build}"
CONTAINER_SDK_DIR="${CONTAINER_SDK_DIR:-/work/sdk}"
KERNEL_BUILD_DIR="${KERNEL_BUILD_DIR:-}"
SDK_DIR="${SDK_DIR:-}"
INSTALL_DEBIAN_CROSS_TOOLCHAINS="${INSTALL_DEBIAN_CROSS_TOOLCHAINS:-0}"

#==============================================================================
# Pre-build Validation
#==============================================================================

if [[ -z "${KERNEL_BUILD_DIR}" ]]; then
  log_error "KERNEL_BUILD_DIR must point to the host Linux kernel build directory."
fi

require_absolute_path "Kernel build directory" "${KERNEL_BUILD_DIR}"
require_dir "Kernel build directory" "${KERNEL_BUILD_DIR}"
require_dir "Repository directory" "${REPO_DIR}"

#==============================================================================
# Docker Build Arguments
#==============================================================================

DOCKER_BUILD_ARGS=(
  --build-arg "INSTALL_DEBIAN_CROSS_TOOLCHAINS=${INSTALL_DEBIAN_CROSS_TOOLCHAINS}"
)

#==============================================================================
# Docker Mounts
#==============================================================================

DOCKER_RUN_ARGS=(
  --rm
  -t
  -v "${REPO_DIR}:${CONTAINER_REPO_DIR}"
  -v "${KERNEL_BUILD_DIR}:${CONTAINER_KERNEL_BUILD_DIR}"
)

if [[ -n "${SDK_DIR}" ]]; then
  require_absolute_path "SDK directory" "${SDK_DIR}"
  require_dir "SDK directory" "${SDK_DIR}"
  DOCKER_RUN_ARGS+=(-v "${SDK_DIR}:${CONTAINER_SDK_DIR}:ro")
fi

#==============================================================================
# Docker Platform and Terminal Options
#==============================================================================

if [[ -t 0 ]]; then
  DOCKER_RUN_ARGS+=(-i)
fi

if [[ -n "${DOCKER_PLATFORM:-}" ]]; then
  DOCKER_BUILD_ARGS+=(--platform "${DOCKER_PLATFORM}")
  DOCKER_RUN_ARGS+=(--platform "${DOCKER_PLATFORM}")
fi

#==============================================================================
# Container Environment
#==============================================================================

DOCKER_ENV=(
  -e "REPO_DIR=${CONTAINER_REPO_DIR}"
  -e "SHAREDMEM_KERNEL_DIR=${CONTAINER_REPO_DIR}/SharedMem/Kernel"
  -e "KERNEL_BUILD_DIR=${CONTAINER_KERNEL_BUILD_DIR}"
)

if [[ -n "${SDK_DIR}" ]]; then
  DOCKER_ENV+=(-e "SDK_DIR=${CONTAINER_SDK_DIR}")
fi

append_optional_env ARCH
append_optional_env CROSS_COMPILE
append_optional_env JOBS

#==============================================================================
# Build Execution
#==============================================================================

log_info "Building Docker image: ${IMAGE_NAME}"
docker build "${DOCKER_BUILD_ARGS[@]}" -t "${IMAGE_NAME}" "${SCRIPT_DIR}"

log_info "Building SharedMem kernel module in Docker"
docker run "${DOCKER_RUN_ARGS[@]}" "${DOCKER_ENV[@]}" "${IMAGE_NAME}"
