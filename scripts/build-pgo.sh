#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${VENOM_PGO_BUILD_ROOT:-$ROOT/build/pgo}"
PROFILE_DIR="${VENOM_PGO_DIR:-$BUILD_ROOT/profiles}"
TRAIN_SITE="${VENOM_PGO_TRAIN_SITE:-$ROOT/tests/fixtures/production-site}"
GEN="$BUILD_ROOT/generate"; USE="$BUILD_ROOT/use"; DIST="$BUILD_ROOT/training-dist"
rm -rf "$GEN" "$USE" "$PROFILE_DIR" "$DIST"; mkdir -p "$PROFILE_DIR"
cmake -S "$ROOT" -B "$GEN" -DCMAKE_BUILD_TYPE=Release -DVENOM_PGO_MODE=generate -DVENOM_PGO_DIR="$PROFILE_DIR" -DBUILD_TESTING=OFF
cmake --build "$GEN" --config Release --parallel
BIN="$GEN/venom"; [[ -x "$BIN" ]] || BIN="$GEN/Release/venom.exe"
"$BIN" build "$TRAIN_SITE" --out "$DIST" --vendor-cache "$ROOT/tests/fixtures/remote-cache" --offline
"$BIN" verify-runtime "$DIST" --target browser --require-real-engine || true
cmake -S "$ROOT" -B "$USE" -DCMAKE_BUILD_TYPE=Release -DVENOM_PGO_MODE=use -DVENOM_PGO_DIR="$PROFILE_DIR" -DBUILD_TESTING=OFF
cmake --build "$USE" --config Release --parallel
echo "[venom] PGO optimized build: $USE"
