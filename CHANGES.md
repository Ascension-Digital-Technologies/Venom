## Unreleased

- Fixed protected function extraction so production builds emit the extracted bridge registry as an authenticated `VQJSE006` QuickJS bytecode record.
- Added fail-closed validation for missing or malformed protected bridge bytecode.
- Added a post-rewrite source-leak guard and strengthened the protected-chess regression test to require one bytecode record and reject engine implementation markers in browser JavaScript.

# Changelog

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
