#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=${VENOM_FUZZ_BUILD:-"$ROOT/build-fuzz"}
SECONDS=${VENOM_FUZZ_SECONDS:-300}
cmake -S "$ROOT" -B "$BUILD" -DVENOM_BUILD_FUZZERS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build "$BUILD" --target venom_package_parser_fuzz
"$BUILD/venom_package_parser_fuzz" "$ROOT/tests/fuzz/corpus/package_parser" -max_total_time="$SECONDS" -timeout=2 -rss_limit_mb=1024
