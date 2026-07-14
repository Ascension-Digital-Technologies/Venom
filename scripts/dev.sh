#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SITE=${1:-examples/protected-chess}
DIST=${2:-dist-dev}
PORT=${PORT:-8080}
EXE="$ROOT/build/venom"
[ -x "$EXE" ] || EXE="$ROOT/build/Release/venom"
[ -x "$EXE" ] || { echo "Venom executable not found. Run scripts/build.sh first." >&2; exit 1; }
exec "$EXE" dev "$ROOT/$SITE" --out "$ROOT/$DIST" --port "$PORT"
