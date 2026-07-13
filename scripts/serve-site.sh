#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${1:-8080}"
DIST="${2:-$ROOT/dist}"
case "$DIST" in /*) DIST_PATH="$DIST" ;; *) DIST_PATH="$ROOT/$DIST" ;; esac
if [ ! -f "$DIST_PATH/index.html" ]; then
  "$ROOT/scripts/build-site.sh" "$ROOT/examples/basic-site" "$DIST_PATH"
fi
echo "[venom] serving $DIST_PATH at http://127.0.0.1:$PORT"
python3 -m http.server "$PORT" --directory "$DIST_PATH"
