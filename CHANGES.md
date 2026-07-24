# Changelog

## 3.0.0 — Runtime include and example certification fix

- Fixed `venom_runtime_native`, `venom_runtime_probe`, and `venom_tjs_native_probe` after header colocation by exposing the repository `src` root instead of the removed top-level `include` directory.
- Strengthened the header-layout architecture gate so stale `PUBLIC include` usage and missing runtime source-root propagation fail immediately.
- Added the missing React/Vite showcase Venom manifest and corrected the React 16 fixture smoke-test location.
- Regenerated the React/Vite example launchers and updated Chrome extension tests for hardened generated adapters and the current offscreen runtime-host model.
- Rebuilt and verified the native compiler plus all native examples; React/Vite framework compatibility remains covered by the registered production-ingestion tests.

### Example-local distribution outputs

### Public browser JavaScript hardening policy

- Harden browser-selected and otherwise public JavaScript across all build targets using the native runtime hardener.
- Enable the policy by default and add `--harden-public-js` / `--no-harden-public-js` plus `build.harden_public_javascript` configuration.
- Keep core runtime JavaScript hardening mandatory even when public JavaScript hardening is disabled.

- All shared example launchers now write protected output to `examples/<example>/dist-venom` instead of creating `dist-<example>-prod` directories at the repository root.
- Chrome extension packaging now defaults to `examples/chrome-extension/dist-venom`.
- Updated example documentation and added a regression that rejects repository-root example distributions.


## 3.0.0
### Colocated source headers

- Removed the top-level `include/` tree and moved every first-party header beside its corresponding `.cpp` or `.c` implementation under `src/<domain>/`.
- Switched project includes from `venom/<domain>/...` to source-root-relative paths such as `package/reader.hpp`.
- Updated CMake, generators, diagnostics, contracts, tests, and documentation for the colocated layout.
- Strengthened the header-layout gate to reject future top-level or nested include trees.


- Fixed Vite compiler discovery so repository-local Venom binaries are found from nested examples and applications; the React launcher now builds the native compiler and passes its exact path through `VENOM_BIN`.

- Fixed local React/Vite showcase package resolution by preserving `file:` dependency symlinks, allowing `@venom/react` to resolve `@venom/runtime` from the example project dependency tree.
- Fixed React/Vite production protection timing by ingesting compiled output from Vite `writeBundle`, after `dist` has been written, instead of `closeBundle`.

- Split hardener-cache ownership out of `src/pipeline/build_support.cpp` into `build_hardener_cache.cpp`, restoring the source-layout gate while preserving cache integrity, concurrency, and hardening behavior.

- Added first-class TurboJS runtime integration and synchronized ABI/package verification.
- Added React and TSX compatibility through Vite production-output ingestion and `@venom/react`.
- Added zero-config `@venom/venom` project detection, guided setup, local compiler discovery, toolchain locking, and `venom dev`.
- Added protected Manifest V3 Chrome extension compilation with generated host adapters.
- Consolidated the primary 3.0 release highlights into one README section.


## Developer watch workflow

- Added `venom dev` for portable, framework-agnostic continuous protected rebuilds.
- Added debounced polling with generated-output, cache, dependency, and toolchain-directory exclusions.
- Added `--interval`, `--debounce`, and `--once` controls.
- Added a CMake-registered integration regression for the development workflow.
- Updated `@venom/venom` to 2.1.0.

## Venom CLI naming cleanup

- Renamed the zero-config package to `@venom/venom`.
- Renamed the CLI to the single `venom` executable.
- Removed the former command names completely; no compatibility aliases are shipped.
- Renamed package directories, integration tests, CMake registrations, documentation, environment coordination, and schema identifiers to remove the old `auto` branding.
- Updated `@venom/venom` to 2.0.0 for the breaking command/package rename.


## Project toolchain lock

- Added reproducible compiler pinning to `@venom/venom` 1.4.0.
- `venom setup` records the exact compiler version and SHA-256 in `.venom/toolchain.lock.json`.
- Added `venom lock` for intentional lock creation or refresh.
- `protect` now fails closed when the selected compiler does not match the project lock.
- `doctor` reports toolchain drift before framework builds begin.
- Added toolchain lock creation, mismatch, and refresh regression coverage.

## React SDK integration pass
- Standardized the public package scope as `@venom/*` across the runtime SDK, Vite integration, React hooks, generated clients, examples, contracts, tests, and documentation.

- Added `@venom/react` with Strict Mode-safe hooks for runtime readiness, protected calls, cancellation, reset, and protected export preloading.
- Added typed hook factories for applications with known protected export sets.
- Updated the React + Vite showcase to display live Venom runtime readiness through the React SDK.
- Added package contract and Node import regressions for the React integration layer.


## React + Vite production ingestion

- Added React project detection to `@venom/vite`.
- Production Vite builds are now protected after JSX/TSX transformation by ingesting Vite's completed output directory.
- Added explicit `auto`, `source`, and `vite-output` plugin modes.
- Added a React 19 + TypeScript + Vite showcase and regression coverage.
- Preserved the existing source-tree workflow for development and legacy integrations.

## Chrome service-worker receiver startup fix

- Added bounded retries for transient Manifest V3 `Could not establish connection` / `Receiving end does not exist` errors while the module service worker initializes.
- Applied the retry handshake to popup lifecycle messages and protected engine-analysis requests from the content controller.
- Preserved fail-closed behavior for permanent messaging, permission, and runtime errors.
- Verified the generated Venom service-worker wrapper imports the emitted authored background module from the correct extension-shell location.

## Chrome extension API boolean preservation

- Disabled Terser `booleans_as_integers` for `extension-*` hardener inputs.
- Chrome extension API schemas receive real booleans instead of numeric `1`/`0` values after hardening.
- Added a regression guard covering the extension hardener policy.

## Chrome extension route-hydration ordering

- Delayed generated extension page adapters until the protected HTML route has fully hydrated.
- Distinguished worker/export readiness (`venom.ready()`) from application readiness (`venom:boot-ready`).
- Prevented popup modules from attaching DOM listeners while the temporary `venom-root` shell is still active.
- Added a regression requiring the full application-readiness handshake before loading authored Chrome adapters.

## TurboJS ABI fingerprint synchronization

- Replaced the stale hardcoded 16-export package fingerprint with a fingerprint derived from `contracts/turbojs-wasm-abi.json` through the generated ABI contract header.
- Kept the package-side FNV-1a canonicalization byte-for-byte aligned with the browser TurboJS engine validator.
- Updated browser fingerprint parsing for the current 24-export release ABI.
- Protected packages and the emitted TurboJS/WASM runtime can no longer drift silently after ABI export changes.
## Unreleased


### Chrome protected-route SRI discovery

- Fixed package integrity verification for Manifest V3 builds whose protected bootstrap keeps a stable manifest route such as `popup.html`.
- Added filename-independent discovery of integrity-bound protected HTML routes, covering popup, options, side-panel, and other extension pages without reverting to the former public `assets/extension` pipeline.
- Fixed Manifest V3 engine-page emission so extension-page runtime hosts do not reference offscreen-only `venom-extension-host.js`, and protected `engine-host.js` modules are not reintroduced as missing public scripts. Added protected-host emission regression coverage and verified Chrome Web Store readiness for the fully protected extension example.
- Isolated heavy JavaScript hardening (worker, runtime, engine, loader, and very large assets) in a dedicated `venom_hardener_worker` process so native TurboJS faults cannot terminate the compiler. The compiler retries one failed worker invocation, validates a length-delimited response envelope, records isolation/retry/failure telemetry, and ships the helper with packaged and installed releases. Added normal and forced-retry hardener stress coverage.


## Bootstrap loader hardener isolation

- Isolated bootstrap-loader hardening in a fresh TurboJS runtime realm.
- Routed loader assets through the deterministic conservative browser-no-eval profile, avoiding seed-sensitive native access violations during build step 14.
- Extended the native hardener lifecycle regression to require conservative handling for both loader and worker assets.
## Chess piece asset template fix

- Fixed regex-free dynamic asset matching for placeholders with literal suffixes such as `{piece}.png`.
- Restored discovery, hashing, emission, and browser remapping of all protected-chess piece images.
- Extended the dynamic browser asset regression to lock the embedded-placeholder matcher behavior.

## TJS heavy-asset hardener isolation

- Isolated worker, runtime, engine, and very large JavaScript assets in fresh embedded TurboJS realms before hardening.
- Routed worker and very large assets directly through the deterministic conservative obfuscation profile, preventing native access violations that occur before JavaScript-level retry handling.
- Preserved production minification, mangling, seeded identifier obfuscation, RC4 string-array encoding, and browser no-eval compatibility.
- Extended the native hardener stress regression to lock the isolated lifecycle and conservative-worker behavior.

## Hardener cache concurrent publication

- Made hardener cache publication safe across simultaneous Venom processes by using unique same-directory temporary files instead of a shared `.tmp` path.
- Preserved atomic publication and reused a valid cache entry when another process wins the publish race.
- Added cache race telemetry to private performance reports and build summaries.
- Extended the hardener cache regression to launch concurrent publisher processes, verify the resulting entry, and reject leftover temporary files.

## TJS string-key collection performance pass

- Cached content hashes for stable non-atom strings used repeatedly as Map and Set keys.
- Extended the native collection benchmark with string-key and object-key lookup profiles.
- Added a string collection-key regression covering identity lookup, equal dynamic strings, Set membership, and later property-key atomization.
- Rejected an object-pointer hash rewrite after it failed to show a reliable benchmark improvement.


## Stabilization pass: TJS ABI ownership and protected runtime regression lock

- Removed the accidental root-level `-` ABI export manifest.
- Made the authoritative TurboJS/WASM ABI lock current with `contracts/turbojs-wasm-abi.json`.
- Added contract checks that reject stray top-level ABI export manifests and non-`venom_tjs_` required exports.
- Strengthened the authoritative ABI smoke test with TJS namespace and source-tree ownership checks.
- Fixed signed-byte conversion warnings in generated worker sealing and package section-name validation.
- Verified the TJS bytecode marker, lazy protected registry activation, embedded WASM export contract, and native bytecode security tests.
- Completed a clean Clang Release build and a full 18-stage protected-chess production build.
- Documented the remaining Linux GCC-only step-6 asset-processing crash as a portability issue; the supported Windows/Clang path passes.


- Added a dependency-free `src/base/` domain with a typed `venom::Error`, stable exit-code taxonomy, and centralized phase-aware error rendering; all authored C++ fatal paths now use the shared API.
- Replaced duplicated example metadata with `VENOM_EXAMPLE_REGISTRY_V2`, generated symmetric launchers for all ten examples, and added the previously missing Vite launcher.
- Added reusable `tools/venom_tools/` helpers plus CI gates for registry integrity, launcher drift, forbidden legacy error paths, and generated runtime snapshot drift.
- Centralized Windows command completion and pause behavior under `tools/windows-scripts/internal/`, removed duplicated launcher error blocks, and added symmetric release-closure entrypoints.
- Rebound structural regressions to the new domain-owned files, generated worker template split, hash-only package naming, and canonical TurboJS/WASM tool locations.
- Enforced first-party source domains with independent CMake object targets, removed the catch-all `src/compiler/` hierarchy, extracted shared `core`, `graph`, and `remote` ownership, and added an acyclic cross-domain include checker.
- Fixed native runtime parity with the canonical route-table format, added native package execution and TurboJS parity CTest gates, and updated the runtime probe for hash-only production package names.

### Source hygiene and semantic launchers

- Consolidated the Vite integration under `packages/` and colocated fuzz targets with their corpora under `tests/fuzz/`.
- Promoted the CLI, build pipeline, language frontends, TurboJS integration, route VM, shared core, module graph, and remote acquisition into first-class `src/` domains; the obsolete `src/compiler/` umbrella was removed.
- Flattened native runtime translation units directly into `src/runtime/` and centralized every authored JavaScript input under `src/templates/`.
- Isolated generated JavaScript and metadata under explicit `src/generated/` ownership folders.
- Grouped runtime and TypeScript generators under `tools/generators/`.
- Replaced numbered example scripts with semantic, cross-platform launcher names backed by `tools/launch_example.py`.
- Added source-layout, launcher-parity, and root-consolidation enforcement tests.


### Release security hardening

- Reject archive symlinks, hard links, devices, FIFOs, traversal, and drive-qualified entries during release verification.
- Enforce portable, unique release-manifest paths with non-negative sizes.
- Validate VBC section names symmetrically in the package writer and reader.
- Add a malicious archive and manifest regression corpus.
## 2.0.0 — Stable production runtime

Venom 2.0.0 consolidates the protected compiler, TurboJS/WASM runtime, typed SDK, production-only build path, example and browser certification, startup observability, and signed release engineering into one stable release.

- Added early immutable-asset preload hints for the loader, runtime, package, TurboJS/WASM module, and worker.
- Added contract-backed median and p95 startup budgets and a report summarizer.
- Preserved the no-persistent-plaintext-cache security rule.

- Added immutable per-phase browser startup timing and `venom:boot-phase` events.
- Added stable `VENOM_BOOT_*` startup error classification.
- Added explicit full-document retry through `__venomRetryBoot()` to avoid reusing partial runtime state.
- Added the versioned runtime-startup contract and executable regression.

- Added authoritative boot-complete telemetry that becomes ready only after package loading, browser module linking, route execution, and script startup finish.
- Expanded real-browser certification to all eight examples across Chromium, Firefox, and WebKit.
- Added structured browser startup failure evidence and UTF-8 Playground qualification.

- Added an authoritative all-example certification contract and production-only certification command.
- Removed remaining stale dev-profile references from project scaffolding, Vite tests, AST tests, release certification, and API-server command validation.
- All certified examples use the same production TurboJS/WASM build, verification, and leak-scan path.

- Fixed protected TypeScript modules losing their `protected module` identity after structural transformation by storing the identity as persistent compiler chunk metadata.
- Protected browser imports such as `../protected/pricing` are now lowered before canonical browser graph serialization even when source comments are erased.
- Removed the `dev` build profile. `venom dev` remains available as a watch/serve workflow but now builds through the same production-grade `prod` path.

- Browser imports of protected modules are now lowered directly to capability-bound bridge functions during compilation.
- Removed protected module specifiers from the emitted browser module graph, eliminating lazy-section and opaque-ID dependency failures such as `../protected/pricing`.
- Added an exact compiler regression and artifact-level verification for the TSX Showcase protected pricing import.

- Fixed final lazy-bundle identity drift by embedding a normalized authored module identity in every browser module and resolving opaque modules through a fail-closed alias registry.

- Fixed production lazy-route script sections so cross-route browser module dependencies, including generated protected facades such as `../protected/pricing`, are physically included in every route section that may link them.
- Fixed generated protected facades being omitted from browser module linking when imported through paths such as `../protected/pricing`.
- Fixed Windows Playground compilation corrupting non-ASCII source through the active console code page; compiler input is now always UTF-8 bytes.

- Fixed browser-to-protected facade linking for relative imports such as `../protected/pricing`, including dependency chunks carrying route aliases.
- Approved the known Emscripten lifecycle exports `_initialize` and `__cxa_increment_exception_refcount` with strict function-kind validation.
- Expanded the README v2 release notes with the stable SDK, TypeScript/TSX, Vite, diagnostics, certification, release-engineering, portability, and compatibility work.

- Added the stable `@venom/runtime` SDK and authoritative runtime API contract.
- Added idempotent initialization, typed protected calls, batching, export preloading, lifecycle status, explicit disposal, and structured `VenomRuntimeError` failures.
- Added generated `venom-client.js` and `venom-client.d.ts` bindings for protected exports.
- Added public runtime readiness/error events and versioned runtime API metadata.
- Fixed C++17 portability issues reported by Windows Clang builds.

## 1.8.1 - Runtime terminology

- Standardized Venom terminology on browser runtime, protected runtime, runtime planning, runtime ownership, and runtime boundaries.
- Renamed diagnostics, report fields, planner identifiers, tests, documentation, and editor messages to remove the retired terminology.
- Updated the explain and LSP schemas to publish `runtime` fields consistently.

- Added a dependency-free LSP server with diagnostics, hover information, and runtime quick fixes.
- Added command-line module graph queries for health, dependencies, dependents, boundaries, and shortest paths.
- Added editor integration documentation and executable regressions.

## 1.7.0 - Observability and diagnostics

- Added contract-backed explain and diagnostics reports with module runtime, dependency, build-evidence, and cache analysis.

## 1.6.0 - Full release certification

- Added the versioned `VENOM_RELEASE_CERTIFICATION_V1` contract for required platforms, browsers, examples, static gates, and public compatibility claims.
- Added `tools/certify_release.py` to build flagship examples, verify real TurboJS/WASM execution, run production leak scans, execute static gates, and emit hashed JSON/Markdown evidence.
- Added `tools/aggregate_certification.py` to fail closed unless Linux x64, Windows x64, macOS arm64, Chromium, Firefox, and WebKit evidence is complete and passing.
- Added a cross-platform GitHub Actions certification matrix and all-browser Playwright qualification.
- Extended the browser E2E runner with machine-readable certification output.
- Added release-certification and aggregation regressions plus complete operator documentation.

## 1.5.0 - Production release engineering

- Expanded CycloneDX inventory across all vendored dependencies with deterministic directory hashes and license classification.
- Upgraded SLSA provenance to cover the compiler, embedded runtime, contract manifest, and toolchain lock.
- Added machine-readable `RELEASE_POLICY.json` with target, ABI, security, channel, sequence, and source-revision constraints.
- Strengthened release verification to validate provenance subject hashes and fail closed on policy drift.
- Fixed the standalone Ed25519 signing command's release-directory argument handling.

## 1.4.0 - Vite and framework ecosystem integration

- Added the first-party `@venom/vite` plugin with serialized Venom builds, content-digest incremental rebuilds, debounced hot-update integration, TypeScript declarations, a virtual status module, and a no-cache development status endpoint.
- Added a Venom + Vite TypeScript showcase with a protected TurboJS/WASM service boundary.
- Added representative React, Vue, and Svelte source-graph qualification fixtures alongside the existing compiled-output framework corpus.
- Added executable plugin hook qualification using an isolated fake Venom compiler, including unchanged-build suppression and status endpoint validation.
- Documented the supported integration model: framework tooling owns transforms and HMR; Venom owns protected packaging and TurboJS/WASM execution.

## 1.3.2 - TypeScript browser runtime preservation

- Preserve file-scope `@venom: browser` and `@venom: protected` directives from authored TypeScript/TSX before structural lowering can remove comments.
- Classify discovered TypeScript module dependencies before transpilation, keeping browser dependency closure and opaque module maps complete.
- Add an Aegis Operations regression that verifies `browser/data/mock-data.ts` and other explicitly browser-owned TypeScript dependencies are never packaged as protected whole-file chunks.
- Restore Example 6 browser module resolution for extensionless imports such as `./data/mock-data`.

## 1.3.1 - Build performance and incremental compilation

- Added content-addressed structural TypeScript/TSX frontend caching.
- Added cache hit/miss/write telemetry for TypeScript, TurboJS bytecode, and JavaScript hardening.
- Added private per-phase `build-performance.json` reports, including quiet builds.
- Made development bridge identities project-stable so unchanged TurboJS bytecode is reusable.
- Made runtime generators publish only when content changes.
- Added a cold/warm benchmark driver and incremental-cache regression.
- Corrected protected TypeScript module validation to reuse the already-validated registry bundle for imported modules.

## 1.2.3 development

- Modularized the authored browser runtime and TurboJS engine wrapper into generated, independently testable JavaScript modules.

- Fixed JavaScript Playground completion values by evaluating pasted code with TurboJS script semantics, so the final expression is returned without requiring an explicit top-level `return`.

- Fixed JavaScript Playground result rendering to unwrap the authoritative TurboJS bridge envelope before nullable execution summaries, preserving objects, arrays, `null`, and `undefined` distinctly.
- Added a native end-to-end bridge regression for the exact playground async wrapper and result object.


- Fixed the JavaScript Playground development engine to validate the current minimal bytecode-only TurboJS/WASM export surface. Hardened development builds no longer require obsolete scaffold exports such as `venom_tjs_engine_abi`, `venom_tjs_execute_source`, or `venom_tjs_wasm_native_stack_capacity`; they validate the shared minimal ABI and `venom_tjs_bridge_abi` instead.
### TurboJS development trust-domain handoff

- Separated hardened asset generation from the release trust-domain flag.
- Development TurboJS playground builds now emit `protectedRelease = false` while retaining fail-closed WASM execution and denied host-source fallback.
- Valid `development-turbojs-compiler` bytecode handoffs are accepted only in development packages; production and `--strict-release` builds remain package-decoder-only.
- Added a regression preventing hardened development builds from being mislabeled as release trust domains.

## Development package contract boundary

- Development builds remain runtime-hardened, integrity-checked, fail-closed, and host-fallback denied without being mislabeled as release packages.
- Release-only `VRDIV003` diversification and TurboJS ABI fingerprint contracts are now required by the package release flag rather than by the independent fail-closed policy.
- `--strict-release` remains available for explicitly promoting a development-profile build to release-contract enforcement.
- Added regression coverage for the JavaScript Playground development package and preserved production package/runtime verification.
- Fixed development hardened-layout builds emitting empty asset names such as `assets/javascript/.js`; role-redacted assets now always use content-derived filenames so SRI remains valid.

## Package section planning consolidation

- Moved VBC section construction out of `build.cpp` into `src/compiler/planning/section_plan.*`.
- Added a typed section-planning input/result boundary for runtime contracts, routes, protected registries, assets, and security metadata.
- Kept `PackagePlan` as the only package writer boundary for development and production builds.
- Reduced `build.cpp` to build orchestration and artifact emission.

## Canonical module graph consolidation

- Added `src/compiler/graph/module_graph.*` as the authoritative module identity and resolution layer.
- Unified general and lazy-route browser module maps around the same graph-owned opaque IDs.
- Removed duplicate extension resolution and module-map generation from package serializers.
- Routed planner dependency resolution through the shared JavaScript/TypeScript candidate resolver.
- Added a direct canonical graph regression and qualified the Aegis production build.

### Canonical package plan

- Added `src/pipeline/planning/package_plan.*` as the single package writer boundary.
- Centralized section ownership, polymorphic ordering, compression/encryption policy, VBC emission, and immediate read-back verification.
- Removed direct low-level `package::Writer` configuration from the build orchestrator.
- Added `package_plan_smoke` and architecture documentation.

## Architecture consolidation

- Restored strict source-layout and documentation gates without increasing size limits.
- Split build protection reporting into `build_report.cpp`.
- Split JavaScript bundle encoding and opaque module-map generation into `js_bundle_encoding.cpp`.
- Moved the generated worker runtime body into a dedicated generated include so its C++ wrapper remains reviewable.
- Moved authoritative machine-readable inputs to the root `contracts/` directory.
- Reorganized AST, module-linking, console, and TypeScript documents into the supported architecture, operations, and reference sections.
- Added the Aegis Operations screenshot to the documentation asset tree so it does not inflate production application packages.

## TSX async bridge and project-aware build log

- Fixed the TSX showcase to await protected TurboJS/WASM exports before passing results into components.
- Added project-specific build banners and a meaningful detail line for every build phase.
- Clarified inline versus lazy protected execution records in build output.


## 1.2.0 - Production browser module route-map fix

### Fixed

- Classify recursively discovered ES module dependencies with the same browser/protected runtime rules as top-level scripts. This keeps browser-only dependencies such as the TSX showcase `./jsx-runtime` inside the browser route bundle while preserving explicitly protected modules as generated facades.
- Prevent production runtime failures reporting `browser module dependency not found` for local browser dependencies discovered after the entry module.

### Native JSX/TSX frontend

- Added classic JSX lowering to `React.createElement`.
- Added fragments, nested elements, expression children, component tags and spread-compatible attributes.
- Added the `examples/tsx-showcase` protected TypeScript integration example.
- Added TSX source-leak regression coverage.

- Added the verified TypeScript AST showcase screenshot to the example and main project documentation.

- Fixed the lazy per-route JavaScript package serializer so browser ES modules receive the compiler-authored specifier-to-opaque-module map in production packages.
- Added a native regression that decodes the actual `VJSB0006` route bundle and verifies extensionless protected and browser imports are mapped before packaging.
- Kept runtime alias fallback fail-closed; the runtime no longer needs to guess around missing compiler metadata.
## Unreleased
### Browser module linker opaque-alias correction

- Parse compiler-authored browser module maps even when generated prefixes precede them.
- Resolve a relative specifier through a uniquely matching package module map when production packaging changes the runtime referrer alias.
- Preserve fail-closed behavior when multiple maps disagree about the same specifier.
- Expanded the browser module linker smoke test to reproduce the opaque referrer mismatch.


- Fixed browser ES modules executed from generated `blob:` URLs so relative imports are linked to absolute generated module URLs before execution. This covers TypeScript extensionless imports, ordinary browser dependencies, and generated protected-module facades.
- Added a browser module linker regression that verifies `../protected/pricing` and `./format` are both resolved without shipping source files.

## TypeScript AST showcase and source-safety pass

- Added `examples/typescript-showcase`, a complete typed browser/protected application.
- Added TypeScript generic call/declaration type-argument erasure.
- Made parameter annotation erasure syntax-aware so call arguments and object literals are preserved.
- Made declaration-modifier erasure context-sensitive so runtime properties such as `value.protected` remain intact.
- Prevented JavaScript and TypeScript compiler inputs from being copied into public static assets.
- Added a source-leak regression and generated protected API declaration checks.
- Added example 4 launchers for Windows and Linux.

## Unreleased — AST re-export and default-export closure

- Added AST-derived local export aliases, explicit re-exports, default exports, namespace re-exports, and export-star metadata.
- Resolved named and default symbols through multi-hop barrel modules without collapsing safe imports to a module-wide runtime.
- Merged `export *` symbol sets while excluding default exports and marking conflicting runtime resolutions for review.
- Kept anonymous/default expression exports and unresolved re-export identities fail-safe under manual review.
- Expanded module-closure regression coverage for aliases, barrels, default imports, default re-exports, and export-star chains.

## Unreleased — AST symbol-level module closure

- Added AST-derived named/default/namespace import binding metadata.
- Propagated runtimes through the exact imported export instead of the imported file's worst runtime.
- Preserved conservative whole-module behavior for side-effect and namespace imports.
- Added mixed-module regression coverage for safe and browser-only exports, aliases, cycles, and external boundaries.


## Unreleased — AST module dependency closure

- Extended the AST protection planner across relative ES module imports.
- Propagated browser and manual-review runtimes through transitive module dependencies.
- Added cycle-safe validation for protected circular module graphs.
- Added manual-review diagnostics for unresolved relative imports and external package boundaries.
- Added `javascript_ast_module_closure_smoke` regression coverage.


## AST call-graph and class capture pass

- Classified `const` arrow functions and function expressions as stable helper dependencies.
- Added direct dependency-call reporting to AST planner recommendations.
- Added class/constructor capture detection with fail-safe manual review.
- Expanded AST planner regression coverage for arrow helpers and class construction.

## Unreleased — AST semantic planning

- Classify browser host access expressed through `globalThis`, `self`, and `window` member paths.
- Treat dynamic `this` binding as a manual-review semantic boundary.
- Detect writes and updates to captured outer bindings during AST scope analysis.
- Preserve protected classification for safe destructuring and default-parameter functions.
- Expand AST planner regression coverage for host aliases, receiver semantics, closure mutation, and modern parameters.


### Hash-only asset routing fixes

- Fixed hardened runtime-WASM URL resolution so `assets/wasm/<hash>.wasm` is resolved from the `assets/` base instead of escaping to `/wasm/<hash>.wasm`.
- Hardened hash-only filename generation so empty asset-role stems can never produce dot-prefixed files such as `.<hash>.css`.
- Extended the production layout regression test to reject every hidden or dot-prefixed generated asset.
This changelog records public Venom releases and meaningful user-facing changes. Internal development passes, documentation polish, repository cleanup, and CI maintenance are intentionally not versioned separately.

Venom follows [Semantic Versioning](docs/operations/versioning.md).

## 1.1.0
- Added professional project, global build-history, build-detail/log, and artifact-management pages to the hosted dashboard.

- Added a professional authenticated dashboard, shared application shell, project overview, recent build activity, account status, responsive navigation, and unified workspace entry points.
- Added a one-command local website launcher that builds the API server, builds the compiler when missing, waits for health readiness, and opens the authenticated site.

- Moved authoritative machine-readable contracts from the repository root into `contracts/`; generated C++ bindings now live under `src/generated/contracts/`.
- Added API-server administrator roles and a protected operations console for global user, job, and audit visibility, including account disable/enable and role controls.

- Split lazy protected registries into candidate-routed module and function chunks with per-chunk integrity, preload selection, and activation caching.
- Added external hashed registry chunks with first-call SHA-256 verification, preload activation, and fail-closed runtime handling.
- Added deterministic, integrity-bound lazy protected-export activation manifests, preload policy configuration, and project IR/cache v10 as the foundation for independently fetched encrypted protected chunks.
- Made TurboJS/WASM ABI verification toolchain-aware for Emscripten reactor exports while keeping required, permitted, and unexpected exports independently reported and fail-closed.

- Hardened the Windows Emscripten pipeline against batch-file argument mangling and made Binaryen post-processing capability-aware across toolchain versions.

- Added authenticated raw `ArrayBuffer` and numeric typed-array transport for protected call inputs and results.

- Improved Emscripten toolchain compatibility by pinning the verified release SDK and compiling the browser TurboJS/WASM runtime from the minimal core source set without POSIX-only `turbojs-libc` helpers.

### Compiler and workflow upgrades

- Added `venom.batch()` for validated parallel protected-export calls with ordered results and independent capability leases, timeouts, cancellation, replay counters, and response validation.

- Added native TypeScript input for `.ts`, `.mts`, `.cts`, and non-JSX `.tsx` sources with line-preserving type erasure, extension-aware module resolution, source diagnostics, and project IR v8 metadata.

- Added typed input/output contracts for protected exports with protected-boundary validation, generated TypeScript declarations, machine-readable contract metadata, and project IR v7 integration.

- Added policy-controlled capability modules for fetch, timers, storage, and Web Crypto with `auto`, `allow`, `deny`, and restricted `read-only` modes.

- Replaced `venom analyze-dist <dist>` with `venom analyze <dist>` and `venom release-check <dist>` with `venom verify <dist>`, removing the former command names entirely.
- Organized production launchers under `scripts/windows/` and `scripts/linux/` with a minimal platform-specific script surface.
- Added a versioned internal project IR with deterministic project and protected-plan fingerprints.
- Added content-addressed incremental caching for native JavaScript hardening and compiled TurboJS bytecode, with `--no-cache` and `--cache-dir` controls.
- Added an embedded Terser AST frontend for syntax validation and precise static import, re-export, and dynamic literal import discovery without Node.js.
- Extended project IR v6 with parser-derived module metrics for nodes, functions, lexical scopes, global references, top-level declarations, imports, and exports.
- Replaced protected-module import/export text lexing with AST-derived statement boundaries, bindings, function signatures, and source-aware unsupported-syntax diagnostics.
- Replaced protected-function lexical-capture and dependency lifting heuristics with AST scope analysis, including nested scopes, destructured parameters, mutable-binding rejection, and precise browser-global detection.
- Replaced regex and balanced-brace protected-function extraction with AST-owned lowering for declarations, async functions, variable-bound arrows, function expressions, default parameters, and source-aware unsupported-form diagnostics.
- Added a unified source-aware diagnostic renderer with stable codes, file/line/column context, source excerpts, actionable remediation, and documentation links.
- Added a focused Python Playwright browser-runtime qualification gate that verifies real protected export execution and clean page, console, and network behavior without demo timing assertions.

## 1.0.0

### First public release

- Introduced Venom's hybrid browser and protected TurboJS/WebAssembly execution model.
- Added protected-by-default JavaScript planning with file- and function-level `@venom` execution directives.
- Added local protection analysis, manual runtime overrides, confidence reporting, and fail-closed production planning.
- Compiled protected JavaScript into TurboJS bytecode and packaged it in polymorphic `.vbc` containers with build-specific bytecode envelopes.
- Added worker-isolated protected execution, WebAssembly-owned streamed decoding, runtime-integrity seals, asset binding, and a private binary capability bridge.
- Added least-privilege host-capability manifests and capability-scoped protected-runtime facades.
- Embedded pinned Terser and javascript-obfuscator hardening directly into the native compiler through TurboJS, removing the external Node.js hardener requirement.
- Added deterministic production builds, release leakage scanning, tamper rejection, runtime provenance checks, and fail-closed release qualification.
- Added structured build phases, elapsed timings, useful build statistics, and `--verbose` / `--quiet` output controls.
- Added complete installation, integration, architecture, security, operations, CLI, and example documentation.
- Included Protected Chess, NOVA TRADE, and Venom Sentinel as complete reference applications.

### Security boundary

- Production distributions do not ship the exact original protected source and cannot reproduce it byte-for-byte.
- Venom raises the cost of reverse engineering, tampering, extraction, and reuse; it does not claim permanent secrecy against an analyst who controls the browser and operating environment.

### Final stabilization

- Fixed function-only candidate-routed lazy chunks by embedding the protected bridge value-codec bootstrap in every independently executable registry chunk. This resolves the protected-chess startup failure `TypeError: not a function` after the IR v12 chunk split.
- Added the separate `venom-api-server/` root project with the existing product website, login and signup, authenticated projects, safe website uploads, temporary workspaces, a controlled Venom terminal, streamed job logs, preview, and artifact downloads.

## Professional account and administration console

- Unified account and administrator pages with the authenticated application shell.
- Added real session inventory, current-device identification, per-session revocation, and sign-out-other-sessions workflow.
- Added account verification, retention, role, security checklist, and audit timeline views.
- Added searchable and filterable administrator user management.
- Added global job-status filtering, platform metrics, and administrator audit timeline.
- Added confirmation and error handling for sensitive account and role operations.

## Website accessibility, SEO, performance, and installability

- Added a public-site service worker that caches only the public shell and never caches API, health, or authenticated workspace responses.
- Expanded the web-app manifest with a stable app identity, scope, display fallbacks, categories, maskable icon support, and useful shortcuts.
- Added active-navigation semantics, keyboard-operable feature tabs, live-region announcements, stronger focus visibility, and reduced-motion behavior.
- Added explicit cache policies: authenticated/API responses are `no-store`, HTML/service-worker metadata is revalidated, and static assets use bounded stale-while-revalidate caching.
- Extended the website quality gate to verify descriptions, canonical URLs, manifests, heading structure, skip links, accessible navigation, service-worker presence, and manifest validity.
- Added Go regression tests for HTTP cache policy behavior.
## Product operations and feedback

- Added authenticated product feedback with category and length validation.
- Persisted feedback in the existing audit stream for administrator review.
- Added a no-store product-status endpoint for dashboard readiness reporting.
- Added structured release notes, maintenance notices, and a public changelog page.
- Added regression tests for feedback persistence, validation, and cache behavior.
- Added first-party product-operations documentation with an explicit no-third-party-tracking policy.
### 1.1.0 — Hash-only production distribution layout

- Standardized hardened output under `assets/app`, `assets/javascript`, `assets/wasm`, and `assets/images`.
- Removed role labels from generated JS, WASM, VBC, VQC, and CSS filenames.
- Removed readable JSON metadata from production `dist`; provenance now stays in the private `.venom/<dist>/` report directory.
- Updated package verification and distribution analysis for role-neutral asset discovery.
- Added a regression test that rejects legacy labeled paths and validates all emitted references.


## AST capture and effect analysis

- Classify captured bindings by declaration kind instead of treating every free identifier equally.
- Require manual review for reads from mutable `let`/`var` state.
- Require manual review for captured non-primitive constants whose identity or serialization may change across runtimes.
- Preserve protected recommendations for dependency-closed helper functions, imports, and primitive constants.
- Detect property writes, deletes, increments, and common mutating method calls on captured objects.
- Expand AST planner regression coverage for mutable reads, object captures, helper dependencies, and captured-object mutation.

## AST dynamic-module and CommonJS boundary pass

- Detects literal and non-literal dynamic `import()` expressions in the AST planner.
- Treats dynamic module execution as an explicit manual-review boundary unless an authoritative Venom rule assigns the runtime.
- Detects literal and computed CommonJS `require()` calls.
- Detects `module.exports` and `exports.*` assignments.
- Adds focused module-closure regressions for dynamic imports, CommonJS modules, and annotation overrides.

## Browser module opaque-ID linking fix
- Added compiler-authored specifier maps for production browser ES modules.
- Fixed extensionless protected imports failing after source paths were redacted to opaque IDs.
- Added an opaque production module-linker regression.

## TypeScript frontend compatibility pass

- Added ambient declaration erasure.
- Added overload-signature and abstract-method erasure.
- Added generic class/function declaration handling.
- Added generic base-class and `implements` handling.
- Added constructor parameter-property support.
- Added optional and definite-assignment class-field handling.
- Added a compatibility corpus and source-location diagnostic regression.
- Documented supported and explicitly unsupported TypeScript syntax.

## TypeScript frontend compatibility pass

- Added ambient declaration erasure.
- Added overload-signature and abstract-method erasure.
- Added generic class/function declaration handling.
- Added generic base-class and `implements` handling.
- Added constructor parameter-property support.
- Added optional and definite-assignment class-field handling.
- Added a compatibility corpus and source-location diagnostic regression.
- Documented supported and explicitly unsupported TypeScript syntax.

### 1.2.0 - TSX production build and console phase correction

- Split runtime and package generation into focused `[current/18]` phases.
- Removed misleading standard-mode per-phase timings; verbose mode reports completed-step timing.
- Fixed production protection-closure record accounting for external lazy `.vqc` registry chunks.
- Added a production TSX build + package/runtime verification regression.
- Removed raw Python tracebacks from example launcher failures.
- Clarified the required `.\\` prefix when invoking Windows launchers from PowerShell.

## Aegis Operations enterprise stress test

- Added a 30-file TypeScript/TSX operations dashboard as Example 6.
- Added five protected TurboJS/WASM analytics exports, deep browser module linking, interactive views, production verification, and source leak coverage.

- Added Example 7, a browser JavaScript playground that compiles and executes user source inside the protected TurboJS/WASM engine with structured console capture and diagnostics.

- Fixed the JavaScript Playground execution boundary: user source is now compiled by the loopback-only native TurboJS development endpoint and transferred as authenticated VTJSBC03 bytecode to TurboJS/WASM. The playground no longer attempts denied host/source fallback execution.

## TurboJS/WASM ABI and browser qualification

- Added `contracts/turbojs-wasm-abi.json` as the authoritative runtime ABI and trust-domain contract.
- Added generated C++ and JavaScript bindings and generated ABI documentation.
- Site builds now parse the actual embedded WebAssembly export section before packaging and fail on ABI drift.
- Added real Playwright qualification for the JavaScript Playground and Aegis Operations in Chromium, Firefox, and WebKit.
- Playground bridge executions now use disposable TurboJS contexts so global state cannot leak between runs.

## 1.2.2 development

- Hardened the JavaScript Playground with per-launch capability tokens, loopback Origin/Host checks, JSON-only compilation requests, source limits, compiler concurrency limits, a reduced subprocess environment, and shorter compilation timeouts.
- Added per-run TurboJS/WASM heap, stack, interrupt, pending-job, bridge-input, result-output, and console-output limits.
- Added bounded and circular-safe result serialization and retained disposable TurboJS contexts for every submitted script.

## 1.3.0 - Structural TypeScript frontend

- Replaced the default text-based TypeScript eraser with the vendored TypeScript compiler API.
- Added deterministic ES2022/ES module lowering for TS, TSX, MTS, and CTS inputs.
- Added source-map production and structured frontend provenance.
- Preserved JSX for Venom's deterministic classic JSX lowering pipeline.
- Retained the native eraser behind `VENOM_TYPESCRIPT_FRONTEND=native` for migration compatibility.
- Added structural TypeScript, TSX, source-map, and integration regressions.

### Chrome extension context compatibility

- Added execution-context policies for extension pages, service workers, isolated and main-world content scripts, DevTools pages, sandbox pages, and static resources.
- JavaScript module dependencies now inherit the Chrome execution world of their manifest entrypoint.
- Added deterministic protected-runtime host planning and compatibility summaries.
- Added warnings for dynamic imports and dynamic `chrome.scripting.executeScript` resources that cannot be resolved statically.
- Added validation preventing protected source from executing directly in main-world or sandbox contexts.

## Chrome extension compatibility Pass 3

- Added generated Manifest V3 RPC client, service-worker broker, protected host adapter, and background wrapper.
- Added bounded payload, timeout, export-name, and in-flight request validation.
- Added offscreen-document readiness probing and one-shot lifecycle recovery after service-worker or host restart.
- Preserved developer service workers by importing them from generated module/classic wrappers.
## Chrome Web Store readiness

- Added Manifest V3 publication-policy verification for extension distributions.
- Added deterministic Chrome-extension ZIP packaging, checksums, and build reports.
- Added permission, CSP, resource, hardening, and development-file audits.
- Integrated store readiness into the Example 9 production launcher.
# Static-arena cleanup hardening

- Removed browser calls into the legacy script-buffer free export after engine execution.
- Moved zeroization to bounds-checked host memory views using the verified payload length.
- Made the runtime compatibility release write-free and clamped every teardown wipe to its fixed arena capacity.
- Added an ABI-13 TurboJS runtime whose compatibility free export cannot inspect or mutate post-execution state.
- Added regression coverage for generated artifacts, worker cleanup, and 10,000 adversarial release calls.

# TurboJS product branding cleanup

- Replaced stale TurboJS product text across every example UI, README, package guide, build message, runtime diagnostic, and current product document.
- Renamed the public `turbojs-benchmark` example and its Windows/Linux launchers to `turbojs-benchmark`.
- Updated Protected Chess runtime identity and benchmark copy to report TurboJS/WASM from both the browser and protected engine.
- Preserved lowercase legacy ABI, bytecode, configuration, and API identifiers required for binary compatibility.

## Collection profiling and Map/Set repeated-access fast path

- Added a native TurboJS collection benchmark that separately measures Map updates, Map lookups, Set churn, and the mixed protected-benchmark workload.
- Added a per-container last-record cache for repeated `get`/`set`/`has` sequences without increasing every Map or Set entry.
- Invalidated the cache during deletion and preserved normal hash-table lookup semantics for unrelated keys.
- Measured direct-native improvements against the stabilization baseline on the review host: Map updates improved about 91%, Map lookups about 4%, Set churn about 11%, and the mixed workload about 46%.
- Rebuilt the compiler and completed the protected chess production build through all 18 stages after the runtime change.


## Collection hash-table density pass

- Reduced the Map/Set hash-table growth threshold from two live records per bucket to one, shortening collision chains without enlarging individual records.
- Preserved the per-container repeated-access cache introduced in the previous collection pass.
- Added benchmark validation for update, lookup, churn, and mixed collection workloads.
- Measured random Map lookup throughput improving from about 571k to about 936k operations per second on the review host, with mixed throughput rising from about 298k to about 345k operations per second.

## Collection integer-key hash pass

- Added a direct integer Map/Set hash path that avoids converting every integer key to IEEE-754 bits.
- Replaced the poorly distributed low-bit numeric hash for integer keys with an odd-multiplier permutation suitable for power-of-two bucket tables.
- Preserved the numeric SameValueZero hash domain so integral numeric keys, signed zero, NaN, and fractional keys retain correct Map/Set semantics.
- Added a native regression test covering thousands of positive and negative integer keys, signed zero, NaN, integral doubles, fractional numbers, and Set membership.
- Extended the native collection benchmark validation before protected-runtime qualification.

## Deterministic asset-reference scanner portability pass

- Replaced the asset planner's `std::regex` reference scanner with deterministic linear parsers for quoted references, CSS `url(...)` values, and unquoted `src`/`href`/`poster` attributes.
- Replaced dynamically compiled template-path regular expressions with a segment-aware placeholder matcher.
- Removed the Linux GCC/libstdc++ crash that previously stopped protected builds during step 6 asset processing.
- Confirmed the GCC build now passes asset planning, runtime generation, and package assembly; a separate GCC-only crash remains in the embedded JavaScript hardener during bootstrap hardening.
- Completed the protected chess production build through all 18 stages with the supported Clang Release toolchain.

### GCC hardener portability regression coverage

- Verified the protected chess production build through all 18 stages with GCC after the deterministic asset-scanner migration.
- Added a native embedded-hardener stress regression covering loader, runtime, worker, and protected-script inputs across repeated runtime shutdown and reinitialization cycles.
- Locked loader binding-marker preservation and persistent hardener lifecycle accounting into the CTest suite.
- Added a deterministic conservative obfuscation retry for seed-sensitive `javascript-obfuscator` transform failures; fallback use is now recorded in performance reports and build summaries.

## Hardener cache integrity pass

- Replaced raw hardened-JavaScript cache files with a versioned cache envelope containing the payload size and SHA-256 digest.
- Invalid or truncated cache entries are now discarded and regenerated instead of entering protected package generation.
- Added cache invalidation accounting to build-performance reports and human-readable build summaries.
- Added a native regression that corrupts a cache entry, verifies deterministic regeneration, and confirms the repaired entry is reusable.
- Updated cache contract smoke tests to the v6 hardener cache identity.
## Chrome extension SRI and annotation pass

- Replaced the bundled Chrome extension example with the updated Velocity Chess project.
- Added `// @venom: browser` annotations to every production JavaScript module in the extension example.
- Fixed package integrity verification for protected extension engine pages nested under `assets/extension/` by resolving loader and stylesheet references relative to the selected HTML page.
- Added regression checks preventing root-relative SRI lookup from returning for nested extension routes.

### Chrome extension protected-runtime correction

- Changed all 23 production JavaScript modules in `examples/chrome-extension` from `// @venom: browser` to `// @venom: protected`.
- Added a regression that requires every extension JavaScript file to opt into protected TurboJS/WASM execution and rejects browser-execution annotations.
- Verified the extension planner reports 23 protected files and 0 browser files, package integrity passes, and all 74 extension tests pass.

## Chrome extension protected-site parity

- Chrome extension builds now retain Venom's normal protected website layout instead of copying authored JavaScript into `assets/extension`.
- The manifest-selected extension page (for example `popup.html`) is emitted as the standard integrity-bound protected route shell at its stable Chrome path.
- Protected `.js`, `.mjs`, and `.cjs` files are no longer eligible for extension passthrough; only required static resources and generated Chrome adapters may remain public.
- Chrome Web Store verification now permits manifest-referenced root HTML pages while continuing to reject unrelated root files.

### Chrome extension WASM CSP enforcement

- Added the Manifest V3 `wasm-unsafe-eval` directive to the protected Chrome extension example.
- Made the Chrome emitter fail closed if the emitted manifest cannot authorize local WebAssembly compilation.
- Made Chrome Web Store readiness reject extension packages that would pass static verification but fail TurboJS/WASM startup under `script-src 'self'`.

### TurboJS ABI fingerprint single-source fix

- Generated one canonical release ABI fingerprint from `contracts/turbojs-wasm-abi.json`.
- Made the VBC package writer and browser TurboJS engine consume the same generated constant.
- Retained an independent runtime self-check against the actual WebAssembly export surface.
- Added detailed package/runtime fingerprint diagnostics and a regression preventing duplicate fingerprint algorithms from drifting.

- Fixed protected Chrome boot failures caused by the hardened JavaScript engine module recomputing the TurboJS ABI fingerprint inconsistently. The browser runtime now validates the exact WASM export names and kinds, then compares the package against the generated canonical ABI fingerprint instead of hashing the validated list again after hardening.

### Chrome MV3 physical adapter execution

- Stopped executing packaged browser adapters through `blob:` module URLs on `chrome-extension:` pages, which Manifest V3 CSP rejects.
- Chrome-facing modules are emitted as hardened physical extension resources while protected engine code remains sealed in the VBC package.
- Added a generated extension-page adapter that waits for `venom.ready()` and protected route hydration before loading authored Chrome/DOM adapters in their original order.
- Offscreen engine pages now load only the protected Venom shell and generated RPC host; compile-time facade modules are not executed in Chrome.
## Chrome extension runtime publication handshake

- Fixed a Manifest V3 bootstrap race where generated page and offscreen adapters could execute before the asynchronous Venom loader published `globalThis.venom`.
- Generated adapters now wait for `venom:ready`, poll for late publication, reject on `venom:boot-error`, and fail closed after a bounded timeout.
- Applied the same handshake to protected offscreen RPC hosts so early Chrome messages cannot trigger a false `Venom protected runtime is unavailable` error.
- Added regression coverage forbidding the old immediate runtime availability check.


### Chrome extension resource URL validation

- Fixed the extension background adapter to load `assets/extension/allowed_sites.txt`, matching the emitted production layout.
- Chrome Web Store readiness now scans literal `chrome.runtime.getURL(...)` references and rejects missing extension-root resources before packaging.
- Added regression coverage for the new resource-reference validation.

### Chrome protected engine start handshake

- Unified popup, background, content-controller, and page-bridge protocol keys on the current `V0110` controller/channel contract.
- Routed popup Start, Stop, and Status through the background service worker so the same process owns controller injection and protected-engine RPC.
- Added a generated `VenomExtensionRPC.ready()` handshake that creates and verifies the offscreen protected runtime before page-controller startup.
- Restored the full isolated-world dependency injection order, including skill-level and mouse-recorder adapters.
- Added regressions covering the brokered lifecycle path and protected runtime readiness gate.

## Zero-config integration pass

- Added `@venom/venom` for one-command adoption by existing projects.
- Added automatic detection for static websites, Chrome extensions, Vite, React, Vue, and Svelte.
- Framework projects now run their existing production build and protect the compiled output without requiring application rewrites.
- Added the single `venom` command for project detection, setup, diagnostics, locking, and protection.
- Added optional input, output, executable, build, and quiet overrides.
- Made `@venom/vite` step aside when `@venom/venom` owns production orchestration, preventing duplicate protection builds.
- Added a real build-to-protection integration regression and a zero-config integration guide.

## Guided zero-config integration

- Added `venom init` to generate `venom.config.json` and a `build:protected` package script.
- Added `venom doctor` to diagnose missing framework build scripts, missing configured outputs, and static-site entry-point issues before protection begins.
- Added configuration support for input, output, profile, build orchestration, build script, Venom executable, and additional arguments.
- Added automatic support for common React, Vite, Vue, and Svelte output directory conventions while retaining CLI overrides.
- Improved command-launch and missing-output diagnostics.
- Added integration coverage for initialization, configuration discovery, diagnostics, and overwrite protection.


## Project-local compiler discovery

- Updated `@venom/venom` to 1.2.0.
- Added deterministic Venom binary resolution across CLI overrides, config, `VENOM_BIN`, project-local toolchains, and system `PATH`.
- Added `venom bin` and compiler-source reporting in `venom doctor`.
- Made missing or invalid compiler paths fail before framework compilation begins.
- Added binary-resolution regressions for project, config, environment, and invalid-path behavior.


## Guided local compiler bootstrap

- Updated `@venom/venom` to 1.3.0.
- Added `venom setup --source <checkout>` to configure and build a project-local Venom compiler.
- The setup command discovers the built executable and records a project-relative path in `venom.config.json`.
- Added build-directory, CMake, and configuration overrides plus an end-to-end setup regression.
