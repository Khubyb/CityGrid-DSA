#!/bin/bash
# Usage (from the project root, after building):  bash tests/run_tests.sh
# Runs every tests/testNN_* folder and compares with expected_output.txt.
BIN="$(pwd)/citygrid"
pass=0; fail=0
for d in tests/test*/; do
    name=$(basename "$d")
    out=$(cd "$d" && "$BIN" junctions.txt incidents.txt script.txt 2>&1 | sed 's/[ \t]*$//')
    exp=$(sed 's/[ \t]*$//' "$d/expected_output.txt")
    if [ "$out" == "$exp" ]; then echo "PASS  $name"; pass=$((pass+1))
    else echo "FAIL  $name"; fail=$((fail+1)); fi
done
echo "Passed: $pass   Failed: $fail"
[ $fail -eq 0 ]
