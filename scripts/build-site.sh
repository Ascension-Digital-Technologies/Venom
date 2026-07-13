#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE="${1:-$ROOT/examples/protected-chess}"
DIST="${2:-$ROOT/dist}"
BUILD_DIR="${3:-$ROOT/build}"
PROFILE="${4:-browser-protect}"
EXE="$BUILD_DIR/venom"
[[ -x "$EXE" ]] || "$ROOT/scripts/build.sh" Release "$(realpath --relative-to="$ROOT" "$BUILD_DIR" 2>/dev/null || echo build)"
"$EXE" build "$SITE" --out "$DIST" --profile "$PROFILE" --hashed
if [[ "$PROFILE" != debug ]]; then python3 "$ROOT/scripts/check-production-leaks.py" "$DIST"; fi
