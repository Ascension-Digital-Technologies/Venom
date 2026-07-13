# Threat model

## Protected assets

Venom attempts to protect or validate:

- original application source presentation;
- packaged route and DOM structure;
- QuickJS executable bytecode records;
- package structure and section bounds;
- consistency of loader, worker, runtime, WebAssembly, stylesheet, and package assets;
- production fallback policy;
- runtime resource limits and supported host-call boundaries.

## Adversaries considered

Venom provides resistance against:

- casual source inspection;
- simple extraction scripts expecting readable JavaScript and HTML;
- accidental corruption;
- stale CDN caches and partial deployments;
- mixing assets from different builds;
- malformed package inputs;
- unsophisticated modifications that do not reconstruct package metadata and runtime bindings.

## Adversaries outside the protection boundary

A determined local operator may:

- instrument the loader and generated runtime;
- inspect or patch workers;
- dump WebAssembly memory;
- intercept host bridge calls;
- observe decoded sections and QuickJS objects at runtime;
- replace the complete distribution and regenerate untrusted metadata;
- automate behavioral analysis;
- modify the browser or execution environment.

Venom raises the cost of these actions but does not prevent them absolutely.

## Trust assumptions

- The build machine and release-signing environment are trusted.
- Vendored dependencies and pinned toolchains are reviewed and verified.
- The production server and deployment pipeline preserve the generated file set.
- External release-signature public keys are obtained through a trusted channel.
- Server-side authorization and secret management remain authoritative.

## Security boundaries

The QuickJS engine executes in WebAssembly, which provides a memory boundary from ordinary browser JavaScript under the browser's WebAssembly model. The surrounding loader, worker, Route VM interpreter, and host bridge are still client-delivered code and participate in the trusted runtime implementation.

The Route VM is a custom interpreter in generated JavaScript. It is a transformation and execution mechanism, not a separate native or WebAssembly sandbox.

## Explicit non-goals

- making client code impossible to reverse engineer;
- hiding server secrets in browser artifacts;
- preventing a user from observing values rendered or used locally;
- guaranteeing publisher authenticity without trusted signatures;
- supporting every browser API or JavaScript framework;
- replacing server-side authorization, CSP, HTTPS, or standard application security.
