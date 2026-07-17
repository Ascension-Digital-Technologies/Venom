#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENOM="${VENOM:-$ROOT/build/venom}"
BROWSER="${BROWSER:-all}"
PROFILE="${PROFILE:-dev}"
OUT="${OUT:-build/compatibility}"
python3 "$ROOT/tools/run_compatibility_suite.py" --venom "$VENOM" --browser "$BROWSER" --profile "$PROFILE" --out "$OUT" "$@"
