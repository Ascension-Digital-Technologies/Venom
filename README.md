# Venom Secure Web Runtime

Venom compiles supported websites into a compact, integrity-bound distribution that keeps browser-facing code compatible while moving selected JavaScript logic into a protected QuickJS/WebAssembly runtime.

The project is designed to increase the cost of static source recovery and unauthorized modification. It is not a substitute for server-side authorization, secret storage, or trusted execution.

> **Version:** 1.34.1  
> **Status:** active development and controlled production evaluation  
> **Flagship example:** [`examples/protected-chess`](examples/protected-chess)

## Why Venom

Ordinary web deployments expose JavaScript source directly. Venom provides a hybrid model:

- browser-native code remains in the browser for DOM, events, rendering, and framework compatibility;
- explicitly protected logic is extracted and compiled to QuickJS bytecode;
- QuickJS executes inside WebAssembly in a dedicated worker;
- browser and protected code communicate through a narrow asynchronous JSON-value bridge;
- generated loader, worker, engine, and runtime JavaScript is aggressively hardened;
- protected package decoding is owned by WebAssembly rather than a readable JavaScript fallback;
- package layout, bridge identifiers, opcode mappings, and section order vary between builds;
- production builds fail closed when the real protected runtime is unavailable.

## Architecture

```text
Browser UI
   │
   │ venom.exports.<name>(plainData)
   ▼
Generated public bridge
   │  session-bound, bounded request/response protocol
   ▼
Dedicated worker
   │
   ▼
QuickJS/WASM protected runtime
   │
   ▼
Compiled protected exports
```

A second WebAssembly runtime owns package parsing, authenticated section lookup, decoding, and route/DOM program execution.

See [`docs/architecture.md`](docs/architecture.md) for the complete design.

## Protected logic

### Protected function in a browser file

```javascript
// @venom: browser

// @venom: protected isolated
function calculatePrice(input) {
  return {
    total: input.quantity * input.unitPrice
  };
}
```

### Browser call

```javascript
await venom.ready();

const result = await venom.exports.calculatePrice({
  quantity: 4,
  unitPrice: 19.95
});
```

Protected calls accept only JSON values: `null`, booleans, finite numbers, strings, arrays, and plain objects. Functions, DOM values, custom class instances, cyclic values, `BigInt`, and other non-JSON values are rejected.

See [`docs/PROTECTED-BRIDGE.md`](docs/PROTECTED-BRIDGE.md).

## Distribution layout

A protected build emits:

```text
dist/
├── index.html
└── assets/
    ├── app/
    │   └── app.<hash>.vbc
    ├── images/
    ├── loader/
    │   └── loader.<hash>.js
    ├── runtime/
    │   ├── engine.<hash>.js
    │   ├── r.<hash>.js
    │   ├── runtime.<hash>.wasm
    │   └── rw.<hash>.wasm
    ├── style/
    │   └── s.<hash>.css
    └── workers/
        └── worker.<hash>.js
```

`index.html` remains stable. Generated assets are content-addressed and bound to the same build. The loader and stylesheet use Subresource Integrity.

## Requirements

- CMake 3.24 or newer
- A C++20 compiler
- Python 3.10 or newer
- Node.js for the release JavaScript hardener
- Emscripten when rebuilding WebAssembly runtimes

Windows development is tested with MSVC and PowerShell. Linux/macOS use the equivalent shell scripts.

## Build Venom

Windows:

```powershell
.\scripts\build.ps1 -Config Release
```

Linux/macOS:

```bash
./scripts/build.sh Release
```

The executable is produced under `build/` according to the selected generator and configuration.

## Build the protected chess example

Windows:

```powershell
.\scripts\setup-js-hardener.ps1
.\scripts\build-site.ps1 -Site examples\protected-chess -Dist dist
.\scripts\serve-site.ps1 -Dist dist -Port 8080
```

Linux/macOS:

```bash
./scripts/setup-js-hardener.sh
./scripts/build-site.sh examples/protected-chess dist
./scripts/serve-site.sh 8080 dist
```

Then open `http://127.0.0.1:8080`.

The chess example demonstrates browser-native UI and game orchestration with protected evaluation, minimax, alpha-beta pruning, and move selection. Read its [standalone guide](examples/protected-chess/README.md).

## Direct CLI use

```text
venom build <site> --out <dist> --profile browser-protect --hashed
venom inspect <dist>
venom verify-runtime <dist> --require-real-engine
```

Use `venom --help` for the authoritative command list.

## Build profiles

- `debug` — readable diagnostics and development behavior.
- `browser-protect` — protected QuickJS/WASM execution with browser compatibility boundaries.
- `native-protect` — native protected build workflow where supported.
- `maximum` — strongest available release settings and validation.

Protected release profiles deny host-JavaScript fallback.

## Runtime and release hardening

Current protected builds include:

- real QuickJS compiled to WebAssembly;
- stripped QuickJS bytecode without protected source text;
- explicit protected exports and a bounded JSON-value bridge;
- numeric export slots and per-worker session binding;
- replay and stale-envelope rejection;
- per-build opaque bridge operation identifiers;
- compact production WebAssembly ABI names;
- WASM-owned package section decoding;
- full package-section order and offset diversification;
- content hashes, loader SRI, and package/runtime binding;
- aggressive AST-aware JavaScript hardening;
- production metadata and decoder leak scanning;
- fail-closed runtime and release verification.

These measures raise reverse-engineering cost but cannot create absolute confidentiality on an attacker-controlled client.

## Validation

Run the complete test suite:

```powershell
.\scripts\test.ps1 -Config Release
```

Inspect a generated distribution:

```powershell
.\scripts\analyze-dist.ps1 -Dist dist
python .\scripts\check-production-leaks.py dist
```

Assess release readiness:

```powershell
.\scripts\readiness.ps1 -Site examples\protected-chess -Dist dist
```

Canonical scripts are documented in [`scripts/README.md`](scripts/README.md). Test-only websites live under `tests/fixtures/sites`; `examples/` intentionally contains only the flagship protected chess application.

## Repository layout

```text
cmake/             CMake modules and test registration
contracts/         generated/runtime contract definitions
docs/              current architecture, security, and release documentation
examples/          supported public examples (protected chess only)
fuzz/              fuzzing targets and corpora
scripts/           stable developer and release entry points
src/               compiler, package, runtime, and CLI sources
tests/             unit, integration, package, and fixture tests
third_party/       pinned third-party source dependencies
tools/             Python implementation tools used by scripts and CI
```

See [`docs/source-layout.md`](docs/source-layout.md).

## Security expectations

Venom protects client-delivered implementation details; it does not provide a trusted client.

Assume a determined operator can:

- inspect and modify their local loader and worker;
- observe bridge requests and responses;
- instrument WebAssembly memory and imports;
- query protected exports as black-box oracles;
- reproduce enough runtime behavior with sufficient effort.

Never place permanent credentials, signing keys, backend authorization decisions, or irreplaceable secrets in protected browser code.

Read [`SECURITY.md`](SECURITY.md), [`docs/security-model.md`](docs/security-model.md), and [`docs/threat-model.md`](docs/threat-model.md).

## Documentation

- [Installation](docs/installation.md)
- [Configuration](docs/configuration.md)
- [Architecture](docs/architecture.md)
- [Protected bridge](docs/PROTECTED-BRIDGE.md)
- [Package format](docs/package-format.md)
- [Production artifact layout](docs/production-artifact-layout.md)
- [Release profiles](docs/release-profiles.md)
- [Release packaging](docs/release-packaging.md)
- [Release signing](docs/release-signing.md)
- [Runtime performance](docs/runtime-performance.md)
- [Compatibility](docs/compatibility.md)
- [Security model](docs/security-model.md)
- [Threat model](docs/threat-model.md)

## Contributing

Read [`CONTRIBUTING.md`](CONTRIBUTING.md). Keep generated artifacts, build directories, historical release bundles, and compatibility fixture sites out of the public example surface.

## License

See [`LICENSE`](LICENSE). Third-party components remain subject to their own licenses and notices.
