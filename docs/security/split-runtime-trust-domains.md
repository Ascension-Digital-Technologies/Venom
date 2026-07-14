# Split runtime trust domains

> **Audience:** Security reviewers and runtime contributors  
> **Status:** Stable  
> **Applies to:** Venom 1.65.3
Venom separates package decoding from protected JavaScript execution. The package runtime owns package upload, validation, section materialization, and retirement. The QuickJS runtime owns bytecode loading and execution.

## Handoff contract

Every decoded QuickJS record carries a short-lived handoff record containing:

- producer and consumer domain identities;
- decoded byte length;
- a byte-level integrity hash;
- a binding hash over route, source, execution order, length, and byte hash.

The QuickJS execution module independently recomputes and validates the handoff before allocating or executing the bytecode. Missing, stale, substituted, or context-confused handoffs fail closed.

## Security benefit

This prevents accidental or casual substitution between the package-decoder and execution domains and makes a generic hook responsible for preserving both the decoded bytes and their execution-context binding. It also keeps trust-domain responsibilities explicit and testable.

## Boundary

Both domains still execute in a user-controlled browser. An analyst who patches both the producer and consumer can bypass the check. This mechanism raises tampering and automation cost; it is not a hardware-backed trust boundary.
