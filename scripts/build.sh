#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${1:-Release}"
BUILD_DIR="${2:-build}"
cmake -S "$ROOT" -B "$ROOT/$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$ROOT/$BUILD_DIR" --config "$CONFIG" --parallel
