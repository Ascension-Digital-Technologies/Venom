#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
[[ -d "$ROOT/tools" ]] || ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
if [[ $# -eq 0 ]]; then
  exec python3 "$ROOT/tools/core_release_closure.py" --repo-root "$ROOT" --build
fi
exec python3 "$ROOT/tools/verify_release.py" "$@"
