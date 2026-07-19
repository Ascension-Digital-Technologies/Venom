from pathlib import Path
root = Path(__file__).resolve().parents[2]
build = (root/'src/pipeline/build.cpp').read_text() + (root/'src/pipeline/build_package_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_audit_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_module_metadata.cpp').read_text()
js = (root/'src/pipeline/js.cpp').read_text()
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
worker = (root/'src/generated/runtime/worker_runtime_js.cpp').read_text()
bootstrap = js + '\n' + worker
qjs = (root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
runtime = (root/'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
required = [
    ('worker asset emission', 'make_worker_runtime_js(' in build and 'workers/' in build),
    ('module worker boot', "new Worker(bootOptions.workerUrl" in js and "type: 'module'" in js),
    ('nonce protocol', 'crypto.getRandomValues' in js and "m.nonce !== nonce" in js),
    ('versioned worker protocol', 'protocol: 1' in bootstrap and 'WORKER_PROTOCOL = 1' in bootstrap),
    ('package hash attestation', 'package hash attestation mismatch' in js),
    ('QuickJS digest attestation', 'QuickJS WASM digest attestation mismatch' in bootstrap),
    ('bounded package fetch', '67108864' in js),
    ('bounded wasm fetch', '33554432' in js),
    ('transfer package buffer', '[packageBytes]' in bootstrap),
    ('worker wasm compilation', 'WebAssembly.compile(wasmBytes)' in bootstrap),
    ('worker wasm instantiate probe', 'probeQuickJsModule' in bootstrap and 'WebAssembly.instantiate(module' in bootstrap),
    ('required QuickJS export presence gate', 'missingRequired' in worker and 'requiredKinds' in worker),
    ('approved Emscripten export allowlist', all(name in worker for name in ('__indirect_function_table', '_emscripten_stack_restore', 'emscripten_stack_get_current'))),
    ('unapproved QuickJS export rejection', 'unapprovedExtras' in worker),
    ('QuickJS export kind validation', 'kindMismatches' in worker),
    ('no brittle exact export-count gate', 'actual.length !== required.length' not in worker),
    ('worker runtime readiness attestation', 'quickJsRuntimeReady: true' in bootstrap and 'readiness attestation missing' in bootstrap),
    ('compiled module reuse', '__venomWorkerQuickJsModule' in qjs),
    ('supplied package bytes', 'options.packageBytes || null' in runtime),
]
missing=[name for name,ok in required if not ok]
if missing: raise SystemExit('missing: '+', '.join(missing))
print('v1.0.1 worker-owned engine isolation smoke: PASS')
