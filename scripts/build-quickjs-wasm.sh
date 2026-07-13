#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "${1:-}" == "--verify-only" ]]; then exec python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --verify-embedded --require-real; fi
[[ $# -eq 1 ]] || { echo "usage: $0 <quickjs-runtime.wasm> | --verify-only" >&2; exit 2; }
exec python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --artifact "$1" --require-real
