# Limitations

> **Applies to:** Venom 1.1.0

Venom provides reverse-engineering resistance for software that must be delivered to a browser. It does not provide a trusted execution environment.

## No permanent client-side secrecy

A user controlling the browser can inspect network responses, workers, WebAssembly memory, runtime state, and decoded values. The exact original protected source is absent, but behavior and equivalent logic may still be reconstructed.

## No protection for browser-runtime code

Code marked `@venom: browser`, framework renderers, DOM manipulation, and other browser-native responsibilities remain JavaScript and can be inspected. Production hardening makes generated assets harder to read but does not remove browser-executed logic.

## Returned values are observable

Arguments supplied by browser code and results returned to browser code are observable at their endpoints. Keep return values minimal and do not return internal models, rule traces, private keys, or sensitive implementation data.

## WebAssembly memory is observable

Range validation, chunked decoding, short-lived buffers, and zeroization reduce exposure windows. They cannot prevent a modified browser or debugger from observing memory before it is cleared.

## Client checks are not authoritative

Licensing, payments, entitlements, fraud decisions, and privileged authorization must be confirmed by a trusted service when security depends on the result.

## Compatibility requires qualification

Complex frameworks, third-party scripts, dynamic imports, browser APIs, workers, and unusual build output may require browser annotations or configuration. Use the compatibility report and browser-equivalence suite before production deployment.

## Performance overhead exists

Worker startup, package verification, WebAssembly initialization, bytecode execution, bridge serialization, and polymorphic decoding add overhead. Measure representative workloads with the production benchmark tools.

## Security depends on release discipline

Skipping hardener setup, provenance checks, leak scans, browser qualification, signature verification, or clean-package testing weakens the release. Stable releases should be produced through the documented closure pipeline.
