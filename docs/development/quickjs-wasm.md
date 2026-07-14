# QuickJS/WASM development

Runtime contributors use the canonical Emscripten controller:

```powershell
python tools/build_emscripten.py --preflight-only
python tools/build_emscripten.py --out-dir build/quickjs-wasm --clean --force
```

The controller owns toolchain discovery, compilation, ABI/export verification, provenance, optimization, embedding, and lifecycle state. Do not manually replace the embedded blob without running strict verification.
