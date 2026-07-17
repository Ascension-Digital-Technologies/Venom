# Threat Model

> **Applies to:** Venom 1.1.0

## Adversary capabilities

Venom assumes an analyst may:

- download every public distribution asset;
- inspect and modify JavaScript, WebAssembly, workers, and package bytes;
- use browser developer tools and custom browser instrumentation;
- replay page flows and record bridge traffic;
- patch loader or runtime code;
- pause execution and inspect memory;
- compare multiple production builds;
- automate extraction and behavioral analysis.

## Defended scenarios

Venom is designed to raise the cost of:

- recovering exact protected source from shipped files;
- using generic JavaScript beautification or deobfuscation as the primary recovery path;
- carving raw canonical QuickJS records from the package;
- reusing fixed Route VM opcode tables across builds;
- replaying captured protected-call frames across counters or sessions;
- silently replacing one runtime asset or package section;
- downgrading a production build to host JavaScript execution;
- publishing an incomplete, unsigned, stale, or unqualified release.

## Trusted inputs

The build machine, compiler source, pinned toolchain inputs, signing environment, and release key custody are trusted. A compromised build host or signing key can produce malicious artifacts that otherwise satisfy the public runtime format.

## Browser-owner limitation

The client device is not trusted. The browser owner can ultimately instrument both sides of every software-only boundary. Protection mechanisms therefore focus on representation change, isolation, narrow protocols, short-lived plaintext, integrity, polymorphism, and release verification rather than claiming hidden computation.

## Server boundary

Secrets and authoritative decisions must remain server-side. A local protected computation may improve user experience or protect implementation details, but it must not be treated as the sole authorization check for privileged operations.

## Residual risk

A sufficiently motivated analyst can reconstruct behavior, patch checks, capture decoded records, or develop build-specific tooling. Venom's goal is to make that work more expensive and less reusable, not impossible.
