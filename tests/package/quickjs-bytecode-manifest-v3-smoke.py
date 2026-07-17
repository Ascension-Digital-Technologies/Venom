#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
runtime = (root / "src/generated/runtime/runtime_js.cpp").read_text(encoding="utf-8")
runtime += (root / 'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
compiler = (root / "src/quickjs/bytecode.cpp").read_text(encoding="utf-8")
wasm = (root / "src/runtime/quickjs_runtime_scaffold.c").read_text(encoding="utf-8")
blob = (root / "src/generated/runtime/quickjs_runtime_wasm_blob.hpp").read_text(encoding="utf-8")
engine = (root / "src/generated/runtime/quickjs_engine_module.cpp").read_text(encoding="utf-8")

accepted = "['VENOM_QJS_BYTECODE_MANIFEST_V3', 'VENOM_QJS_BYTECODE_MANIFEST_V2', 'VENOM_QJS_BYTECODE_MANIFEST_V1', 'VENOM_QUICKJS_BYTECODE_MANIFEST_V1']"
checks = {
    "browser runtime accepts V3": accepted in runtime,
    "compiler emits V3": 'VENOM_QJS_BYTECODE_MANIFEST_V3\\n' in compiler,
    "WASM self-manifest is V3": 'VENOM_QJS_BYTECODE_MANIFEST_V3\\n' in wasm,
    "WASM self-manifest version is 3": '"version=3\\n"' in wasm,
    "embedded blob is non-empty": 'kQuickJsRuntimeWasmBlobSize' in blob,
    "WASM host-effect helper is defined": 'function applyWasmHostEffects(effects, bindings)' in engine,
    "WASM host-effect helper is used": 'applyWasmHostEffects(wasmResult.hostEffects, bindings)' in engine,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"{name}: {'PASS' if ok else 'FAIL'}")
if failed:
    print("failed: " + ", ".join(failed), file=sys.stderr)
    raise SystemExit(1)
