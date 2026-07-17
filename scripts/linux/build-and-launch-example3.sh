#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
[[ -d "$ROOT/tools" ]] || ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
exec python3 "$ROOT/tools/build_and_launch_example.py" 3 "$@"
