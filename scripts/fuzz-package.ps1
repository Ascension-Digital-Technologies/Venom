$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Build = if ($env:VENOM_FUZZ_BUILD) { $env:VENOM_FUZZ_BUILD } else { Join-Path $Root 'build-fuzz' }
$Seconds = if ($env:VENOM_FUZZ_SECONDS) { $env:VENOM_FUZZ_SECONDS } else { '300' }
cmake -S $Root -B $Build -DVENOM_BUILD_FUZZERS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build $Build --target venom_package_parser_fuzz
& (Join-Path $Build 'venom_package_parser_fuzz.exe') (Join-Path $Root 'tests/fuzz/corpus/package_parser') "-max_total_time=$Seconds" '-timeout=2' '-rss_limit_mb=1024'
exit $LASTEXITCODE
