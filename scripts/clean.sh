#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
rm -rf   "$ROOT/build" "$ROOT/build-debug" "$ROOT/build-release" "$ROOT/build-fullqjs" "$ROOT/build-werror"   "$ROOT/dist" "$ROOT/dist-basic" "$ROOT/dist-basic-wasm" "$ROOT/dist-basic-protect" "$ROOT/dist-basic-release"   "$ROOT/dist-release" "$ROOT/dist-protect" "$ROOT/dist-wasm"
