# Hardened dist analysis — v0.88.0

v0.88.0 adds a local diagnostic tool for hardened/browser output folders:

```bash
scripts/analyze-dist.sh dist-hardened --venom build/venom --out build/dist-analysis.json
```

Windows:

```bat
analyze-dist.bat dist-hardened --venom build\venom.exe --out build\dist-analysis.json
```

The analyzer reports:

- emitted files, sizes, and SHA-256 hashes
- loader package/engine/WASM references
- `venom verify-runtime` result
- strict `verify-runtime --require-real-engine` result
- QuickJS WASM export coverage
- QuickJS WASM runtime-value truth checks

The strict runtime-value check exists because export names alone are not enough. A stale `.wasm` can expose the ABI12 symbol names while still returning an older `venom_qjs_engine_version`.
