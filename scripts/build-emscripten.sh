#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARGS=("$@")
python3 "$ROOT/tools/build_emscripten.py" --repo-root "$ROOT" "${ARGS[@]}"
