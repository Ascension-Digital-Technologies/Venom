# Venom documentation

> **Audience:** users, integrators, operators, security reviewers, and runtime contributors  
> **Applies to:** Venom 1.65.3

This is the canonical documentation map for Venom Secure Web Runtime. Start with the workflow that matches your goal, then use the architecture and security references when you need deeper implementation detail.

## Get started

- [Install Venom](getting-started/installation.md)
- [Build your first protected project](getting-started/first-project.md)
- [Protect an existing website](getting-started/existing-project.md)
- [Deploy a production distribution](getting-started/deployment.md)
- [Build Venom from source](development/building-from-source.md)

## Integrate Venom

- [Annotations and execution realms](guides/annotations.md)
- [Protected functions](guides/protected-functions.md)
- [Protected modules](guides/protected-modules.md)
- [Browser bridge](guides/browser-bridge.md)
- [Routing and route hydration](guides/routing.md)
- [Assets and remote vendoring](guides/assets.md)
- [Configuration](guides/configuration.md)
- [Debugging](guides/debugging.md)

## Architecture

- [Architecture overview](architecture/overview.md)
- [Compiler pipeline](architecture/compiler-pipeline.md)
- [Protected runtime](architecture/protected-runtime.md)
- [Trust boundaries](architecture/trust-boundaries.md)
- [Package format](package-format.md)
- [Route hydration](architecture/route-hydration.md)

## Security and hardening

- [Security model](security/security-model.md)
- [Threat model](security/threat-model.md)
- [Production hardening](security/production-hardening.md)
- [Known limitations](security/limitations.md)
- [Binary capability bridge](security/binary-capability-bridge.md)
- [Session capability leases](security/session-capability-leases.md)
- [Streamed WASM-owned decoding](security/streamed-wasm-decoding.md)
- [WebAssembly memory hardening](security/wasm-memory-hardening.md)
- [Build-specific bytecode envelopes](security/build-specific-bytecode-envelopes.md)
- [Per-build bytecode permutation](security/per-build-bytecode-permutation.md)
- [Runtime integrity seals](security/runtime-integrity-seals.md)
- [Split runtime trust domains](security/split-runtime-trust-domains.md)
- [Release signing](security/release-signing.md)

## Compatibility and evidence

- [Support matrix](compatibility/support-matrix.md)
- [Framework guidance](compatibility/framework-guidance.md)
- [Framework qualification](compatibility/framework-qualification.md)
- [Browser equivalence testing](compatibility/browser-equivalence.md)
- [Compatibility evidence](compatibility/compatibility-evidence.md)

## CLI and configuration reference

- [CLI reference](reference/cli.md)
- [Configuration reference](reference/configuration.md)
- [JavaScript API](reference/javascript-api.md)
- [Annotation reference](reference/annotations.md)
- [Production output layout](reference/output-layout.md)
- [Exit codes and automation](reference/exit-codes.md)

## Build, test, and release

- [Repository layout](development/repository-layout.md)
- [QuickJS/WASM development](development/quickjs-wasm.md)
- [Testing](development/testing.md)
- [Release closure](development/release-closure.md)
- [Stable 1.65.3 verification](development/release-1.65.3-verification.md)
- [Final repository audit](development/final-repository-audit.md)
- [Build performance](performance/build-performance.md)
- [Runtime benchmarking](performance/runtime-benchmarking.md)

## Operations

- [Runtime management](runtime-management.md)
- [Update management](update-management.md)
- [Stable contracts](STABLE-CONTRACTS.md)
- [Release packaging](release-packaging.md)
- [Release qualification](RELEASE-QUALIFICATION.md)
- [Release checklist](RELEASE-CHECKLIST.md)

## Documentation policy

- [Documentation style guide](STYLE-GUIDE.md)
- [Documentation maintenance](MAINTENANCE.md)
- The root [README](../README.md) is the product landing page and quick-start guide.
- Pages in the categorized directories above are canonical for current workflows.
- Uppercase and root-level legacy reference pages remain for deep implementation history; new links should prefer the canonical categorized pages.
- Security documentation describes reverse-engineering resistance and integrity controls, not permanent secrecy in an attacker-controlled browser.
