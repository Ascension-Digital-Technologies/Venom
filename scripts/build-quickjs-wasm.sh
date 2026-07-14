#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; OUT="$ROOT/build/quickjs-wasm"; EMCC_ARG="${EMCC:-}"; MODE=controller; CLEAN=0
[[ "${1:-}" == --verify-only ]] && exec python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --verify-embedded --require-real
[[ $# -eq 1 && -f "${1:-}" ]] && exec python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --artifact "$1" --require-real
A=("$@"); for ((i=0;i<${#A[@]};i++));do case "${A[$i]}" in --controller-build)MODE=build;;--preflight-only)MODE=preflight;;--out-dir)((i++));OUT="${A[$i]}";;--emcc)((i++));EMCC_ARG="${A[$i]}";;--clean)CLEAN=1;;--force);;--status)exec python3 "$ROOT/tools/quickjs_runtime_lifecycle.py" status --repo-root "$ROOT" --out-dir "$OUT";;*) [[ "$MODE" == preflight ]]&&OUT="${A[$i]}"||{ echo "unknown argument ${A[$i]}" >&2;exit 2;};;esac;done

# Reject recursive wrapper/controller output growth before invoking any toolchain.
if [[ "$OUT" == *"/quickjs-wasm/quickjs-wasm/quickjs-wasm"* ]]; then
  echo "refusing recursively nested QuickJS/WASM output path: $OUT" >&2
  exit 2
fi
if [[ "$MODE" == controller ]];then exec python3 "$ROOT/tools/build_emscripten.py" --repo-root "$ROOT" --out-dir "$OUT" ${EMCC_ARG:+--emcc "$EMCC_ARG"};fi
if [[ "$MODE" == preflight ]];then mkdir -p "$OUT";exec python3 "$ROOT/tools/quickjs_wasm_preflight.py" --repo-root "$ROOT" --manifest "$OUT/quickjs-wasm-preflight.txt" --json "$OUT/quickjs-wasm-preflight.json" --allow-missing-emcc;fi
[[ -n "$EMCC_ARG" ]]||EMCC_ARG="$(python3 "$ROOT/tools/quickjs_runtime_lifecycle.py" resolve-emcc --repo-root "$ROOT" --format path)"; ((CLEAN))&&rm -rf "$OUT";mkdir -p "$OUT"; W="$OUT/quickjs-runtime.wasm"; E="$OUT/quickjs-runtime.emcc-exports.json";python3 "$ROOT/tools/quickjs_release_abi.py" "$E"; X="$(python3 -c 'import json,sys;print(json.dumps(json.load(open(sys.argv[1]))))' "$E")"
"$EMCC_ARG" "$ROOT/src/runtime/quickjs_runtime_scaffold.c" "$ROOT/third_party/quickjs/quickjs.c" "$ROOT/third_party/quickjs/quickjs-libc.c" "$ROOT/third_party/quickjs/libregexp.c" "$ROOT/third_party/quickjs/libunicode.c" "$ROOT/third_party/quickjs/dtoa.c" -I"$ROOT/third_party/quickjs" -DCONFIG_VERSION='"2026-06-04"' -DCONFIG_BIGNUM -DVENOM_ENABLE_UPSTREAM_QJS_WASM=1 -DVENOM_QJS_MINIMAL_EXPORTS=1 -O3 -flto -DNDEBUG -g0 -ffile-prefix-map="$ROOT"=. -fdebug-prefix-map="$ROOT"=. -fmacro-prefix-map="$ROOT"=. -sSTANDALONE_WASM=1 -sWASM=1 -sFILESYSTEM=0 -sENVIRONMENT=web,worker -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16777216 -sMAXIMUM_MEMORY=268435456 -sSTACK_SIZE=4194304 -sMALLOC=emmalloc -sERROR_ON_UNDEFINED_SYMBOLS=0 -sASSERTIONS=0 -sSAFE_HEAP=0 -sSTACK_OVERFLOW_CHECK=0 -sEXPORTED_FUNCTIONS="$X" --no-entry -o "$W"
WASM_FEATURE_FLAGS=(--detect-features --enable-bulk-memory-opt --enable-nontrapping-float-to-int); if O="$(python3 "$ROOT/tools/quickjs_runtime_lifecycle.py" resolve-wasm-opt --repo-root "$ROOT" --format path 2>/dev/null)";then "$O" "$W" -O3 --strip-all "${WASM_FEATURE_FLAGS[@]}" -o "$W.opt";mv "$W.opt" "$W";fi
python3 "$ROOT/tools/check_wasm_release_strings.py" "$W";python3 "$ROOT/tools/quickjs_wasm_cutover.py" --repo-root "$ROOT" --out-dir "$OUT" --emcc "$EMCC_ARG" --artifact "$W" --require-real;python3 "$ROOT/tools/quickjs_runtime_lifecycle.py" write-state --repo-root "$ROOT" --out-dir "$OUT" --emcc "$EMCC_ARG"
