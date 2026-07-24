#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
engine = (root / 'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
runtime = (root / 'src/runtime/turbojs_runtime_scaffold.c').read_text(encoding='utf-8')
worker = (root / 'src/generated/runtime/worker_runtime_template.hpp').read_text(encoding='utf-8')
checks = {
    'serialized wasm execution': 'let wasmExecutionQueue = Promise.resolve();' in engine and 'enqueueWasmExecution(() => executeWithWasmScaffold' in engine,
    'main buffer host wipe': 'zeroWasmRange(instance, ptr, requested);' in engine,
    'diversification host wipe': 'zeroWasmRange(instance, contractPtr, contract.byteLength);' in engine,
    'engine avoids legacy free': 'venom_tjs_script_buffer_free(' not in engine,
    'worker avoids legacy free': 'venom_tjs_script_buffer_free(' not in worker,
    'worker subtraction bounds': 'bytes.byteLength <= memorySize - ptr' in worker,
    'context free clamps source': 'venom_tjs_bounded_size(g_script_buffer_size, SOURCE_CAP)' in runtime,
    'allocation clears prior source': ('venom_tjs_bounded_size(g_script_buffer_size, SOURCE_CAP)' in runtime and 'g_script_buffer_expected_hash = 0u;' in runtime),
    'compatibility free is write-free': 'Compatibility-only release.' in runtime and 'if (g_active_script_ctx == ctx && g_script_buffer_size) venom_tjs_secure_zero' not in runtime,
    'free resets expected hash': 'g_script_buffer_expected_hash = 0u;' in runtime,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('TurboJS runtime memory lifecycle checks failed: ' + ', '.join(failed))
print('TurboJS runtime memory lifecycle smoke: ok')
