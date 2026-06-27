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
#   sudo ./run-sysview.sh [options]
#
# Options:
#   -a, --addr ADDR      RTT region backend address (hex, default: 0x10040000)
#   -s, --size SIZE      RTT region size (hex, default: 0x20000)
#   -p, --port PORT      SystemView TCP port (default: 19111)
#   -c, --channel CH     RTT channel (default: 2)
#   --rtt-timeout-ms MS  RTTCB discovery timeout in milliseconds (default: 0)
#   --log-dir DIR        Directory for generated log and record files
#   -k, --ko PATH        Path to SharedMem.ko (default: auto-detect)
#   --force-reload       Unload an existing SharedMem module before loading
#   -h, --help           Show this help message
#
# Examples:
#   sudo ./run-sysview.sh
#   sudo ./run-sysview.sh --port 19111
#   sudo ./run-sysview.sh --addr 0x80000000 --size 0x10000
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Script Directory
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ==============================================================================
# Default Configuration
# ==============================================================================

RTT_REGION_ADDR="0x10040000"
MEM_SIZE="0x20000"
SYSVIEW_PORT="19111"
SYSVIEW_CHANNEL="2"
RTT_TIMEOUT_MS="0"
LOG_DIR=""
KO_PATH=""
FORCE_RELOAD=0
MODULE_LOADED_BY_SCRIPT=0
TRACEHUB_BIN=""
TRACEHUB_PID=""
ORIGINAL_ARGS=("$@")

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
  -a, --addr ADDR      RTT region backend address (hex, default: $RTT_REGION_ADDR)
  -s, --size SIZE      RTT region size (hex, default: $MEM_SIZE)
  -p, --port PORT      SystemView TCP port (default: $SYSVIEW_PORT)
  -c, --channel CH     RTT channel (default: $SYSVIEW_CHANNEL)
  --rtt-timeout-ms MS  RTTCB discovery timeout in milliseconds (default: $RTT_TIMEOUT_MS, 0 waits until interrupted)
  --log-dir DIR        Directory for generated log and record files
  -k, --ko PATH        Path to SharedMem.ko (default: auto-detect)
  --force-reload       Unload an existing SharedMem module before loading
  -h, --help           Show this help message

Examples:
  sudo $0
  sudo $0 --port 19111
  sudo $0 --addr 0x80000000 --size 0x10000

EOF
}

check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        print_error "This script requires root privileges"
        echo "Please run with: sudo $0 $*"
        exit 1
    fi
}

check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        print_error "This script requires the Linux SharedMem backend"
        print_info "Use ./build.sh --backend memshm and run tracehub with --shm for host simulation"
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

load_kernel_module() {
    local ko_path="${KO_PATH:-/lib/modules/$(uname -r)/SharedMem.ko}"

    # Convert hex to decimal for module parameters
    local module_rtt_region_addr_dec=$((RTT_REGION_ADDR))
    local mem_size_dec=$((MEM_SIZE))

    # Check if module is already loaded
    if lsmod | grep -q "^SharedMem"; then
        if [ "$FORCE_RELOAD" -eq 0 ]; then
            print_error "SharedMem module is already loaded"
            print_info "Stop the existing user or rerun with --force-reload to replace it"
            exit 1
        fi
        print_info "Unloading existing SharedMem module..."
        if ! rmmod SharedMem; then
            print_error "Failed to unload existing SharedMem module"
            exit 1
        fi
        sleep 1
    fi

    # Check if ko file exists
    if [ ! -f "$ko_path" ]; then
        print_error "Kernel module not found: $ko_path"
        print_info "Please install the kernel module or specify the path with -k/--ko option"
        exit 1
    fi

    print_info "Loading SharedMem module (rtt_region_addr=$RTT_REGION_ADDR, rtt_region_size=$MEM_SIZE)..."
    print_info "Kernel module: $ko_path"
    if ! insmod "$ko_path" phys_addrs="$module_rtt_region_addr_dec" mem_sizes="$mem_size_dec"; then
        print_error "Failed to load kernel module"
        exit 1
    fi
    MODULE_LOADED_BY_SCRIPT=1

    # Wait for device node
    sleep 1
    if [ ! -e /dev/shared_mem0 ]; then
        print_error "Device node /dev/shared_mem0 not created"
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
    if [ -x "$SCRIPT_DIR/tracehub" ]; then
        TRACEHUB_BIN="$SCRIPT_DIR/tracehub"
        return
    fi

    print_error "tracehub executable not found"
    print_info "Expected: $SCRIPT_DIR/tracehub"
    exit 1
}

# ==============================================================================
# Parse Arguments
# ==============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--addr)
            require_option_argument "$1" "${2-}"
            RTT_REGION_ADDR="$2"
            shift 2
            ;;
        -s|--size)
            require_option_argument "$1" "${2-}"
            MEM_SIZE="$2"
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
check_root "${ORIGINAL_ARGS[@]}"

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
    "--device" "/dev/shared_mem0"
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
