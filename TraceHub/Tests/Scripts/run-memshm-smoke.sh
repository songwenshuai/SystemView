#!/usr/bin/env bash

set -eEuo pipefail

# ==============================================================================
# TraceHub MEMSHM Smoke Runner
# ==============================================================================
#
# Project decision:
#   This script must remain self-contained for independent smoke validation.
#   Do not extract shared helpers into sourced common scripts.
#
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
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

contains_c1_control_bytes() {
    local log_file="$1"

    od -An -tx1 -v "$log_file" | awk '
        function hex_value(hex,    i, c, n, p) {
            n = 0
            for (i = 1; i <= length(hex); i++) {
                c = tolower(substr(hex, i, 1))
                p = index("0123456789abcdef", c)
                if (p == 0) {
                    return -1
                }
                n = n * 16 + p - 1
            }
            return n
        }
        function clear_utf8() {
            utf8_need = 0
            utf8_len = 0
            utf8_code = 0
            utf8_min = 0
        }
        function start_utf8(byte) {
            if (byte >= 194 && byte <= 223) {
                utf8_need = 1
                utf8_len = 2
                utf8_code = byte % 32
                utf8_min = 128
                return 1
            }
            if (byte >= 224 && byte <= 239) {
                utf8_need = 2
                utf8_len = 3
                utf8_code = byte % 16
                utf8_min = 2048
                return 1
            }
            if (byte >= 240 && byte <= 244) {
                utf8_need = 3
                utf8_len = 4
                utf8_code = byte % 8
                utf8_min = 65536
                return 1
            }
            return 0
        }
        function finish_utf8() {
            if (utf8_code < utf8_min ||
                (utf8_code >= 55296 && utf8_code <= 57343) ||
                utf8_code > 1114111 ||
                (utf8_code >= 128 && utf8_code <= 159)) {
                found = 1
                clear_utf8()
                return 0
            }
            clear_utf8()
            return 1
        }
        {
            for (i = 1; i <= NF; i++) {
                byte = hex_value($i)
                reprocess = 1
                while (reprocess) {
                    reprocess = 0
                    if (utf8_need > 0) {
                        if (byte >= 128 && byte <= 191) {
                            utf8_code = utf8_code * 64 + (byte % 64)
                            utf8_need--
                            if (utf8_need == 0) {
                                finish_utf8()
                            }
                            continue
                        }
                        clear_utf8()
                        reprocess = 1
                        continue
                    }
                    if (byte >= 128 && byte <= 159) {
                        found = 1
                        continue
                    }
                    if (byte >= 160 && byte <= 191) {
                        continue
                    }
                    start_utf8(byte)
                }
            }
        }
        END {
            if (found) {
                exit 0
            }
            exit 1
        }
    '
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

clean_text_log_valid() {
    local log_file="$1"

    if LC_ALL=C tr -d '\011\012' < "$log_file" | LC_ALL=C grep -q '[[:cntrl:]]'; then
        printf "Text log contains terminal/control bytes: %s\n" "$log_file" >&2
        return 1
    fi
    if contains_c1_control_bytes "$log_file"; then
        printf "Text log contains C1 control bytes: %s\n" "$log_file" >&2
        return 1
    fi
    return 0
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

    grep -q "Linux: Application event" "$linux_log" || return 1
    grep -q "RTOS: Task scheduler tick" "$rtos_log" || return 1
    grep -q "Linux boot banner without timestamp" "$linux_log" || return 1
    grep -q "RTOS boot banner without timestamp" "$rtos_log" || return 1
    grep -q "\[LINUX\].*Linux: Application event" "$swimlane_log" || return 1
    grep -q "\[RTOS\].*RTOS: Task scheduler tick" "$swimlane_log" || return 1
    grep -q "\[LINUX\].*Linux boot banner without timestamp" "$swimlane_log" || return 1
    grep -q "\[RTOS\].*RTOS boot banner without timestamp" "$swimlane_log" || return 1
    clean_text_log_valid "$linux_log" || return 1
    clean_text_log_valid "$rtos_log" || return 1
    clean_text_log_valid "$swimlane_log" || return 1
    swimlane_order_valid "$swimlane_log" || return 1
}

main() {
    local tracehub_bin
    local linux_sim_bin
    local rtos_sim_bin
    local sysview_tcp_client_bin

    tracehub_bin="$(resolve_executable "tracehub" "tracehub")"
    linux_sim_bin="$(resolve_executable "rtt_sim_linux" "Tests/rtt_sim_linux")"
    rtos_sim_bin="$(resolve_executable "rtt_sim_rtos" "Tests/rtt_sim_rtos")"
    sysview_tcp_client_bin="$(resolve_executable "rtt_sim_sysview_tcp_client" "Tests/rtt_sim_sysview_tcp_client")"

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
    printf "Validated clean source text logs, clean swimlane ordering, SystemView SVDat output, and SystemView TCP delivery\n"
}

main "$@"
