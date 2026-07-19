#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
js = (root / "src/pipeline/js.cpp").read_text(encoding="utf-8")
encoding = (root / "src/pipeline/js_bundle_encoding.cpp").read_text(encoding="utf-8")
combined = js + "\n" + encoding
hardener = (root / "src/pipeline/native_js_hardener.cpp").read_text(encoding="utf-8")

for token in (
    'native_js_hardener::harden(',
    '"protected-module"',
    '"protected-script"',
    '"protected-registry"',
    '"protected-registry-chunk"',
    'opaque_bytecode_label(',
    'module->runtime_source',
):
    assert token in combined, f"missing pre-bytecode hardening token: {token}"

for token in (
    "const protectedKind = kind === 'protected-script' || kind === 'protected-module'",
    "const bytecodeKind = kind.startsWith('protected-')",
    'stringArray: !bytecodeKind',
    'controlFlowFlattening: !bytecodeKind',
    'deadCodeInjection: !bytecodeKind',
    'selfDefending: !bytecodeKind',
):
    assert token in hardener, f"missing protected hardener policy token: {token}"

print("pre-bytecode source hardening smoke: PASS")
