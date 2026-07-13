from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = (root / "src/compiler/js.cpp").read_text(encoding="utf-8")
assert "function normalizeProtectedArgs(args)" in source
assert "JSON.stringify(Array.isArray(args)?args:[])" in source
assert "JSON.parse(encoded)" in source
assert "invalid-protected-arguments" in source
print("protected argument normalization smoke: PASS")
