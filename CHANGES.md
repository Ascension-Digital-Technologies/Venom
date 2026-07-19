# Changelog

- Centralized the remaining 32 implementation headers under `include/venom/internal/<domain>/`, converted all private includes to explicit `venom/internal/...` paths, removed first-party source include directories from CMake, and enforced a zero-header `src/` tree.
- Consolidated all 47 exported C/C++ headers into the central `include/venom/` tree, removed every domain-local `src/*/include/` directory, and updated CMake, generators, architecture gates, tests, and documentation to use the unified layout.
- Added a dependency-free `src/base/` domain with a typed `venom::Error`, stable exit-code taxonomy, and centralized phase-aware error rendering; all authored C++ fatal paths now use the shared API.
- Replaced duplicated example metadata with `VENOM_EXAMPLE_REGISTRY_V2`, generated symmetric launchers for all ten examples, and added the previously missing Vite launcher.
- Added reusable `tools/venom_tools/` helpers plus CI gates for registry integrity, launcher drift, forbidden legacy error paths, and generated runtime snapshot drift.
- Centralized Windows command completion and pause behavior under `tools/windows-scripts/internal/`, removed duplicated launcher error blocks, and added symmetric release-closure entrypoints.
- Rebound structural regressions to the new domain-owned files, generated worker template split, hash-only package naming, and canonical QuickJS/WASM tool locations.
- Enforced first-party source domains with independent CMake object targets, removed the catch-all `src/compiler/` hierarchy, extracted shared `core`, `graph`, and `remote` ownership, and added an acyclic cross-domain include checker.
- Fixed native runtime parity with the canonical route-table format, added native package execution and QuickJS parity CTest gates, and updated the runtime probe for hash-only production package names.

### Source hygiene and semantic launchers

- Consolidated the Vite integration under `packages/` and colocated fuzz targets with their corpora under `tests/fuzz/`.
- Promoted the CLI, build pipeline, language frontends, QuickJS integration, route VM, shared core, module graph, and remote acquisition into first-class `src/` domains; the obsolete `src/compiler/` umbrella was removed.
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

Venom 2.0.0 consolidates the protected compiler, QuickJS/WASM runtime, typed SDK, production-only build path, example and browser certification, startup observability, and signed release engineering into one stable release.

- Added early immutable-asset preload hints for the loader, runtime, package, QuickJS/WASM module, and worker.
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
- All certified examples use the same production QuickJS/WASM build, verification, and leak-scan path.

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

- Added the stable `@venom-js/runtime` SDK and authoritative runtime API contract.
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
- Added `tools/certify_release.py` to build flagship examples, verify real QuickJS/WASM execution, run production leak scans, execute static gates, and emit hashed JSON/Markdown evidence.
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

- Added the first-party `@venom-js/vite` plugin with serialized Venom builds, content-digest incremental rebuilds, debounced hot-update integration, TypeScript declarations, a virtual status module, and a no-cache development status endpoint.
- Added a Venom + Vite TypeScript showcase with a protected QuickJS/WASM service boundary.
- Added representative React, Vue, and Svelte source-graph qualification fixtures alongside the existing compiled-output framework corpus.
- Added executable plugin hook qualification using an isolated fake Venom compiler, including unchanged-build suppression and status endpoint validation.
- Documented the supported integration model: framework tooling owns transforms and HMR; Venom owns protected packaging and QuickJS/WASM execution.

## 1.3.2 - TypeScript browser runtime preservation

- Preserve file-scope `@venom: browser` and `@venom: protected` directives from authored TypeScript/TSX before structural lowering can remove comments.
- Classify discovered TypeScript module dependencies before transpilation, keeping browser dependency closure and opaque module maps complete.
- Add an Aegis Operations regression that verifies `browser/data/mock-data.ts` and other explicitly browser-owned TypeScript dependencies are never packaged as protected whole-file chunks.
- Restore Example 6 browser module resolution for extensionless imports such as `./data/mock-data`.

## 1.3.1 - Build performance and incremental compilation

- Added content-addressed structural TypeScript/TSX frontend caching.
- Added cache hit/miss/write telemetry for TypeScript, QuickJS bytecode, and JavaScript hardening.
- Added private per-phase `build-performance.json` reports, including quiet builds.
- Made development bridge identities project-stable so unchanged QuickJS bytecode is reusable.
- Made runtime generators publish only when content changes.
- Added a cold/warm benchmark driver and incremental-cache regression.
- Corrected protected TypeScript module validation to reuse the already-validated registry bundle for imported modules.

## 1.2.3 development

- Modularized the authored browser runtime and QuickJS engine wrapper into generated, independently testable JavaScript modules.

- Fixed JavaScript Playground completion values by evaluating pasted code with QuickJS script semantics, so the final expression is returned without requiring an explicit top-level `return`.

- Fixed JavaScript Playground result rendering to unwrap the authoritative QuickJS bridge envelope before nullable execution summaries, preserving objects, arrays, `null`, and `undefined` distinctly.
- Added a native end-to-end bridge regression for the exact playground async wrapper and result object.


- Fixed the JavaScript Playground development engine to validate the current minimal bytecode-only QuickJS/WASM export surface. Hardened development builds no longer require obsolete scaffold exports such as `venom_qjs_engine_abi`, `venom_qjs_execute_source`, or `venom_qjs_wasm_native_stack_capacity`; they validate the shared minimal ABI and `venom_qjs_bridge_abi` instead.
### QuickJS development trust-domain handoff

- Separated hardened asset generation from the release trust-domain flag.
- Development QuickJS playground builds now emit `protectedRelease = false` while retaining fail-closed WASM execution and denied host-source fallback.
- Valid `development-quickjs-compiler` bytecode handoffs are accepted only in development packages; production and `--strict-release` builds remain package-decoder-only.
- Added a regression preventing hardened development builds from being mislabeled as release trust domains.

## Development package contract boundary

- Development builds remain runtime-hardened, integrity-checked, fail-closed, and host-fallback denied without being mislabeled as release packages.
- Release-only `VRDIV003` diversification and QuickJS ABI fingerprint contracts are now required by the package release flag rather than by the independent fail-closed policy.
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

- Fixed the TSX showcase to await protected QuickJS/WASM exports before passing results into components.
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
- Made QuickJS/WASM ABI verification toolchain-aware for Emscripten reactor exports while keeping required, permitted, and unexpected exports independently reported and fail-closed.

- Hardened the Windows Emscripten pipeline against batch-file argument mangling and made Binaryen post-processing capability-aware across toolchain versions.

- Added authenticated raw `ArrayBuffer` and numeric typed-array transport for protected call inputs and results.

- Improved Emscripten toolchain compatibility by pinning the verified release SDK and compiling the browser QuickJS/WASM runtime from the minimal core source set without POSIX-only `quickjs-libc` helpers.

### Compiler and workflow upgrades

- Added `venom.batch()` for validated parallel protected-export calls with ordered results and independent capability leases, timeouts, cancellation, replay counters, and response validation.

- Added native TypeScript input for `.ts`, `.mts`, `.cts`, and non-JSX `.tsx` sources with line-preserving type erasure, extension-aware module resolution, source diagnostics, and project IR v8 metadata.

- Added typed input/output contracts for protected exports with protected-boundary validation, generated TypeScript declarations, machine-readable contract metadata, and project IR v7 integration.

- Added policy-controlled capability modules for fetch, timers, storage, and Web Crypto with `auto`, `allow`, `deny`, and restricted `read-only` modes.

- Replaced `venom analyze-dist <dist>` with `venom analyze <dist>` and `venom release-check <dist>` with `venom verify <dist>`, removing the former command names entirely.
- Organized production launchers under `scripts/windows/` and `scripts/linux/` with a minimal platform-specific script surface.
- Added a versioned internal project IR with deterministic project and protected-plan fingerprints.
- Added content-addressed incremental caching for native JavaScript hardening and compiled QuickJS bytecode, with `--no-cache` and `--cache-dir` controls.
- Added an embedded Terser AST frontend for syntax validation and precise static import, re-export, and dynamic literal import discovery without Node.js.
- Extended project IR v6 with parser-derived module metrics for nodes, functions, lexical scopes, global references, top-level declarations, imports, and exports.
- Replaced protected-module import/export text lexing with AST-derived statement boundaries, bindings, function signatures, and source-aware unsupported-syntax diagnostics.
- Replaced protected-function lexical-capture and dependency lifting heuristics with AST scope analysis, including nested scopes, destructured parameters, mutable-binding rejection, and precise browser-global detection.
- Replaced regex and balanced-brace protected-function extraction with AST-owned lowering for declarations, async functions, variable-bound arrows, function expressions, default parameters, and source-aware unsupported-form diagnostics.
- Added a unified source-aware diagnostic renderer with stable codes, file/line/column context, source excerpts, actionable remediation, and documentation links.
- Added a focused Python Playwright browser-runtime qualification gate that verifies real protected export execution and clean page, console, and network behavior without demo timing assertions.

## 1.0.0

### First public release

- Introduced Venom's hybrid browser and protected QuickJS/WebAssembly execution model.
- Added protected-by-default JavaScript planning with file- and function-level `@venom` execution directives.
- Added local protection analysis, manual runtime overrides, confidence reporting, and fail-closed production planning.
- Compiled protected JavaScript into QuickJS bytecode and packaged it in polymorphic `.vbc` containers with build-specific bytecode envelopes.
- Added worker-isolated protected execution, WebAssembly-owned streamed decoding, runtime-integrity seals, asset binding, and a private binary capability bridge.
- Added least-privilege host-capability manifests and capability-scoped protected-runtime facades.
- Embedded pinned Terser and javascript-obfuscator hardening directly into the native compiler through QuickJS, removing the external Node.js hardener requirement.
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
- Added five protected QuickJS/WASM analytics exports, deep browser module linking, interactive views, production verification, and source leak coverage.

- Added Example 7, a browser JavaScript playground that compiles and executes user source inside the protected QuickJS/WASM engine with structured console capture and diagnostics.

- Fixed the JavaScript Playground execution boundary: user source is now compiled by the loopback-only native QuickJS development endpoint and transferred as authenticated VQJSBC03 bytecode to QuickJS/WASM. The playground no longer attempts denied host/source fallback execution.

## QuickJS/WASM ABI and browser qualification

- Added `contracts/quickjs-wasm-abi.json` as the authoritative runtime ABI and trust-domain contract.
- Added generated C++ and JavaScript bindings and generated ABI documentation.
- Site builds now parse the actual embedded WebAssembly export section before packaging and fail on ABI drift.
- Added real Playwright qualification for the JavaScript Playground and Aegis Operations in Chromium, Firefox, and WebKit.
- Playground bridge executions now use disposable QuickJS contexts so global state cannot leak between runs.

## 1.2.2 development

- Hardened the JavaScript Playground with per-launch capability tokens, loopback Origin/Host checks, JSON-only compilation requests, source limits, compiler concurrency limits, a reduced subprocess environment, and shorter compilation timeouts.
- Added per-run QuickJS/WASM heap, stack, interrupt, pending-job, bridge-input, result-output, and console-output limits.
- Added bounded and circular-safe result serialization and retained disposable QuickJS contexts for every submitted script.

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

