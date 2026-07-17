# Production Hardening

> **Applies to:** Venom 1.0.1

A production build must use the `prod` profile and pass the release verification pipeline. Development output is not a production substitute even though it uses the real QuickJS/WASM execution path.

## Required controls

- install and verify the locked JavaScript hardener;
- use the verified embedded QuickJS/WASM runtime;
- build the entire distribution in one invocation;
- require fail-closed execution with no host-JavaScript fallback;
- verify package, loader, worker, runtime, stylesheet, and WASM bindings;
- run leakage scans and runtime provenance checks;
- qualify routes, browser APIs, modules, and protected exports;
- sign stable release metadata;
- preserve release evidence.

## Application boundary

Protect algorithms and business rules that operate on plain data. Keep DOM rendering, framework lifecycle, browser handles, and compatibility-sensitive APIs in the browser realm. Return only the minimum information needed by the UI.

## Deployment

Serve the generated files over HTTPS. Apply a restrictive Content Security Policy appropriate to the application and generated asset set. Use immutable caching for hashed assets and deploy the complete distribution atomically.

## Verification commands

```powershell
venom analyze-dist dist
venom verify-runtime dist
venom release-check dist
```

For the complete release pipeline, use [Production release verification](../operations/release-verification.md).

## Prohibited shortcuts

Do not ship source maps, readable protected modules, debug manifests, internal engineering reports, development runtime files, unsigned stable metadata, or a distribution assembled from files produced by different builds.
