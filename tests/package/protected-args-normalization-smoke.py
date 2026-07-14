from pathlib import Path

root = Path(__file__).resolve().parents[2]
loader = (root / "src/compiler/pipeline/js.cpp").read_text(encoding="utf-8")
worker = (root / "src/generated/runtime/worker_runtime_js.cpp").read_text(encoding="utf-8")

assert "function encodeJson(value,label)" in loader
assert "Array.isArray(args)?args:[]" in loader
assert "JSON.parse(encoded)" in loader
assert "error.code='INVALID_ARGUMENTS'" in loader
assert "!Array.isArray(args)" in worker
assert "args.length > MAX_BRIDGE_ARGUMENTS" in worker
assert "bridge arguments violate json-value-v1" in worker
print("protected argument normalization smoke: PASS")
