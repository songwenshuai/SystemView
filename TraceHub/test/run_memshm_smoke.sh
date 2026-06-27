#!/usr/bin/env bash

set -eEuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build/HOST_Debug}"

if [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$ROOT_DIR/$BUILD_DIR"
fi

SHM_NAME="/rtt_sim"
BASE_ADDRESS="0x10000000"
SYSVIEW_PORT="19112"
POLL_TIMEOUT_SEC=20

TRACEHUB_PID=""
LINUX_PID=""
RTOS_PID=""
WORK_DIR=""

resolve_executable() {
    local binary_name="$1"
    local build_relative_path="$2"
    local candidate="$BUILD_DIR/$build_relative_path"

    if [ -x "$candidate" ]; then
        printf "%s\n" "$candidate"
        return 0
    fi

    printf "Missing executable: %s\n" "$binary_name" >&2
    printf "Expected executable: %s\n" "$candidate" >&2
    return 1
}

find_first_file() {
    local pattern="$1"

    find "$WORK_DIR" -maxdepth 1 -type f -name "$pattern" -size +0c -print | sort | head -n 1
}

dump_log_tail() {
    local file
    local files=(
        "tracehub.out"
        "tracehub.err"
        "linux.out"
        "linux.err"
        "rtos.out"
        "rtos.err"
        "sysview_client.out"
        "sysview_client.err"
    )

    for file in "${files[@]}"; do
        if [ -f "$WORK_DIR/$file" ]; then
            printf "\n===== %s =====\n" "$file" >&2
            tail -n 40 "$WORK_DIR/$file" >&2
        fi
    done
}

stop_process() {
    local pid="$1"

    if [ -z "$pid" ]; then
        return 0
    fi
    if kill -0 "$pid" 2>/dev/null; then
        kill -INT "$pid" 2>/dev/null || true
    fi
}

cleanup() {
    local exit_status=$?
    local pid

    trap - EXIT INT TERM

    stop_process "$RTOS_PID"
    stop_process "$LINUX_PID"
    stop_process "$TRACEHUB_PID"
    sleep 1

    for pid in "$RTOS_PID" "$LINUX_PID" "$TRACEHUB_PID"; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done

    for pid in "$RTOS_PID" "$LINUX_PID" "$TRACEHUB_PID"; do
        if [ -n "$pid" ]; then
            wait "$pid" 2>/dev/null || true
        fi
    done

    if [ "$exit_status" -eq 0 ]; then
        rm -rf "$WORK_DIR"
    else
        printf "Smoke artifacts retained in: %s\n" "$WORK_DIR" >&2
        dump_log_tail
    fi

    return "$exit_status"
}

wait_until() {
    local timeout_sec="$1"
    local description="$2"
    local deadline

    shift 2
    deadline=$((SECONDS + timeout_sec))
    while [ "$SECONDS" -lt "$deadline" ]; do
        if "$@"; then
            return 0
        fi
        sleep 1
    done

    printf "Timed out waiting for: %s\n" "$description" >&2
    return 1
}

linux_ready() {
    grep -q "RTT CB ready" "$WORK_DIR/linux.out" 2>/dev/null
}

rtos_ready() {
    grep -q "Found valid RTT CB" "$WORK_DIR/rtos.out" 2>/dev/null
}

swimlane_order_valid() {
    local swimlane_log="$1"

    awk '
        ($2 == "[LINUX]" || $2 == "[RTOS]") && $1 ~ /^\[[0-9][0-9]*\]$/ {
            ts = $1
            gsub(/^\[/, "", ts)
            gsub(/\]$/, "", ts)
            ts += 0
            if (count > 0 && ts < last) {
                exit 1
            }
            last = ts
            count++
        }
        END {
            if (count < 2) {
                exit 1
            }
        }
    ' "$swimlane_log"
}

smoke_artifacts_ready() {
    local linux_log
    local rtos_log
    local swimlane_log
    local sysview_file

    linux_log="$(find_first_file 'linux_*.log')"
    rtos_log="$(find_first_file 'rtos_*.log')"
    swimlane_log="$(find_first_file 'swimlane_*.log')"
    sysview_file="$(find_first_file 'sysview_*.SVDat')"

    [ -n "$linux_log" ] || return 1
    [ -n "$rtos_log" ] || return 1
    [ -n "$swimlane_log" ] || return 1
    [ -n "$sysview_file" ] || return 1

    grep -q "Linux A53" "$linux_log" || return 1
    grep -q "RTOS R5" "$rtos_log" || return 1
    grep -q "Linux A53 boot banner without timestamp" "$linux_log" || return 1
    grep -q "RTOS R5 boot banner without timestamp" "$rtos_log" || return 1
    grep -q "\[LINUX\].*Linux A53" "$swimlane_log" || return 1
    grep -q "\[RTOS\].*RTOS R5" "$swimlane_log" || return 1
    grep -q "\[LINUX\].*Linux A53 boot banner without timestamp" "$swimlane_log" || return 1
    grep -q "\[RTOS\].*RTOS R5 boot banner without timestamp" "$swimlane_log" || return 1
    swimlane_order_valid "$swimlane_log" || return 1
}

main() {
    local tracehub_bin
    local linux_sim_bin
    local rtos_sim_bin
    local sysview_tcp_client_bin

    tracehub_bin="$(resolve_executable "tracehub" "tracehub")"
    linux_sim_bin="$(resolve_executable "rtt_sim_linux" "test/rtt_sim_linux")"
    rtos_sim_bin="$(resolve_executable "rtt_sim_rtos" "test/rtt_sim_rtos")"
    sysview_tcp_client_bin="$(resolve_executable "rtt_sim_sysview_tcp_client" "test/rtt_sim_sysview_tcp_client")"

    WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/tracehub-smoke.XXXXXX")"
    trap cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM

    cd "$WORK_DIR"

    "$tracehub_bin" \
        --addr "$BASE_ADDRESS" \
        --size 0 \
        --shm "$SHM_NAME" \
        --memshm-reset \
        --rtt-timeout-ms 10000 \
        --swimlane \
        --systemview-port "$SYSVIEW_PORT" \
        > tracehub.out 2> tracehub.err &
    TRACEHUB_PID="$!"

    sleep 1

    "$linux_sim_bin" > linux.out 2> linux.err &
    LINUX_PID="$!"
    wait_until 10 "Linux simulator RTTCB initialization" linux_ready

    "$rtos_sim_bin" > rtos.out 2> rtos.err &
    RTOS_PID="$!"
    wait_until 10 "RTOS simulator RTTCB validation" rtos_ready

    wait_until "$POLL_TIMEOUT_SEC" "MEMSHM smoke output artifacts" smoke_artifacts_ready

    "$sysview_tcp_client_bin" 127.0.0.1 "$SYSVIEW_PORT" 1 10000 \
        > sysview_client.out 2> sysview_client.err

    printf "MEMSHM smoke validation passed\n"
    printf "Validated Linux log, RTOS log, swimlane ordering, SystemView SVDat output, and SystemView TCP delivery\n"
}

main "$@"
