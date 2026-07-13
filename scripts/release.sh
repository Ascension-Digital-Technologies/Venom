#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
python3 "$ROOT/tools/public_release_gate.py" "$ROOT"
BUILD_DIR="${BUILD_DIR:-build/release}"
DIST="${DIST:-build/qualified-chess-dist}"
SEED="${SEED:-1350001}"
BROWSER="${BROWSER:-chromium}"
if [[ "${SKIP_BUILD:-0}" != "1" ]]; then "$ROOT/scripts/build.sh" Release "$BUILD_DIR"; fi
VENOM="$ROOT/$BUILD_DIR/venom"
python3 "$ROOT/tools/release_qualification.py" --venom "$VENOM" --dist "$DIST" --seed "$SEED" --browser "$BROWSER"
python3 "$ROOT/tools/package_release.py" --repo-root "$ROOT" --build-dir "$ROOT/$BUILD_DIR" --out "$ROOT/build/release-package" --archive zip --sign none
