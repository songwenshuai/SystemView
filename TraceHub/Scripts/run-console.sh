#!/bin/bash

# ==============================================================================
# TraceHub Console Mode Runner
# ==============================================================================
#
# Linux SharedMem backend runner. This script loads the SharedMem kernel
# module and runs tracehub in console mode, using stdin/stdout as a virtual
# serial terminal.
#
# Usage:
#   sudo Scripts/run-console.sh [options]
#
# Options:
#   --phys-addr ADDR     SharedMem physical address (hex, default: 0x10040000)
#   --mem-size SIZE      SharedMem size (hex, default: 0x20000)
#   --device PATH        SharedMem device path (default: /dev/shared_mem0)
#   -c, --channel CH     Console RTT text channel (default: selected log default)
#   --linux              Display Linux logs (default)
#   --rtos               Display RTOS logs
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
#   sudo Scripts/run-console.sh
#   sudo Scripts/run-console.sh --rtos
#   sudo Scripts/run-console.sh --linux --channel 3
#   sudo Scripts/run-console.sh --phys-addr 0x80000000 --mem-size 0x10000
#
# ==============================================================================

set -euo pipefail

# ==============================================================================
# Script Directory
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

. "$SCRIPT_DIR/tracehub-runner-common.sh"

# ==============================================================================
# Default Configuration
# ==============================================================================

RTT_REGION_ADDR="0x10040000"
MEM_SIZE="0x20000"
DEVICE_PATH="/dev/shared_mem0"
CONSOLE_CHANNEL="0"
CONSOLE_CHANNEL_SPECIFIED=0
CONSOLE_LINUX_SELECTED=0
CONSOLE_RTOS_SELECTED=0
CONSOLE_LOG_KIND="linux"
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

print_info()    { echo -e "${BLUE}[INFO]${NC} $1" >&2; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1" >&2; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1" >&2; }
print_section() { echo -e "${CYAN}=== $1 ===${NC}" >&2; }

# ==============================================================================
# Helper Functions
# ==============================================================================

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Console Mode: Use stdin/stdout as virtual serial terminal.
Press Ctrl+C to exit.

Options:
  --phys-addr ADDR     SharedMem physical address (hex, default: $RTT_REGION_ADDR)
  --mem-size SIZE      SharedMem size (hex, default: $MEM_SIZE)
  --device PATH        SharedMem device path (default: $DEVICE_PATH)
  -c, --channel CH     Console RTT text channel (default: selected log default)
  --linux              Display Linux logs (default)
  --rtos               Display RTOS logs
  --rtt-timeout-ms MS  RTTCB discovery timeout in milliseconds (default: $RTT_TIMEOUT_MS, 0 waits until interrupted)
  --log-dir DIR        Directory for generated log and record files
  -k, --ko PATH        Path to SharedMem.ko (default: auto-detect)
  --force-reload       Unload an existing SharedMem module before loading
  --use-loaded-module  Use an already loaded SharedMem module
  -h, --help           Show this help message

Environment:
  TRACEHUB_BIN         Explicit tracehub executable path. If unset, this script
                       checks $ROOT_DIR/tracehub and
                       $ROOT_DIR/install/usershell/TraceHub/bin/tracehub.

Examples:
  sudo $0
  sudo $0 --rtos
  sudo $0 --linux --channel 3
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

load_kernel_module() {
    if ! tracehub_runner_load_kernel_module; then
        exit 1
    fi
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
    if ! TRACEHUB_BIN="$(tracehub_runner_resolve_executable "$ROOT_DIR")"; then
        exit 1
    fi
}

resolve_console_log() {
    if [ "$CONSOLE_LINUX_SELECTED" -eq 1 ] && [ "$CONSOLE_RTOS_SELECTED" -eq 1 ]; then
        print_error "--linux and --rtos are mutually exclusive"
        exit 1
    fi

    if [ "$CONSOLE_RTOS_SELECTED" -eq 1 ]; then
        CONSOLE_LOG_KIND="rtos"
        if [ "$CONSOLE_CHANNEL_SPECIFIED" -eq 0 ]; then
            CONSOLE_CHANNEL="1"
        fi
    else
        CONSOLE_LOG_KIND="linux"
        if [ "$CONSOLE_CHANNEL_SPECIFIED" -eq 0 ]; then
            CONSOLE_CHANNEL="0"
        fi
    fi
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
        -c|--channel)
            require_option_argument "$1" "${2-}"
            CONSOLE_CHANNEL="$2"
            CONSOLE_CHANNEL_SPECIFIED=1
            shift 2
            ;;
        --linux)
            CONSOLE_LINUX_SELECTED=1
            shift
            ;;
        --rtos)
            CONSOLE_RTOS_SELECTED=1
            shift
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
resolve_console_log

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
    "--console"
)

if [ "$CONSOLE_LOG_KIND" = "linux" ]; then
    TRACEHUB_ARGS+=("--linux" "--linux-channel" "$CONSOLE_CHANNEL")
else
    TRACEHUB_ARGS+=("--rtos" "--rtos-channel" "$CONSOLE_CHANNEL")
fi

if [ -n "$LOG_DIR" ]; then
    TRACEHUB_ARGS+=("--log-dir" "$LOG_DIR")
fi

# Run tracehub
print_section "Console Mode - RTT Virtual Serial Terminal"
print_info "Console service: $CONSOLE_LOG_KIND log, channel $CONSOLE_CHANNEL"
echo "" >&2

trap 'forward_signal INT' INT
trap 'forward_signal TERM' TERM

run_tracehub
