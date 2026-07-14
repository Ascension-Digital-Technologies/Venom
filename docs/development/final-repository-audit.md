# Final repository audit

> **Applies to:** Venom 1.64.4  
> **Purpose:** Record the final source-tree and publication checks before the first public release tag.

## Repository state

- Handwritten compiler source is organized by `commands`, `core`, `pipeline`, and `services`.
- Build-system input templates live under `cmake/templates/`.
- Generated browser runtime and embedded WASM artifacts live under `src/generated/runtime/`.
- Public examples are limited to `protected-chess`, `nova-trade`, and `bot-detection`.
- Build, test, example, release, and verification entry points live under `scripts/`.
- Release publication is fail-closed and requires signed release-set metadata.

## Intentionally large generated files

`src/generated/runtime/quickjs_runtime_wasm_blob.hpp` is large by design because it embeds the verified QuickJS/WASM runtime. It must be regenerated through the canonical runtime build and provenance tooling, never edited manually.

## Final tag checklist

1. Run `scripts/release-closure.ps1 -BrowserRuntimeTests` on the supported Windows toolchain.
2. Confirm the complete CTest suite passes with `ctest --test-dir build -C Release --output-on-failure`.
3. Confirm all three public examples build in development and production profiles.
4. Verify browser-equivalence and runtime-performance evidence.
5. Configure the protected Ed25519 signing secrets and GitHub release environment.
6. Add a real `.github/CODEOWNERS` file only when the final GitHub username or organization team is known.
7. Publish the signed release set, SBOM, provenance, checksums, and release notes together.

## Remaining non-blocking work

- Expand upstream pinned framework applications beyond representative output-pattern fixtures.
- Publish repeatable benchmark results from named hardware and browser versions.
- Add a real CODEOWNERS mapping once repository ownership is finalized.
- Continue QuickJS and Emscripten security-update qualification through the runtime lifecycle gate.

## Small-file audit

Small files are retained only when they define an active narrow interface, diagnostic probe, generated contract, fixture, or platform wrapper. Legacy no-op QuickJS bridge and placeholder bytecode APIs were removed in 1.64.7 after repository-wide call-site verification. The unused native `vm::Interpreter` scaffold was removed in 1.64.6 because the production route runtime is implemented by the encoded route program and browser/WASM executors, not that class.

## API compatibility cleanup policy

Compatibility aliases are retained only while active call sites or a documented public migration window require them. Once the repository and supported public examples use the canonical API, obsolete aliases are removed and guarded by source-layout tests. The pre-AEAD `CryptoProvider` and `NoCryptoProvider` aliases were removed in 1.64.8; the canonical names are `AeadProvider` and `NoAeadProvider`.
