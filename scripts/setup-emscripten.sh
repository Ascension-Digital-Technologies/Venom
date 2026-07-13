#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/build/emscripten-setup"
EXTRA=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --out-dir)
      OUT_DIR="$2"; shift 2 ;;
    --check-only)
      EXTRA+=("--check-only"); shift ;;
    --allow-missing)
      EXTRA+=("--allow-missing"); shift ;;
    --skip-download)
      EXTRA+=("--skip-download"); shift ;;
    --emsdk-dir)
      EXTRA+=("--emsdk-dir" "$2"); shift 2 ;;
    --version)
      EXTRA+=("--version" "$2"); shift 2 ;;
    --emcc)
      EXTRA+=("--emcc" "$2"); shift 2 ;;
    *)
      echo "usage: setup-emscripten.sh [--check-only] [--allow-missing] [--skip-download] [--emsdk-dir DIR] [--version VERSION] [--emcc PATH] [--out-dir DIR]" >&2
      exit 2 ;;
  esac
done
python3 "$ROOT/tools/setup_emscripten.py" --repo-root "$ROOT" --out-dir "$OUT_DIR" "${EXTRA[@]}"
