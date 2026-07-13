#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-$ROOT/build-fuzz}"
SECONDS_PER_TARGET="${VENOM_FUZZ_SECONDS:-300}"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVENOM_BUILD_FUZZERS=ON -DVENOM_ENABLE_SANITIZERS=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build "$BUILD" --parallel
"$BUILD/venom_package_parser_fuzz" "$ROOT/tests/fuzz/corpus/package_parser" -max_total_time="$SECONDS_PER_TARGET" -timeout=3
"$BUILD/venom_runtime_contract_fuzz" "$ROOT/tests/fuzz/corpus/runtime_contracts" -max_total_time="$SECONDS_PER_TARGET" -timeout=3
"$BUILD/venom_package_parser_differential_fuzz" "$ROOT/tests/fuzz/corpus/package_parser" -max_total_time="$SECONDS_PER_TARGET" -timeout=3
python3 "$ROOT/tools/run_fuzz_gate.py" --replay "$BUILD/venom_package_parser_replay" --corpus "$ROOT/tests/fuzz/corpus/package_parser" --json-out "$BUILD/package-fuzz-gate.json"
python3 "$ROOT/tools/run_fuzz_gate.py" --replay "$BUILD/venom_runtime_contract_replay" --corpus "$ROOT/tests/fuzz/corpus/runtime_contracts" --json-out "$BUILD/contract-fuzz-gate.json"
