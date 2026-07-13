# Venom - Secure Web Runtime

**Venom is a protected web compiler and dual-engine browser runtime.** It transforms supported static and client-rendered websites into compact distributions containing polymorphic route/DOM bytecode, stripped native QuickJS bytecode, content-addressed runtime assets, and an integrity-checked application package.

Venom uses a 256-bit, domain-separated per-build diversification system for protection-oriented layout changes. Independent derived streams control Route VM opcode mappings, instruction layouts, masks, route and string ordering, package section ordering, and selected host-bridge wire identifiers. The master seed is never written to the distribution.

Protected runtime JavaScript is also specialized per build. Selected internal function identifiers are replaced with opaque build-specific names, and high-signal production diagnostics are mapped to compact build-specific error codes. Deterministic builds reproduce the same mappings; ordinary protected builds do not.

Venom uses two coordinated execution layers:

1. the **Venom Route VM**, a custom bytecode interpreter implemented in WebAssembly for route selection, packaged page reconstruction, and deterministic DOM construction; and
2. a **real QuickJS engine compiled to WebAssembly**, which reads and executes native QuickJS object bytecode for application JavaScript.

> **Current release:** 1.31.4  
> **Architecture:** Route VM + QuickJS/WASM execution layers  
> **Project status:** active development; appropriate for controlled production evaluation after the full release-closure and target-browser validation workflows pass.

QuickJS reaches supported browser functionality through Venom's versioned host bridge rather than unrestricted direct access to the browser environment. Venom is intended to reduce readable source exposure, resist casual static extraction, detect malformed or mixed-build deployments, and enforce fail-closed runtime policy while preserving ordinary static hosting.

Venom is not an absolute confidentiality, anti-tamper, or sandbox boundary. Browser-delivered package bytes, runtime code, keys, WebAssembly memory, and materialized data remain observable to a determined operator. Secrets and security-critical authorization must remain on trusted servers.

## What Venom does

A Venom build can:

- discover HTML routes, stylesheets, scripts, and local assets;
- preserve inline and external script ordering;
- vendor supported remote HTTPS scripts and pin them in `venom.lock`;
- compile route and deterministic DOM construction data into polymorphic Route VM bytecode;
- compile JavaScript with real QuickJS in compile-only mode and serialize native QuickJS object bytecode;
- strip QuickJS source and debug records from protected bytecode output;
- package routes, strings, assets, policies, bytecode, and integrity metadata into a `.vbc` application package;
- emit grouped, content-addressed loader, worker, runtime, WebAssembly, package, and stylesheet assets;
- reject malformed, stale, incomplete, or mixed-build runtime components;
- execute QuickJS bytecode without a production host-JavaScript or source-decode fallback;
- apply configured execution, memory, recursion, host-call, route, and event limits;
- provide compatibility scanning, package inspection, runtime verification, fuzzing, and reproducible release packaging tools.

A successful build does not guarantee complete browser-semantic compatibility. Applications must be tested through their critical user journeys in every supported target browser.

Venom includes a Playwright-based real-browser validator for generated distributions. It runs observable DOM, event, asynchronous, fetch, route, and error checks in Chromium, Firefox, and WebKit and binds its JSON report to the exact tested distribution digest. See [Real-browser validation](docs/browser-validation.md).

### Browser startup performance evidence

Browser validation now records browser launch, context creation, navigation, fixture readiness, action, and complete scenario timings. The separate budget evaluator applies broad CI regression ceilings without presenting them as universal latency guarantees:

```bash
python tools/browser_performance.py build/*browser-validation.json \
  --json-out build/browser-performance.json
```

See [Browser startup performance](docs/browser-performance.md).


## Signed compatibility evidence

Browser reports and the compatibility matrix can be packaged into a deterministic, optionally Ed25519-signed evidence bundle:

```bash
python tools/package_compatibility_evidence.py \
  --matrix build/compatibility-matrix.json \
  --reports build/*browser-validation.json \
  --manifests examples/*/venom.browser.json \
  --out build/venom-compatibility-evidence.zip

python tools/verify_compatibility_evidence.py \
  build/venom-compatibility-evidence.zip
```

The bundle binds the exact reports, fixture contracts, promotion policy result, required browsers, and source revision. Add `--private-key` and `--public-key` to sign it with the existing Ed25519 release-signing system.

## Production output

Every site build emits one stable entry document and grouped, content-addressed assets:

```text
dist/
  index.html
  assets/
    app/
      app.<hash>.vbc
    style/
      s.<hash>.css
    images/
      <name>.<hash>.<ext>
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

`index.html` references the loader and stylesheet with Subresource Integrity. Generated metadata binds the expected package and runtime asset set for consistency checking. Venom's verification tools reject unexpected files, missing assets, hash mismatches, forbidden source-evaluation paths, invalid package metadata, and non-real engine artifacts in strict mode. These checks detect alteration relative to the generated build; publisher authenticity requires a separately trusted release signature.


### WASM-owned, build-specialized Route VM execution

Protected distributions no longer ship the Route VM opcode table, bytecode parser, polymorphic decoder, or route interpreter in readable JavaScript. The C/WASM runtime validates and decodes the route program and emits a bounded, build-specific binary DOM-command stream. Each protected build remaps physical DOM command identifiers and permutes payload-field order using independently derived diversification streams; JavaScript validates the self-described mapping and only applies the resulting browser mutations. Development source retains the legacy implementation for diagnostics, but protected output is verified to exclude it.

## Application-specialized protection

Protected builds generate a least-privilege host capability manifest from the application’s actual JavaScript usage. Unused fetch, timer, and event surfaces are omitted, unknown operations are denied, and browser DOM references use authenticated, route-generation-scoped handles. See [Application-specialized browser bridge](docs/capability-secure-browser-bridge.md).

## Architecture at a glance

```text
Source website
      |
      v
Venom compiler
  - route and HTML analysis
  - CSS and asset collection
  - Route VM bytecode generation
  - QuickJS native bytecode compilation
      |
      v
Protected application package (.vbc)
  - route table and route bytecode
  - shared string table
  - QuickJS bytecode records
  - assets, policy, and integrity metadata
      |
      v
Generated browser runtime
      |
      +-------------------------------+
      |                               |
      v                               v
Route VM interpreter             QuickJS/WASM runtime
implemented in generated JS      real QuickJS engine
reconstructs packaged routes     reads QuickJS object bytecode
and emits DOM operations         executes application logic
      |                               |
      +---------------+---------------+
                      |
                      v
              Venom host bridge
                      |
                      v
             Supported browser APIs
```

The **Route VM** executes a narrow, package-specific instruction set for routes and deterministic DOM construction inside WebAssembly. Protected builds diversify its physical encoding through opcode remapping, operand masks, instruction-word reordering, string ordering, route ordering, section ordering, and a build-specific binary DOM-command protocol. JavaScript receives only a validated command stream whose physical command identifiers and payload-field order differ between builds.

The **QuickJS execution layer** is a real QuickJS runtime compiled to WebAssembly. Build-time QuickJS uses compile-only mode and serializes executable objects with source and debug information stripped. Browser execution uses `JS_ReadObject`, `JS_EvalFunction`, and QuickJS job processing through the WebAssembly runtime.

QuickJS accesses selected browser services through Venom's host bridge. The bridge validates operations and applies implemented policy and resource checks, but it is not a complete browser API implementation or an independently audited security sandbox.

For deeper design information, read [Product overview](docs/product-overview.md), [Dual-engine architecture](docs/dual-vm-architecture.md), [Runtime](docs/runtime.md), [Package format](docs/package-format.md), [Compatibility](docs/compatibility.md), and [Threat model](docs/threat-model.md).

## Application-specialized runtime modules

Protected builds derive a runtime-module plan from the syntax-aware capability graph. Core Route VM, DOM, and QuickJS support remain present, while optional network, timer, event, storage, and navigation surfaces are selected from actual application usage. Unused network, timer, and event implementations are replaced with compact fail-closed stubs before minification and hashing. The selected module list is recorded in `assets/app/build.json`.


## Requirements

### Common requirements

- CMake 3.20 or newer
- a C11 and C++17 compiler
- Python 3.9 or newer
- Git for toolchain setup and source development
- a local HTTP server for browser testing

### Windows

Supported development environments include:

- Windows 10 or Windows 11;
- Visual Studio 2022 or newer with the Desktop development with C++ workload;
- MSVC with C11 atomics support, or `clang-cl`;
- PowerShell 5.1+ or PowerShell 7+;
- Ninja is recommended for CMake presets.

QuickJS requires C11 atomics. Venom's CMake configuration checks this and stops with a clear error when the selected MSVC toolset is too old.

### Linux, macOS, and WSL

Use a current Clang or GCC toolchain with CMake, Ninja or Make, Python 3, and standard development libraries. Clang is required for libFuzzer targets. Linux is the primary CI environment; macOS and WSL use the same shell entry points but should be validated on the intended deployment toolchain.

### WebAssembly toolchain

Production browser releases require a real QuickJS/WASM artifact generated with the pinned Emscripten workflow. The repository's release gates reject stale or scaffold artifacts.

The current CI pin is recorded in `.github/workflows/release-hardening.yml`, and toolchain metadata is recorded in `toolchains.lock.json`.

## Repository layout

```text
.github/             GitHub Actions, issue forms, and pull-request template
cmake/               build options, warnings, platform, and test registration
docs/                architecture, security, operations, maintenance, releases
examples/            sample sites and compatibility fixtures
fuzz/                libFuzzer and deterministic replay targets
scripts/             canonical build, test, serve, setup, benchmark, and release commands
src/compiler/        CLI, site compiler, asset processing, generated runtime sources
src/package/         package format, reader/writer, compression, hashing, crypto
src/quickjs/         QuickJS engine, bytecode, ABI, and host bridge integration
src/runtime/         native/WASM runtime, package runtime, and browser host adapters
src/vm/              protected VM encoding, polymorphism, opcodes, interpreter
tests/               unit, integration, browser, package, runtime, and security tests
third_party/quickjs/ vendored QuickJS sources and upstream license
tools/               release, ABI, analysis, signing, verification, and fuzz utilities
```

The repository root intentionally contains project metadata only. All operational entry points live in `scripts/`.

## Install a verified binary release

Binary releases include a verifier and cross-platform installer. The installer verifies supply-chain metadata before changing the installation directory, supports mandatory Ed25519 signatures, installs releases into versioned directories, and records a bounded uninstall receipt.

```bash
python3 tools/install_release.py install venom-v1.13.0-linux-x86_64.zip
venom --version
```

For a signed stable release:

```bash
python3 tools/install_release.py install venom-release.zip \
  --require-signature \
  --public-key venom-release-public.pem
```

See [Installing Venom releases](docs/installation.md).

## Quick start

### Windows PowerShell

```powershell
# 1. Install and activate the pinned Emscripten SDK
.\scripts\setup-emscripten.ps1

# 2. Build and embed the production QuickJS/WASM runtime
.\scripts\build-quickjs-wasm.ps1

# 3. Build Venom and compile the example site
.\scripts\build-site.ps1 -Site .\examples\basic-site -Dist .\dist

# 4. Serve the generated distribution
.\scripts\serve-site.ps1 -Port 8080 -Dist .\dist
```

Open `http://127.0.0.1:8080` in a supported browser.

### Windows Command Prompt

```bat
scripts\setup-emscripten.bat
scripts\build-quickjs-wasm.bat
scripts\build-site.bat examples\basic-site dist
scripts\serve-site.bat 8080 dist
```

### Linux, macOS, or WSL

```bash
./scripts/setup-emscripten.sh
./scripts/build-quickjs-wasm.sh
./scripts/build-site.sh examples/basic-site dist Release
./scripts/serve-site.sh 8080 dist
```

## QuickJS/WASM runtime lifecycle

Normal website builds reuse the verified embedded QuickJS/WASM runtime and do not require Emscripten on every invocation. The standalone runtime script automatically discovers an explicit toolchain, `EMCC`, `EMSDK`, the repository-local `build/emsdk` installation, or `emcc` on `PATH`.

```powershell
.\scripts\build-quickjs-wasm.ps1 -Status
.\scripts\build-quickjs-wasm.ps1 -Verify
.\scripts\build-quickjs-wasm.ps1       # builds only when stale
.\scripts\build-quickjs-wasm.ps1 -Force
```

See [QuickJS/WASM runtime lifecycle](docs/quickjs-runtime-lifecycle.md).

## Native development build

The native build validates the compiler, package implementation, native runtime probes, and most regression suites. It does not replace the browser-complete release closure.

### CMake presets

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Release-oriented local build:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Strict warnings-as-errors build:

```bash
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
```

### Script wrappers

Windows PowerShell:

```powershell
.\scripts\build.ps1 -Config Release -BuildDir build\windows-release -FullQuickJS
.\scripts\test.ps1 -Config Release -BuildDir build\windows-release -FullQuickJS
```

Unix-like shells:

```bash
./scripts/build.sh Release build/release fullquickjs
./scripts/test.sh Release build/release fullquickjs
```

## Project configuration and diagnostics

Copy `venom.toml.example` to `venom.toml`, adjust the source and output paths, and build without repeating project policy flags:

```bash
venom build --config venom.toml
venom doctor
venom doctor --format json
```

Command-line values override configuration values. Production fail-closed constraints cannot be disabled by configuration. Every successful build emits `assets/app/build.json` with package/runtime versions, backend selection, dependency-lock digest, package digest, and source/config provenance. See [Project configuration](docs/configuration.md) and [CLI contract](docs/cli-contract.md).

## Command-line usage

After building, locate `venom` or `venom.exe` under the selected build directory.

```text
venom build <site-dir> --out <dist>
venom keygen --out venom.key [--force]
venom release-check <dist-or-package> [options]
venom verify-runtime <dist-or-package> [options]
venom inspect <package.vbc> [--key-file venom.key]
venom --version
```

### Build a site

```bash
venom build examples/basic-site --out dist
```

Version 1.1.0 uses a production-only build policy and adds project configuration, environment diagnostics, stable exit categories, JSON automation output, and build provenance. Historical profile, runtime, backend, hashing, package-mode, and fallback flags may still be parsed for compatibility, but the compiler normalizes every build to the hardened browser target:

- profile: `browser-protect`;
- runtime: WebAssembly;
- QuickJS backend: real WASM engine;
- hashed grouped assets;
- external protected package;
- runtime crypto provider;
- strict release checks;
- no host-JavaScript fallback;
- no debug manifest.

### Remote scripts and vendoring

Supported static HTTPS scripts are downloaded during compilation, included in the protected package, and pinned in a source-local `venom.lock` file.

```bash
venom build site --out dist \
  --vendor-cache .venom/vendor-cache \
  --vendor-lock site/venom.lock
```

Use offline mode only after the required resources exist in the vendor cache:

```bash
venom build site --out dist \
  --vendor-cache .venom/vendor-cache \
  --vendor-lock site/venom.lock \
  --offline
```

Dependency pins change only when explicitly refreshed:

```bash
venom build site --out dist \
  --vendor-cache .venom/vendor-cache \
  --vendor-lock site/venom.lock \
  --refresh-vendors
```

Review lockfile changes before committing them.

### Verify a distribution

```bash
venom release-check dist --target browser
venom verify-runtime dist --target browser --require-real-engine
```

`release-check` validates production metadata, package policy, protected layout, integrity records, and security target. `verify-runtime` additionally validates the bound runtime assets and engine contract.

### Inspect a package

```bash
venom inspect dist/assets/app/app.<hash>.vbc
```

Inspection reports package metadata intended for authorized diagnostics. Protected internal records remain governed by the package format and key policy.

### Native/private package keys

The CLI includes key generation and keyed inspection support:

```bash
venom keygen --out venom.key
venom inspect package.vbc --key-file venom.key
```

Never commit `venom.key`, place it in a browser-delivered distribution, or pass it through public CI logs. Browser builds intentionally use a client-runnable protection model and cannot keep an embedded decryption key secret.

## Versioned product contracts

Venom exposes machine-readable contract versions for automation and release validation:

```bash
venom contracts
venom contracts --format json
```

The host bridge source of truth is `contracts/host-api.json`. Generated C definitions and documentation must remain synchronized:

```bash
python tools/generate_host_api.py --check
```

## Static compatibility preflight

Scan a site before building to detect known denied, unsupported, or partially supported APIs:

```bash
venom compatibility check ./site
venom compatibility check ./site --format json
```

An incompatible result exits with code `20`, making the command suitable for CI gates. Static scanning is conservative and does not replace browser differential testing.

## Syntax-aware capability analysis

Venom can inspect an application before compilation and produce a machine-readable capability graph:

```bash
venom analyze ./site
venom analyze ./site --format json
venom compatibility check ./site --format json
```

The analyzer ignores comments and ordinary string contents, scans inline scripts, distinguishes guarded UMD CommonJS branches from unconditional Node dependencies, and reports precise source locations for syntax, module, browser API, and framework usage. See [Capability analysis](docs/capability-analysis.md).

### Protected module graph discovery

Venom recursively discovers package-local ES module dependencies from static imports, literal `import()` expressions, and HTML `modulepreload` hints. Dependency chunks are compiled into protected QuickJS records and are not executed as additional route entry scripts. Literal dynamic imports are packaged through the `VQJSMB04` in-memory QuickJS module loader. Non-literal and remote dynamic imports remain unsupported; framework-level support still requires browser evidence. See [Dynamic module graphs](docs/dynamic-module-graphs.md).

## Browser and website compatibility
### DOM compatibility modules

Venom now plans and injects forms, observer, animation-frame, and event-constructor capabilities only when the application capability graph requires them. The cross-browser fixture corpus includes property reflection, form submission, event bubbling, `MutationObserver`, and `requestAnimationFrame` behavior.

### Evidence-backed support tiers

Compatibility fixtures declare one of three support tiers:

- **Probe** — an exact integration is being measured; no support claim is made.
- **Candidate** — at least one required browser passes, but the full support policy is incomplete.
- **Supported** — Chromium, Firefox, and WebKit all pass the declared behavioral contract with sufficient checks.

The compatibility matrix enforces these tiers. A fixture cannot be labeled `supported` unless its bound reports satisfy the required-browser policy.


Venom targets supported static and client-rendered websites whose JavaScript and browser API usage are represented by the current QuickJS host bridge.

The repository includes support or infrastructure for ordinary HTML and CSS, multiple HTML routes, inline and external scripts, selected ES modules, common DOM operations, events, timers, promises, fetch, storage, navigation, forms, console behavior, and selected browser-global shims.

Venom is **not a complete browser implementation**. Applications that depend on unsupported Web APIs, exact native DOM object identity, browser extensions, Node.js APIs, unrestricted dynamic source evaluation, undocumented browser behavior, or unimplemented framework internals may require changes.

| Target | Current documentation status |
|---|---|
| Chromium-based desktop browsers | Primary development and validation target |
| Firefox | Intended target; validate every release and application |
| Safari/WebKit | Intended target; validate every release and application |
| Mobile Chromium/WebView | Requires application-specific testing |
| Legacy browsers | Unsupported |
| Browsers without WebAssembly or Workers | Unsupported |

Run the compatibility preflight before building:

```bash
venom compatibility check ./site
venom compatibility check ./site --format json
```

The preflight is advisory and pattern-based. It cannot prove behavioral equivalence. A successful build and clean preflight must be followed by end-to-end browser testing. See [Compatibility](docs/compatibility.md).

## Production readiness report

Venom includes a conservative orchestration tool that combines source compatibility, generated-artifact, provenance, runtime-verification, and deployment checks into one report. It does not replace browser journey testing or prove server configuration that cannot be observed from static files.

```bash
./scripts/readiness.sh examples/basic-site --dist dist --venom ./build/release/venom --require-real-engine
```

```powershell
.\scripts\readiness.ps1 -Site .\examples\basic-site -Dist .\dist -Venom .\build\windows-release\Release\venom.exe -RequireRealEngine
```

Machine-readable CI output is available with `--format json`. The command returns release-policy exit code `60` when blockers remain. Add `--strict` to treat deployment warnings as blockers. A `READY` result means the checks represented by this tool passed; it does not establish browser-wide behavioral equivalence, publisher authenticity, or resistance to a determined browser operator.

## Fixture-driven compatibility evidence

Venom browser validation is driven by versioned `venom.browser.json` manifests stored with each application fixture. A manifest declares routes, readiness conditions, browser actions, and observable assertions without hard-coding a single test site into the runner. Reports identify both the exact distribution digest and the fixture-manifest digest. Multiple reports can be aggregated with `tools/compatibility_matrix.py` to produce a release compatibility matrix. This evidence demonstrates only the tested scenarios; it is not a blanket claim of framework or browser support.

## Real-browser validation

Static compatibility analysis and successful compilation are not browser behavior evidence. Validate the generated distribution in real browser engines:

```bash
./scripts/browser-validate.sh dist --browser all --manifest examples/browser-compat-site/venom.browser.json --json-out browser-validation.json
```

PowerShell:

```powershell
.\scripts\browser-validate.ps1 -Dist .\dist -Browser all -JsonOut .\browser-validation.json
```

The report can be attached to the consolidated production-readiness check:

```bash
python tools/production_readiness.py examples/browser-compat-site \
  --dist dist \
  --venom build/release/venom \
  --require-real-engine \
  --browser-report browser-validation.json
```

A passing result applies only to the scenarios and browser engines named in the report. Application-specific critical journeys still need dedicated tests.


### Production-bundle compatibility evidence

The browser matrix includes a production-style ES module fixture with hashed asset names, minified modules, a multi-chunk static import graph, DOM hydration patterns, events, promises, timers, CSS, and multiple routes. This demonstrates support for those bundle characteristics; it does **not** by itself claim broad React, Vue, Svelte, or other framework compatibility. Framework support is documented only after an actual generated framework fixture passes continuously.

```bash
venom build examples/production-bundle-site --out dist-production-bundle
python tools/browser_validation.py dist-production-bundle \
  --browser all \
  --manifest examples/production-bundle-site/venom.browser.json \
  --json-out production-bundle-validation.json
```


### Pinned framework compatibility probe

Venom now includes an exact React 16 production UMD fixture rather than a framework-shaped approximation. The fixture uses React 16.0.0 and ReactDOM 16.0.1 and checks initial rendering, class-component state, synthetic click handling, and a state-driven DOM update.

```bash
venom build examples/react16-umd-site --out dist-react16
python tools/browser_validation.py dist-react16 \
  --browser all \
  --manifest examples/react16-umd-site/venom.browser.json \
  --json-out react16-browser-validation.json
```

A passing result supports only this pinned UMD combination and the declared behaviors. It does not imply support for current React, hooks, React Router, Next.js, Server Components, or arbitrary React packages. See [Framework compatibility](docs/framework-compatibility.md).


## Generated product contracts

Venom defines package, Route VM, DOM protocol, QuickJS, host bridge, configuration, and lockfile contracts in `contracts/product-contracts.json`. Generated C++ and Python bindings prevent version drift across the compiler, embedded runtimes, release tools, and documentation.

```bash
python tools/generate_product_contracts.py --check
venom contracts --format json
```

Protected packages use the `minimal-v1` metadata profile. Diagnostic parity, audit, cache, console-capture, and failure-report sections that are not required for execution are omitted from production packages and retained only in development builds.

## Testing and validation

### Standard test suite

```bash
ctest --preset dev
ctest --preset strict
```

Or use the platform scripts:

```bash
./scripts/test.sh Release build/release fullquickjs
```

```powershell
.\scripts\test.ps1 -Config Release -BuildDir build\windows-release -FullQuickJS
```

### Distribution analysis

```bash
./scripts/analyze-dist.sh dist --strict
```

```powershell
.\scripts\analyze-dist.ps1 -Dist dist -Strict
```

The strict analyzer validates the grouped artifact layout, loader references, forbidden markers, package/runtime metadata, real-engine status, and runtime truth values.

### Release closure

A production candidate must rebuild the checked-in QuickJS/WASM asset and pass the browser-complete closure gate:

```bash
./scripts/check-release-closure.sh --require-real
cmake --preset release-closure
cmake --build --preset release-closure
ctest --preset release-closure
```

The closure configuration does not permit browser prerequisites to be silently skipped.

### Fuzzing

Clang/libFuzzer build:

```bash
cmake --preset fuzz
cmake --build --preset fuzz
./scripts/fuzz-all.sh
```

Windows and non-Clang environments can run the deterministic replay gates through the provided PowerShell, batch, and Python tools.

### Runtime size and performance budgets

```bash
python tools/runtime_performance.py --format json --json-out runtime-performance.json
python tools/runtime_performance.py --dist dist --format json
```

The checked-in policy in `contracts/runtime-performance.json` gates raw and compressed QuickJS/WASM size, release ABI width, and optional distribution size. See [Runtime performance](docs/runtime-performance.md).

### Runtime benchmarks

```bash
./scripts/benchmark-runtime.sh --dist dist --json-out performance.json
```

Benchmark results are environment-specific. Record processor, memory, operating system, compiler, browser, power mode, and site corpus when comparing runs.


## Verified runtime release chain

Production browser builds require provenance proving that the embedded QuickJS/WebAssembly artifact is the real upstream engine, uses native `VQJSBC03` object bytecode, satisfies the release ABI, and requires no source fallback. Scaffold or contract-only runtime blobs are rejected before Venom writes the output distribution.

Tagged releases build one canonical QuickJS/WASM artifact and use the same embedded runtime for sanitizer/fuzz closure, Windows/Linux/macOS packages, and real-browser validation. The release publication manifest binds the canonical runtime SHA-256 digest. See [Verified Runtime Release Chain](docs/releases/v1.16.0-verified-runtime-release-chain.md).

## Production assurance

Venom separates native correctness, security analysis, browser-complete closure, and artifact reproducibility into independent CI gates. Release archives can be compared byte-for-byte with:

```bash
python tools/check_reproducibility.py first-release.zip second-release.zip
```

Protected website output may intentionally differ between builds because per-build polymorphism is a security feature. Reproducibility applies to release tooling and artifacts produced from identical fixed inputs. See [Production assurance](docs/production-assurance.md).

## Release publication set

Venom can bind all platform packages and compatibility evidence into one deterministic publication set with optional Ed25519 signing:

```bash
scripts/package-release-set.sh --version 1.12.0 --source-revision "$GIT_COMMIT" --packages venom-v*.zip --compatibility-evidence venom-compatibility-evidence.zip --out release-set
scripts/verify-release-set.sh release-set --expect-version 1.12.0 --require-evidence
```

See [Release publication sets](docs/release-publication-set.md).

## Release packaging

Create a source or binary release using the canonical packaging layer:

```bash
./scripts/package-release.sh --help
```

Verify a produced release:

```bash
./scripts/verify-release.sh --help
```

Release packaging supports source inventory checks, checksums, signatures, release metadata, runtime verification, and deterministic timestamps where configured. Read [Release packaging](docs/release-packaging.md), [Release signing](docs/release-signing.md), and [Production artifact integrity](docs/production-artifact-integrity.md).

## Deployment

Venom output is static and can be hosted by Nginx, Apache, Caddy, object storage, a CDN, GitHub Pages-compatible static hosting, or another server that preserves the files and MIME types.

Recommended server behavior:

- serve `.wasm` as `application/wasm`;
- serve `.js` as JavaScript and `.css` as `text/css`;
- do not rewrite hashed asset requests to `index.html`;
- use immutable long-lived caching for hashed assets;
- use a short cache lifetime or revalidation for `index.html`;
- enable HTTPS in production;
- preserve Subresource Integrity attributes and avoid HTML post-processing;
- configure a Content Security Policy compatible with WebAssembly and workers;
- serve all files from the same trusted origin unless cross-origin isolation and CORS are deliberately configured.

Example Nginx cache policy:

```nginx
location = /index.html {
    add_header Cache-Control "no-cache";
}

location /assets/ {
    add_header Cache-Control "public, max-age=31536000, immutable";
}

types {
    application/wasm wasm;
}
```

Route handling is packaged by Venom; validate direct navigation and refresh behavior for every application route using the intended production server configuration.

## Route-generation isolation

Venom binds timers, fetches, event callbacks, DOM handles, and QuickJS contexts to the route generation that created them. Navigation cancels active asynchronous resources and invalidates stale work before the next route becomes active. Late completions are rejected rather than being allowed to mutate a newer route.

## Runtime containment

Protected builds enforce route-scoped limits for host calls, concurrent fetches, response sizes, active timers, event dispatches, DOM handles, and route lifetime. Budget exhaustion fails closed, while navigation cancels route-owned asynchronous work and resets the next generation. See [Runtime resource policy](docs/runtime-resource-policy.md).

## Security guidance

Venom is a client-side protection and consistency system. Its defensible security goals are to reduce readable source exposure, raise static-analysis cost, diversify protected package structure, reject malformed input, and detect stale or mixed-build components.

- Keep credentials, private keys, authorization decisions, and privileged business logic on trusted servers.
- Treat all browser-delivered code, package data, runtime constants, and materialized values as recoverable by a determined client.
- Do not weaken fail-closed runtime checks to make an incompatible application appear to work.
- Preserve the generated file set exactly and deploy all assets from the same build atomically.
- Use HTTPS, a suitable Content Security Policy, and standard web security headers.
- Run `release-check`, `verify-runtime`, strict distribution analysis, target-browser tests, and fuzz/replay gates before publication.
- Use externally trusted release signatures when publisher authenticity is required.

### Protection and encryption terminology

The browser-compatible runtime provider uses a runtime-decodable authenticated encoding layer intended to deter casual extraction and detect corruption. It is not an audited secret-key confidentiality boundary against the browser operator.

The optional libsodium provider uses XChaCha20-Poly1305 with an external 256-bit key for native/private workflows. Browser runtimes reject those packages because a secret shipped to the browser cannot remain secret.

SHA-256 section records and generated asset hashes detect inconsistency relative to the build. They do not, by themselves, prove who published a replacement distribution. See [Security model](docs/security-model.md), [Threat model](docs/threat-model.md), and [Cryptographic release model](docs/cryptographic-release-model.md).

Report vulnerabilities privately according to [SECURITY.md](SECURITY.md).

## Troubleshooting

### `QuickJS WASM interpreter unavailable; source-decode fallback denied`

The distribution does not contain a usable real QuickJS/WASM runtime, or the runtime failed to initialize. Run the Emscripten setup and QuickJS/WASM rebuild scripts, rebuild the site, and verify it with `--require-real-engine`.

### `bound asset hash mismatch`

A generated asset was changed, partially copied, cached from another build, or served from an inconsistent deployment. Delete the entire destination distribution, rebuild it atomically, and deploy all files from the same build together.

### WebAssembly MIME or download errors

Confirm the server returns `application/wasm` for `.wasm` files and does not redirect asset requests to HTML.

### Windows cannot find `venom.exe`

Use `scripts/build.ps1` or `scripts/build.bat`; they search both single-configuration and Visual Studio multi-configuration output layouts. The production site script uses `build/windows-release` by default.

### Emscripten is installed but `emcc` is missing

Run the setup script from a fresh shell or use the provided build scripts, which resolve the pinned SDK entry point directly. See [Emscripten build](docs/emscripten-build.md).

## Project governance

- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
- [License](LICENSE)
- [Release history](docs/releases/README.md)
- [Maintenance notes](docs/maintenance/README.md)

## License

Venom source code is currently distributed under an **all rights reserved** license; see [LICENSE](LICENSE). Third-party components retain their own licenses, including QuickJS under `third_party/quickjs/LICENSE`.

## Rebuilding the Route VM WebAssembly runtime

The protected Route VM is a separate standalone WebAssembly artifact from the QuickJS/WASM JavaScript engine. Rebuild and embed it after changing `src/runtime/wasm_runtime.c` or the physical Route VM contract:

```powershell
.\scripts\build-route-wasm.ps1
```

```bash
./scripts/build-route-wasm.sh
```

The build requires LLVM/Clang with the `wasm32` target. The scripts automatically use the repository-local Emscripten Clang installation when available, verify the exact six-export browser ABI, and replace `src/compiler/wasm_runtime_blob.hpp` with the verified artifact. Production browser tests must pass before release.

## Automatic browser script selection

Venom keeps JavaScript protected by default. Strong DOM/browser evidence and known browser-only libraries automatically select the native browser realm. Classic scripts on the same route are kept together so shared globals remain compatible. Override with `data-venom="browser"`, `data-venom="protected"`, `// @venom: browser`, or `// @venom: protected`.

## Execution planning reports

Every build emits `build/reports/execution-plan.json` and `build/reports/execution-plan.txt` inside the distribution. These reports explain, for each script, whether Venom selected native browser execution or protected QuickJS/WASM execution, the confidence score, the evidence that triggered the decision, route order, and final flags. Explicit `data-venom` and file-scope `@venom` directives remain authoritative.

### Function-level realm planning

Venom recognizes `// @venom: browser` and `// @venom: protected` immediately before top-level function declarations and arrow-function assignments. Every build emits `build/reports/function-plan.json` and `function-plan.txt`. In v1.31.1, a browser-selected function conservatively promotes its containing classic script to the browser realm so lexical state and closure semantics remain correct; the report marks this with `promoted_whole_file=true`. This is the safe foundation for later generated cross-realm stubs.


## Function extraction safety analysis (v1.31.2)

Every build emits `function-extraction-plan.txt`, `function-extraction-plan.json`, and `realm-bridge-contract.json`. Venom analyzes explicitly selected functions for realm-bound syntax and reports whether each function is already in its requested realm, requires conservative whole-file promotion, is blocked from extraction, or is a candidate for the generated asynchronous JSON-value bridge. Synchronous cross-realm calls remain fail-closed until a semantics-preserving synchronous transport exists.


## Worker RPC protocol foundation (v1.31.3)

Bridge candidates are now embedded into the hardened worker as a closed allowlist. The worker exposes a versioned capability query and validates asynchronous invocation requests against request-id, message-size, concurrency, candidate, argument-count, and JSON-value constraints. The worker validates candidates and forwards JSON envelopes through the QuickJS/WASM bridge ABI. Results are returned only when protected bytecode has registered the candidate in `globalThis.__venomProtectedBridge`; otherwise invocation fails closed with `candidate-not-registered`. No function is silently executed in host JavaScript.


## QuickJS protected bridge ABI (v1.31.4)

The release QuickJS/WASM ABI now includes bounded bridge input and result buffers, JSON envelope parsing inside QuickJS, protected registry lookup, function invocation, result serialization, exception propagation, execution-budget enforcement, and secure buffer release. The worker retains a dedicated QuickJS instance and context after attestation and reports `executorReady=true` only when this ABI is available. Physical compiler extraction and protected registry generation remain the next step; unregistered candidates fail closed.
