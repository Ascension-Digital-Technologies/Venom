# Security model

Venom is a client-side protection and deployment-consistency system. It raises reverse-engineering cost and rejects malformed or inconsistent runtime inputs, but it cannot create an absolute trust boundary on a client-controlled browser.

## Defensible security goals

Venom aims to:

- remove immediately readable application source from protected output;
- execute stripped native QuickJS bytecode through a real QuickJS/WASM runtime;
- encode routes and deterministic DOM construction in a diversified custom bytecode format;
- reject malformed package structures and invalid section bounds;
- detect stale, missing, altered, or mixed-build assets relative to generated metadata;
- deny host-JavaScript and source-decode fallback in protected production builds;
- enforce configured execution, memory, recursion, host-call, route, and event limits;
- make static extraction and unauthorized modification more expensive.

## Non-goals

Venom does not claim:

- permanent browser-side confidentiality;
- an unextractable package key;
- a secure enclave or trusted execution environment;
- complete browser sandboxing;
- publisher authenticity without a separately trusted signature;
- protection for secrets or authorization logic shipped to the browser;
- universal compatibility with arbitrary websites or Web APIs.

## Browser threat boundary

A determined operator controls the browser, developer tools, network responses, workers, storage, cached files, loader code, generated runtime JavaScript, WebAssembly memory, and downloaded package bytes.

Any value required for browser execution can ultimately be observed or reproduced. Server credentials, private keys, privileged business logic, and authorization decisions must remain on trusted servers.

## Browser-compatible package protection

The browser-compatible provider uses a runtime-decodable authenticated encoding layer. It obscures section contents, raises casual extraction cost, and validates envelope consistency before use.

It must not be described as audited cryptographic confidentiality against the browser operator. The runtime necessarily contains enough logic and constants to decode browser-runnable packages.

## Native/private encryption

The optional libsodium provider uses XChaCha20-Poly1305 with an external 256-bit key for native or private tooling workflows. This is genuine authenticated encryption when the key remains outside the distributed artifact.

Browser runtimes intentionally reject libsodium-sealed packages because a key delivered to the browser would not remain secret.

## Integrity and authenticity

Venom uses several different checks with different purposes:

| Mechanism | Purpose | Limitation |
|---|---|---|
| FNV package/section hashes | Fast structural corruption checks | Not cryptographic authentication |
| SHA-256 decoded-section records | Stronger consistency checks after decoding | A complete replacement distribution can carry regenerated records |
| Content-addressed asset names and bindings | Detect partial, stale, or mixed-build deployment | Do not independently identify the publisher |
| Subresource Integrity | Verify referenced resources against HTML metadata | HTML replacement can also replace SRI values |
| External release signatures | Establish publisher authenticity when verified against a trusted public key | Must be explicitly generated, distributed, and verified |

Use “integrity-checked” or “build-consistency-verified” for default browser distributions. Reserve “authenticated publisher release” for workflows that actually verify an external signature against a trusted key.

## Host bridge

QuickJS accesses selected browser services through Venom's host bridge. The bridge applies implemented operation validation, metadata checks, size limits, lifecycle checks, and denial behavior.

The bridge is not a complete browser implementation or a formally verified capability security system. Unsupported APIs and semantic differences remain compatibility risks.

## Required production practices

- Deploy the entire generated distribution atomically.
- Use HTTPS, CSP, and standard web security headers.
- Keep server secrets and privileged decisions off the client.
- Run release checks, runtime verification, strict distribution analysis, browser tests, and fuzz/replay gates.
- Use externally trusted release signatures when publisher authenticity is required.
- Treat protection as defense in depth, not as the sole security control.
