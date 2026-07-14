# Security Model

> **Applies to:** Venom 1.0.1  
> **Audience:** application architects, security reviewers, and release owners

Venom is a client-side reverse-engineering resistance system. It removes the exact protected source from the production distribution, changes the stored representation, isolates execution, narrows the public bridge, applies per-build polymorphism, and verifies the production runtime and release artifacts. It does not create a secret execution environment on a device controlled by an adversary.

## Primary security objective

The objective is to increase the time, specialization, tooling, and per-build effort required to recover, modify, or reuse protected behavior. Venom is especially valuable against ordinary source inspection, reusable JavaScript deobfuscation, fixed-signature package extraction, low-effort patching, and direct reuse of proprietary implementation logic.

## Protected source representation

Protected JavaScript is compiled into QuickJS bytecode. The production distribution does not include the original protected module text, comments, formatting, source maps, or most source-level identifiers. Canonical QuickJS records are wrapped in build-specific `VQJSE006` envelopes before package emission.

The exact original source therefore cannot be recovered byte-for-byte from `dist/`. A determined analyst may still observe runtime behavior or decoded records and reconstruct equivalent logic.

## Polymorphic production builds

Production builds can vary package section ordering, padding, identifiers, aliases, physical Route VM opcodes, instruction-field layouts, masks, host-call classes, DOM command identifiers, string ordering, and stored bytecode envelopes. Equivalent application behavior can therefore have materially different physical representations across builds.

Venom does not claim to renumber upstream QuickJS interpreter opcodes. Its own Route VM uses build-specific physical opcodes, while QuickJS records are transformed by build-specific envelopes and byte permutations.

## Runtime isolation

Protected execution occurs inside QuickJS/WASM hosted by a dedicated worker. The page receives an asynchronous API rather than direct access to protected runtime objects. Package decoding and QuickJS execution are treated as separate responsibilities with a validated handoff record.

Worker isolation is a software boundary, not a hardware security boundary. A browser owner can instrument workers and WebAssembly execution.

## Bridge security

Protected calls use a private `MessagePort` and transferable binary frames. Frames include session generation, monotonic counters, opaque capabilities, rotated operation identifiers, integrity data, bounded payloads, and single-use capability leases. The worker rejects stale, replayed, malformed, cross-session, or unknown-capability frames.

This reduces the value of generic message hooks and prevents straightforward replay. It does not provide a server-held authorization secret because both endpoints execute on the client.

## WebAssembly memory handling

The package runtime receives the package in validated chunks, retains it in WASM-owned storage, materializes sections lazily, validates ranges, and explicitly clears temporary buffers at the earliest safe point. Bridge and bytecode paths use centralized range checks and zeroization.

WebAssembly memory remains inspectable to a browser owner. Zeroization reduces lifetime and accidental retention; it cannot guarantee that a browser engine has not copied or observed data.

## Integrity and fail-closed behavior

Production assets are hash-bound. Runtime policy, capability tables, bridge opcodes, ABI identity, route mappings, package metadata, and bytecode handoffs are validated at relevant boundaries. Missing, stale, mismatched, malformed, or tampered components stop execution rather than falling back to readable host JavaScript.

## Release security

The release toolchain verifies runtime provenance, hardener output, production leakage policy, source-package completeness, compatibility evidence, and signed release metadata. Stable packages require Ed25519 signatures and reject unsigned or mismatched release sets.

## Out of scope

Venom does not protect:

- credentials or private keys embedded in the client;
- authoritative authorization or transaction decisions that belong on a server;
- secrets against an analyst with full browser and operating-system control;
- logic intentionally marked for browser execution;
- data returned to browser code after a protected computation;
- vulnerabilities in application logic, browser engines, or third-party dependencies.

## Recommended architecture

Use Venom for valuable client-side algorithms, validation, scoring, transformation, and decision support that must execute locally. Keep irreversible authorization, privileged credentials, signing keys, payment authority, and source-of-truth decisions on trusted infrastructure.

Continue with [Threat model](threat-model.md), [Protection strengths](protection-strengths.md), [Production hardening](production-hardening.md), and [Limitations](limitations.md).

## Review checklist

Before approving a deployment, confirm that protected source is absent from the production output, the browser bridge exposes only required exports, production verification passes, signed release metadata is validated, and any authoritative security decision is still confirmed by trusted infrastructure. Record the exact Venom version, browser matrix, runtime provenance, and release evidence with the deployment.

Release owners should review this model together with the application-specific data-flow and server-side authority design.
