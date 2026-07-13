param(
  [string]$BuildDir = "build-fuzz",
  [int]$SecondsPerTarget = 300
)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
cmake -S $Root -B $BuildDir -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVENOM_BUILD_FUZZERS=ON -DVENOM_ENABLE_SANITIZERS=ON -DCMAKE_CXX_COMPILER=clang++
cmake --build $BuildDir --parallel
& "$BuildDir/venom_package_parser_fuzz" "$Root/tests/fuzz/corpus/package_parser" "-max_total_time=$SecondsPerTarget" -timeout=3
& "$BuildDir/venom_runtime_contract_fuzz" "$Root/tests/fuzz/corpus/runtime_contracts" "-max_total_time=$SecondsPerTarget" -timeout=3
& "$BuildDir/venom_package_parser_differential_fuzz" "$Root/tests/fuzz/corpus/package_parser" "-max_total_time=$SecondsPerTarget" -timeout=3
python "$Root/tools/run_fuzz_gate.py" --replay "$BuildDir/venom_package_parser_replay.exe" --corpus "$Root/tests/fuzz/corpus/package_parser" --json-out "$BuildDir/package-fuzz-gate.json"
python "$Root/tools/run_fuzz_gate.py" --replay "$BuildDir/venom_runtime_contract_replay.exe" --corpus "$Root/tests/fuzz/corpus/runtime_contracts" --json-out "$BuildDir/contract-fuzz-gate.json"
