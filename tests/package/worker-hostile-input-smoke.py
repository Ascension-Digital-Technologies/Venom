from pathlib import Path
root = Path(__file__).resolve().parents[2]
js = (root / 'src/pipeline/js.cpp').read_text()
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
runtime = (root / 'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
checks = {
    'worker nonce rejection': 'm.nonce !== nonce' in js,
    'worker protocol version': 'WORKER_PROTOCOL = 1' in js and 'm.protocol !== WORKER_PROTOCOL' in js,
    'same-origin worker fetch': "parsed.origin !== self.location.origin" in js,
    'package attestation': 'package hash attestation mismatch' in js,
    'QuickJS digest attestation': 'QuickJS WASM digest attestation mismatch' in js,
    'worker control size ceiling': '1024' in js,
    'worker startup deadline': '15000' in js,
    'package encoded ceiling': '67108864' in js,
    'wasm encoded ceiling': '33554432' in js,
    'redirect denial': "redirect: 'error'" in js,
    'queue disposal state': 'disposed' in runtime,
    'fetch abort ownership': 'AbortController' in runtime,
    'bounded pending operations': '256' in runtime,
    'bounded fetch response': '1048576' in runtime,
}
missing = [name for name, ok in checks.items() if not ok]
if missing:
    raise SystemExit('missing adversarial controls: ' + ', '.join(missing))
print('v1.0.1 hostile worker/host boundary smoke: PASS')
