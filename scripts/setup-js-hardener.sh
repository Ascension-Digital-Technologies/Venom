#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TOOL="$ROOT/tools/js-hardener"
cd "$TOOL"
command -v node >/dev/null 2>&1 || { echo 'Node.js 20 or newer is required.' >&2; exit 1; }
command -v npm >/dev/null 2>&1 || { echo 'npm is required.' >&2; exit 1; }
NODE_MAJOR=$(node -p "Number(process.versions.node.split('.')[0])")
[ "$NODE_MAJOR" -ge 20 ] || { echo "Node.js 20 or newer is required (found $(node --version))." >&2; exit 1; }

needs_install=0
[ -f node_modules/terser/package.json ] || needs_install=1
[ -f node_modules/javascript-obfuscator/package.json ] || needs_install=1
if [ "$needs_install" -eq 0 ]; then
  node -e "await Promise.all([import('terser'), import('javascript-obfuscator')]);" >/dev/null 2>&1 || needs_install=1
fi

if [ "$needs_install" -ne 0 ]; then
  echo '[venom] installing pinned JavaScript hardener dependencies...'
  rm -rf node_modules
  if [ -f package-lock.json ] && grep -Eq 'applied-caas-gateway|internal\.api\.openai\.org' package-lock.json; then
    rm -f package-lock.json
  fi
  npm install --ignore-scripts --no-audit --no-fund --save-exact
fi

node -e "const [t,o]=await Promise.all([import('terser'),import('javascript-obfuscator')]); if(typeof t.minify!=='function'||!(o.default||o).obfuscate) process.exit(2);"
TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/venom-js-hardener-check.XXXXXX")
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM
printf '%s\n' 'function add(a,b){return a+b;}globalThis.__venomHardenerCheck=add(2,3);' > "$TMP_DIR/input.js"
node "$TOOL/harden.mjs" "$TMP_DIR/input.js" "$TMP_DIR/output.js" runtime 1362
[ -s "$TMP_DIR/output.js" ] || { echo 'JavaScript hardener self-test produced no output.' >&2; exit 1; }
node --check "$TMP_DIR/output.js"
echo '[venom] release JavaScript hardener installed and verified'
