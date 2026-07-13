#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT/tools/js-hardener"
command -v node >/dev/null 2>&1 || { echo 'Node.js 20 or newer is required.' >&2; exit 1; }
command -v npm >/dev/null 2>&1 || { echo 'npm is required.' >&2; exit 1; }
npm install --ignore-scripts --no-audit --no-fund
echo '[venom] release JavaScript hardener ready'
