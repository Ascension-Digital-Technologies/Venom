#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE="${1:-$ROOT/examples/basic-site}"
DIST="${2:-$ROOT/dist}"
CONFIG="${3:-Release}"
if [[ "${VENOM_REBUILD_RUNTIME:-0}" == "1" ]]; then "$ROOT/scripts/build-quickjs-wasm.sh" --force; fi
case "$DIST" in /*) DIST_PATH="$DIST" ;; *) DIST_PATH="$ROOT/$DIST" ;; esac
"$ROOT/scripts/build.sh" "$CONFIG" build
VENOM="$ROOT/build/venom"
if [ ! -x "$VENOM" ]; then
  if [ -x "$ROOT/build/$CONFIG/venom" ]; then VENOM="$ROOT/build/$CONFIG/venom"; else echo "[venom] error: venom executable not found under build/" >&2; exit 1; fi
fi
echo "[venom] production build $SITE => $DIST_PATH"
"$VENOM" build "$SITE" --out "$DIST_PATH"
"$VENOM" verify-runtime "$DIST_PATH" --target browser --require-real-engine
PACKAGE=$(find "$DIST_PATH/assets/app" -maxdepth 1 -type f -name 'app.*.vbc' | head -n 1 || true)
if [ -z "$PACKAGE" ]; then echo "[venom] error: package was not emitted under $DIST_PATH/assets/app" >&2; exit 1; fi
echo "[venom] package: $PACKAGE"
