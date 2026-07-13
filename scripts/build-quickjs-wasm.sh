#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="build"
ABI_MODE="${VENOM_QJS_ABI_MODE:-release}"
OPT_PROFILE="${VENOM_QJS_OPT_PROFILE:-balanced}"
MEM_PROFILE="${VENOM_QJS_MEMORY_PROFILE:-medium}"
OUT_DIR="$ROOT/build/quickjs-wasm"
EMCC_ARG=""
FORCE=0
CLEAN=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --preflight-only) MODE="preflight" ;;
    --status) MODE="status" ;;
    --verify) MODE="verify" ;;
    --force) FORCE=1 ;;
    --clean) CLEAN=1 ;;
    --emcc) EMCC_ARG="$2"; shift ;;
    --out-dir) OUT_DIR="$2"; shift ;;
    --abi-mode) ABI_MODE="$2"; shift ;;
    --optimization-profile) OPT_PROFILE="$2"; shift ;;
    --memory-profile) MEM_PROFILE="$2"; shift ;;
    *) OUT_DIR="$1" ;;
  esac
  shift
done
LIFECYCLE="$ROOT/tools/quickjs_runtime_lifecycle.py"
if [[ "$CLEAN" == 1 ]]; then rm -rf "$OUT_DIR"; fi
mkdir -p "$OUT_DIR"
if [[ "$MODE" == "status" ]]; then
  python3 "$LIFECYCLE" status --repo-root "$ROOT" --out-dir "$OUT_DIR" ${EMCC_ARG:+--emcc "$EMCC_ARG"} --abi-mode "$ABI_MODE" --optimization-profile "$OPT_PROFILE" --memory-profile "$MEM_PROFILE"
  exit $?
fi
if [[ "$MODE" == "verify" ]]; then
  python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --out-dir "$OUT_DIR" --verify-embedded --require-real
  exit $?
fi
EMCC="$(python3 "$LIFECYCLE" resolve-emcc --repo-root "$ROOT" ${EMCC_ARG:+--emcc "$EMCC_ARG"} --format path)" || {
  echo "[venom] Emscripten not found. Run scripts/setup-emscripten.sh or pass --emcc." >&2; exit 3;
}
EMCC_SOURCE="$(python3 "$LIFECYCLE" status --repo-root "$ROOT" --out-dir "$OUT_DIR" --emcc "$EMCC" --abi-mode "$ABI_MODE" --optimization-profile "$OPT_PROFILE" --memory-profile "$MEM_PROFILE" --format json | python3 -c 'import json,sys; print(json.load(sys.stdin)["emcc_source"])')"
echo "[venom] using $EMCC_SOURCE Emscripten: $EMCC"
if [[ "$FORCE" != 1 ]]; then
  STATUS_JSON="$(python3 "$LIFECYCLE" status --repo-root "$ROOT" --out-dir "$OUT_DIR" --emcc "$EMCC" --abi-mode "$ABI_MODE" --optimization-profile "$OPT_PROFILE" --memory-profile "$MEM_PROFILE" --format json)"
  CURRENT="$(printf '%s' "$STATUS_JSON" | python3 -c 'import json,sys; print("1" if json.load(sys.stdin)["current"] else "0")')"
  if [[ "$CURRENT" == 1 ]]; then
    echo "[venom] QuickJS WASM runtime is current; rebuild skipped. Use --force to rebuild."
    python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --out-dir "$OUT_DIR" --verify-embedded --require-real
    exit $?
  fi
  REASONS="$(printf '%s' "$STATUS_JSON" | python3 -c 'import json,sys; print(", ".join(json.load(sys.stdin)["reasons"]))')"
  echo "[venom] QuickJS WASM runtime is stale: $REASONS"
fi
case "$OPT_PROFILE" in
  size) OPT_FLAGS=(-Oz) ;;
  balanced) OPT_FLAGS=(-O3 -flto) ;;
  performance) OPT_FLAGS=(-O3 -flto -fno-exceptions -fno-rtti) ;;
  *) echo "[venom] invalid VENOM_QJS_OPT_PROFILE: $OPT_PROFILE (size|balanced|performance)" >&2; exit 2 ;;
esac
case "$MEM_PROFILE" in
  small) INITIAL_MEMORY=16777216; MAXIMUM_MEMORY=33554432 ;;
  medium) INITIAL_MEMORY=33554432; MAXIMUM_MEMORY=67108864 ;;
  large) INITIAL_MEMORY=67108864; MAXIMUM_MEMORY=134217728 ;;
  *) echo "[venom] invalid VENOM_QJS_MEMORY_PROFILE: $MEM_PROFILE (small|medium|large)" >&2; exit 2 ;;
esac

PREFLIGHT_MANIFEST="$OUT_DIR/quickjs-wasm-preflight.txt"
PREFLIGHT_JSON="$OUT_DIR/quickjs-wasm-preflight.json"
if [[ "$MODE" == "preflight" ]]; then
  python3 "$ROOT/tools/quickjs_wasm_preflight.py" \
    --repo-root "$ROOT" \
    --emcc "$EMCC" \
    --allow-missing-emcc \
    --manifest "$PREFLIGHT_MANIFEST" \
    --json "$PREFLIGHT_JSON"
  echo "[venom] wrote QuickJS WASM preflight report: $PREFLIGHT_MANIFEST"
  exit 0
fi
python3 "$ROOT/tools/quickjs_wasm_preflight.py" \
  --repo-root "$ROOT" \
  --emcc "$EMCC" \
  --manifest "$PREFLIGHT_MANIFEST" \
  --json "$PREFLIGHT_JSON"
if ! command -v "$EMCC" >/dev/null 2>&1; then
  echo "[venom] emcc was not found. Install/activate Emscripten, or set EMCC=/path/to/emcc." >&2
  exit 3
fi
EXPORTS_JSON="$OUT_DIR/quickjs-runtime.emcc-exports.json"
if [[ "$ABI_MODE" == "release" ]]; then
  python3 "$ROOT/tools/quickjs_release_abi.py" "$EXPORTS_JSON"
else
  python3 - "$ROOT/src/runtime/quickjs_runtime_scaffold.c" "$EXPORTS_JSON" <<'PY'
import json, re, sys
src, out = sys.argv[1:]
text = open(src, encoding='utf-8').read()
names = sorted(set(re.findall(r'VENOM_QJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))
open(out, 'w', encoding='utf-8').write(json.dumps(['_' + n for n in names], indent=2))
print(f"[venom] wrote {len(names)} debug ABI exports to {out}")
PY
fi
"$EMCC" \
  "$ROOT/src/runtime/quickjs_runtime_scaffold.c" \
  "$ROOT/third_party/quickjs/quickjs.c" \
  "$ROOT/third_party/quickjs/quickjs-libc.c" \
  "$ROOT/third_party/quickjs/libregexp.c" \
  "$ROOT/third_party/quickjs/libunicode.c" \
  "$ROOT/third_party/quickjs/dtoa.c" \
  -I"$ROOT/third_party/quickjs" \
  -DVENOM_ENABLE_UPSTREAM_QJS_WASM=1 \
  $([[ "$ABI_MODE" == "release" ]] && printf "%s" "-DVENOM_QJS_MINIMAL_EXPORTS=1" || printf "%s" "-DVENOM_QJS_DEBUG_EXPORTS=1") \
  -DVENOM_WASM_NATIVE_STACK_CAPACITY=4194304 \
  -DQUICKJS_NG_BUILD \
  -D_GNU_SOURCE \
  -DNDEBUG \
  -g0 \
  "-ffile-prefix-map=$ROOT=." \
  "-fdebug-prefix-map=$ROOT=." \
  "-fmacro-prefix-map=$ROOT=." \
  "${OPT_FLAGS[@]}" \
  -sSTANDALONE_WASM=1 \
  -sASSERTIONS=0 \
  -sSAFE_HEAP=0 \
  -sSTACK_OVERFLOW_CHECK=0 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sSTACK_SIZE=4194304 \
  -sINITIAL_MEMORY="$INITIAL_MEMORY" \
  -sMAXIMUM_MEMORY="$MAXIMUM_MEMORY" \
  -sEXPORTED_FUNCTIONS=@"$EXPORTS_JSON" \
  -Wl,--no-entry \
  -Wl,--strip-all \
  -o "$OUT_DIR/quickjs-runtime.wasm"
WASM_OPT="$(python3 "$LIFECYCLE" resolve-wasm-opt --repo-root "$ROOT" --format path 2>/dev/null || true)"
if [[ -n "$WASM_OPT" ]]; then
  case "$OPT_PROFILE" in size) WASM_OPT_LEVEL=-Oz ;; balanced) WASM_OPT_LEVEL=-O3 ;; performance) WASM_OPT_LEVEL=-O4 ;; esac
  # Emscripten may emit post-MVP instructions such as memory.copy, memory.fill,
  # and trunc_sat conversions. Detect and explicitly permit those features
  # before Binaryen validates and optimizes the standalone module.
  WASM_FEATURE_FLAGS=(
    --detect-features
    --enable-bulk-memory-opt
    --enable-nontrapping-float-to-int
  )
  echo "[venom] applying Binaryen $WASM_OPT_LEVEL with detected Emscripten features using $WASM_OPT"
  "$WASM_OPT" "$WASM_OPT_LEVEL" "${WASM_FEATURE_FLAGS[@]}" --strip-debug --strip-producers "$OUT_DIR/quickjs-runtime.wasm" -o "$OUT_DIR/quickjs-runtime.opt.wasm"
  mv "$OUT_DIR/quickjs-runtime.opt.wasm" "$OUT_DIR/quickjs-runtime.wasm"
else
  echo "[venom] wasm-opt not found; keeping optimized Emscripten output."
fi
python3 "$ROOT/tools/check_wasm_release_strings.py" "$OUT_DIR/quickjs-runtime.wasm"
cat > "$OUT_DIR/quickjs-build-profile.txt" <<EOF
abi=$ABI_MODE
optimization=$OPT_PROFILE
memory=$MEM_PROFILE
initial_memory=$INITIAL_MEMORY
maximum_memory=$MAXIMUM_MEMORY
EOF
python3 "$ROOT/tools/quickjs_wasm_cutover.py" \
  --repo-root "$ROOT" \
  --out-dir "$OUT_DIR" \
  --emcc "$EMCC" \
  --artifact "$OUT_DIR/quickjs-runtime.wasm"
python3 "$LIFECYCLE" write-state --repo-root "$ROOT" --out-dir "$OUT_DIR" --emcc "$EMCC" --abi-mode "$ABI_MODE" --optimization-profile "$OPT_PROFILE" --memory-profile "$MEM_PROFILE" >/dev/null
python3 "$ROOT/tools/runtime_performance.py" --repo-root "$ROOT" --artifact "$OUT_DIR/quickjs-runtime.wasm" --json-out "$OUT_DIR/runtime-performance.json"
echo "[venom] built, embedded, benchmarked, and verified upstream QuickJS WASM artifact: $OUT_DIR/quickjs-runtime.wasm"
