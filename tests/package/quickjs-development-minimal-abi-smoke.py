#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
engine = (root / 'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')

# Development uses the same minimal bytecode-only WASM export surface as a
# release. Only the accepted trust-domain producer differs.
assert "const required = RELEASE_ABI_REQUIRED_EXPORTS;" in engine
assert "e.venom_qjs_bridge_abi() >>> 0" in engine
assert "QuickJS WASM bridge ABI mismatch" in engine

# The obsolete scaffold/debug surface must not be mandatory in loadWasm().
load = engine[engine.index('async function loadWasm()'):engine.index('function createContext', engine.index('async function loadWasm()'))]
for obsolete in (
    "'venom_qjs_engine_abi'",
    "'venom_qjs_engine_version'",
    "'venom_qjs_execute_source'",
    "'venom_qjs_wasm_native_stack_capacity'",
):
    assert obsolete not in load, obsolete
assert 'e.venom_qjs_engine_abi() !== 12' not in load
assert 'nativeStackCapacity < 2097152' not in load
print('quickjs development minimal ABI: PASS')
