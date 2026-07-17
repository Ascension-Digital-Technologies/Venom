#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${PROFILE:-prod}"
PORT="${PORT:-8080}"
DIST="${DIST:-$ROOT/dist-protected-chess-$PROFILE}"
"$ROOT/scripts/build-site.sh" "$ROOT/examples/protected-chess" "$DIST" "$ROOT/build" "$PROFILE"
echo "[venom] serving protected-chess at http://127.0.0.1:$PORT/"
if command -v xdg-open >/dev/null 2>&1; then xdg-open "http://127.0.0.1:$PORT/" >/dev/null 2>&1 || true;
elif command -v open >/dev/null 2>&1; then open "http://127.0.0.1:$PORT/" >/dev/null 2>&1 || true; fi
exec python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$DIST"
