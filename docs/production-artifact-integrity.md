# v0.98.0 production artifact integrity

The production output uses three complementary integrity boundaries.

1. `index.html` pins `assets/loader/loader.<hash>.js` and `assets/style/s.<hash>.css` with SHA-256 Subresource Integrity.
2. `package-binding.vbind` binds six emitted assets by SHA-256: runtime JS, companion runtime WASM, stylesheet, QuickJS engine JS, QuickJS WASM, and worker JS.
3. The versioned worker protocol independently validates the package FNV-1a allowlist hash and QuickJS WASM SHA-256 digest before returning package bytes and the compiled WebAssembly module.

The worker also enforces same-origin URLs, redirect denial, encoded-size ceilings, a 32-byte nonce, and protocol version 1. The loader validates the response nonce, protocol, package hash, and WASM digest before invoking `bootVenom`.

Run the tamper gate with:

```bash
ctest --test-dir build -R venom_production_tamper_gate_smoke --output-on-failure
```

The test proves fail-closed rejection for modified package bytes, QuickJS WASM, runtime WASM, worker JavaScript, loader JavaScript, and stylesheet content.
## Build-time dependency integrity

Remote vendor records bind normalized source URLs to the exact downloaded script digest. Standard SHA-256, SHA-384, and SHA-512 SRI declarations are verified before compilation. The protected package records a zero-unvendored-remote requirement, and release verification fails if any runtime remote-script URL chunk remains.

