# Production artifact layout — v1.0.1

All website builds emit one stable root document and immutable hashed production assets:

```text
dist/
  index.html
  assets/
    app/
      app.<hash>.vbc
    style/
      s.<hash>.css
    loader/
      loader.<hash>.js
    runtime/
      engine.<hash>.js
      r.<hash>.js
      runtime.<hash>.wasm
      rw.<hash>.wasm
    workers/
      worker.<hash>.js
```

Route HTML shells, `sw.js`, `favicon.ico`, source maps, debug manifests, unhashed app packages, and flat runtime internals are not emitted. The compiler no longer emits a development compatibility layout; every website build uses this production layout.

`tools/scan_protected_dist.py <dist>` is the fail-closed structural gate. `tools/analyze_dist.py <dist> --strict` is the broader release diagnostic gate: it verifies the grouped layout, loader references, forbidden source-evaluation/debug markers, package runtime metadata, strict real-engine status, and QuickJS WASM runtime truth values.

## Integrity chain

`index.html` pins the loader and stylesheet with SHA-256 Subresource Integrity. The protected package binds runtime JS, runtime WASM, CSS, QuickJS engine JS, QuickJS WASM, and worker JS by SHA-256. `verify-runtime` rejects missing or changed bound assets and rejects SRI mismatches.
## Packaged remote scripts

Static HTTPS script dependencies are downloaded during the build and compiled into `assets/app/app.<hash>.vbc`; they do not add public files or change the nine-file production layout. Runtime network-script chunks are rejected by production verification.

