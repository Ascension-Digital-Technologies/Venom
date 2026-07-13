# Changes

## 1.36.4

- Fixed Windows PowerShell 5.1 argument binding in `setup-js-hardener.ps1`.
- Node's `-e` flag is now passed through an explicit argument array instead of being misread as an abbreviated PowerShell common parameter.
- All checked npm/Node invocations now use named `-Program` and `-Arguments` parameters.


- Updated the Windows JavaScript hardener installer to keep failures visible instead of closing immediately.
- Added `-NoPause` for CI and unattended setup.
- Preserved the actual npm/Node failure message and PowerShell stack details.
- Updated the batch launcher to return the installer exit code while relying on the PowerShell script to pause only on failure.
- Kept the self-repairing dependency install and end-to-end hardener verification from 1.36.2.

## 1.36.2

- Fixed the JavaScript hardener installer on Windows PowerShell and Node.js 24.
- Installer now checks npm exit codes, repairs partial installs, removes non-portable private-registry lockfiles, and verifies both imports with an end-to-end hardening self-test.
- Hardened builds now show a direct setup command when dependencies are missing or corrupt.

## 1.36.1

- Fixed Windows PowerShell 5.1 executable discovery for Visual Studio multi-config builds.
- `build-site.ps1` now resolves `build/<Config>/venom.exe`, rebuilds when needed, and resolves again after a successful build.
- The release workflow uses the same shared executable resolver across Windows generators.

## 1.36.0

- Added a fail-fast public-release repository gate.
- Added consolidated third-party notices, support guidance, code of conduct, and a public release checklist.
- Included changelog, security, support, conduct, and notice files in packaged releases.
- Added GitHub feature-request metadata and Dependabot coverage for GitHub Actions.
- Updated the README and release workflow for public release-candidate status.

## 1.35.0

- Added deterministic per-build diversification with `--seed`.
- Added protected-chess Playwright qualification for Chromium, Firefox, and WebKit.
- Added autoplay, throughput, reset, and cancellation browser assertions.
- Added fail-closed mutation tests for package, loader, runtime WASM, and missing runtime assets.
- Added same-seed reproducibility and different-seed diversification gates.
- Added authoritative `scripts/release.*` release qualification entry points.

# Changelog

## 1.34.1

Repository consistency and release-maintenance cleanup:

- normalize the changelog and remove duplicated historical headings;
- replace stale `basic-site` test names with neutral site-preview or protected-chess names;
- update CLI examples to use `examples/protected-chess`;
- make release packaging tests read the project version dynamically instead of assuming `1.0.1`;
- repair the release packaging smoke test so it validates the single supported chess example.

## 1.34.0

Release-candidate repository cleanup:

- retain `examples/protected-chess` as the only supported public example;
- move compatibility and parser websites under `tests/fixtures/sites`;
- reduce scripts to canonical build, test, serve, analysis, readiness, and release entry points;
- remove historical hotfix, migration, maintenance, and generated-manifest debris;
- rewrite the root README around the current architecture and workflow;
- add a complete standalone README for the Protected Chess Lab;
- restore missing compiler build sources so clean checkouts configure and compile correctly.

## 1.33.0–1.33.6

Protection hardening series:

- add aggressive AST-aware JavaScript hardening for loader, worker, engine, and runtime assets;
- remove production extraction reports, source paths, stable candidate names, and descriptive WASM ABI symbols;
- add opaque per-build bridge envelopes, numeric export slots, session binding, and replay protection;
- move package section decode and decompression behind the WASM-owned boundary;
- diversify complete package section ordering and physical offsets per build;
- add mandatory production leak scanning and fail-closed release verification.

## 1.32.0–1.32.5

Protected bridge and chess integration series:

- introduce `venom.ready()`, `venom.call()`, `venom.exports`, and `venom.info()`;
- add bounded JSON transport, timeouts, cancellation, concurrency limits, and sanitized errors;
- extract the chess engine into protected QuickJS bytecode while retaining browser-native UI logic;
- fix bridge readiness ordering, export pre-registration, and QuickJS Promise result handling;
- modernize the chess example into the Venom Protected Chess Lab.
