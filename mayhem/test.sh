#!/usr/bin/env bash
#
# mayhem/test.sh — RUN upstream's ctest suite and the known-answer probe (both built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# 1) Known-answer probe: libebml computes fixed inputs -> exact values (CRC-32 check value, EBML
#    wire encodings, parse-back of every element type, MemIOCallback semantics). The expected
#    lines below were cross-checked against an independent EBML encoder (mayhem/gen_seeds.py) and
#    zlib.crc32. A missing probe, or one that prints nothing, fails every assertion.
KAT=/mayhem/kat
[ -x "$KAT" ] || { echo "missing $KAT (build.sh should have built it)" >&2; emit_ctrf "libebml-kat" 0 1 0; exit 1; }
ACTUAL="$("$KAT" 2>&1)"
EXPECTED='KAT crc32=cbf43926
KAT sizelen=1,1,2,2
KAT codedvalue300=412c
KAT readcoded=300
KAT rendered=57
KAT tree=1b4d5a01b48182012c8281fe8383616263848668c3a96c6c6f85843fc000008688f26c5ac8eecb0000468783deadbea088918107a38393817a
KAT children=8
KAT uint=300
KAT sint=-2
KAT str=abc
KAT uni=h\xc3\xa9llo
KAT float=1.50
KAT date=0
KAT bin=deadbe
KAT subuint=7
KAT deepstr=z
KAT memio=4,3456,7,10
KAT memio_atend=0
KAT head=46
KAT headcrc=1
KAT doctype=matroska'
EXPECTED="$(printf '%b' "$EXPECTED")"
passed=0; failed=0
while IFS= read -r line; do
  if printf '%s\n' "$ACTUAL" | grep -qxF -- "$line"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1)); echo "KAT MISMATCH: expected '$line'" >&2
  fi
done <<< "$EXPECTED"

# 2) Upstream's own ctest suite (exit-code runner; the probe above is what makes the oracle behavioral).
if [ -d build/normal ]; then
  CT="$(cd build/normal && ctest 2>&1)" || true
  echo "$CT" | tail -5
  ct_total="$(printf '%s\n' "$CT" | sed -n 's/.*tests passed, [0-9]* tests failed out of \([0-9]*\).*/\1/p')"
  ct_failed="$(printf '%s\n' "$CT" | sed -n 's/.*tests passed, \([0-9]*\) tests failed out of.*/\1/p')"
  if [ -z "$ct_total" ] || [ "$ct_total" -lt 8 ]; then
    echo "ctest ran fewer than the 8 upstream tests" >&2; failed=$((failed + 1))
  else
    passed=$((passed + ct_total - ct_failed)); failed=$((failed + ct_failed))
  fi
else
  echo "missing build/normal (build.sh should have built the upstream tests)" >&2; failed=$((failed + 1))
fi

emit_ctrf "libebml-kat+ctest" "$passed" "$failed" 0
