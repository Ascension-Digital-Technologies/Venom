#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
js = (root / "src/pipeline/js.cpp").read_text(encoding="utf-8")
encoding = (root / "src/pipeline/js_bundle_encoding.cpp").read_text(encoding="utf-8")
rewriting = (root / "src/pipeline/js_rewriting.cpp").read_text(encoding="utf-8")
discovery = (root / "src/pipeline/js_discovery.cpp").read_text(encoding="utf-8")
combined = js + "\n" + encoding
hardener = (root / "src/pipeline/native_js_hardener.cpp").read_text(encoding="utf-8")

for token in (
    'native_js_hardener::harden(',
    '"protected-module"',
    '"protected-script"',
    'opaque_bytecode_label(',
    'module->runtime_source',
):
    assert token in combined, f"missing pre-bytecode hardening token: {token}"

# Protected bridge registries are package-private compiler inputs and are never
# emitted as JavaScript. They must compile directly to stripped native records,
# avoiding the embedded obfuscator path that can fault on Windows.
assert 'compile_native_turbojs_bytecode(\n        rewrite_result.registry_source' in js
assert 'compile_native_turbojs_bytecode(\n        source_chunk.source' in js
assert 'ast_harden_release_js("protected-registry"' not in js
assert 'ast_harden_release_js("protected-registry-chunk"' not in js

# Explicit browser assets and large isolated units must bypass protected frontend
# paths that are neither required nor safe for minified/deep source trees.
assert '(chunk.flags & JsChunkBrowser) != 0u) continue;' in discovery
assert 'record.isolated && dependency_declaration.size() >= 16384u' in rewriting

for token in (
    "const protectedKind = kind === 'protected-script' || kind === 'protected-module'",
    "const bytecodeKind = kind.startsWith('protected-')",
    'if (!bytecodeKind)',
):
    assert token in hardener, f"missing protected hardener policy token: {token}"

print("pre-bytecode source hardening smoke: PASS")
