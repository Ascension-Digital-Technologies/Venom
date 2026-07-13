#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CONFIG=${1:-Release}
BUILD_DIR=${2:-build}
FULL_QUICKJS=${3:-}
ARGS="-S $ROOT -B $ROOT/$BUILD_DIR -DCMAKE_BUILD_TYPE=$CONFIG"
if [ "$FULL_QUICKJS" = "fullquickjs" ] || [ "$FULL_QUICKJS" = "full-qjs" ]; then
  ARGS="$ARGS -DVENOM_ENABLE_FULL_QUICKJS=ON"
fi
cmake $ARGS
cmake --build "$ROOT/$BUILD_DIR" --config "$CONFIG" --parallel
