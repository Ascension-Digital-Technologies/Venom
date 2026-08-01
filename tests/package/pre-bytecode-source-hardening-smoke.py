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
# emitted as JavaScript, but the compiled record still stores an atom table of
# the source's own identifiers. They must be routed through the "protected-*"
# hardener path -- which native_js_hardener.cpp resolves to a javascript-obfuscator
# identifier-renaming pass (plus numbersToExpressions/simplify/transformObjectKeys),
# not just Terser mangling -- before compiling to stripped native records.
# Skipping the hardener call entirely (rather than just the obfuscator step)
# leaves the author's original identifiers readable in the shipped bytecode.
# Note: stringArray, deadCodeInjection, controlFlowFlattening, and selfDefending
# are deliberately NOT applied to protected-* content even though they are
# available for other kinds -- each measured a large (5x-35x) constant-factor
# runtime slowdown against a real deeply-recursive protected search function,
# and they protect JavaScript *source text* from a human reader, a threat
# model that mostly does not apply here since this output is immediately
# compiled to source-free bytecode rather than ever shipping as JS text.
assert 'harden_registry_source(' in js
assert 'compile_native_turbojs_bytecode(\n        hardened_registry_source' in js
assert 'compile_native_turbojs_bytecode(\n        hardened_chunk_source' in js
assert 'native_js_hardener::harden("protected-script", source, seed)' in js
assert 'ast_harden_release_js("protected-registry"' not in js
assert 'ast_harden_release_js("protected-registry-chunk"' not in js

# Explicit browser assets and large isolated units must bypass protected frontend
# paths that are neither required nor safe for minified/deep source trees.
assert '(chunk.flags & JsChunkBrowser) != 0u) continue;' in discovery
assert 'record.isolated && dependency_declaration.size() >= 16384u' in rewriting

for token in (
    "const protectedKind = kind === 'protected-script' || kind === 'protected-module'",
    # protected-* content goes through javascript-obfuscator (no more blanket
    # skip) for identifier renaming, but the expensive per-access/per-call
    # transforms are explicitly excluded for it -- see the comment above.
    "stringArray: !browserFacingKind && !protectedKind",
    "controlFlowFlattening: !browserFacingKind && !protectedKind",
    "deadCodeInjection: !browserFacingKind && !protectedKind",
    "selfDefending: kind !== 'worker' && !browserFacingKind && !protectedKind",
    # numbersToExpressions/simplify/transformObjectKeys measured cheap and
    # stay enabled for protectedKind (no !protectedKind exclusion).
    "numbersToExpressions: kind !== 'worker' && !browserFacingKind",
    # The isolated-subprocess safety net (see requires_worker_isolation)
    # still applies to protected-script/protected-module regardless of size,
    # independent of which obfuscator transforms are enabled.
    'kind == "protected-script" || kind == "protected-module" ||',
):
    assert token in hardener, f"missing protected hardener policy token: {token}"

print("pre-bytecode source hardening smoke: PASS")
