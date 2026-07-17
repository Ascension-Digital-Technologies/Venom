#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
js = (root / "src/compiler/pipeline/js.cpp").read_text(encoding="utf-8")
hardener = (root / "src/compiler/pipeline/native_js_hardener.cpp").read_text(encoding="utf-8")

for token in (
    'native_js_hardener::harden(',
    '"protected-module"',
    '"protected-script"',
    '"protected-registry"',
    '"protected-registry-chunk"',
    'opaque_bytecode_label(',
    'development ? chunk.source',
):
    assert token in js, f"missing pre-bytecode hardening token: {token}"

for token in (
    "const protectedKind = kind === 'protected-script' || kind === 'protected-module'",
    'renameGlobals: protectedKind',
    'stringArrayThreshold: protectedKind ? 0.86',
    'controlFlowFlatteningThreshold: protectedKind ? 0.34',
    'deadCodeInjectionThreshold: protectedKind ? 0.045',
):
    assert token in hardener, f"missing protected hardener policy token: {token}"

print("pre-bytecode source hardening smoke: PASS")
