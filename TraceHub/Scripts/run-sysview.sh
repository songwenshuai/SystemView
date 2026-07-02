#!/bin/bash

# ==============================================================================
# TraceHub SystemView Only Mode Runner
# ==============================================================================
#
# Linux SharedMem backend runner. This script loads the SharedMem kernel
# module and runs tracehub in SystemView only mode, providing TCP server for
# SEGGER SystemView service.
#
# Usage:
#   sudo Scripts/run-sysview.sh [options]
#
# Options:
#   --phys-addr ADDR     SharedMem physical address (hex, default: 0x10040000)
#   --mem-size SIZE      SharedMem size (hex, default: 0x20000)
#   --device PATH        SharedMem device path (default: /dev/shared_mem0)
#   -p, --port PORT      SystemView TCP port (default: 19111)
#   -c, --channel CH     SystemView RTT channel (default: 2)
#   --rtt-timeout-ms MS  RTTCB discovery timeout in milliseconds (default: 0)
#   --log-dir DIR        Directory for generated log and record files
#   -k, --ko PATH        Path to SharedMem.ko (default: auto-detect)
#   --force-reload       Unload an existing SharedMem module before loading
#   --use-loaded-module  Use an already loaded SharedMem module
#   -h, --help           Show this help message
#
# Environment:
#   TRACEHUB_BIN         Explicit tracehub executable path
#
# Examples:
#   sudo Scripts/run-sysview.sh
#   sudo Scripts/run-sysview.sh --port 19111
#   sudo Scripts/run-sysview.sh --phys-addr 0x80000000 --mem-size 0x10000
#
# Project decision:
#   This script must remain self-contained for independent field use.
#   Do not extract shared helpers into sourced common scripts.
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Script Directory
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# ==============================================================================
# Default Configuration
# ==============================================================================

RTT_REGION_ADDR="0x10040000"
MEM_SIZE="0x20000"
DEVICE_PATH="/dev/shared_mem0"
SYSVIEW_PORT="19111"
SYSVIEW_CHANNEL="2"
RTT_TIMEOUT_MS="0"
LOG_DIR=""
KO_PATH=""
FORCE_RELOAD=0
USE_LOADED_MODULE=0
MODULE_LOADED_BY_SCRIPT=0
TRACEHUB_BIN="${TRACEHUB_BIN:-}"
TRACEHUB_PID=""

# ==============================================================================
# Color Output
# ==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_section() { echo -e "${CYAN}=== $1 ===${NC}"; }

# ==============================================================================
# Helper Functions
# ==============================================================================

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

SystemView Only Mode: Run SystemView TCP server (Terminal disabled).
Connect with SEGGER SystemView application to port $SYSVIEW_PORT.

Options:
  --phys-addr ADDR     SharedMem physical address (hex, default: $RTT_REGION_ADDR)
  --mem-size SIZE      SharedMem size (hex, default: $MEM_SIZE)
  --device PATH        SharedMem device path (default: $DEVICE_PATH)
  -p, --port PORT      SystemView TCP port (default: $SYSVIEW_PORT)
  -c, --channel CH     SystemView RTT channel (default: $SYSVIEW_CHANNEL)
  --rtt-timeout-ms MS  RTTCB discovery timeout in milliseconds (default: $RTT_TIMEOUT_MS, 0 waits until interrupted)
  --log-dir DIR        Directory for generated log and record files
  -k, --ko PATH        Path to SharedMem.ko (default: auto-detect)
  --force-reload       Unload an existing SharedMem module before loading
  --use-loaded-module  Use an already loaded SharedMem module
  -h, --help           Show this help message

Environment:
  TRACEHUB_BIN         Explicit tracehub executable path. If unset, this script
                       requires exactly one SMEM tracehub under $ROOT_DIR/build.

Examples:
  sudo $0
  sudo $0 --port 19111
  sudo $0 --phys-addr 0x80000000 --mem-size 0x10000

EOF
}

check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        print_error "This script requires the Linux SharedMem backend"
        print_info "Use $ROOT_DIR/build.sh --backend memshm and run tracehub with --shm for host simulation"
        exit 1
    fi
}

require_option_argument() {
    local option="$1"
    local value="${2-}"

    if [ -z "$value" ] || [[ "$value" == -* ]]; then
        print_error "$option requires an argument"
        exit 1
    fi
}

validate_module_options() {
    if [ "$FORCE_RELOAD" -eq 1 ] && [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--force-reload and --use-loaded-module are mutually exclusive"
        return 1
    fi

    return 0
}

require_module_update_privilege() {
    if [ "$(id -u)" -ne 0 ]; then
        print_error "Loading or replacing SharedMem requires root privileges"
        print_info "Use --use-loaded-module only when SharedMem is already loaded and $DEVICE_PATH is readable and writable"
        return 1
    fi

    return 0
}

require_device_access() {
    local device_path="$1"

    if [ ! -e "$device_path" ]; then
        print_error "Device node $device_path does not exist"
        return 1
    fi

    if [ ! -r "$device_path" ] || [ ! -w "$device_path" ]; then
        print_error "$device_path is not readable and writable"
        return 1
    fi

    return 0
}

load_kernel_module() {
    local ko_path="${KO_PATH:-/lib/modules/$(uname -r)/SharedMem.ko}"
    local module_rtt_region_addr_dec
    local mem_size_dec

    validate_module_options || exit 1

    if lsmod | grep -q "^SharedMem"; then
        if [ "$FORCE_RELOAD" -eq 1 ]; then
            require_module_update_privilege || exit 1

            print_info "Unloading existing SharedMem module..."
            if ! rmmod SharedMem; then
                print_error "Failed to unload existing SharedMem module"
                exit 1
            fi
            sleep 1
        elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
            require_device_access "$DEVICE_PATH" || exit 1
            print_success "Using already loaded SharedMem module"
            return 0
        else
            print_error "SharedMem module is already loaded"
            print_info "Use --use-loaded-module to reuse it or --force-reload to replace it"
            exit 1
        fi
    elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--use-loaded-module requires an already loaded SharedMem module"
        exit 1
    fi

    require_module_update_privilege || exit 1

    if [ ! -f "$ko_path" ]; then
        print_error "Kernel module not found: $ko_path"
        print_info "Please install the kernel module or specify the path with -k/--ko option"
        exit 1
    fi

    module_rtt_region_addr_dec=$((RTT_REGION_ADDR))
    mem_size_dec=$((MEM_SIZE))

    print_info "Loading SharedMem module (rtt_region_addr=$RTT_REGION_ADDR, rtt_region_size=$MEM_SIZE)..."
    print_info "Kernel module: $ko_path"
    if ! insmod "$ko_path" phys_addrs="$module_rtt_region_addr_dec" mem_sizes="$mem_size_dec"; then
        print_error "Failed to load kernel module"
        exit 1
    fi
    MODULE_LOADED_BY_SCRIPT=1

    sleep 1
    if ! require_device_access "$DEVICE_PATH"; then
        if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ]; then
            rmmod SharedMem 2>/dev/null || true
            MODULE_LOADED_BY_SCRIPT=0
        fi
        exit 1
    fi

    print_success "Kernel module loaded successfully"
}

cleanup() {
    local status=$?

    trap - EXIT INT TERM

    if [ -n "$TRACEHUB_PID" ] && kill -0 "$TRACEHUB_PID" 2>/dev/null; then
        kill -TERM "$TRACEHUB_PID" 2>/dev/null || true
        wait "$TRACEHUB_PID" 2>/dev/null || true
        TRACEHUB_PID=""
    fi

    print_info "Cleaning up..."
    if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ] && lsmod | grep -q "^SharedMem"; then
        rmmod SharedMem 2>/dev/null || true
        MODULE_LOADED_BY_SCRIPT=0
        print_info "Kernel module unloaded"
    fi

    exit "$status"
}

forward_signal() {
    local signal="$1"

    if [ -n "$TRACEHUB_PID" ] && kill -0 "$TRACEHUB_PID" 2>/dev/null; then
        kill "-$signal" "$TRACEHUB_PID" 2>/dev/null || true
    fi
}

run_tracehub() {
    local status

    "$TRACEHUB_BIN" "${TRACEHUB_ARGS[@]}" &
    TRACEHUB_PID=$!

    if wait "$TRACEHUB_PID"; then
        status=0
    else
        status=$?
        if kill -0 "$TRACEHUB_PID" 2>/dev/null; then
            if wait "$TRACEHUB_PID"; then
                status=0
            else
                status=$?
            fi
        fi
    fi

    TRACEHUB_PID=""
    return "$status"
}

resolve_tracehub() {
    local candidate
    local found=""
    local found_count=0

    if [ -n "$TRACEHUB_BIN" ]; then
        if [ -f "$TRACEHUB_BIN" ] && [ -x "$TRACEHUB_BIN" ]; then
            return 0
        fi

        print_error "tracehub executable configured by TRACEHUB_BIN is not executable: $TRACEHUB_BIN"
        exit 1
    fi

    for candidate in \
        "$ROOT_DIR/build/host-smem-debug/tracehub" \
        "$ROOT_DIR/build/host-smem-release/tracehub" \
        "$ROOT_DIR/build/linux-arm64-smem-debug/tracehub" \
        "$ROOT_DIR/build/linux-arm64-smem-release/tracehub"
    do
        if [ -f "$candidate" ] && [ -x "$candidate" ]; then
            found="$candidate"
            found_count=$((found_count + 1))
        fi
    done

    if [ "$found_count" -eq 1 ]; then
        TRACEHUB_BIN="$found"
        return 0
    fi

    if [ "$found_count" -gt 1 ]; then
        print_error "multiple SMEM tracehub executables found"
        print_info "Set TRACEHUB_BIN to the exact executable path"
        exit 1
    fi

    print_error "tracehub executable not found"
    print_info "Set TRACEHUB_BIN or provide one of:"
    print_info "  $ROOT_DIR/build/host-smem-debug/tracehub"
    print_info "  $ROOT_DIR/build/host-smem-release/tracehub"
    print_info "  $ROOT_DIR/build/linux-arm64-smem-debug/tracehub"
    print_info "  $ROOT_DIR/build/linux-arm64-smem-release/tracehub"
    exit 1
}

# ==============================================================================
# Parse Arguments
# ==============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --phys-addr)
            require_option_argument "$1" "${2-}"
            RTT_REGION_ADDR="$2"
            shift 2
            ;;
        --mem-size)
            require_option_argument "$1" "${2-}"
            MEM_SIZE="$2"
            shift 2
            ;;
        --device)
            require_option_argument "$1" "${2-}"
            DEVICE_PATH="$2"
            shift 2
            ;;
        -p|--port)
            require_option_argument "$1" "${2-}"
            SYSVIEW_PORT="$2"
            shift 2
            ;;
        -c|--channel)
            require_option_argument "$1" "${2-}"
            SYSVIEW_CHANNEL="$2"
            shift 2
            ;;
        --rtt-timeout-ms)
            require_option_argument "$1" "${2-}"
            RTT_TIMEOUT_MS="$2"
            shift 2
            ;;
        --log-dir)
            require_option_argument "$1" "${2-}"
            LOG_DIR="$2"
            shift 2
            ;;
        -k|--ko)
            require_option_argument "$1" "${2-}"
            KO_PATH="$2"
            shift 2
            ;;
        --force-reload)
            FORCE_RELOAD=1
            shift
            ;;
        --use-loaded-module)
            USE_LOADED_MODULE=1
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# ==============================================================================
# Main
# ==============================================================================

check_linux

# Resolve executable before loading the kernel module
resolve_tracehub

# Set up cleanup trap
trap cleanup EXIT

# Load kernel module
load_kernel_module

# Build tracehub arguments
TRACEHUB_ARGS=(
    "--addr" "$RTT_REGION_ADDR"
    "--size" "$MEM_SIZE"
    "--shm" "$DEVICE_PATH"
    "--rtt-timeout-ms" "$RTT_TIMEOUT_MS"
    "--systemview-port" "$SYSVIEW_PORT"
    "--systemview-channel" "$SYSVIEW_CHANNEL"
)

if [ -n "$LOG_DIR" ]; then
    TRACEHUB_ARGS+=("--log-dir" "$LOG_DIR")
fi

# Run tracehub
print_section "SystemView Only Mode - RTT TCP Server"
print_info "SystemView service: port $SYSVIEW_PORT, channel $SYSVIEW_CHANNEL"
echo ""

trap 'forward_signal INT' INT
trap 'forward_signal TERM' TERM

run_tracehub
