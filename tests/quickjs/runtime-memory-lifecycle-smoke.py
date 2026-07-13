#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
engine = (root / 'src/compiler/quickjs_engine_module.cpp').read_text(encoding='utf-8')
runtime = (root / 'src/runtime/quickjs_runtime_scaffold.c').read_text(encoding='utf-8')
checks = {
    'serialized wasm execution': 'let wasmExecutionQueue = Promise.resolve();' in engine and 'enqueueWasmExecution(() => executeWithWasmScaffold' in engine,
    'main buffer finally free': "if (ptr && typeof e.venom_qjs_script_buffer_free === 'function')" in engine,
    'diversification buffer free': 'e.venom_qjs_script_buffer_free(mutableContext.wasmContext >>> 0, contractPtr >>> 0);' in engine,
    'context free zeroes source': 'if (g_script_buffer_size) venom_qjs_secure_zero(g_source, g_script_buffer_size);' in runtime,
    'allocation clears prior source': ('if (g_script_buffer_size) venom_qjs_secure_zero(g_source, g_script_buffer_size);' in runtime and 'g_script_buffer_expected_hash = 0u;' in runtime),
    'free resets expected hash': 'g_script_buffer_expected_hash = 0u;' in runtime,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('QuickJS runtime memory lifecycle checks failed: ' + ', '.join(failed))
print('QuickJS runtime memory lifecycle smoke: ok')
