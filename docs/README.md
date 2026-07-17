# Venom Production Documentation

> **Applies to:** Venom 1.1.0  
> **Audience:** application teams, release engineers, security reviewers, and operators

This documentation describes the supported production use of Venom: installing the compiler, protecting an existing website, integrating browser and protected execution, building and verifying distributions, deploying static output, and operating signed releases.

Internal implementation history and engineering-process records are intentionally excluded from the production documentation set.

## Start here

| Goal | Guide |
|---|---|
| Install a released build | [Installation](getting-started/installation.md) |
| Build Venom from source | [Build from source](getting-started/build-from-source.md) |
| Protect a first site | [First project](getting-started/first-project.md) |
| Integrate an existing application | [Existing-site integration](getting-started/existing-project.md) |
| Deploy the generated distribution | [Deployment](getting-started/deployment.md) |

## Integrate Venom

| Topic | Guide |
|---|---|
| Plan protected and browser execution | [Protection planner](guides/protection-planner.md) |
| Select browser or protected execution | [Annotations](guides/annotations.md) |
| Expose protected functions | [Protected functions](guides/protected-functions.md) |
| Define typed protected API boundaries | [Typed bridge contracts](guides/typed-bridge-contracts.md) |
| Compile TypeScript source directly | [TypeScript input](guides/typescript.md) |
| Package protected ES modules | [Protected modules](guides/protected-modules.md) |
| Call protected exports safely | [Browser bridge](guides/browser-bridge.md) |
| Transfer binary arrays | [Binary protected values](guides/binary-protected-values.md) |
| Execute independent protected calls together | [Batched protected calls](guides/batched-protected-calls.md) |
| Configure routes and navigation | [Routing](guides/routing.md) |
| Manage images, fonts, CSS, and other assets | [Assets](guides/assets.md) |
| Configure a site | [Configuration](guides/configuration.md) |
| Control browser host access | [Capability modules](guides/capability-modules.md) |
| Diagnose build and runtime failures | [Debugging](guides/debugging.md) |

## Understand the runtime

| Topic | Guide |
|---|---|
| End-to-end architecture | [Architecture overview](architecture/overview.md) |
| Compilation stages | [Compiler pipeline](architecture/compiler-pipeline.md) |
| Runtime execution model | [Protected runtime](architecture/protected-runtime.md) |
| Package structure | [VBC package format](architecture/package-format.md) |
| Route execution | [Route hydration](architecture/route-hydration.md) |
| Trust and observation boundaries | [Trust boundaries](architecture/trust-boundaries.md) |

## Review security properties

| Topic | Guide |
|---|---|
| Security model and guarantees | [Security model](security/security-model.md) |
| Threat assumptions | [Threat model](security/threat-model.md) |
| Protection layers and evidence | [Protection strengths](security/protection-strengths.md) |
| Production hardening requirements | [Production hardening](security/production-hardening.md) |
| Binary bridge protocol | [Binary capability bridge](security/binary-capability-bridge.md) |
| Streamed WASM decoding | [Streamed WASM decoding](security/streamed-wasm-decoding.md) |
| WASM memory handling | [WASM memory hardening](security/wasm-memory-hardening.md) |
| Build-specific bytecode records | [Bytecode envelopes](security/build-specific-bytecode-envelopes.md) |
| Per-build byte permutation | [Bytecode permutation](security/per-build-bytecode-permutation.md) |
| Runtime tamper detection | [Runtime integrity seals](security/runtime-integrity-seals.md) |
| Session-bound capabilities | [Session capability leases](security/session-capability-leases.md) |
| Decoder/executor separation | [Split runtime trust domains](security/split-runtime-trust-domains.md) |
| Release signing | [Release signing](security/release-signing.md) |
| Explicit limitations | [Limitations](security/limitations.md) |

## Operate and verify

| Topic | Guide |
|---|---|
| Development and production profiles | [Build profiles](operations/build-profiles.md) |
| Verify a production release | [Release verification](operations/release-verification.md) |
| Package signed releases | [Release packaging](operations/release-packaging.md) |
| Understand release numbers and prerelease stages | [Versioning policy](operations/versioning.md) |
| Historical release evidence | [Release evidence](operations/release-evidence.md) |
| Deploy and manage runtime versions | [Runtime management](operations/runtime-management.md) |
| Update and rollback installed releases | [Update management](operations/update-management.md) |
| Qualify source and protected behavior | [Browser equivalence](operations/browser-equivalence.md) |
| Framework support evidence | [Framework qualification](operations/framework-qualification.md) |
| Supported compatibility matrix | [Compatibility matrix](operations/compatibility-matrix.md) |
| Package compatibility evidence | [Compatibility evidence](operations/compatibility-evidence.md) |
| Measure protected runtime performance | [Runtime performance](operations/runtime-performance.md) |
| Measure compiler build performance | [Build performance](operations/build-performance.md) |

## Reference

| Reference | Contents |
|---|---|
| [CLI](reference/cli.md) | Commands, common options, and automation usage |
| [Configuration](reference/configuration.md) | Supported `venom.toml` settings |
| [Annotations](reference/annotations.md) | Exact annotation forms and behavior |
| [JavaScript API](reference/javascript-api.md) | `venom.ready`, protected exports, values, and errors |
| [Output layout](reference/output-layout.md) | Production distribution paths and responsibilities |
| [Exit codes](reference/exit-codes.md) | Stable process-result categories for automation |
| [Compiler diagnostics](reference/diagnostics.md) | Stable error codes, source locations, and remediation guidance |
| [Host API contract](generated/host-api-contract.md) | Generated browser-host capability contract |

## Public examples

- [Protected Chess](../examples/protected-chess/README.md)
- [NOVA TRADE](../examples/nova-trade/README.md)
- [Venom Sentinel Bot Detection](../examples/bot-detection/README.md)

## Production documentation policy

Every documented command must correspond to a shipped command or script. Every local link is validated by the release documentation gate. Security claims must describe implemented behavior and must retain the browser-owner limitation: a determined analyst controlling the client can instrument delivered code even when the original protected source is absent from the distribution.

## Architecture overview

- [Venom architecture overview](architecture/product-overview.md)

## Security audit

- [Internal security audit and remediation plan](audit/README.md)
