#!/usr/bin/env bash
# Allow for non POSIX compliant bash


# Runs the rv32ui-p/rv32um-p riscv-tests against nf-functional.
# Exit 0 = all tests passed; 1 = at least one failure; 2 = harness error.

# guarantee that a fail is because of riscv-tests, not a script error
set -euo pipefail


readonly RISCV_TESTS_DIR="${RISCV_TESTS_DIR:-$HOME/software/riscv-tests/isa}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly NF_FUNCTIONAL="${NF_FUNCTIONAL:-$script_dir/../../build/bin/nf-functional}"

[[ -x $NF_FUNCTIONAL ]] || { echo "error: nf-functional not built: $NF_FUNCTIONAL" >&2; exit 2; }
[[ -d $RISCV_TESTS_DIR ]] || { echo "error: no riscv-tests dir: $RISCV_TESTS_DIR" >&2; exit 2; }

pass=0
fail=0
for elf in "$RISCV_TESTS_DIR"/rv32ui-p-* "$RISCV_TESTS_DIR"/rv32um-p-*; do
    [[ -f $elf && -x $elf ]] || continue   # skip *.dump, non-executables
    name="$(basename "$elf")"
    if output="$("$NF_FUNCTIONAL" "$elf" 2>&1)"; then
        echo "PASS $name"
        pass=$((pass + 1))
    else
        rc=$?
        echo "FAIL $name (exit $rc)"
        [[ $rc -eq 1 ]] || echo "  note: exit $rc is a harness/simulator error, not a test verdict"
        printf '  %s\n' "${output//$'\n'/$'\n  '}"
        fail=$((fail + 1))
    fi
done

echo
echo "$pass passed, $fail failed"
(( fail == 0 ))