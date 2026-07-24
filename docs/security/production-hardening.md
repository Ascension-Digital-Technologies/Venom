# Production Hardening

> **Applies to:** Venom 2.0.0

A production build must use the `prod` profile and pass the release verification pipeline. Development output is not a production substitute even though it uses the real TurboJS/WASM execution path.

## Required controls

- install and verify the locked JavaScript hardener;
- use the verified embedded TurboJS/WASM runtime;
- build the entire distribution in one invocation;
- require fail-closed execution with no host-JavaScript fallback;
- verify package, loader, worker, runtime, stylesheet, and WASM bindings;
- run leakage scans and runtime provenance checks;
- qualify routes, browser APIs, modules, and protected exports;
- sign stable release metadata;
- preserve release evidence.

Production asset emission is allowlist-driven. Venom follows assets referenced by
HTML routes, their linked stylesheets, and the planned JavaScript/module graph.
Unreferenced input files are not copied or embedded merely because they exist in
the project directory. After emission, the compiler reopens `dist/` and requires
the file set to match the build plan exactly; unexpected files, symlinks, missing
files, and unsupported filesystem entries fail the build and remove the partial
distribution.

## Application boundary

Protect algorithms and business rules that operate on plain data. Keep DOM rendering, framework lifecycle, browser handles, and compatibility-sensitive APIs in the browser runtime. Return only the minimum information needed by the UI.

## Deployment

Serve the generated files over HTTPS. Apply a restrictive Content Security Policy appropriate to the application and generated asset set. Use immutable caching for hashed assets and deploy the complete distribution atomically.

## Verification commands

```powershell
venom analyze dist
venom verify-runtime dist
venom verify dist
```

For the complete release pipeline, use [Production release verification](../operations/release-verification.md).

## Prohibited shortcuts

Do not ship source maps, readable protected modules, debug manifests, internal engineering reports, development runtime files, unsigned stable metadata, unreferenced project files, or a distribution assembled from files produced by different builds.
