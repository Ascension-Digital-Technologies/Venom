# Venom 3.0.0 Release Notes

**Release:** Venom 3.0.0  
**Platforms:** Windows x86-64 and Linux x86-64  
**Release date:** July 28, 2026

Venom 3.0.0 is the first major release built around the complete protected TurboJS/WASM production pipeline. It combines native compilation, encrypted VBC packaging, process-isolated JavaScript hardening, framework integration, typed runtime SDKs, fail-closed browser execution, and release verification in one supported toolchain.

## Highlights

### Protected TurboJS/WASM production pipeline

Venom compiles protected application logic into verified TurboJS execution records, packages those records into encrypted and compressed VBC sections, and loads them through integrity-bound browser runtime assets. Production builds deny readable host-JavaScript fallback and stop when required runtime, package, ABI, or integrity contracts do not match.

### Process-isolated JavaScript hardening

Heavy JavaScript hardening now runs in the companion `venom_hardener_worker` process. A hardener failure cannot terminate the main compiler process. Venom validates the worker protocol, retries one failed invocation, records private telemetry, and fails closed when safe hardening cannot be completed.

Both executables are mandatory parts of a binary release:

```text
bin/
├── venom[.exe]
└── venom_hardener_worker[.exe]
```

Keep them together in the same directory. The worker is an internal companion executable and is not intended to be invoked directly by users.

### React, Vite, TypeScript, and TSX

Version 3.0.0 adds supported production ingestion of Vite output, React integration through `@venom/react`, stable `useSyncExternalStore` runtime snapshots, TypeScript and TSX frontend handling, CSS asset discovery, and protected framework examples.

Vite applications are compiled by Vite first. Venom then protects the completed `dist` directory rather than scanning the project root, source tree, or `node_modules`.

### Hardened anonymous production layout

Production distributions use content-addressed, role-neutral filenames:

```text
dist/
├── index.html
└── assets/
    ├── app/          # VBC, lazy protected chunks, and CSS
    ├── javascript/   # loader, runtime, engine, and worker JavaScript
    ├── wasm/         # TurboJS/WASM runtime
    └── images/       # reachable hashed images
```

The old `assets/loader`, `assets/runtime`, `assets/style`, and `assets/workers` directories are not part of the 3.0.0 contract.

### Private diagnostics outside public distributions

Internal build reports are written to a sibling private directory:

```text
<project>/.venom/<output-name>/
```

This includes provenance, execution plans, bridge reports, API contracts, performance data, and `build.json`. These files are deliberately excluded from the public `dist` directory.

### Production asset security boundary

The package planner now consumes only assets approved by the reachable public-asset graph. Private, secret, repository, dependency, cache, previous-output, key, database, backup, log, lock, and source-map paths are rejected from production asset packaging.

### Release verification and fail-closed behavior

Venom 3.0.0 verifies package structure, section authentication, runtime hashes, ABI fingerprints, release policy, public leakage, generated artifacts, and real TurboJS/WASM execution. Invalid or incomplete release inputs stop the build or verification process rather than silently downgrading protection.

## Framework and runtime fixes included

- Fixed Vite side-effect CSS imports being parsed as JavaScript.
- Fixed Vite launchers to protect compiled frontend output instead of project roots.
- Removed an invalid embedded hardener brace that caused `expecting catch or finally`.
- Fixed React error #185 by returning referentially stable runtime status snapshots.
- Removed unsupported top-level `await` from the Vite framework showcase while preserving broad browser targets.
- Removed document-level VBC preload hints that workers could not reliably consume.
- Added Windows resolution for `npm.cmd` and other command shims without enabling shell execution.
- Prevented duplicate Venom invocation during Vite `writeBundle` processing.

## Release contents

### Windows x86-64

```text
venom-3.0.0-windows-x86_64/
├── bin/
│   ├── venom.exe
│   └── venom_hardener_worker.exe
├── LICENSE
├── NOTICE.md
├── README.txt
├── RELEASE_NOTES.md
└── SHA256SUMS
```

### Linux x86-64

```text
venom-3.0.0-linux-x86_64/
├── bin/
│   ├── venom
│   └── venom_hardener_worker
├── LICENSE
├── NOTICE.md
├── README.txt
├── RELEASE_NOTES.md
└── SHA256SUMS
```

## Installation

Extract the platform archive and keep both files from `bin/` together. Add that `bin` directory to `PATH`, or invoke `venom` by its full path.

Verify the installation:

```text
venom --version
venom doctor --profile production
```

On Linux, preserve executable permissions when extracting the `.tar.gz` archive.

## Upgrade notes

- Binary deployments must install the new hardener worker alongside the main compiler.
- Do not copy only `venom.exe` or `venom`; protected builds may fail when the worker cannot be located.
- Automation that reads internal reports must use `.venom/<output-name>/` rather than public `dist` paths.
- Tools that assume role-named asset directories must be updated to the 3.0.0 content-addressed layout.
- Vite integrations should pass completed Vite output to Venom, not raw framework source.

## Compatibility

The supplied binary packages target Windows x86-64 and Linux x86-64. Protected web output is intended for modern browsers with WebAssembly, Web Workers, ES modules, SubtleCrypto, and the runtime capabilities required by the selected build.

## Security note

Venom raises the cost of static inspection, source recovery, modification, and reuse, but no client-delivered protection can make browser code impossible to observe. A determined analyst who controls the browser can instrument execution. Applications must continue to enforce authorization, secrets, and trust-sensitive decisions on systems the user does not control.
