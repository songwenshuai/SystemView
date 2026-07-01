#!/bin/bash

# ==============================================================================
# TraceHub Runner Environment Check
# ==============================================================================
#
# Validate the tracehub executable path and the Linux SharedMem runner
# environment without depending on any other repository script.
#
# Project decision:
#   This script must remain self-contained for independent field use.
#   Do not source it from other scripts or extract it into shared helpers.
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
KO_PATH=""
FORCE_RELOAD=0
USE_LOADED_MODULE=0
KEEP_MODULE=0
LOAD_MODULE=0
CHECK_TRACEHUB=1
MODULE_LOADED_BY_SCRIPT=0
TRACEHUB_BIN="${TRACEHUB_BIN:-}"

# ==============================================================================
# Color Output
# ==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info()    { echo -e "${BLUE}[INFO]${NC} $1" >&2; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1" >&2; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1" >&2; }

# ==============================================================================
# Helper Functions
# ==============================================================================

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Validate the tracehub executable path and the Linux SharedMem runner
environment. By default, this script checks that tracehub is executable and
that an already loaded SharedMem module exposes a readable and writable device.

Options:
  --phys-addr ADDR     SharedMem physical address used with --load-module (default: $RTT_REGION_ADDR)
  --mem-size SIZE      SharedMem size used with --load-module (default: $MEM_SIZE)
  --device PATH        SharedMem device path (default: $DEVICE_PATH)
  -k, --ko PATH        Path to SharedMem.ko (default: /lib/modules/\$(uname -r)/SharedMem.ko)
  --force-reload       Unload an existing SharedMem module before --load-module
  --use-loaded-module  Require and validate an already loaded SharedMem module
  --load-module        Load SharedMem before validating the device
  --keep-module        Keep the module loaded after --load-module
  --skip-tracehub      Do not validate the tracehub executable path
  -h, --help           Show this help message

Environment:
  TRACEHUB_BIN         Explicit tracehub executable path. If unset, this script
                       checks $ROOT_DIR/tracehub and
                       $ROOT_DIR/install/bin/tracehub.
EOF
}

check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        print_error "This script requires the Linux SharedMem backend"
        print_info "Use $ROOT_DIR/build.sh --backend memshm and run tracehub with --shm for host simulation"
        return 1
    fi

    return 0
}

require_option_argument() {
    local option="$1"
    local value="${2-}"

    if [ -z "$value" ] || [[ "$value" == -* ]]; then
        print_error "$option requires an argument"
        return 1
    fi

    return 0
}

validate_module_options() {
    if [ "$FORCE_RELOAD" -eq 1 ] && [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--force-reload and --use-loaded-module are mutually exclusive"
        return 1
    fi

    if [ "$KEEP_MODULE" -eq 1 ] && [ "$LOAD_MODULE" -eq 0 ]; then
        print_error "--keep-module requires --load-module"
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

resolve_tracehub() {
    local candidate

    if [ -n "$TRACEHUB_BIN" ]; then
        if [ -f "$TRACEHUB_BIN" ] && [ -x "$TRACEHUB_BIN" ]; then
            print_success "tracehub executable: $TRACEHUB_BIN"
            return 0
        fi

        print_error "tracehub executable configured by TRACEHUB_BIN is not executable: $TRACEHUB_BIN"
        return 1
    fi

    for candidate in \
        "$ROOT_DIR/tracehub" \
        "$ROOT_DIR/install/bin/tracehub"
    do
        if [ -f "$candidate" ] && [ -x "$candidate" ]; then
            TRACEHUB_BIN="$candidate"
            print_success "tracehub executable: $TRACEHUB_BIN"
            return 0
        fi
    done

    print_error "tracehub executable not found"
    print_info "Set TRACEHUB_BIN or provide one of:"
    print_info "  $ROOT_DIR/tracehub"
    print_info "  $ROOT_DIR/install/bin/tracehub"
    return 1
}

check_loaded_module() {
    if ! lsmod | grep -q "^SharedMem"; then
        print_error "SharedMem module is not loaded"
        print_info "Use --load-module to load it or load it externally before running this check"
        return 1
    fi

    require_device_access "$DEVICE_PATH" || return 1
    print_success "SharedMem module and device are usable"
    return 0
}

load_kernel_module() {
    local ko_path="${KO_PATH:-/lib/modules/$(uname -r)/SharedMem.ko}"
    local module_rtt_region_addr_dec
    local mem_size_dec

    validate_module_options || return 1

    if lsmod | grep -q "^SharedMem"; then
        if [ "$FORCE_RELOAD" -eq 1 ]; then
            require_module_update_privilege || return 1

            print_info "Unloading existing SharedMem module..."
            if ! rmmod SharedMem; then
                print_error "Failed to unload existing SharedMem module"
                return 1
            fi
            sleep 1
        elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
            require_device_access "$DEVICE_PATH" || return 1
            print_success "Using already loaded SharedMem module"
            return 0
        else
            print_error "SharedMem module is already loaded"
            print_info "Use --use-loaded-module to reuse it or --force-reload to replace it"
            return 1
        fi
    elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--use-loaded-module requires an already loaded SharedMem module"
        return 1
    fi

    require_module_update_privilege || return 1

    if [ ! -f "$ko_path" ]; then
        print_error "Kernel module not found: $ko_path"
        print_info "Please install the kernel module or specify the path with -k/--ko option"
        return 1
    fi

    module_rtt_region_addr_dec=$((RTT_REGION_ADDR))
    mem_size_dec=$((MEM_SIZE))

    print_info "Loading SharedMem module (rtt_region_addr=$RTT_REGION_ADDR, rtt_region_size=$MEM_SIZE)..."
    print_info "Kernel module: $ko_path"
    if ! insmod "$ko_path" phys_addrs="$module_rtt_region_addr_dec" mem_sizes="$mem_size_dec"; then
        print_error "Failed to load kernel module"
        return 1
    fi
    MODULE_LOADED_BY_SCRIPT=1

    sleep 1
    if ! require_device_access "$DEVICE_PATH"; then
        if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ]; then
            rmmod SharedMem 2>/dev/null || true
            MODULE_LOADED_BY_SCRIPT=0
        fi
        return 1
    fi

    print_success "Kernel module loaded successfully"
    return 0
}

cleanup() {
    local status=$?

    trap - EXIT INT TERM

    if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ] && [ "$KEEP_MODULE" -eq 0 ]; then
        rmmod SharedMem 2>/dev/null || true
        MODULE_LOADED_BY_SCRIPT=0
        print_info "Kernel module unloaded"
    fi

    exit "$status"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --phys-addr)
                require_option_argument "$1" "${2-}" || return 1
                RTT_REGION_ADDR="$2"
                shift 2
                ;;
            --mem-size)
                require_option_argument "$1" "${2-}" || return 1
                MEM_SIZE="$2"
                shift 2
                ;;
            --device)
                require_option_argument "$1" "${2-}" || return 1
                DEVICE_PATH="$2"
                shift 2
                ;;
            -k|--ko)
                require_option_argument "$1" "${2-}" || return 1
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
            --load-module)
                LOAD_MODULE=1
                shift
                ;;
            --keep-module)
                KEEP_MODULE=1
                shift
                ;;
            --skip-tracehub)
                CHECK_TRACEHUB=0
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage >&2
                return 1
                ;;
        esac
    done

    return 0
}

main() {
    parse_arguments "$@" || exit 1
    check_linux || exit 1
    validate_module_options || exit 1

    if [ "$CHECK_TRACEHUB" -eq 1 ]; then
        resolve_tracehub || exit 1
    fi

    if [ "$LOAD_MODULE" -eq 1 ]; then
        trap cleanup EXIT
        trap 'exit 130' INT
        trap 'exit 143' TERM
        load_kernel_module || exit 1
    else
        check_loaded_module || exit 1
    fi

    print_success "TraceHub runner environment check completed"
}

main "$@"
