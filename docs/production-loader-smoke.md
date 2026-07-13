# v0.98.0 production loader smoke

The production loader smoke verifies the public browser entry path instead of bypassing it.

The harness imports the emitted:

```text
assets/loader/loader.<hash>.js
```

and checks that the loader resolves:

```text
../app/app.<hash>.vbc
../style/s.<hash>.css
../runtime/r.<hash>.js
../runtime/engine.<hash>.js
../runtime/runtime.<hash>.wasm
../runtime/rw.<hash>.wasm
../workers/worker.<hash>.js
```

The test also simulates the versioned release-worker protocol, validates the package hash and QuickJS WASM SHA-256 attestation, compiles the QuickJS WASM module, delivers the package bytes to the runtime, verifies package binding token delivery, and boots with `Function`/`eval` blocked. This catches regressions that direct `bootVenom(...)` tests can miss, especially incorrect grouped-layout URLs.

Run it through CTest:

```bash
ctest --test-dir build -R venom_production_loader_smoke --output-on-failure
```

## Large-site and vendored-script regression

`venom_production_site_boot_smoke` creates a production site with a DOM payload larger than the former companion-runtime capacity, a local `fpCollect.min.js` that declares `const fpCollect`, and a static CDN script supplied through the deterministic vendor cache. It verifies that:

- the companion WASM runtime does not return status `14`;
- injected host binding names do not collide with local declarations;
- the CDN script becomes protected package bytecode rather than a runtime URL chunk;
- the public loader boots with source evaluation disabled.

```bash
ctest --test-dir build -R venom_production_site_boot_smoke --output-on-failure
```
