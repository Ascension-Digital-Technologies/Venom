# Changelog

## 1.65.2 — Documentation maintenance release

### Documentation

- Removed obsolete release-candidate reports from the stable source package.
- Rebuilt the documentation index around current installation, integration, architecture, security, compatibility, operations, and contributor workflows.
- Updated stale applicability banners to the current stable documentation baseline.
- Expanded the route-hydration, architecture-overview, and automation/exit-code references.
- Added a documentation maintenance policy and stable-package rule forbidding obsolete RC reports.
- Normalized the changelog into one descending version history and restored the missing 1.63.0 boundary.

## 1.65.1 — Stable release

### Fixed

- Restored every CMake module, compiler pipeline source, platform build wrapper, Emscripten controller, performance tool, and linked guide required by a clean extracted source build.
- Added package-completeness gates for local CMake modules and every CMake-referenced source-package input.
- Repaired stale release tests for the leased binary bridge, dynamic asset resolution, QuickJS runtime lifecycle, remote vendoring, hashed package discovery, and browser API shim boundaries.
- Removed unused JavaScript rewriting helpers exposed by warnings-enabled clean builds.

### Verified

- Clean extracted Windows configuration, compilation, and runtime execution completed successfully.
- The complete 127-test native CTest inventory passes.
- Documentation, changelog, source-layout, runtime provenance, signed release-policy, verified-runtime-chain, and final repository gates pass.
- The publication README, three public examples, framework qualification, browser-equivalence tooling, performance tooling, and signed release workflow are included.

### Security

- Retains the binary leased capability bridge, streamed WASM-owned package decoding, short-lived memory buffers, build-specific bytecode envelopes and permutation, distributed integrity seals, and split package-decoder/QuickJS trust-domain handoff.
- Venom raises reverse-engineering and tampering cost; it does not claim permanent secrecy against an analyst who controls the browser.

## 1.64.9

- Made the legacy local release entrypoints fail closed and require Ed25519 signing keys for stable archives.
- Routed local releases through the final repository gate before qualification and packaging.
- Updated real-browser CI to cover every fixture in `tests/compatibility-suite.json`, including modern React, Vue, and Svelte output patterns.
- Added a policy regression that prevents unsigned stable release commands and compatibility-suite/workflow drift.

## 1.64.8

- Removed the unused pre-AEAD `CryptoProvider` and `NoCryptoProvider` compatibility aliases after repository-wide call-site verification.
- Removed the duplicate secondary changelog header and duplicate current-version entry.
- Added repository gates that require one top-level changelog heading and one entry for every release version.
- Documented the rule that obsolete compatibility aliases are removed once all internal and public call sites use the canonical API.

## 1.64.7

- Removed the unused no-op `install_host_bridge_placeholder()` API.
- Removed the unused `compile_placeholder_bytecode()` alias and legacy `VQJSBC01` byte-buffer compiler.
- Corrected the native runtime CMake option description so it no longer refers to active runtime code as stubs.
- Extended the source-layout gate to prevent legacy placeholder APIs from returning.

## 1.64.6

- Removed the unused `vm::Interpreter` scaffold and its header; no production, test, or tooling path instantiated it.
- Kept the real route bytecode encoder, opcode model, polymorphic mapping, and WASM/browser executors intact.
- Extended the source-layout gate so the dead interpreter scaffold cannot return.
- Audited remaining small source files and retained only narrow interfaces, probes, generated contracts, and platform wrappers with active call sites.

## 1.64.5

- Removed unused no-op runtime loader and host initialization translation units.
- Removed the redundant native runtime facade duplicated by `package_runtime`.
- Moved the runtime JavaScript template interface into compiler services.
- Moved the generated host API contract beneath `src/generated/runtime`.
- Added source-layout enforcement against placeholder runtime stubs.

## 1.64.4

- Moved the generated version header template from `src/compiler/core/` to `cmake/templates/`, keeping build-system inputs out of the source tree.
- Added source-layout enforcement preventing `.in` templates from returning under `src/`.
- Deduplicated repeated changelog version headings while preserving their historical notes.
- Removed the placeholder-only `CODEOWNERS` file rather than publishing invalid ownership metadata.
- Added repository-wide `.editorconfig` and `.clang-format` policies.
- Added a final repository audit and publication checklist.

## 1.64.3

- Restored the canonical `fnv1a32` helper in the shared runtime template before integrity-seal and trust-handoff code uses it.
- Added a generated-runtime hash-helper regression that executes the extracted integrity path under Node instead of relying only on JavaScript syntax checks.
- Prevented WASM and JavaScript runtime generation from shipping an integrity path with an unbound hash dependency.

## 1.64.2

- Fixed a generated-runtime boot failure where the WASM bridge removed the JavaScript `opcodeMap` declaration but left integrity-seal initialization referencing that deleted local.
- WASM-owned route execution now seals the browser-visible runtime state with a `null` JavaScript opcode component while the route opcode map remains owned and validated inside WASM.
- Added a regression gate preventing declaration-only opcode-map removal from returning.

## 1.64.1

- Rebuilt and re-embedded the package/route WASM runtime after the streamed upload ABI changes.
- Added an explicit runtime feature bit for streamed package upload support.
- The browser bridge now rejects stale embedded runtimes before attempting package upload.
- Added a source/blob ABI synchronization regression.

## 1.64.0

- Added a build-bound bytecode handoff contract between the WASM-owned package decoder and QuickJS execution runtime.
- Bound decoded records to route, source, execution order, length, and byte hash.
- Added independent consumer-side validation and fail-closed rejection before protected execution.
- Added documentation and regression coverage for the split trust-domain boundary.

## 1.63.0

### Security

- Added per-session rotation for bridge invoke, cancel, result, and error operation codes.
- Replaced stable transmitted capabilities with counter-bound, single-use capability leases.
- Bound every lease to the worker generation and ephemeral session key.
- Preserved monotonic replay rejection and fail-closed session invalidation.

### Documentation and tests

- Added the session capability lease security guide and CTest regression coverage.

## 1.62.0

### Security

- Added compiler-generated worker integrity seals over capability IDs, protected candidate registries, registry bytecode, and bridge opcodes.
- Added runtime integrity seals over release diversification, QuickJS ABI identity, and route opcode maps, with checks before protected route execution.
- Added fail-closed regression coverage and documentation for distributed runtime tamper checks.

## 1.61.0

### Security

- Upgraded protected QuickJS records to the `VQJSE006` envelope.
- Added a build-specific 16-byte lane permutation with a validated map fingerprint.
- Bound the permutation to build salt, route, source identity, and execution order.
- Preserved the pinned upstream QuickJS ABI rather than introducing a fragile custom opcode fork.
- Added symmetry, wrong-key, partial-block, metadata, and runtime reconstruction regression coverage.

## 1.60.0

### Security

- Added build-specific `VQJSE005` envelopes around protected QuickJS bytecode and module-bundle records before package emission.
- Bound each envelope stream to the build diversification salt, route, source identity, and script order so equivalent source produces different stored records across builds.
- Added envelope ABI fingerprint and inner-record integrity validation before QuickJS execution.
- Erased the stored encoded record immediately after runtime decoding and retained the existing short-lived plaintext zeroization path.
- Updated release inspection to count protected envelope records instead of relying on raw `VQJSBC03` markers in packaged script sections.

### Testing

- Added `venom_build_specific_bytecode_envelope_smoke` to enforce compiler/runtime symmetry, per-build variation, integrity checks, metadata, and release inspection.

## 1.59.0

### Security

- Centralized QuickJS WebAssembly memory access behind bounds-checked range helpers.
- Added explicit zeroization for bridge envelopes, copied results, and WASM script/input ranges before allocation release.
- Removed direct scattered protected-call writes through `memory.buffer`.
- Added regression coverage for memory-range validation and plaintext-lifetime hardening.

### Documentation

- Added an explicit WebAssembly memory threat-boundary and hardening guide.

## 1.58.0

### Security

- Added ordered 64 KiB package upload sessions into WASM-owned resident storage.
- Removed the persistent JavaScript `pkg.bytes` package copy after successful parsing.
- Added explicit package retirement zeroization and fail-closed upload state validation.
- Reused the resident package for lazy section decoding and route execution instead of recopying the complete package.

### Added

- Added the Venom Sentinel bot-detection example with protected scoring logic, browser fingerprint collection, browser-equivalence manifest, and Windows/Linux build-and-serve scripts.
- Added streamed package-decoding documentation and regression coverage.

## 1.57.0

### Security

- Replaced readable array-based protected-call messages with transferable binary capability frames over a private `MessageChannel`.
- Added opaque per-build capability identifiers, worker generations, monotonic counters, replay rejection, strict frame length validation, and session-bound integrity tags.
- Preserved the public `venom.exports.*` API and the validated `json-value-v1` semantic contract.
- Added binary bridge documentation and regression coverage.

## 1.56.0

### Added

- Framework qualification fixtures for modern React/Vite, Vue/Vite, and Svelte/Vite production output patterns.
- Generated framework qualification matrix and machine-readable evidence tool.
- Framework-specific hybrid integration guidance.
- Release-closure framework qualification step before browser equivalence execution.

### Changed

- Expanded the compatibility corpus from six to nine behavioral fixtures.
- Clarified that framework qualification applies to pinned representative output patterns rather than every framework release or plugin.

## 1.55.0

### Added

- Runtime benchmark schema v3 with cold-start, runtime-ready, heap, protected-call latency, concurrent throughput, artifact-size, command-timing, and baseline-regression measurements.
- A no-cache local distribution server for repeatable browser benchmarks.
- Checked-in runtime benchmark budgets and a standalone benchmark tool regression test.
- Browser-enabled release closure now benchmarks the NOVA TRADE protected `assessOrder` API and records hashable JSON evidence.
- A detailed runtime performance benchmarking guide.

### Changed

- Runtime performance claims are explicitly bound to the measured fixture, payload, browser, hardware, and build profile.

## 1.54.0

### Build performance

- Added automatic `sccache`/`ccache` compiler-launcher discovery with strict explicit modes.
- Added an optional compiler precompiled header containing only stable standard-library dependencies.
- Enabled MSVC `/MP` target compilation by default.
- Separated QuickJS IPO/LTO from Venom IPO so contributor builds avoid unnecessary whole-program optimization cost.
- Added a reproducible clean/no-op/incremental build performance reporter and CTest contract.
- Added a fast cached development preset and build-performance documentation.

## 1.53.0

### Added

- Playwright source-versus-distribution browser equivalence runner.
- Hash-bound compatibility evidence reports for source trees, Venom distributions, and scenario manifests.
- Release-closure integration for the complete compatibility corpus through `-BrowserRuntimeTests`.
- Browser equivalence documentation and CTest smoke coverage.

### Changed

- Compatibility qualification now distinguishes expected-behavior validation from direct original-versus-protected equivalence.

## 1.52.0

### Added

- Rebuilt the root README as a publication-grade product landing page with layered security positioning, architecture diagrams, quick-start integration, production layout, release closure, examples, and explicit limitations.
- Added a structured documentation hub covering installation, existing-site integration, annotations, protected functions and modules, browser bridge APIs, routing, assets, debugging, architecture, security, compatibility, CLI reference, testing, and release operations.
- Rewrote the NOVA TRADE and Protected Chess guides as detailed flagship integration examples.
- Added governance, roadmap, citation metadata, compatibility/security issue templates, and a documentation style guide.
- Added an automated documentation gate for required files, local links, version consistency, example depth, and unsupported security claims.

### Changed

- Consolidated product-facing guidance around the current two-profile, real QuickJS/WASM, fail-closed runtime model.
- Strengthened security messaging to communicate Venom's multi-layer reverse-engineering resistance without overstating browser-side secrecy.

## 1.51.0

### Added

- Added a canonical cross-platform release-closure runner and Windows, batch, and shell wrappers.
- Added one-command clean configuration, warnings-as-errors build, complete CTest execution, production doctor checks, dev/prod builds for every public example, distribution analysis, production leak scanning, and local reproducible release packaging.
- Added a machine-readable per-step closure report with dedicated logs and fail-fast or keep-going execution modes.
- Added a static regression that prevents the platform wrappers and canonical release runner from drifting apart.

## 1.50.3

### Fixed

- Added the direct `<functional>` dependency required by `js_rewriting.cpp` after the JavaScript pipeline split.
- Restored MSVC Release builds that previously failed while resolving `std::function` in the dependency traversal logic.
- Added a portability smoke check that verifies standard-library facilities used by split compiler units have their owning headers included directly.

## 1.50.2

### Changed

- Organized compiler headers into `commands`, `core`, `pipeline`, and `services` directories matching their implementation units.
- Updated all internal includes, tests, CMake generation paths, and documentation to use responsibility-based header paths.
- Added a source-layout gate that rejects headers placed directly in `src/compiler/`.

## 1.50.1

### Changed

- Split release artifact discovery and package inspection helpers out of `security_analysis.cpp` into `security_artifact_inspection.cpp`.
- Kept release-policy evaluation focused while preserving the existing public security API and fail-closed checks.
- Added independent source-layout limits for the artifact-inspection and policy-analysis units.

## 1.50.0

### Changed

- Split release artifact analysis from CLI reporting and key management.
- Added `security_analysis.cpp` and an internal report contract, reducing the public security command unit to focused orchestration.
- Preserved release-check behavior while making package inspection independently testable and maintainable.

## 1.49.9

### Changed

- Split runtime metadata generation into focused core, audit/policy, and module/execution translation units.
- Reduced the largest runtime metadata source from 1,625 lines to three bounded implementation files.
- Updated static release contracts and source-layout enforcement for the new boundaries.

## 1.49.8

### Changed

- Split QuickJS bridge, execution-policy, replay/audit, module-runtime, and host-capability metadata generation out of `build.cpp` into `build_runtime_metadata.cpp`.
- Reduced the main build orchestrator from 2,393 lines to 794 lines while preserving the public `compiler/pipeline/build.hpp` API.
- Updated path-sensitive release contracts and source-layout enforcement for the new internal runtime-metadata boundary.

## 1.49.7

### Changed

- Split package policy, layout, integrity, binding, lazy-section, and diversification metadata out of the main build orchestrator into `build_package_metadata.cpp`.
- Reduced `src/compiler/pipeline/build.cpp` from 2,953 lines to roughly 2,400 lines while preserving the public build API.
- Updated path-sensitive release contracts to inspect the new internal package-metadata boundary.

## 1.49.6

### Changed

- Split protected-module transformation and function bridge rewriting out of `js.cpp` into `js_rewriting.cpp`.
- Kept the public JavaScript compiler API stable while making rewrite policy independently testable and maintainable.

## 1.49.5

### Changed

- Split function-realm planning, extraction analysis, and bridge-contract reporting from `js.cpp` into `js_planning.cpp`.
- Added an internal `js_planning.hpp` contract while preserving the public `compiler/pipeline/js.hpp` API.
- Updated protected-argument regression coverage to validate the current JSON-value bridge protocol and worker-side argument limits.

## 1.49.4

### Changed

- Split JavaScript and module discovery out of `src/compiler/pipeline/js.cpp` into `js_discovery.cpp`.
- Isolated HTML script extraction, browser-runtime evidence, modulepreload handling, recursive module graph collection, and classic browser-realm closure behind an internal compiler interface.
- Preserved the public `compiler/pipeline/js.hpp` API while reducing the primary JavaScript pipeline translation unit.

## 1.49.3

### Changed

- Split release-build support concerns out of `src/compiler/pipeline/build.cpp` into `build_support.cpp`.
- Added an explicit internal `compiler/pipeline/build_support.hpp` interface for file I/O, runtime provenance checks, WASM export compaction, JavaScript hardening, and route-shell emission.
- Reduced the primary build pipeline translation unit while preserving the existing public compiler API.

### Validation

- Rebuilt and linked the complete `venom_core` target after the split.
- Re-ran source-layout, repository-consistency, route-provenance, and final release gates.

## 1.49.2

### Changed

- Extracted the 4,000-line browser runtime from a C++ raw string into `src/runtime/templates/runtime.js`.
- Added deterministic build-time embedding through `tools/embed_runtime_template.py`.
- Reduced `src/generated/runtime/runtime_js.cpp` to the small runtime assembly and marker-substitution layer.
- Made Python 3.10 an explicit configure-time requirement because runtime source generation is part of the canonical build.

## 1.49.0

### Security
- Made the tagged release set Ed25519-signed and fail-closed when protected signing keys are unavailable.
- Made the primary release workflow the sole tag-triggered publication authority.
- Added release workflow concurrency and least-privilege default permissions.

### Changed
- Moved the release-hardening workflow to manual execution only.
- Added an explicit `--require-signature` contract to release-set packaging and regression coverage.
- Updated release documentation and version references.

## Earlier changes

## 1.48.9

### Fixed
- Added artifact-derived provenance for the embedded package/route WASM runtime, including SHA-256, exports, ABI, and DOM command contract verification.
- Corrected the Route VM trust boundary: `VPOL0010-v2-fixed16` is verified in the protected package plan rather than falsely claimed as a WASM literal.
- Added browser-style implied end-tag handling for lists, paragraphs, options, and table structures.
- Expanded protected route hydration regression coverage for malformed and omitted-end-tag HTML.

## 1.48.5

- Fixed unbalanced HTML close tags producing `WASM DOM leaveElement tried to pop root` during large-page boot.
- The HTML compiler now tracks open elements, ignores unmatched closing tags like a browser, closes implicitly omitted tags, and never emits a route-level root underflow.
- Added a DOM stack-balance regression and validated a full NOVA TRADE development build.

- Fixed the no-argument `build-quickjs-wasm.ps1` path: it now verifies the embedded release runtime first and only requires `-Artifact` when a contributor is replacing that runtime.
- Added automatic discovery of generated QuickJS/WASM artifacts in standard build locations.
- Added one-command build-and-serve launchers for Protected Chess and NOVA TRADE on Windows, Linux, and macOS.
- Each example uses an isolated output directory and configurable profile/port.

## 1.48.0

- Added NOVA TRADE as a second flagship example with protected order-risk, portfolio analytics, and signal generation modules.
- Split the standalone trading-terminal template into maintainable HTML, CSS, browser JavaScript, and protected-module source files.
- Fixed remote script vendoring for redirecting HTTPS CDNs by allowing up to eight HTTPS-only redirects while retaining lock/SRI verification.
- Added regressions for the two-example repository policy and redirect-safe curl invocation.

- Added `venom update check|install|status|rollback`.
- Added stable and preview release channels.
- Added SHA-256 package verification and optional Ed25519 release-set verification.
- Added stable-contract upgrade checks before installation.
- Added atomic installation history and rollback support.
- Added update-management documentation.

## 1.46.0

- Added `venom new` and `venom init` project scaffolding.
- Added embedded QuickJS/WASM runtime install, verify, update, and path commands.
- Added `venom config validate` and `venom config print`.
- Added stable CLI help and production-oriented project templates.

## 1.45.0

### Installable platform releases and stable contracts

- Added platform-normalized binary packages with `bin/`, `runtime/`, `licenses/`, and installer entry points.
- Added machine-readable `CONTRACTS.json` to every release.
- Added contract-upgrade validation that rejects accidental ABI/schema regressions.
- Fixed the release installer command-line argument mismatch and added `verify` support.
- Added deterministic target triplets for Windows, Linux, and macOS archives.
- Added release-package smoke tests for install, status, uninstall, and contract compatibility.

- Added a canonical compatibility-suite manifest and one-command runner.
- Added generated JSON and Markdown browser compatibility matrices.
- Added support-tier promotion summaries and framework/version reporting.
- Added a compatibility-report regression test and release artifact.
- Documented evidence scope and claim limitations.


- Added optional typed input/output contracts for protected-module exports.
- Enforced contracts inside QuickJS for both synchronous and Promise results.
- Added early development-facade validation and generated TypeScript declarations.
- Added contract compiler diagnostics and regression coverage.

## 1.43.0

- Added `venom analyze-dist` for deployable bundle composition and hygiene analysis.
- Added text and JSON reports for size categories, largest assets, duplicate bytes, loose root assets, and known production leak markers.
- Added a CI-friendly nonzero result when a distribution requires review.
- Added end-to-end distribution analyzer regression coverage.

## 1.42.0

- Replaced metadata-only development cache keys with SHA-256 content-addressed source and compiler identities.
- Added debounced source watching without the duplicate startup rebuild.
- Added staging-directory builds and atomic output swaps.
- Failed rebuilds now keep serving the last successful distribution.
- Replaced fixed-interval live-reload polling with Server-Sent Events and a polling fallback.
- Expanded `/__venom_status` with build state, duration, changed files, and diagnostics.
- Added regression coverage for same-size/same-timestamp source changes, cache validity, atomic replacement, and failed-build preservation.

## 1.40.0

- Replaced regex-only protected-module discovery with a syntax-aware lexical parser that ignores comments and string/template contents and balances nested parameter syntax.
- Added static relative protected-to-protected imports with named and namespace bindings.
- Added a topologically ordered protected module graph with explicit cycle and missing-dependency diagnostics.
- Wrapped every protected module in an independent lexical closure to prevent private binding collisions.
- Kept dependency-module exports private; only browser entry-module exports are registered on the public protected bridge.
- Added diagnostics `VENOM-E2205` through `VENOM-E2216` for dynamic imports, malformed imports, missing dependencies, unsupported import forms, and cycles.
- Expanded the protected-module smoke suite with private dependency exports, duplicate private helper names, default parameters, cycle rejection, and dynamic-import rejection.

## 1.39.0

- Added file-level `// @venom: protected module` compilation.
- Named function exports receive generated browser module facades and protected QuickJS registry entries.
- Added fail-closed `VENOM-E2201` through `VENOM-E2204` diagnostics for unsupported imports, export forms, missing exports, and duplicate public names.
- Added a protected-module fixture and end-to-end extraction smoke test.
- Added restart-safe development build caching keyed by source-tree digest and compiler executable identity.
- Expanded the primary README with protected-module architecture, API usage, and live-development guidance.

- Added `venom dev` with source watching, automatic development rebuilds, a local HTTP server, and browser live reload.
- Expanded `venom doctor` with actionable native toolchain, Node/npm, hardener, QuickJS/WASM, Emscripten, and repository checks.
- Added the portable `tools/dev_server.py` development runtime.
- Updated product and example documentation for the daily development workflow.

- Replaced the historical build-profile surface with exactly two profiles: `dev` and `prod`.
- Kept the real QuickJS/WASM protected execution path in both profiles to prevent development/production behavior drift.
- Made `dev` readable and diagnostic while keeping protected export execution fail-closed.
- Made `prod` the only hardened deployment profile with hashed assets, AST JavaScript hardening, metadata stripping, diversification, and leak scanning.
- Removed `venom.browser.json` and vendor notice text files from production distributions and protected packages.
- Rewrote the root README and Protected Chess Lab README with detailed architecture maps, API guidance, build flows, layouts, security expectations, and troubleshooting.
- Updated scripts and configuration documentation for the simplified profile model.

## 1.36.5

- Removed `venom.browser.json` from deployable assets and protected packages; it is browser-test metadata only.
- Relocated third-party attribution to `assets/app/third-party-notices.txt` in production distributions.
- Added a clean production asset-layout regression test.

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
