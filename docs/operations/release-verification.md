# Production Release Verification

> **Applies to:** Venom 1.1.0  
> **Purpose:** prove that a source tree, compiler binary, examples, runtime artifacts, and signed release package satisfy the production contracts

Venom release verification is fail closed. A release is not qualified merely because the compiler builds or an example opens in a browser. The release closure validates the repository, embedded runtimes, hardener, native tests, production distributions, browser behavior, leakage policy, package signatures, and generated evidence.

## One-command closure

Windows:

```powershell
.\scripts\windows\release-closure.ps1 -Parallel 8
```

Linux:

```bash
bash scripts/linux/package-release.sh --help
```

To include real-browser equivalence tests:

```powershell
.\scripts\windows\release-closure.ps1  -Parallel 8
```

## Verification stages

1. **Repository gate** — validates required files, version consistency, documentation, release policy, CMake source completeness, and changelog structure.
2. **Runtime provenance** — verifies that the embedded QuickJS/WASM and package runtimes match their expected ABI, exports, fingerprints, and source records.
3. **Hardener verification** — installs the locked Node dependency graph, imports required modules, performs a real hardening pass, and validates the generated JavaScript.
4. **Clean native build** — configures and builds the compiler and native probes using the release profile.
5. **CTest closure** — executes the complete registered test inventory.
6. **Example qualification** — builds Protected Chess, NOVA TRADE, and Bot Detection under a single production-grade profile.
7. **Distribution inspection** — validates asset binding, package layout, source leakage policy, runtime fingerprints, and fail-closed metadata.
8. **Browser equivalence** — when enabled, compares source and protected distributions in real browsers using the checked-in scenario manifests.
9. **Performance budgets** — records build and runtime evidence and fails on configured regression ceilings where enabled.
10. **Signed packaging** — creates and verifies the release set, manifests, checksums, provenance, and detached signature.

## Output

The closure writes evidence beneath:

```text
build/release-closure-output/
├── logs/
├── examples/
├── compatibility/
├── performance/
├── package/
└── release-closure-report.json
```

The JSON report identifies every stage, command, result, duration, and evidence location. Preserve it with the published release artifacts.

## Expected result

A successful run ends with:

```text
[venom] RELEASE CLOSURE: PASS
```

Any failed stage invalidates the release. Do not bypass a failed hardener, runtime provenance, source-leakage, compatibility, or signature check to publish a stable build.

## Manual distribution verification

For a previously built distribution:

```powershell
venom analyze dist
venom verify dist
venom verify-runtime dist
```

The exact command names exposed by the installed binary are documented in [CLI reference](../reference/cli.md).

## Clean-package verification

Before publication, extract the produced source archive into a new directory and repeat:

```powershell
.\scripts\windows\build.ps1 -Config Release
ctest --test-dir build -C Release --output-on-failure -j 1
```

This detects omitted CMake modules, source files, generated inputs, templates, scripts, or documentation that may be present in a working checkout but missing from the archive.

## Browser requirements

Install Playwright browser engines before requesting browser runtime tests:

```powershell
python -m pip install playwright==1.52.0
python -m playwright install chromium firefox webkit
```

Browser qualification uses Python Playwright strictly as test tooling; Node.js is not required by the Venom compiler, hardener, or runtime. Pull requests and main-branch pushes run the Chromium runtime gate. The scheduled nightly job runs Chromium, Firefox, and WebKit.

The gate validates navigation, protected runtime readiness, execution of a real protected export, failed network requests, page errors, and console errors. It intentionally does not assert demo autoplay timing or application performance.

## Signing requirements

Stable release packaging requires the configured Ed25519 private key, public key, and key identifier. The private key must remain outside the repository and should be supplied through a protected release environment. See [Release signing](../security/release-signing.md) and [Release packaging](release-packaging.md).

> **Verified WASM toolchain:** Venom pins Emscripten 4.0.10 for reproducible release builds. The minimal QuickJS/WASM target excludes QuickJS POSIX libc helpers, allowing newer Emscripten releases to compile the web/worker runtime without `environ` or `sighandler_t` compatibility failures.
