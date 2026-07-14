# Changelog

All notable changes to Venom are documented here. Entries are ordered newest first.

## 2.0.0-alpha.30

- Audited public documentation against the alpha.29 source and current GitHub baseline before publication.
- Corrected the README version badge, documentation anchors, and contribution link.
- Documented `--verbose` and `--quiet` build-output controls in the CLI reference.
- Clearly labeled legacy release evidence and security-audit records as historical.
- Added Markdown heading-anchor validation to the public release gate.
- Corrected stale wording in the package-format guide.

## 2.0.0-alpha.29

- Added structured phase-by-phase compiler tracing for protected builds.
- Added elapsed timings and useful counts for site discovery, protected planning, assets, bytecode, package sections, and emitted artifacts.
- Added `--verbose` / `-v` for deeper planner and runtime diagnostics and `--quiet` / `-q` for concise automation output.
- Kept JSON build output machine-readable by suppressing human progress tracing in `--format json` mode.

## 2.0.0-alpha.28

- Replaced placeholder clone commands with the canonical public repository URL.
- Standardized source checkout instructions on `https://github.com/Ascension-Digital-Technologies/Venom.git` and the `Venom` checkout directory.
- Verified repository-link consistency across all public Markdown documentation.

## 2.0.0-alpha.27

- Polished the public documentation hierarchy and corrected stale version references.
- Added contributor and community standards for GitHub collaboration.
- Fixed broken support navigation and clarified the separation between product documentation and historical audit records.
- Normalized changelog structure and release ordering.

## 2.0.0-alpha.26

- Removed generated build-status artifacts from the source archive.
- Removed stale CMake registrations for deleted Node hardener parity tests.
- Simplified the scripts guide around supported entrypoints and platform roles.
- Performed a repository hygiene pass without changing compiler or runtime behavior.

## 2.0.0-alpha.25

- Fixed the native QuickJS hardener crashing on Windows with `0xC00000FD` while compiling production distributions.
- Increased the MSVC-linked `venom.exe` stack reserve to 16 MiB, safely above the hardener runtime's 8 MiB QuickJS stack guard.
- Kept the larger stack scoped to the compiler executable; runtime and browser artifacts are unchanged.

## 2.0.0-alpha.24

- Removed the obsolete Node hardener setup scripts, package files, installation hooks, CI probes, and parity-only test registrations.
- Retained Node.js only where independently required for Playwright browser validation.

## 2.0.0-alpha.23

- Replaced the production Node.js hardener subprocess with an in-process C++/QuickJS hardener.
- Embedded pinned Terser 5.49.0 and javascript-obfuscator 5.4.7 payloads in the native compiler.
- Preserved role-specific options, deterministic seed behavior, and byte-identical parity fixtures without requiring Node.js or `node_modules`.

## 2.0.0-alpha.22

- Enforce per-chunk `VENOM_HOST_CAPABILITIES_V3` declarations at QuickJS binding creation.
- Replace the full `__venomRuntime` injection with a frozen capability-scoped facade.
- Add stable `VNM-CAP-1001` undeclared-capability diagnostics and runtime enforcement regression coverage.

## 2.0.0-alpha.21

- Fix browser boot for `VENOM_HOST_CAPABILITIES_V3`.
- Preserve V1/V2 manifest compatibility.
- Parse per-chunk least-privilege capability records at runtime.

## 2.0.0-alpha.20

- Replaced the protected chess example with the updated user-supplied engine and corrected its Venom directive placement.
- Added least-privilege per-chunk host capability manifests.
- Reduced the default capability set to `__venomRuntime`.
- Marked undeclared host capabilities as denied in the capability policy.

### Additional alpha.20 fixes

- Fixed protected function extraction so production builds emit the extracted bridge registry as an authenticated `VQJSE006` QuickJS bytecode record.
- Added fail-closed validation for missing or malformed protected bridge bytecode.
- Added a post-rewrite source-leak guard and strengthened the protected-chess regression test to require one bytecode record and reject engine implementation markers in browser JavaScript.


## 2.0.0-alpha.19

- Added lexical capture classification for protected function lowering.
- Immutable primitive constants and pure helper functions continue to lift automatically.
- Mutable bindings, unsupported constant initializers, imported bindings, and browser-only globals now fail closed with precise diagnostics.
- `isolated` no longer unconditionally bypasses dependency analysis; large isolated units require a separate realm-safety verification.
- Added regular-expression literal masking, nested-function parameter recognition, and numeric-literal filtering to reduce false capture results.
- Protection closure failures now include the source, function, and lowering reason.

## 2.0.0 — Alpha 2 function-level planner

- Added function-level protection analysis for declarations and arrow functions.
- Added per-function purity, confidence, execution-realm, source, and reason reporting.
- Added strict planner enforcement for unresolved and low-confidence recommendations.
- Added `--min-confidence`, `--planner-min-confidence`, `--report`, and planner configuration rules.
- Preserved annotation, CLI, and configuration precedence over automatic recommendations.

### Alpha.1 foundation

- Added the local-only Venom v2 protection planner and `venom plan` command.
- Added manual `--protect` and `--browser` overrides with precedence over automatic recommendations.
- Added `standard`, `strong`, and `maximum` protection-level CLI contracts.
- Added planner and protection configuration hooks without introducing a server dependency.
- Preserved protected-by-default behavior and existing `@venom` annotations.

## 1.0.2 — QuickJS/WASM controller recursion fix

- Fixed the Emscripten controller invoking the QuickJS/WASM wrapper without its internal build-mode flag.
- Prevented recursive `quickjs-wasm/quickjs-wasm/...` output directory growth on Windows and Unix-like systems.
- Added defensive rejection of recursively nested runtime output paths.
- Added workflow coverage for the controller-to-wrapper handoff and PowerShell array syntax.

## 1.0.1 — Example gallery and production console polish

- Added official screenshots for Protected Chess, NOVA TRADE, and Venom Sentinel.
- Standardized Windows launcher behavior so double-clicked scripts remain open after success or failure.
- Added a shared production console formatter for build, test, setup, example, and release workflows.
- Added launcher-policy regression coverage and refreshed script documentation.

## 1.0.0 — First public stable release

### Highlights

- First stable public release of Venom Secure Web Runtime.
- Ships the production-grade hybrid browser and QuickJS/WASM execution model, polymorphic VBC packaging, binary leased capability bridge, streamed WASM-owned decoding, build-specific QuickJS bytecode envelopes, runtime integrity seals, and signed fail-closed release tooling.
- Includes the polished documentation hub, publication README, three complete public examples, framework qualification, browser-equivalence tooling, runtime/build performance tooling, and source-package completeness gates.
- Validated through clean extracted builds, the complete native CTest inventory, repository and documentation gates, runtime provenance checks, and release-policy verification.

### Security boundary

- The exact original protected source is not shipped in production distributions and cannot be recovered byte-for-byte from them.
- Venom raises reverse-engineering, tampering, and automation cost; it does not claim permanent secrecy against an analyst who controls the browser and operating environment.

## 2.0.0-alpha.6

- Added canonical Ed25519-signed browser release roots.
- Bound all distribution files, build metadata, package hash, versions, and protection closure into `VENOM_RELEASE_ROOT.json`.
- Added fail-closed signed distribution publication tooling.
- Added tamper, unsigned-release, and signature regression tests.

## 2.0.0-alpha.14

- Parse named protected-function parameter lists with balanced JavaScript ranges.
- Support multiline destructuring and nested default expressions in protected named declarations.
- Keep generator and receiver-bound method syntax fail-closed until their stateful bridge contracts are implemented.

## 2.0.0-alpha.18

- Added exact grammar parsing for file- and function-level realm directives.
- Realm-like typos such as `protectedd` and `notbrowser` now fail closed.
- Rejected illegal modifiers such as `browser isolated` and unknown function-level modifiers.
- Preserved valid `protected isolated` and file-scope `protected module` forms.
- Added exact-realm-directive grammar regression coverage.

## 2.0.0-alpha.17

- Reject conflicting browser and protected directives at file scope before realm planning.
- Reject contradictions between HTML `data-venom` attributes and source-level realm directives.
- Add a permanent fail-closed realm-conflict regression test.

## 2.0.0-alpha.16

- Bind function-level directives deterministically to the immediately following supported declaration.
- Allow whitespace and documentation comments between a directive and declaration.
- Reject orphaned, end-of-file, and consecutive ambiguous realm directives.
- Preserve leading file-scope realm annotations without treating them as function directives.

## 2.0.0-alpha.15

- Restricted function-level `@venom:` directives to actual JavaScript comments.
- Prevented directive-like text inside strings and template literals from changing execution realms.
- Added multiline block-comment and JSDoc-style directive handling.
- Added a permanent comment-only directive regression fixture while preserving protected arrows, chess, and bot-detection compatibility.
