#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CONFIG=${1:-Release}
BUILD_DIR=${2:-build}
FULL_QUICKJS=${3:-}
"$ROOT/scripts/build.sh" "$CONFIG" "$BUILD_DIR" "$FULL_QUICKJS"
ctest --test-dir "$ROOT/$BUILD_DIR" --build-config "$CONFIG" --output-on-failure
