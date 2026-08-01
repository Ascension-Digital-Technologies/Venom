# Venom 3.5.0 Release Notes

**Release:** Venom 3.5.0  
**Status:** Production release

Venom 3.5.0 focuses on resilience, release integrity, and maintainability. It keeps the protected TurboJS/WebAssembly architecture introduced in 3.0 while hardening the native JavaScript pipeline, closing additional Chrome extension and package-verification gaps, improving repeated collection operations, and making the public repository easier to build and contribute to.

## Highlights

### Isolated, deterministic JavaScript hardening

Heavy loader, worker, runtime, engine, and large-asset hardening now runs through isolated TurboJS realms in the mandatory `venom_hardener_worker` companion process. Conservative browser-no-eval profiles avoid seed-sensitive native failures while retaining minification, mangling, encoded string arrays, identifier transformation, and supported control-flow hardening.

The main compiler validates the worker response envelope, retries a failed invocation once where safe, and fails closed when the worker is missing or malformed.

### Race-safe hardener cache

Concurrent Venom builds no longer publish through one shared temporary cache path. Each publisher uses a unique same-directory temporary file and atomically installs the completed cache entry. When another process wins the race, Venom validates and reuses the published entry. Cache race information remains in private `.venom/` reports rather than public distributions.

### Protected Chrome extension routes

Manifest V3 release verification can discover integrity-bound protected pages by manifest route rather than relying on role-named filenames. This covers popup, options, side-panel, and similar extension pages while preserving anonymous content-addressed runtime assets. Engine-page generation also avoids offscreen-only host references.

### Stronger release and package boundaries

Venom 3.5.0 expands fail-closed checks for unsafe archive entries, traversal, links and special files, non-portable or duplicate manifest paths, malformed VBC section names, sensitive project assets, stale ABI metadata, source maps, readable internal names, and public engineering reports.

### Runtime and compiler improvements

Repeated stable string keys used in TurboJS `Map` and `Set` operations cache their content hashes, reducing repeated hashing work without changing key identity semantics. Native benchmarks and regressions cover string and object keys, dynamic equal strings, Set membership, and later atomization.

The source tree now enforces first-party domains, colocated headers, explicit generated-code ownership, canonical TurboJS/WASM paths, semantic example launchers, and acyclic cross-domain dependencies.

## Installation

Binary packages contain two required executables that must remain together:

```text
bin/
├── venom[.exe]
└── venom_hardener_worker[.exe]
```

Verify an installation with:

```text
venom --version
venom doctor --profile production
```

## Upgrade notes

- Keep `venom` and `venom_hardener_worker` from the same 3.5.0 package together.
- Regenerate protected distributions; do not mix 3.5.0 binaries with old generated runtime assets or package metadata.
- Update `@venom/runtime`, `@venom/react`, `@venom/vite`, and `@venom/venom` together where those packages are used.
- Re-run production qualification and browser certification after upgrading.
- Tools that inspect public role-named runtime directories remain unsupported; production output continues to use anonymous content-addressed assets.

## Verification

A release candidate should pass the strict native build, CTest suite, generated-artifact checks, contract validation, package verification, runtime ABI checks, protected example certification, and supported browser qualification described in the release-engineering documentation.

See [README.md](README.md), [SECURITY.md](SECURITY.md), and [docs/operations/release-verification.md](docs/operations/release-verification.md) for the supported workflow and security position.
