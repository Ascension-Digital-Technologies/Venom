#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE="${1:-$ROOT/examples/protected-chess}"
DIST="${2:-$ROOT/dist}"
BUILD_DIR="${3:-$ROOT/build}"
PROFILE="${4:-prod}"
SKIP_COMPILER_BUILD="${VENOM_SKIP_COMPILER_BUILD:-0}"
case "$PROFILE" in dev|prod) ;; *) echo "profile must be dev or prod" >&2; exit 2 ;; esac
if [[ "$SKIP_COMPILER_BUILD" != "1" ]]; then
  "$ROOT/scripts/build.sh" Release "$(realpath --relative-to="$ROOT" "$BUILD_DIR" 2>/dev/null || echo build)"
else
  echo "[venom] warning: compiler rebuild explicitly skipped"
fi
EXE="$BUILD_DIR/venom"
[[ -x "$EXE" ]] || { echo "venom executable not found: $EXE" >&2; exit 1; }
echo "[venom] compiler: $EXE"
if command -v sha256sum >/dev/null 2>&1; then echo "[venom] compiler fingerprint: $(sha256sum "$EXE" | awk '{print $1}')"; fi
rm -rf "$DIST"
"$EXE" build "$SITE" --out "$DIST" --profile "$PROFILE"
if [[ "$PROFILE" == prod ]]; then
  python3 "$ROOT/scripts/check-production-leaks.py" "$DIST"
  "$EXE" release-check "$DIST" --target browser
  "$EXE" verify-runtime "$DIST" --require-real-engine
fi
