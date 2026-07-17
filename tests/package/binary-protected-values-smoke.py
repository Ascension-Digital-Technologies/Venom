from pathlib import Path
root=Path(__file__).resolve().parents[2]
loader=(root/'src/compiler/pipeline/js.cpp').read_text()
worker=(root/'src/generated/runtime/worker_runtime_js.cpp').read_text()
rewrite=(root/'src/compiler/pipeline/js_rewriting.cpp').read_text()
assert "binary-json-v2" in loader
assert "encodePayload" in loader and "decodePayload" in loader
assert "encodeBridgePayload" in worker and "decodeBridgePayload" in worker
assert "__venomBridgeRevive" in rewrite and "__venomBridgeEncode" in rewrite
assert "SharedArrayBuffer" in (root/'docs/guides/binary-protected-values.md').read_text()
print("binary protected values smoke: PASS")
