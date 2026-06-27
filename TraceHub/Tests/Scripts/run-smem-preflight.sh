#!/usr/bin/env bash

set -eEuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

BUILD_DIR="$ROOT_DIR/build/HOST_Debug"
RTT_REGION_ADDR="0x10040000"
MEM_SIZE="0x20000"
DEVICE_PATH="/dev/shared_mem0"
KO_PATH=""
FORCE_RELOAD=0
USE_LOADED_MODULE=0
KEEP_MODULE=0
MODULE_LOADED_BY_SCRIPT=0
RTT_TIMEOUT_MS=5000
STARTUP_GRACE_SEC=8
DURATION_SEC=0
LINUX_CHANNEL=0
RTOS_CHANNEL=1
SYSVIEW_CHANNEL=2
WORK_DIR=""
USER_LOG_DIR=""
TRACEHUB_PID=""
REQUIRE_DATA=1
LINUX_MARKER=""
RTOS_MARKER=""
RUN_MARKER=""

show_usage() {
    cat << EOF
Usage: bash Tests/Scripts/run-smem-preflight.sh [OPTIONS]

Validate the Linux SMEM hardware path with a real SharedMem device and target
RTT layout. The script starts tracehub in swimlane + SystemView mode and
passes only if tracehub survives startup validation, the configured runtime
window, and the default data artifact assertions.

Options:
  --build-dir DIR          Build directory containing tracehub (default: $BUILD_DIR)
  -a, --addr ADDR          RTT region backend address (hex, default: $RTT_REGION_ADDR)
  -s, --size SIZE          RTT region size (hex, default: $MEM_SIZE)
  -d, --device PATH        SharedMem device path (default: $DEVICE_PATH)
  -k, --ko PATH            Path to SharedMem.ko (default: /lib/modules/\$(uname -r)/SharedMem.ko)
  --force-reload           Unload an existing SharedMem module before loading
  --use-loaded-module      Use an already loaded SharedMem module without replacing it
  --keep-module            Keep the module loaded when this script loaded it
  --rtt-timeout-ms MS      RTTCB discovery timeout in milliseconds (default: $RTT_TIMEOUT_MS)
  --startup-grace-sec SEC  Seconds to wait for tracehub startup validation (default: $STARTUP_GRACE_SEC)
  --duration-sec SEC       Additional seconds to keep tracehub running after startup validation (default: $DURATION_SEC)
  --linux-channel CH       Linux RTT up-buffer channel (default: $LINUX_CHANNEL)
  --rtos-channel CH        RTOS RTT up-buffer channel (default: $RTOS_CHANNEL)
  --systemview-channel CH  SystemView RTT up-buffer channel (default: $SYSVIEW_CHANNEL)
  --log-dir DIR            Directory for generated logs (default: temporary directory)
  --startup-only           Validate startup and main log creation only
  --require-data           Require linux_*.log, rtos_*.log, and sysview_*.SVDat to be non-empty (default)
  --linux-marker REGEX     Require a Linux log line matching REGEX
  --rtos-marker REGEX      Require an RTOS log line matching REGEX
  -h, --help               Show this help message

Examples:
  sudo bash Tests/Scripts/run-smem-preflight.sh --build-dir build/HOST_Debug -k /path/to/SharedMem.ko
  sudo bash Tests/Scripts/run-smem-preflight.sh --force-reload --rtt-timeout-ms 10000
  bash Tests/Scripts/run-smem-preflight.sh --use-loaded-module --device /dev/shared_mem0
  bash Tests/Scripts/run-smem-preflight.sh --use-loaded-module --startup-only
EOF
}

fail() {
    printf "SMEM preflight failed: %s\n" "$1" >&2
    exit 1
}

require_option_argument() {
    local option="$1"
    local value="${2-}"

    if [ -z "$value" ] || [[ "$value" == -* ]]; then
        fail "$option requires an argument"
    fi
}

check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        fail "SMEM preflight requires Linux"
    fi
}

check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        fail "loading or replacing SharedMem requires root privileges"
    fi
}

check_positive_decimal() {
    local name="$1"
    local value="$2"

    if ! [[ "$value" =~ ^[0-9]+$ ]] || [ "$value" -eq 0 ]; then
        fail "$name must be a positive decimal value"
    fi
}

check_nonnegative_decimal() {
    local name="$1"
    local value="$2"

    if ! [[ "$value" =~ ^[0-9]+$ ]]; then
        fail "$name must be a non-negative decimal value"
    fi
}

check_integer_literal() {
    local name="$1"
    local value="$2"

    if ! [[ "$value" =~ ^0[xX][0-9a-fA-F]+$|^[0-9]+$ ]]; then
        fail "$name must be a decimal or hexadecimal integer"
    fi
}

integer_to_decimal() {
    local value="$1"

    if [[ "$value" =~ ^0[xX] ]]; then
        printf "%u\n" "$((16#${value:2}))"
    else
        printf "%u\n" "$((10#$value))"
    fi
}

resolve_path() {
    local path="$1"

    if [[ "$path" == /* ]]; then
        printf "%s\n" "$path"
    else
        printf "%s/%s\n" "$ROOT_DIR" "$path"
    fi
}

resolve_tracehub() {
    local candidate="$BUILD_DIR/tracehub"

    if [ -x "$candidate" ]; then
        printf "%s\n" "$candidate"
        return 0
    fi

    printf "Missing executable: tracehub\n" >&2
    printf "Expected executable: %s\n" "$candidate" >&2
    return 1
}

dump_artifacts() {
    local file

    for file in "$WORK_DIR"/tracehub.out "$WORK_DIR"/tracehub.err "$WORK_DIR"/main_*.log; do
        if [ -f "$file" ]; then
            printf "\n===== %s =====\n" "$(basename "$file")" >&2
            tail -n 80 "$file" >&2
        fi
    done
}

require_nonempty_artifact() {
    local pattern="$1"
    local label="$2"
    local file

    file="$(find "$WORK_DIR" -maxdepth 1 -type f -name "$pattern" -newer "$RUN_MARKER" -size +0c -print -quit)"
    if [ -z "$file" ]; then
        printf "%s data was not created in %s\n" "$label" "$WORK_DIR" >&2
        return 1
    fi
    return 0
}

require_log_marker() {
    local pattern="$1"
    local label="$2"
    local marker="$3"
    local file

    while IFS= read -r file; do
        if grep -E -q -- "$marker" "$file"; then
            return 0
        fi
    done < <(find "$WORK_DIR" -maxdepth 1 -type f -name "$pattern" -newer "$RUN_MARKER" -size +0c -print)

    printf "%s marker was not found in %s: %s\n" "$label" "$WORK_DIR" "$marker" >&2
    return 1
}

stop_tracehub() {
    if [ -z "$TRACEHUB_PID" ]; then
        return 0
    fi
    if kill -0 "$TRACEHUB_PID" 2>/dev/null; then
        kill -INT "$TRACEHUB_PID" 2>/dev/null || true
        sleep 1
    fi
    if kill -0 "$TRACEHUB_PID" 2>/dev/null; then
        kill -TERM "$TRACEHUB_PID" 2>/dev/null || true
    fi
    wait "$TRACEHUB_PID" 2>/dev/null || true
    TRACEHUB_PID=""
}

cleanup() {
    local status=$?

    trap - EXIT INT TERM

    stop_tracehub

    if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ] && [ "$KEEP_MODULE" -eq 0 ]; then
        rmmod SharedMem 2>/dev/null || true
        MODULE_LOADED_BY_SCRIPT=0
    fi

    if [ -n "$RUN_MARKER" ]; then
        rm -f "$RUN_MARKER"
    fi

    if [ "$status" -ne 0 ] && [ -n "$WORK_DIR" ]; then
        printf "SMEM preflight artifacts retained in: %s\n" "$WORK_DIR" >&2
        dump_artifacts
    elif [ "$status" -eq 0 ] && [ -n "$WORK_DIR" ] && [ -z "$USER_LOG_DIR" ]; then
        rm -rf "$WORK_DIR"
    fi

    exit "$status"
}

load_kernel_module() {
    local ko_path="${KO_PATH:-/lib/modules/$(uname -r)/SharedMem.ko}"
    local module_rtt_region_addr_dec
    local mem_size_dec

    if lsmod | grep -q "^SharedMem"; then
        if [ "$FORCE_RELOAD" -eq 1 ]; then
            check_root
            rmmod SharedMem || fail "failed to unload existing SharedMem module"
            sleep 1
        elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
            [ -e "$DEVICE_PATH" ] || fail "SharedMem is loaded but $DEVICE_PATH does not exist"
            [ -r "$DEVICE_PATH" ] && [ -w "$DEVICE_PATH" ] || fail "$DEVICE_PATH is not readable and writable"
            return 0
        else
            fail "SharedMem is already loaded; use --use-loaded-module or --force-reload"
        fi
    elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
        fail "--use-loaded-module requires an already loaded SharedMem module"
    fi

    check_root

    [ -f "$ko_path" ] || fail "kernel module not found: $ko_path"

    module_rtt_region_addr_dec="$(integer_to_decimal "$RTT_REGION_ADDR")"
    mem_size_dec="$(integer_to_decimal "$MEM_SIZE")"
    if [ "$mem_size_dec" -eq 0 ]; then
        fail "--size must be non-zero when loading SharedMem"
    fi

    insmod "$ko_path" phys_addrs="$module_rtt_region_addr_dec" mem_sizes="$mem_size_dec" ||
        fail "failed to load SharedMem module"
    MODULE_LOADED_BY_SCRIPT=1

    sleep 1
    [ -e "$DEVICE_PATH" ] || fail "device node was not created: $DEVICE_PATH"
    [ -r "$DEVICE_PATH" ] && [ -w "$DEVICE_PATH" ] || fail "$DEVICE_PATH is not readable and writable"
}

validate_runtime_options() {
    check_integer_literal "--addr" "$RTT_REGION_ADDR"
    check_integer_literal "--size" "$MEM_SIZE"
    check_positive_decimal "--rtt-timeout-ms" "$RTT_TIMEOUT_MS"
    check_positive_decimal "--startup-grace-sec" "$STARTUP_GRACE_SEC"
    check_nonnegative_decimal "--duration-sec" "$DURATION_SEC"
    check_nonnegative_decimal "--linux-channel" "$LINUX_CHANNEL"
    check_nonnegative_decimal "--rtos-channel" "$RTOS_CHANNEL"
    check_nonnegative_decimal "--systemview-channel" "$SYSVIEW_CHANNEL"

    if [ $((STARTUP_GRACE_SEC * 1000)) -le "$RTT_TIMEOUT_MS" ]; then
        fail "--startup-grace-sec must be greater than --rtt-timeout-ms"
    fi

    if [ "$FORCE_RELOAD" -eq 1 ] && [ "$USE_LOADED_MODULE" -eq 1 ]; then
        fail "--force-reload and --use-loaded-module are mutually exclusive"
    fi

    if [ -n "$USER_LOG_DIR" ]; then
        [ -d "$USER_LOG_DIR" ] || fail "--log-dir does not exist or is not a directory: $USER_LOG_DIR"
    fi
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --build-dir)
                require_option_argument "$1" "${2-}"
                BUILD_DIR="$(resolve_path "$2")"
                shift 2
                ;;
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
            -d|--device)
                require_option_argument "$1" "${2-}"
                DEVICE_PATH="$2"
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
            --keep-module)
                KEEP_MODULE=1
                shift
                ;;
            --rtt-timeout-ms)
                require_option_argument "$1" "${2-}"
                RTT_TIMEOUT_MS="$2"
                shift 2
                ;;
            --startup-grace-sec)
                require_option_argument "$1" "${2-}"
                STARTUP_GRACE_SEC="$2"
                shift 2
                ;;
            --duration-sec)
                require_option_argument "$1" "${2-}"
                DURATION_SEC="$2"
                shift 2
                ;;
            --linux-channel)
                require_option_argument "$1" "${2-}"
                LINUX_CHANNEL="$2"
                shift 2
                ;;
            --rtos-channel)
                require_option_argument "$1" "${2-}"
                RTOS_CHANNEL="$2"
                shift 2
                ;;
            --systemview-channel)
                require_option_argument "$1" "${2-}"
                SYSVIEW_CHANNEL="$2"
                shift 2
                ;;
            --log-dir)
                require_option_argument "$1" "${2-}"
                USER_LOG_DIR="$(resolve_path "$2")"
                shift 2
                ;;
            --startup-only)
                REQUIRE_DATA=0
                shift
                ;;
            --require-data)
                REQUIRE_DATA=1
                shift
                ;;
            --linux-marker)
                require_option_argument "$1" "${2-}"
                LINUX_MARKER="$2"
                shift 2
                ;;
            --rtos-marker)
                require_option_argument "$1" "${2-}"
                RTOS_MARKER="$2"
                shift 2
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                fail "unknown option: $1"
                ;;
        esac
    done
}

run_preflight() {
    local tracehub_bin
    local status
    local deadline

    tracehub_bin="$(resolve_tracehub)"

    if [ -n "$USER_LOG_DIR" ]; then
        WORK_DIR="$USER_LOG_DIR"
    else
        WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/tracehub-smem-preflight.XXXXXX")"
    fi
    RUN_MARKER="$WORK_DIR/.smem_preflight_start"
    : > "$RUN_MARKER" || fail "failed to create run marker in $WORK_DIR"

    "$tracehub_bin" \
        --addr "$RTT_REGION_ADDR" \
        --size "$MEM_SIZE" \
        --device "$DEVICE_PATH" \
        --rtt-timeout-ms "$RTT_TIMEOUT_MS" \
        --swimlane \
        --linux-channel "$LINUX_CHANNEL" \
        --rtos-channel "$RTOS_CHANNEL" \
        --systemview-channel "$SYSVIEW_CHANNEL" \
        --log-dir "$WORK_DIR" \
        > "$WORK_DIR/tracehub.out" 2> "$WORK_DIR/tracehub.err" &
    TRACEHUB_PID="$!"

    sleep "$STARTUP_GRACE_SEC"

    if ! kill -0 "$TRACEHUB_PID" 2>/dev/null; then
        if wait "$TRACEHUB_PID"; then
            status=0
        else
            status=$?
        fi
        TRACEHUB_PID=""
        printf "tracehub exited before SMEM startup validation completed, status=%s\n" "$status" >&2
        return 1
    fi

    if [ "$DURATION_SEC" -gt 0 ]; then
        deadline=$((SECONDS + DURATION_SEC))
        while [ "$SECONDS" -lt "$deadline" ]; do
            if ! kill -0 "$TRACEHUB_PID" 2>/dev/null; then
                if wait "$TRACEHUB_PID"; then
                    status=0
                else
                    status=$?
                fi
                TRACEHUB_PID=""
                printf "tracehub exited during sustained SMEM validation, status=%s\n" "$status" >&2
                return 1
            fi
            sleep 1
        done
    fi

    stop_tracehub

    if ! find "$WORK_DIR" -maxdepth 1 -type f -name 'main_*.log' -newer "$RUN_MARKER" -size +0c | grep -q .; then
        printf "main log was not created in %s\n" "$WORK_DIR" >&2
        return 1
    fi

    if [ "$REQUIRE_DATA" -eq 1 ]; then
        require_nonempty_artifact 'linux_*.log' "Linux log" || return 1
        require_nonempty_artifact 'rtos_*.log' "RTOS log" || return 1
        require_nonempty_artifact 'sysview_*.SVDat' "SystemView SVDat" || return 1
    fi
    if [ -n "$LINUX_MARKER" ]; then
        require_log_marker 'linux_*.log' "Linux log" "$LINUX_MARKER" || return 1
    fi
    if [ -n "$RTOS_MARKER" ]; then
        require_log_marker 'rtos_*.log' "RTOS log" "$RTOS_MARKER" || return 1
    fi

    printf "SMEM preflight validation passed\n"
    printf "Validated SharedMem access, RTTCB discovery, channel layout, configured runtime, and default or requested artifact assertions\n"
}

main() {
    parse_arguments "$@"
    check_linux
    validate_runtime_options

    trap cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM

    load_kernel_module
    run_preflight
}

main "$@"
