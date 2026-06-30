#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

TRICE_DIR="${TRICE_DIR:-$REPO_ROOT/Trice}"
GOLDEN_DIR="${GOLDEN_DIR:-$SCRIPT_DIR}"
OFFICIAL_DIR="${OFFICIAL_DIR:-$GOLDEN_DIR/official}"
WORK_ROOT="${WORK_ROOT:-$GOLDEN_DIR/.work/go-golden-report}"
WORK_DIR="$WORK_ROOT/src"
REPORT_ROOT="${REPORT_ROOT:-$GOLDEN_DIR/reports}"
TIMESTAMP="$(date -u +%Y%m%dT%H%M%SZ)"
REPORT_DIR="$REPORT_ROOT/go-golden-report-$TIMESTAMP"

export CGO_ENABLED="${CGO_ENABLED:-1}"

handle_signal() {
	local status="$1"

	printf '\nInterrupted, stopping Go golden report.\n' >&2
	exit "$status"
}

trap 'handle_signal 130' INT
trap 'handle_signal 143' TERM

fail() {
	printf 'error: %s\n' "$1" >&2
	exit 1
}

require_file() {
	local path="$1"

	[[ -f "$path" ]] || fail "required file is missing: $path"
}

require_dir() {
	local path="$1"

	[[ -d "$path" ]] || fail "required directory is missing: $path"
}

copy_tree() {
	local src="$1"
	local dst="$2"

	mkdir -p "$dst"
	if command -v rsync >/dev/null 2>&1; then
		rsync -a --exclude '.git' "$src"/ "$dst"/
	else
		( cd "$src" && tar cf - --exclude '.git' . ) | ( cd "$dst" && tar xf - )
	fi
}

reset_work_dir() {
	case "$WORK_DIR" in
		"$GOLDEN_DIR"/.work/*)
			rm -rf "$WORK_DIR"
			;;
		*)
			fail "refusing to delete work directory outside $GOLDEN_DIR/.work: $WORK_DIR"
			;;
	esac
	mkdir -p "$WORK_DIR"
}

write_hashes() {
	local out="$REPORT_DIR/reference-hashes.sha256"

	if command -v shasum >/dev/null 2>&1; then
		shasum -a 256 \
			"$OFFICIAL_DIR/demoTIL.json" \
			"$OFFICIAL_DIR/demoLI.json" \
			"$OFFICIAL_DIR/til.c" \
			"$TRICE_DIR/src/trice.h" \
			"$TRICE_DIR/src/triceOn.h" \
			"$OFFICIAL_DIR/_test/testdata/triceCheck.c" \
			> "$out"
	elif command -v sha256sum >/dev/null 2>&1; then
		sha256sum \
			"$OFFICIAL_DIR/demoTIL.json" \
			"$OFFICIAL_DIR/demoLI.json" \
			"$OFFICIAL_DIR/til.c" \
			"$TRICE_DIR/src/trice.h" \
			"$TRICE_DIR/src/triceOn.h" \
			"$OFFICIAL_DIR/_test/testdata/triceCheck.c" \
			> "$out"
	else
		printf 'sha256 tool not found\n' > "$out"
	fi
}

write_workspace_provenance() {
	local out="$REPORT_DIR/workspace-provenance.txt"

	{
		printf 'Workspace: %s\n' "$WORK_DIR"
		printf 'Trice source: %s\n' "$TRICE_DIR"
		printf 'Official overlay source: %s\n' "$OFFICIAL_DIR"
		printf '\n'
		printf 'Build sequence:\n'
		printf '1. Delete and recreate the workspace directory.\n'
		printf '2. Copy the local Trice directory into the workspace.\n'
		printf '3. Overlay TriceOfficialGolden/official into the same workspace.\n'
		printf '\n'
		printf 'The script does not read trice-1.2.5.tar.gz at runtime.\n'
		printf 'The tar command in copy_tree is only a directory-to-directory copy fallback when rsync is not available.\n'
		printf '\n'
		printf 'Origin rules after overlay:\n'
		printf '%s\n' '- cmd, pkg, scripts, go.mod, and go.sum come from the local Trice directory unless an official overlay file has the same relative path.'
		printf '%s\n' '- _test, demoTIL.json, demoLI.json, til.c, and official test files come from TriceOfficialGolden/official.'
		printf '%s\n' '- src comes from the local Trice directory. Keep Trice/src unchanged when producing a stable golden report.'
		printf '%s\n' '- internal contains the local Trice implementation plus official test files from TriceOfficialGolden/official/internal.'
	} > "$out"
}

prepare_official_cgo_sources() {
	local log_file="$REPORT_DIR/prepare-triceCheck.log"

	printf 'Preparing official CGO triceCheck.c IDs\n'
	(
		cd "$WORK_DIR"
		go run ./cmd/trice insert \
			-src _test/testdata/triceCheck.c \
			-til demoTIL.json \
			-li demoLI.json \
			-liPath=relative \
			-IDMethod upward
	) > "$log_file" 2>&1 || {
		printf 'error: official CGO source preparation failed, see %s\n' "$log_file" >&2
		exit 1
	}
}

write_git_state() {
	if git -C "$TRICE_DIR" rev-parse --show-toplevel >/dev/null 2>&1; then
		git -C "$TRICE_DIR" rev-parse HEAD > "$REPORT_DIR/trice-git-head.txt" 2>&1 || true
		git -C "$TRICE_DIR" status --short > "$REPORT_DIR/trice-git-status.txt" 2>&1 || true
	else
		printf 'not a git work tree\n' > "$REPORT_DIR/trice-git-head.txt"
		printf 'not a git work tree\n' > "$REPORT_DIR/trice-git-status.txt"
	fi
}

write_report_header() {
	{
		printf '# Go Golden Report\n\n'
		printf 'Generated UTC: %s\n\n' "$TIMESTAMP"
		printf 'Trice source: `%s`\n\n' "$TRICE_DIR"
		printf 'Official golden source: `%s`\n\n' "$OFFICIAL_DIR"
		printf 'Workspace: `%s`\n\n' "$WORK_DIR"
		printf 'CGO_ENABLED: `%s`\n\n' "$CGO_ENABLED"
		printf 'The workspace is built from the local `Trice` directory plus the files in `TriceOfficialGolden/official`.\n\n'
		printf '| Case | Package | Result | Elapsed | JSONL | stderr |\n'
		printf '| --- | --- | --- | --- | --- | --- |\n'
	} > "$REPORT_DIR/report.md"

	printf 'case\tpackage\tresult\texit_code\telapsed_seconds\tjsonl\tstderr\tdescription\n' > "$REPORT_DIR/summary.tsv"
}

run_case() {
	local name="$1"
	local package_path="$2"
	local description="$3"
	local json_file="$REPORT_DIR/$name.jsonl"
	local stderr_file="$REPORT_DIR/$name.stderr"
	local result="PASS"
	local status=0
	local start_epoch=0
	local end_epoch=0
	local elapsed=0

	printf 'Running %s (%s)\n' "$name" "$package_path"

	start_epoch="$(date +%s)"
	set +e
	( cd "$WORK_DIR" && go test -count=1 -json "$package_path" ) > "$json_file" 2> "$stderr_file"
	status=$?
	set -e
	end_epoch="$(date +%s)"
	elapsed=$((end_epoch - start_epoch))

	if [[ "$status" -eq 130 || "$status" -eq 143 ]]; then
		result="INTERRUPTED"
		printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
			"$name" "$package_path" "$result" "$status" "$elapsed" \
			"$(basename "$json_file")" "$(basename "$stderr_file")" "$description" \
			>> "$REPORT_DIR/summary.tsv"
		printf '| `%s` | `%s` | %s | `%ss` | `%s` | `%s` |\n' \
			"$name" "$package_path" "$result" "$elapsed" \
			"$(basename "$json_file")" "$(basename "$stderr_file")" \
			>> "$REPORT_DIR/report.md"
		printf 'Interrupted during %s after %ss\n' "$name" "$elapsed" >&2
		exit "$status"
	fi

	if [[ "$status" -ne 0 ]]; then
		result="FAIL"
		OVERALL_STATUS=1
	fi

	printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
		"$name" "$package_path" "$result" "$status" "$elapsed" \
		"$(basename "$json_file")" "$(basename "$stderr_file")" "$description" \
		>> "$REPORT_DIR/summary.tsv"

	printf '| `%s` | `%s` | %s | `%ss` | `%s` | `%s` |\n' \
		"$name" "$package_path" "$result" "$elapsed" \
		"$(basename "$json_file")" "$(basename "$stderr_file")" \
		>> "$REPORT_DIR/report.md"

	printf 'Finished %s: %s in %ss\n' "$name" "$result" "$elapsed"
}

require_dir "$TRICE_DIR"
require_file "$TRICE_DIR/go.mod"
require_dir "$OFFICIAL_DIR"
require_file "$OFFICIAL_DIR/demoTIL.json"
require_file "$OFFICIAL_DIR/demoLI.json"
require_file "$OFFICIAL_DIR/til.c"
require_file "$OFFICIAL_DIR/_test/testdata/triceCheck.c"
require_file "$TRICE_DIR/src/trice.h"
require_file "$TRICE_DIR/src/triceOn.h"

command -v go >/dev/null 2>&1 || fail "go command is not available"

mkdir -p "$REPORT_DIR"
reset_work_dir
copy_tree "$TRICE_DIR" "$WORK_DIR"
copy_tree "$OFFICIAL_DIR" "$WORK_DIR"

( cd "$WORK_DIR" && go version ) > "$REPORT_DIR/go-version.txt" 2>&1 || true
( cd "$WORK_DIR" && go env GOVERSION GOOS GOARCH CGO_ENABLED CC CXX GOMOD ) > "$REPORT_DIR/go-env.txt" 2>&1 || true
write_git_state
write_hashes
write_workspace_provenance
prepare_official_cgo_sources
write_report_header

OVERALL_STATUS=0

TEST_CASES=(
	"internal_decoder|./internal/decoder|Official decoder package tests"
	"internal_fmtspec|./internal/fmtspec|Official printf format parser tests"
	"internal_trex_decoder|./internal/trexDecoder|Official trex decoder success and error path tests"
	"runtime_none_direct_rtt32|./_test/staticB_di_nopf_rtt32|NONE framing, direct output, RTT32"
	"runtime_cobs_deferred_multi_uart|./_test/dblB_de_multi_cobs_ua|COBS framing, deferred multi-pack, UARTA"
	"runtime_cobs_deferred_single_uart|./_test/dblB_de_single_cobs_ua|COBS framing, deferred single-pack, UARTA"
	"runtime_tcobsv1_direct_rtt32|./_test/staticB_di_tcobs_rtt32|TCOBSv1 framing, direct output, RTT32"
	"runtime_tcobsv1_deferred_multi_uart|./_test/dblB_de_multi_tcobs_ua|TCOBSv1 framing, deferred multi-pack, UARTA"
	"runtime_tcobsv1_big_endian|./_test/be_dblB_de_tcobs_ua|TCOBSv1 framing with big-endian transfer order"
	"runtime_xtea_cobs_direct_rtt32|./_test/staticB_di_xtea_cobs_rtt32|XTEA plus COBS, direct output, RTT32"
	"runtime_xtea_cobs_deferred_multi_uart|./_test/dblB_de_multi_xtea_cobs_ua|XTEA plus COBS, deferred multi-pack, UARTA"
)

for entry in "${TEST_CASES[@]}"; do
	IFS='|' read -r name package_path description <<< "$entry"
	run_case "$name" "$package_path" "$description"
done

{
	printf '\nSummary file: `%s`\n\n' "summary.tsv"
	printf 'Preparation log: `%s`\n\n' "prepare-triceCheck.log"
	printf 'Workspace provenance: `%s`\n\n' "workspace-provenance.txt"
	printf 'Reference hashes: `%s`\n\n' "reference-hashes.sha256"
	printf 'Go environment: `%s`, `%s`\n\n' "go-version.txt" "go-env.txt"
} >> "$REPORT_DIR/report.md"

printf 'Go golden report written to %s\n' "$REPORT_DIR"
exit "$OVERALL_STATUS"
