from pathlib import Path
root=Path(__file__).resolve().parents[2]
engine=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
assert "const ephemeralContext = !protectedRelease && !!chunk.bridgeCandidate" in engine
assert "if (!ephemeralContext) contexts.set(context.key, mutableContext)" in engine
assert "e.venom_qjs_context_free(mutableContext.wasmContext" in engine
print('JavaScript playground disposable QuickJS context: PASS')
