# Runtime Integrity Seals

> **Applies to:** Venom 1.1.0

Venom computes and rechecks integrity seals over critical browser and worker runtime state. The checks are distributed across startup and execution boundaries rather than concentrated in one optional boot check.

## Worker seal

The protected worker seals capability mappings, registry bytecode, and active bridge operation identifiers. It validates the seal during initialization and before protected invocations.

## Browser runtime seal

The browser runtime seals release policy, TurboJS ABI identity, and route mapping state visible to the browser layer. It revalidates that state before route execution.

## Failure behavior

A mismatch closes or rejects the affected execution path. Production does not continue with mutated tables or silently downgrade to another backend.

## Security boundary

A browser owner may patch both state and verification code. Integrity seals raise the complexity of casual mutation and generic patching; they are not hardware-backed attestation.

See [Security model](security-model.md), [Binary bridge](binary-capability-bridge.md), and [Split runtime trust domains](split-runtime-trust-domains.md).
